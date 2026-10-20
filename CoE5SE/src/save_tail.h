// The save tail: what CoE5SE writes after the last marker of every save and turn file.

#define SAVE_TAG(a, b, c, d)  ((int)((unsigned)(a) | (unsigned)(b) << 8 | (unsigned)(c) << 16 | (unsigned)(d) << 24))
#define SAVE_LIST_MAGIC       SAVE_TAG('C', '5', 'S', 'L')
#define SAVE_LIST_VERSION     1
#define SAVE_CHUNK_MAX        (4 << 20)
#define SAVE_CHUNKS_MAX       64
#define SAVE_STRING_MAX       255
#define SAVE_TAG_GAME         SAVE_TAG('G', 'A', 'M', 'E')
#define SAVE_TAG_END          SAVE_TAG('E', 'N', 'D', ' ')
#define SAVE_GAME_VERSION     1
#define SAVE_GAME_KINDS       6
_Static_assert(SAVE_LIST_MAGIC == 0x4c533543, "C5SL, little-endian");

typedef enum { SAVE_FILE_SAVE, SAVE_FILE_TURN } SaveFile;
static const char *const SAVE_FILE_NAMES[] = {"save", "turn"};

#ifdef __APPLE__
#define SAVE_PLATFORM 1
#else
#define SAVE_PLATFORM 0
#endif
static const char *savePlatformName(int platform) { return platform == 0 ? "windows" : platform == 1 ? "mac" : "unknown"; }

// ---- Payloads -----------------------------------------------------------------------------------
// A payload being built.
typedef struct { unsigned char *bytes; size_t size, capacity; int failed; } SaveBuffer;

static void saveBufferAdd(SaveBuffer *b, const void *data, size_t size) {
    if (b->failed) return;
    if (b->size + size > SAVE_CHUNK_MAX) {
        b->failed = 1;
        return;
    }
    if (b->size + size > b->capacity) {
        size_t capacity = b->capacity ? b->capacity : 256;
        while (capacity < b->size + size) capacity *= 2;
        unsigned char *bytes = realloc(b->bytes, capacity);
        if (!bytes) {
            b->failed = 1;
            return;
        }
        b->bytes = bytes;
        b->capacity = capacity;
    }
    if (size) memcpy(b->bytes + b->size, data, size);
    b->size += size;
}

static void saveBufferInt(SaveBuffer *b, int value) {
    unsigned char le[4] = {(unsigned char)value, (unsigned char)(value >> 8), (unsigned char)(value >> 16), (unsigned char)(value >> 24)};
    saveBufferAdd(b, le, 4);
}

// A string: its length as one byte, then its bytes.
static void saveBufferString(SaveBuffer *b, const char *text) {
    size_t length = strlen(text);
    unsigned char size = (unsigned char)(length > SAVE_STRING_MAX ? SAVE_STRING_MAX : length);
    saveBufferAdd(b, &size, 1);
    saveBufferAdd(b, text, size);
}

// Reading a payload: a cursor that fails, and stays failed, when a field runs past the payload.
typedef struct { const unsigned char *at, *end; int failed; } SaveCursor;

static int saveCursorInt(SaveCursor *c) {
    if (c->failed || c->end - c->at < 4) {
        c->failed = 1;
        return 0;
    }
    int value = (int)((unsigned)c->at[0] | (unsigned)c->at[1] << 8 | (unsigned)c->at[2] << 16 | (unsigned)c->at[3] << 24);
    c->at += 4;
    return value;
}

// Copies a string into `out` (`size` bytes, at least SAVE_STRING_MAX + 1 for every name to fit).
static void saveCursorString(SaveCursor *c, char *out, size_t size) {
    out[0] = '\0';
    if (c->failed || c->at >= c->end || c->end - c->at - 1 < c->at[0]) {
        c->failed = 1;
        return;
    }
    size_t length = c->at[0], kept = length < size ? length : size - 1;
    memcpy(out, c->at + 1, kept);
    out[kept] = '\0';
    c->at += 1 + length;
}

// ---- The list -----------------------------------------------------------------------------------
typedef struct { int tag, size; unsigned char *payload; } SaveChunk;
typedef struct { SaveChunk chunks[SAVE_CHUNKS_MAX]; int count, dropped; } SaveList;

// The tag as its 4 characters, for the log.
static const char *saveTagText(int tag, char out[5]) {
    for (int i = 0; i < 4; i++) {
        unsigned char ch = (unsigned char)(tag >> (8 * i));
        out[i] = ch >= 0x20 && ch < 0x7f ? (char)ch : '?';
    }
    out[4] = '\0';
    return out;
}

// Adds a built payload to the list, which then owns its bytes.
static void saveListAdd(SaveList *list, int tag, SaveBuffer *b) {
    char text[5];
    if (b->failed || list->count >= SAVE_CHUNKS_MAX) {
        writeLog("save_chunk_dropped", "tag=%s bytes=%zu reason=%s", saveTagText(tag, text), b->size,
                 b->failed ? "out_of_memory_or_too_big" : "too_many_chunks");
        free(b->bytes);
        list->dropped++;
    } else list->chunks[list->count++] = (SaveChunk){tag, (int)b->size, b->bytes};
    *b = (SaveBuffer){0};
}

static void saveListFree(SaveList *list) {
    for (int i = 0; i < list->count; i++) free(list->chunks[i].payload);
    list->count = 0;
}

// What the names chunks wrote, for save_names_written.
typedef struct { int mods, spells; } SaveNamesWritten;

// The names chunks (save_names.h, included after this file).
static void addSaveNameChunks(SaveList *list, SaveNamesWritten *written);
static const char *useSaveNameChunks(const SaveList *list, SaveFile file);
static int isSaveNameTag(int tag);
static void clearChangedSpells(void);
static void addModContentChunk(SaveList *list);
#define SAVE_TAG_MODC SAVE_TAG('M', 'O', 'D', 'C')
#define SAVE_TAG_MODH SAVE_TAG('M', 'O', 'D', 'H')

// The GAME chunk's last slot of the game's own content for kind `kind`.
static int saveVanillaLastIndex(int kind) {
    if (kind == 0) return VANILLA_SPELL_COUNT - 1;
    if (kind == SAVE_GAME_KINDS - 1) return VANILLA_SPELL_PATH_COUNT - 1;
    return -1;
}

// Builds the list for a file being written: GAME, the names chunks, then MODH and MODC, last.
static void buildSaveList(SaveList *list, SaveNamesWritten *written) {
    SaveBuffer game = {0};
    saveBufferInt(&game, SAVE_GAME_VERSION);
    saveBufferInt(&game, SAVE_PLATFORM);
    char version[32];
    snprintf(version, sizeof version, "api %d", PLUGIN_API_VERSION);
    saveBufferString(&game, version);
    for (int k = 0; k < SAVE_GAME_KINDS; k++) saveBufferInt(&game, saveVanillaLastIndex(k));
    saveListAdd(list, SAVE_TAG_GAME, &game);
    addSaveNameChunks(list, written);
    addModContentChunk(list);
}

// Writes the list to the file and returns its bytes.
static long long writeSaveList(void *file, const SaveList *list) {
    long long bytes = 0;
    WRITE_INT(SAVE_LIST_MAGIC, file);
    WRITE_INT(SAVE_LIST_VERSION, file);
    bytes += 8;
    for (int i = 0; i < list->count; i++) {
        WRITE_INT(list->chunks[i].tag, file);
        WRITE_INT(list->chunks[i].size, file);
        if (list->chunks[i].size) WRITE_ARRAY(list->chunks[i].payload, 1, list->chunks[i].size, file);
        bytes += 8 + list->chunks[i].size;
    }
    WRITE_INT(SAVE_TAG_END, file);
    WRITE_INT(0, file);
    WRITE_INT(SAVE_LIST_MAGIC, file);
    return bytes + 12;
}

// What the last read of a list found: save_names_read's fields.
typedef struct {
    const char *result, *reason;
    char skipped[5 * SAVE_CHUNKS_MAX + 1];
    int writtenOn;
    int chunks, unknown;
} SaveListRead;
static SaveListRead lastSaveListRead;

// Reads 4 bytes as an int; 0 on a short read.
static int readSaveInt(void *file, int *value) {
    *value = 0;
    return READ_ARRAY(value, 4, 1, file) == 4;
}

// Reads the list after the RXC1 block into `list`, all of it or nothing.
static const char *readSaveList(void *file, SaveList *list, const char **reason) {
    int magic, version, tag, size;
    *reason = "-";
    if (!readSaveInt(file, &magic)) return "no_block";
    if (magic != SAVE_LIST_MAGIC) return *reason = "other_bytes", "no_block";
    if (!readSaveInt(file, &version)) return *reason = "short_read", "bad_block";
    if (version > SAVE_LIST_VERSION) return "newer_format";
    if (version < 1) return *reason = "bad_version", "bad_block";
    for (;;) {
        if (!readSaveInt(file, &tag) || !readSaveInt(file, &size)) return *reason = "short_read", "bad_block";
        if (tag == SAVE_TAG_END) {
            if (size) return *reason = "bad_end", "bad_block";
            break;
        }
        if (list->count >= SAVE_CHUNKS_MAX) return *reason = "too_many_chunks", "bad_block";
        if (size < 0 || size > SAVE_CHUNK_MAX) return *reason = "chunk_too_big", "bad_block";
        unsigned char *payload = malloc((size_t)size + 1);
        if (!payload) return *reason = "out_of_memory", "bad_block";
        list->chunks[list->count++] = (SaveChunk){tag, size, payload};
        if (size && READ_ARRAY(payload, 1, size, file) != size) return *reason = "short_read", "bad_block";
    }
    if (!readSaveInt(file, &magic) || magic != SAVE_LIST_MAGIC) return *reason = "no_closing_magic", "bad_block";
    return NULL;
}

// Notes a known tag skipped for its payload version.
static void skipSaveChunk(SaveListRead *read, int tag) {
    char text[5];
    size_t used = strlen(read->skipped);
    snprintf(read->skipped + used, sizeof read->skipped - used, "%s%s", used ? "," : "", saveTagText(tag, text));
}

// Reads the GAME chunk: which platform wrote the file.
static void readSaveGameChunk(const SaveChunk *chunk, SaveListRead *read) {
    SaveCursor c = {chunk->payload, chunk->payload + chunk->size, 0};
    char writer[SAVE_STRING_MAX + 1];
    int version = saveCursorInt(&c), platform = saveCursorInt(&c);
    saveCursorString(&c, writer, sizeof writer);
    if (c.failed || version != SAVE_GAME_VERSION) {
        skipSaveChunk(read, chunk->tag);
        return;
    }
    read->writtenOn = platform;
}

// ---- The tail -----------------------------------------------------------------------------------
// Writes the tail after a file's last marker: RXC1, then the list.
static void writeSaveTail(void *file, int onlyPlayer, SaveFile kind) {
    writeExtraCastCounts(file, onlyPlayer, SAVE_FILE_NAMES[kind]);
    SaveList list = {0};
    SaveNamesWritten written = {0};
    buildSaveList(&list, &written);
    long long bytes = writeSaveList(file, &list);
    writeLog("save_names_written", "file=%s mods=%d spells=%d bytes=%lld dropped=%d", SAVE_FILE_NAMES[kind], written.mods, written.spells,
             bytes, list.dropped);
    saveListFree(&list);
}

// Reads the tail after a file's last marker.
static void readSaveTail(void *file, int onlyPlayer, SaveFile kind) {
    SaveListRead *read = &lastSaveListRead;
    *read = (SaveListRead){.result = "no_block", .reason = "-", .writtenOn = -1};
    if (kind == SAVE_FILE_SAVE) clearChangedSpells();
    int castCounts = readExtraCastCounts(file, onlyPlayer, SAVE_FILE_NAMES[kind]);
    SaveList list = {0};
    if (castCounts < 0) read->result = "bad_block", read->reason = "cast_counts_block";
    else if (castCounts > 0) {
        const char *failed = readSaveList(file, &list, &read->reason);
        if (failed) read->result = failed;
        else {
            read->chunks = list.count;
            for (int i = 0; i < list.count; i++) {
                if (list.chunks[i].tag == SAVE_TAG_GAME) readSaveGameChunk(&list.chunks[i], read);
                else if (!isSaveNameTag(list.chunks[i].tag) && list.chunks[i].tag != SAVE_TAG_MODC && list.chunks[i].tag != SAVE_TAG_MODH)
                    read->unknown++;
            }
            read->result = useSaveNameChunks(&list, kind);
        }
    }
    writeLog("save_names_read", "file=%s result=%s reason=%s skipped=%s written_on=%s chunks=%d unknown=%d", SAVE_FILE_NAMES[kind],
             read->result, read->reason, read->skipped[0] ? read->skipped : "-", savePlatformName(read->writtenOn), read->chunks,
             read->unknown);
    saveListFree(&list);
}

// ---- The hooks ----------------------------------------------------------------------------------
#ifndef __APPLE__
// The game writes each file's last marker with its own fwrite.
typedef size_t (*GameFwriteFn)(const void *data, size_t size, size_t count, void *file);
typedef int (*GameFcloseFn)(void *file);

static size_t writeSaveEnd(const void *marker, size_t size, size_t count, void *file) {
    size_t written = GAME_FN(GameFwriteFn, RVA_FWRITE)(marker, size, count, file);
    writeSaveTail(file, -1, SAVE_FILE_SAVE);
    return written;
}

// The server screen's copy of save_game writes its marker with write_int(0x69, file).
typedef int (*GameWriteIntFn)(int value, void *file);

static int writeServerSaveEnd(int marker, void *file) {
    int written = GAME_FN(GameWriteIntFn, RVA_FILE_WRITE_INT)(marker, file);
    writeSaveTail(file, -1, SAVE_FILE_SAVE);
    return written;
}

static int closeSaveRead(void *file) {
    readSaveTail(file, -1, SAVE_FILE_SAVE);
    return GAME_FN(GameFcloseFn, RVA_FCLOSE)(file);
}

// The turn file writer's player, which the marker's stub stores here.
static volatile int *turnWriterPlayerSlot;

static size_t writeTurnEnd(const void *marker, size_t size, size_t count, void *file) {
    size_t written = GAME_FN(GameFwriteFn, RVA_FWRITE)(marker, size, count, file);
    writeSaveTail(file, turnWriterPlayerSlot ? *turnWriterPlayerSlot : -1, SAVE_FILE_TURN);
    return written;
}

// A turn file read's player.
static int closeTurnRead(void *file) {
    readSaveTail(file, turnFilePlayer, SAVE_FILE_TURN);
    return GAME_FN(GameFcloseFn, RVA_FCLOSE)(file);
}

#define SAVE_TAIL_STUB_SIZE 0x20
#define SAVE_TAIL_R14_CODE  0xa0
#define SAVE_TAIL_HOOKS     5
// @platform-split save_tail: Windows writes the tail after the marker's fwrite, Mac at the fclose
static int installSaveTailHooks(void) {
    static const struct { unsigned int callRva, targetRva; void *hook; const char *name; int keepR14; } CALLS[SAVE_TAIL_HOOKS] = {
        {RVA_SAVE_GAME_END_MARKER,   RVA_FWRITE,         (void *)writeSaveEnd,       "save_game_end",   0},
        {RVA_LOAD_GAME_END_MARKER,   RVA_FCLOSE,         (void *)closeSaveRead,      "load_game_end",   0},
        {RVA_SAVE_2H_END_MARKER,     RVA_FWRITE,         (void *)writeTurnEnd,       "save_2h_end",     1},
        {RVA_LOAD_2H_END_MARKER,     RVA_FCLOSE,         (void *)closeTurnRead,      "load_2h_end",     0},
        {RVA_SERVER_SAVE_END_MARKER, RVA_FILE_WRITE_INT, (void *)writeServerSaveEnd, "server_save_end", 0},
    };
    _Static_assert(sizeof CALLS / sizeof CALLS[0] * SAVE_TAIL_STUB_SIZE <= SAVE_TAIL_R14_CODE, "the stubs end before the r14 code");
    unsigned char *stubs = allocNear(gameBase + RVA_TEXT_SECTION);
    if (!stubs) {
        writeLog("save_tail_hooks_skipped", "reason=no_near_page");
        return 0;
    }
    int ok = 0;
    for (size_t i = 0; i < sizeof CALLS / sizeof CALLS[0]; i++) {
        void *hook = CALLS[i].hook;
        if (CALLS[i].keepR14) {
            unsigned char *code = stubs + SAVE_TAIL_R14_CODE, *slot = stubs + SAVE_TAIL_R14_CODE + 0x20;
            int disp = (int)(slot - (code + 7));
            code[0] = 0x44; code[1] = 0x89; code[2] = 0x35; memcpy(code + 3, &disp, 4);
            code[7] = 0xff; code[8] = 0x25; memset(code + 9, 0, 4);
            memcpy(code + 13, &hook, 8);
            hook = code;
            turnWriterPlayerSlot = (volatile int *)slot;
        }
        ok += redirectCall(CALLS[i].callRva, CALLS[i].targetRva, stubs + SAVE_TAIL_STUB_SIZE * i, hook, CALLS[i].name, NULL);
    }
    return ok;
}
#else
// Each file's last marker comes right before its fclose.
static int closeSaveWritten(FILE *file) {
    writeSaveTail(file, -1, SAVE_FILE_SAVE);
    return fclose(file);
}

static int closeSaveRead(FILE *file) {
    readSaveTail(file, -1, SAVE_FILE_SAVE);
    return fclose(file);
}

static int closeTurnRead(FILE *file) {
    readSaveTail(file, turnFilePlayer, SAVE_FILE_TURN);
    return fclose(file);
}

// The turn file writer's player comes in x1: the padding code before it copies it from x19.
static int closeTurnWritten(FILE *file, long long player) {
    writeSaveTail(file, (int)player, SAVE_FILE_TURN);
    return fclose(file);
}

// Points the fclose `bl` at `callRva` to `code`.
static int pointSaveTailCall(unsigned int callRva, unsigned char *code, const char *name) {
    uint32_t word = blTo(gameBase + callRva, code);
    if (writeGameCode(gameBase + callRva, &word, 4)) return 1;
    writeLog("save_tail_hook_skipped", "name=%s va=0x%llx reason=code_unwritable", name, callRva + 0x100000000ULL);
    return 0;
}

// Checks that the `bl` at `callRva` calls fclose; logs and returns 0 when it doesn't.
static int isSaveTailFclose(unsigned int callRva, const char *name) {
    if (blTarget(gameBase + callRva) == gameBase + RVA_FCLOSE) return 1;
    writeLog("save_tail_hook_skipped", "name=%s va=0x%llx reason=not_the_expected_call", name, callRva + 0x100000000ULL);
    return 0;
}

#define SAVE_TAIL_HOOKS 4
static int installSaveTailHooks(void) {
    static const struct { unsigned int callRva; void *hook; const char *name; } CALLS[] = {
        {RVA_SAVE_GAME_FCLOSE,     (void *)closeSaveWritten, "save_game_end"},
        {RVA_LOAD_GAME_END_MARKER, (void *)closeSaveRead,    "load_game_end"},
        {RVA_LOAD_2H_END_MARKER,   (void *)closeTurnRead,    "load_2h_end"},
    };
    int ok = 0;
    for (size_t i = 0; i < sizeof CALLS / sizeof CALLS[0]; i++) {
        if (!isSaveTailFclose(CALLS[i].callRva, CALLS[i].name)) continue;
        unsigned char *trampoline = newTrampoline(CALLS[i].hook);
        if (!trampoline) writeLog("save_tail_hook_skipped", "name=%s reason=no_padding", CALLS[i].name);
        else ok += pointSaveTailCall(CALLS[i].callRva, trampoline, CALLS[i].name);
    }
    if (isSaveTailFclose(RVA_SAVE_2H_FCLOSE, "save_2h_end")) {
        uint32_t closeCode[8] = {0xaa1303e1u, 0x58000070u, 0xd61f0200u, 0xd503201fu};
        void *hook = (void *)closeTurnWritten;
        memcpy(&closeCode[4], &hook, 8);
        unsigned char *code = takePadding(sizeof closeCode);
        if (!code || !writeGameCode(code, closeCode, sizeof closeCode)) writeLog("save_tail_hook_skipped", "name=save_2h_end reason=no_padding");
        else ok += pointSaveTailCall(RVA_SAVE_2H_FCLOSE, code, "save_2h_end");
    }
    return ok;
}
#endif

// Installs the hooks.
static void saveTailInit(void) {
    writeLog("save_tail_hooks_installed", "ok=%d of %d", installSaveTailHooks(), SAVE_TAIL_HOOKS);
}
