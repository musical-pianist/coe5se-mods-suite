// Locks a plugin DLL, hashes it, and checks the hash against approved.txt.
#pragma once
#ifdef _WIN32
#include <bcrypt.h>
#else
#include <CommonCrypto/CommonDigest.h>
#include <CoreFoundation/CoreFoundation.h>
#include <fcntl.h>
#endif
#include "approvals.h"

#ifdef _WIN32
// Opens a DLL so nobody can change, replace, or delete it while the handle is open.
static __attribute__((unused)) HANDLE lockDllFile(const char *path) {
    return CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

static __attribute__((unused)) void unlockDllFile(HANDLE file) {
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
}

// Writes the open file's SHA-256 to `hex` as lowercase hex.
static __attribute__((unused)) int hashFileSha256(HANDLE file, char *hex) {
    static unsigned char buf[65536];
    unsigned char digest[32];
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    if (file == INVALID_HANDLE_VALUE || SetFilePointer(file, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) return 0;
    int ok = BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0)) &&
             BCRYPT_SUCCESS(BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0));
    DWORD n;
    while (ok && (ok = ReadFile(file, buf, sizeof buf, &n, NULL)) && n > 0)
        ok = BCRYPT_SUCCESS(BCryptHashData(hash, buf, n, 0));
    ok = ok && BCRYPT_SUCCESS(BCryptFinishHash(hash, digest, sizeof digest, 0));
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    for (int i = 0; ok && i < 32; i++) snprintf(hex + 2 * i, 3, "%02x", digest[i]);
    return ok;
}
#else
// macOS can't lock a file against writers, so the handle is a plain read-only descriptor.
static __attribute__((unused)) HANDLE lockDllFile(const char *path) {
    int fd = open(path, O_RDONLY);
    return fd < 0 ? INVALID_HANDLE_VALUE : (HANDLE)(intptr_t)fd;
}

static __attribute__((unused)) void unlockDllFile(HANDLE file) {
    if (file != INVALID_HANDLE_VALUE) close((int)(intptr_t)file);
}

// Writes the open file's SHA-256 to `hex` as lowercase hex.
static __attribute__((unused)) int hashFileSha256(HANDLE file, char *hex) {
    static unsigned char buf[65536];
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    int fd = (int)(intptr_t)file;
    if (file == INVALID_HANDLE_VALUE || lseek(fd, 0, SEEK_SET) != 0) return 0;
    CC_SHA256_CTX ctx;
    CC_SHA256_Init(&ctx);
    ssize_t n;
    while ((n = read(fd, buf, sizeof buf)) > 0) CC_SHA256_Update(&ctx, buf, (CC_LONG)n);
    if (n < 0) return 0;
    CC_SHA256_Final(digest, &ctx);
    for (int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) snprintf(hex + 2 * i, 3, "%02x", digest[i]);
    return 1;
}
#endif

// Asks the player whether to load `path`, a DLL whose hash isn't approved.
static __attribute__((unused)) int askApproval(const char *mod, const char *path, const char *hash, int isCoe5se) {
    char text[2048];
    const char *file = strrchr(path, '/');
    if (!file) file = strrchr(path, '\\');
    (void)hash;
    snprintf(text, sizeof text,
             "New or updated plugin: %s (%s)\nLoad?%s",
             mod, file ? file + 1 : path,
             isCoe5se ? "" : "\n\nApproval also in Mod Settings, CoE5SE.");
#define APPROVAL_PROMPT_TITLE "CoE5SE: Approve plugin?"
#ifdef _WIN32
    return MessageBoxA(NULL, text, APPROVAL_PROMPT_TITLE,
                       MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_TOPMOST | MB_SETFOREGROUND) == IDYES;
#else
    CFStringRef message = CFStringCreateWithCString(NULL, text, kCFStringEncodingUTF8);
    CFOptionFlags response = kCFUserNotificationDefaultResponse;
    SInt32 failed = CFUserNotificationDisplayAlert(0, kCFUserNotificationCautionAlertLevel, NULL, NULL, NULL,
                                                   CFSTR(APPROVAL_PROMPT_TITLE), message, CFSTR("No"), CFSTR("Yes"), NULL, &response);
    if (message) CFRelease(message);
    return !failed && response == kCFUserNotificationAlternateResponse;
#endif
}

// Checks the DLL `name`'s hash: approved, or approved now by auto-approve or the player.
static __attribute__((unused)) int approveDll(const char *name, const char *mod, const char *source, const char *path, const char *hash,
                      int autoApprove, int isCoe5se) {
    if (isHashApproved(hash)) return 1;
    const char *via = autoApprove ? "auto" : askApproval(mod, path, hash, isCoe5se) ? "prompt" : NULL;
    if (!via) {
        writeLog("loader_plugin_skipped", "name=%s source=%s reason=not_approved sha256=%s", name, source, hash);
        return 0;
    }
    if (autoApprove) writeLog("loader_plugin_auto_approved", "name=%s source=%s sha256=%s", name, source, hash);
    else writeLog("loader_plugin_approved", "name=%s source=%s sha256=%s via=prompt", name, source, hash);
    if (!approveHash(hash, name)) writeLog("loader_approval_not_saved", "name=%s reason=write_failed", name);
    return 1;
}

// Loads the DLL at `path`.
static __attribute__((unused)) void *loadDllFile(const char *path, char *error, size_t errorSize) {
#ifdef _WIN32
    DWORD oldMode = 0;
    SetThreadErrorMode(SEM_FAILCRITICALERRORS, &oldMode);
    HMODULE module = LoadLibraryExA(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    DWORD code = GetLastError();
    SetThreadErrorMode(oldMode, NULL);
    if (!module) snprintf(error, errorSize, "%lu", code);
    return (void *)module;
#else
    void *module = dlopen(path, RTLD_NOW);
    const char *text = module ? NULL : dlerror();
    if (!module) snprintf(error, errorSize, "%s", text ? text : "?");
    return module;
#endif
}
