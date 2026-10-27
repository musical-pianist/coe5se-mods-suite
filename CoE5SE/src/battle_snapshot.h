// The battle snapshot file through memory: its writes kept until the close, its reads answered from one read.
#pragma once

typedef struct {
    unsigned int address;
    unsigned char kind;
} SnapshotSite;

enum { SNAPSHOT_OPEN_WRITE, SNAPSHOT_CLOSE_WRITE, SNAPSHOT_WRITE, SNAPSHOT_OPEN_READ, SNAPSHOT_CLOSE_READ, SNAPSHOT_READ,
       SNAPSHOT_KINDS };

#ifdef __APPLE__
#include "battle_snapshot_sites_mac.h"
#define SNAPSHOT_SITES      BATTLE_SNAPSHOT_MAC_SITES
#define SNAPSHOT_SITE_COUNT BATTLE_SNAPSHOT_MAC_SITE_COUNT
#else
#include "battle_snapshot_sites.h"
#define SNAPSHOT_SITES      BATTLE_SNAPSHOT_SITES
#define SNAPSHOT_SITE_COUNT BATTLE_SNAPSHOT_SITE_COUNT
#endif

#define SNAPSHOT_CHUNK (1u << 20)

typedef void *(*SnapshotFopenFn)(const char *, const char *);
typedef int (*SnapshotFcloseFn)(void *);
typedef size_t (*SnapshotIoFn)(void *, size_t, size_t, void *);

static struct {
    void *writing;
    long long countedAtOpen;
    int mismatchLogged;
    int direct;
    unsigned char *out;
    size_t outLength, outCapacity;
    void *reading;
    unsigned char *in;
    size_t inLength, inAt, inCapacity;
} snapshot;

// The game's fopen, fclose, fwrite and fread, set at install (the unit tests give libc's).
static SnapshotFopenFn snapshotFopen;
static SnapshotFcloseFn snapshotFclose;
static SnapshotIoFn snapshotFwrite, snapshotFread;

static void *snapshotGameOpen(const char *name, const char *mode) { return snapshotFopen(name, mode); }
static int snapshotGameClose(void *file) { return snapshotFclose(file); }
static size_t snapshotGameWrite(const void *p, size_t size, size_t count, void *file) { return snapshotFwrite((void *)p, size, count, file); }
static size_t snapshotGameRead(void *p, size_t size, size_t count, void *file) { return snapshotFread(p, size, count, file); }

// Grows `*buffer` to hold at least `need` bytes; 0 when it can't.
static int snapshotGrow(unsigned char **buffer, size_t *capacity, size_t need) {
    if (need <= *capacity) return 1;
    size_t grown = *capacity ? *capacity : SNAPSHOT_CHUNK;
    while (grown < need) {
        if (grown > SIZE_MAX / 2) return 0;
        grown *= 2;
    }
    unsigned char *p = realloc(*buffer, grown);
    if (!p) return 0;   // No log: the caller goes on through the game's own calls
    *buffer = p, *capacity = grown;
    return 1;
}

// The writer's fopen: the game's, and the stream is buffered until its close.
static void *snapshotOpenWrite(const char *name, const char *mode) {
    void *file = snapshotGameOpen(name, mode);
    if (file && !snapshot.writing) {
        snapshot.writing = file;
        snapshot.direct = 0;
        snapshot.outLength = 0;
        snapshot.countedAtOpen = *(long long *)(gameBase + RVA_FILE_BYTES_WRITTEN);
    }
    return file;
}

// fwrite in the writer: the snapshot stream's bytes go to the buffer.
static size_t snapshotWrite(const void *p, size_t size, size_t count, void *file) {
    if (!file || file != snapshot.writing || snapshot.direct) return snapshotGameWrite(p, size, count, file);
    if (!size || !count) return 0;
    if (count > SIZE_MAX / size || !snapshotGrow(&snapshot.out, &snapshot.outCapacity, snapshot.outLength + size * count)) {
        if (snapshot.outLength) snapshotGameWrite(snapshot.out, 1, snapshot.outLength, file);
        snapshot.outLength = 0;
        snapshot.direct = 1;
        return snapshotGameWrite(p, size, count, file);
    }
    memcpy(snapshot.out + snapshot.outLength, p, size * count);
    snapshot.outLength += size * count;
    return count;
}

// The writer's fclose: the buffer in one write, then the game's fclose.
static int snapshotCloseWrite(void *file) {
    if (file && file == snapshot.writing) {
        long long counted = *(long long *)(gameBase + RVA_FILE_BYTES_WRITTEN) - snapshot.countedAtOpen;
        if (!snapshot.direct && counted != (long long)snapshot.outLength && !snapshot.mismatchLogged) {
            snapshot.mismatchLogged = 1;
            writeLog("battle_snapshot_mismatch", "counted=%lld buffered=%zu", counted, snapshot.outLength);
        }
        if (!snapshot.direct && snapshot.outLength) snapshotGameWrite(snapshot.out, 1, snapshot.outLength, file);
        snapshot.writing = NULL;
        snapshot.outLength = 0;
    }
    return snapshotGameClose(file);
}

// The reader's fopen: the game's, then the whole file read into memory.
static void *snapshotOpenRead(const char *name, const char *mode) {
    void *file = snapshotGameOpen(name, mode);
    if (!file || snapshot.reading) return file;
    snapshot.inLength = snapshot.inAt = 0;
    for (;;) {
        if (!snapshotGrow(&snapshot.in, &snapshot.inCapacity, snapshot.inLength + SNAPSHOT_CHUNK)) break;
        size_t want = snapshot.inCapacity - snapshot.inLength;
        size_t got = snapshotGameRead(snapshot.in + snapshot.inLength, 1, want, file);
        snapshot.inLength += got;
        if (got < want) break;
    }
    snapshot.reading = file;
    return file;
}

// fread in the reader: the snapshot stream's bytes from memory, as the game's fread hands them.
static size_t snapshotRead(void *p, size_t size, size_t count, void *file) {
    if (!file || file != snapshot.reading) return snapshotGameRead(p, size, count, file);
    if (!size || !count) return 0;
    size_t total = count > SIZE_MAX / size ? SIZE_MAX : size * count;
    size_t take = snapshot.inLength - snapshot.inAt;
    if (take > total) take = total;
    memcpy(p, snapshot.in + snapshot.inAt, take);
    snapshot.inAt += take;
    if (take < total) take += snapshotGameRead((unsigned char *)p + take, 1, total - take, file);
    return take / size;
}

// The reader's fclose.
static int snapshotCloseRead(void *file) {
    if (file && file == snapshot.reading) snapshot.reading = NULL;
    return snapshotGameClose(file);
}

// The game function each kind of site calls, and the hook it gets.
static unsigned int snapshotTarget(int kind) {
    switch (kind) {
    case SNAPSHOT_OPEN_WRITE: case SNAPSHOT_OPEN_READ: return RVA_FOPEN;
    case SNAPSHOT_CLOSE_WRITE: case SNAPSHOT_CLOSE_READ: return RVA_FCLOSE;
    case SNAPSHOT_WRITE: return RVA_FWRITE;
    default: return RVA_FREAD;
    }
}
static void *snapshotHook(int kind) {
    static void *const HOOKS[SNAPSHOT_KINDS] = {(void *)snapshotOpenWrite, (void *)snapshotCloseWrite, (void *)snapshotWrite,
                                                (void *)snapshotOpenRead, (void *)snapshotCloseRead, (void *)snapshotRead};
    return HOOKS[kind];
}

// 1 when the site holds the game's call of its kind's function.
static int snapshotSiteIsCall(const SnapshotSite *s) {
    const unsigned char *at = gameBase + s->address;
#ifdef __APPLE__
    return blTarget(at) == gameBase + snapshotTarget(s->kind);
#else
    int rel;
    memcpy(&rel, at + 1, 4);
    return at[0] == 0xe8 && at + 5 + rel == gameBase + snapshotTarget(s->kind);
#endif
}

// Points the sites of the kinds in `kinds` (a bit each) at their hooks; the count done.
static int snapshotPoint(unsigned kinds, void *const *stubs) {
    int ok = 0;
    for (int i = 0; i < SNAPSHOT_SITE_COUNT; i++) {
        const SnapshotSite *s = &SNAPSHOT_SITES[i];
        if (!(kinds & (1u << s->kind))) continue;
#ifdef __APPLE__
        uint32_t word = blTo(gameBase + s->address, stubs[s->kind]);
        ok += writeGameCode(gameBase + s->address, &word, 4);
#else
        ok += redirectCall(s->address, snapshotTarget(s->kind), stubs[s->kind], snapshotHook(s->kind), "battle_snapshot",
                           NULL);
#endif
    }
    return ok;
}

// Points every site at its hook, the opens last, and logs battle_snapshot_installed.
static void battleSnapshotInit(void) {
    snapshotFopen = GAME_FN(SnapshotFopenFn, RVA_FOPEN);
    snapshotFclose = GAME_FN(SnapshotFcloseFn, RVA_FCLOSE);
    snapshotFwrite = GAME_FN(SnapshotIoFn, RVA_FWRITE);
    snapshotFread = GAME_FN(SnapshotIoFn, RVA_FREAD);
    if (!checkGameFunction("battle_snapshot_check", "")) {
        writeLog("battle_snapshot_installed", "ok=0 of %d reason=game_code_changed", SNAPSHOT_SITE_COUNT);
        return;
    }
    for (int i = 0; i < SNAPSHOT_SITE_COUNT; i++)
        if (!snapshotSiteIsCall(&SNAPSHOT_SITES[i])) {
            writeLog("battle_snapshot_installed", "ok=0 of %d reason=not_the_expected_call address=0x%x", SNAPSHOT_SITE_COUNT,
                     SNAPSHOT_SITES[i].address);
            return;
        }
    void *stubs[SNAPSHOT_KINDS];
#ifdef __APPLE__
    for (int k = 0; k < SNAPSHOT_KINDS; k++)
        if (!(stubs[k] = newTrampoline(snapshotHook(k)))) {
            writeLog("battle_snapshot_installed", "ok=0 of %d reason=no_padding", SNAPSHOT_SITE_COUNT);
            return;
        }
#else
    unsigned char *page = allocNear(gameBase + RVA_TEXT_SECTION);
    if (!page) {
        writeLog("battle_snapshot_installed", "ok=0 of %d reason=no_near_page", SNAPSHOT_SITE_COUNT);
        return;
    }
    makePagesReadExecute(page, 0x1000);
    for (int k = 0; k < SNAPSHOT_KINDS; k++) stubs[k] = page + 0x20 * k;
#endif
    int count[SNAPSHOT_KINDS] = {0};
    for (int i = 0; i < SNAPSHOT_SITE_COUNT; i++) count[SNAPSHOT_SITES[i].kind]++;
    int writeSide = count[SNAPSHOT_WRITE] + count[SNAPSHOT_CLOSE_WRITE], readSide = count[SNAPSHOT_READ] + count[SNAPSHOT_CLOSE_READ];
    int ok = snapshotPoint(1u << SNAPSHOT_WRITE | 1u << SNAPSHOT_CLOSE_WRITE, stubs);
    if (ok == writeSide) ok += snapshotPoint(1u << SNAPSHOT_OPEN_WRITE, stubs);
    int okRead = snapshotPoint(1u << SNAPSHOT_READ | 1u << SNAPSHOT_CLOSE_READ, stubs);
    if (okRead == readSide) okRead += snapshotPoint(1u << SNAPSHOT_OPEN_READ, stubs);
    writeLog("battle_snapshot_installed", "ok=%d of %d", ok + okRead, SNAPSHOT_SITE_COUNT);
}
