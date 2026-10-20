// The offer of a newer stub, on the main menu.
#include <errno.h>
#include "approve_dll.h"
#include "stub_version.h"
#include "stub_hash.h"

#ifdef __APPLE__
#define STUB_FILE            "coe5se.dylib"
#define SHIPPED_STUB_VERSION STUB_VERSION
#define SHIPPED_STUB_SHA256  SHIPPED_MAC_STUB_SHA256
#else
#define STUB_FILE            "glu32.dll"
#define SHIPPED_STUB_VERSION GLU32_VERSION
#define SHIPPED_STUB_SHA256  SHIPPED_GLU32_SHA256
#endif
#define STUB_NEW_FILE     STUB_FILE ".new"
#define STUB_OLD_FILE     STUB_FILE ".old"
#define STUB_OFFERED_FILE "stub_update_offered.txt"

#ifdef _WIN32
#define LAST_ERROR() ((unsigned long)GetLastError())
#else
#define LAST_ERROR() ((unsigned long)errno)
#endif

// The four files, each as a full path.
static int stubUpdatePaths(char shipped[MAX_PATH], char installed[MAX_PATH], char fresh[MAX_PATH], char old[MAX_PATH]) {
    char game[MAX_PATH], coe5se[MAX_PATH];
    HMODULE loader = GetModuleHandleA(LOADER_DLL_NAME);
    if (!loader || !GetModuleFileNameA(loader, coe5se, sizeof coe5se) || !GetModuleFileNameA(NULL, game, sizeof game)) return 0;
    char *slash = strrchr(coe5se, PATH_SEP[0]);
    if (slash) *slash = '\0';
    slash = strrchr(game, PATH_SEP[0]);
    if (slash) *slash = '\0';
    return buildPath(shipped, MAX_PATH, __func__, "%s" PATH_SEP STUB_FILE, coe5se) &&
           buildPath(installed, MAX_PATH, __func__, "%s" PATH_SEP STUB_FILE, game) &&
           buildPath(fresh, MAX_PATH, __func__, "%s" PATH_SEP STUB_NEW_FILE, game) &&
           buildPath(old, MAX_PATH, __func__, "%s" PATH_SEP STUB_OLD_FILE, game);
}

// Writes the file's SHA-256 to `hex`.
static int stubFileSha256(const char *path, char hex[SHA256_HEX_LEN + 1]) {
    HANDLE file = lockDllFile(path);
    int ok = hashFileSha256(file, hex);
    unlockDllFile(file);
    return ok;
}

// Copies `from` to `to`, replacing it, with `from`'s permissions on Mac.
static int copyFileBytes(const char *from, const char *to) {
    static unsigned char buf[65536];
    FILE *in = fopen(from, "rb"), *out = in ? fopen(to, "wb") : NULL;
    int ok = in && out;
    while (ok) {
        size_t n = fread(buf, 1, sizeof buf, in);
        if (n && fwrite(buf, 1, n, out) != n) ok = 0;
        if (n < sizeof buf) {
            if (ferror(in)) ok = 0;
            break;
        }
    }
    if (out && fclose(out)) ok = 0;
    if (in) fclose(in);
#ifdef __APPLE__
    struct stat st;
    if (ok && !stat(from, &st)) chmod(to, st.st_mode & 0777);
#endif
    return ok;
}

// Puts the shipped stub in the game folder.
static const char *installStub(const char *shipped, const char *installed, const char *fresh, const char *old,
                               unsigned long *error) {
    char hash[SHA256_HEX_LEN + 1] = "";
    *error = 0;
    if (!copyFileBytes(shipped, fresh)) {
        *error = LAST_ERROR();
        remove(fresh);
        return "copy_failed";
    }
    if (!stubFileSha256(fresh, hash) || strcmp(hash, SHIPPED_STUB_SHA256)) {
        remove(fresh);
        return "copy_unexpected_hash";
    }
#ifdef _WIN32
    if (!MoveFileExA(installed, old, MOVEFILE_REPLACE_EXISTING)) {
        *error = LAST_ERROR();
        remove(fresh);
        return "rename_running_failed";
    }
    if (!MoveFileExA(fresh, installed, 0)) {
        *error = LAST_ERROR();
        MoveFileExA(old, installed, 0);
        remove(fresh);
        return "rename_new_failed";
    }
#else
    (void)old;
    if (rename(fresh, installed)) {
        *error = LAST_ERROR();
        remove(fresh);
        return "rename_new_failed";
    }
#endif
    return NULL;
}

// 1 if the player was asked about the shipped stub before.
static int stubOfferedBefore(void) {
    char path[MAX_PATH], line[SHA256_HEX_LEN + 2] = "";
    if (!userFileToRead(COE5SE_USER_DIR, STUB_OFFERED_FILE, path, sizeof path)) return 0;
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int offered = fgets(line, sizeof line, f) && !strncmp(line, SHIPPED_STUB_SHA256, SHA256_HEX_LEN);
    fclose(f);
    return offered;
}

// Records that the player was asked about the shipped stub, so it isn't offered again.
static void rememberStubOffered(void) {
    char path[MAX_PATH];
    if (!coe5seUserFile(STUB_OFFERED_FILE, path, sizeof path)) return;
    FILE *f = fopen(path, "w");
    int ok = f && fprintf(f, "%s\nCoE5SE asked about this " STUB_FILE ". Delete this file to be asked again.\n", SHIPPED_STUB_SHA256) > 0;
    if (f && fclose(f)) ok = 0;
    if (!ok) writeLog("stub_update_not_remembered", "path=%s", path);
}

// The offer's text.
#define STUB_OFFER_TEXT \
    STUB_FILE " out of date (not copied over manually).\n" \
    "Update now? The game restarts."

// Restarts the game after an update; the unit tests put a fake here.
static void (*stubUpdateRelaunch)(const char *reason) = relaunchGame;

// Yes: puts the shipped stub in the game folder, then restarts the game, which runs the new stub.
static const char *updateStubAndRestart(const char *shipped, const char *installed, const char *fresh, const char *old) {
    unsigned long error;
    const char *reason = installStub(shipped, installed, fresh, old, &error);
    if (reason) {
        writeLog("stub_update_failed", "reason=%s error=%lu from=%s to=%s", reason, error, shipped, installed);
        return reason;
    }
    writeLog("stub_updated", "file=" STUB_FILE " version=%d path=%s", SHIPPED_STUB_VERSION, installed);
    stubUpdateRelaunch("stub_updated");
    return NULL;
}

// Once per launch: offers the player a newer stub when CoE5SE's release has one.
static void offerStubUpdate(void) {
    char shipped[MAX_PATH], installed[MAX_PATH], fresh[MAX_PATH], old[MAX_PATH], hash[SHA256_HEX_LEN + 1] = "";
    if (!stubUpdatePaths(shipped, installed, fresh, old)) return;
#ifdef _WIN32
    if (GetFileAttributesA(old) != INVALID_FILE_ATTRIBUTES)
        writeLog("stub_old_removed", "path=%s ok=%d", old, remove(old) == 0);
#endif
    HMODULE loader = GetModuleHandleA(LOADER_DLL_NAME);
    uint32_t (*stubVersionOf)(void) = (uint32_t (*)(void))(void *)GetProcAddress(loader, "coe5seStubVersion");
    uint32_t running = stubVersionOf ? stubVersionOf() : 0;
    if (!running || running >= SHIPPED_STUB_VERSION) return;
    if (!stubFileSha256(installed, hash)) {
        writeLog("stub_update_skipped", "reason=no_stub_in_game_folder path=%s", installed);
        return;
    }
    if (!strcmp(hash, SHIPPED_STUB_SHA256)) return;
    if (!stubFileSha256(shipped, hash) || strcmp(hash, SHIPPED_STUB_SHA256)) {
        writeLog("stub_update_skipped", "reason=unexpected_hash path=%s sha256=%s expected=" SHIPPED_STUB_SHA256, shipped,
                 hash[0] ? hash : "-");
        return;
    }
    if (stubOfferedBefore()) {
        writeLog("stub_update_skipped", "reason=asked_before file=" STUB_FILE " running_version=%u", running);
        return;
    }
    writeLog("stub_update_offered", "file=" STUB_FILE " running_version=%u version=%d", running, SHIPPED_STUB_VERSION);
    int choice = messageBox("CoE5SE update", STUB_OFFER_TEXT, "Yes", "No", 0, 0, 0);
    writeLog("stub_update_choice", "choice=%s", choice == 1 ? "yes" : "no");
    rememberStubOffered();
    if (choice != 1) return;
    if (!updateStubAndRestart(shipped, installed, fresh, old)) return;
    static char text[POPUP_TEXT_SIZE];
    snprintf(text, sizeof text,
             "CoE5SE couldn't update " STUB_FILE ".\n\n"
             "To update it by hand, close the game and copy\n%.200s\n"
             "over\n%.200s",
             shipped, installed);
    okBox("CoE5SE update", text);
}
