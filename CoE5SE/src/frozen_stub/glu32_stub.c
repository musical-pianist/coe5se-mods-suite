// glu32.dll, the Windows stub: the file the player copies into the game folder once. CoE5 imports
// glu32.dll and doesn't ship one, so Windows loads this file instead of System32's. Every glu32
// export is a one-instruction jump into the real System32 glu32.dll.
//
// This file passes every GLU call through, and starts the stub's steps (stub.h) on the game's main
// thread: it points CoE5.exe's import of SDL2's SDL_SetMainReady at itself, so they run right before
// SDL_main, outside Windows' loader lock (deferToMainReady): the import of any DLL whose name starts
// with SDL (SDL2.dll today, SDL3.dll if an update renames it). If System32's glu32.dll won't load, the
// game can't start, since it imports glu32; every other failure logs loader_inactive and the game
// runs vanilla.

#include <windows.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define LOG_TAG "stub"
#define STARTUP_LOG_PID   // every line ends with pid=
#include "startup_log.h"
#include "approve_dll.h"
#include "auto_enable.h"
#include "coe5config.h"
#include "loader_start.h"
#include "stub.h"
#include "stub_version.h"   // GLU32_VERSION

// For each export: a pointer, filled in from the real glu32.dll, and the export itself, a thunk
// "name: jmp *real_name" that jumps through it.
#define GLU_EXPORT(name, ordinal) \
    void *real_##name; \
    __asm__(".text\n.globl " #name "\n" #name ":\n    jmp *real_" #name "(%rip)\n");
#include "glu32_exports.h"
#undef GLU_EXPORT

// Loads System32's glu32.dll and points every thunk at its export. Returns 0 if it won't load.
static int resolveRealGlu32(void) {
    char path[MAX_PATH];
    GetSystemDirectoryA(path, sizeof path);
    strncat(path, "\\glu32.dll", sizeof path - strlen(path) - 1);
    HMODULE real = LoadLibraryA(path);
    if (!real) {
        writeLog("loader_glu32_missing", "path=%s error=%lu", path, GetLastError());
        return 0;
    }
    int missing = 0;
#define GLU_EXPORT(name, ordinal) \
    real_##name = (void *)GetProcAddress(real, #name); \
    if (!real_##name) missing++;
#include "glu32_exports.h"
#undef GLU_EXPORT
    writeLog("loader_glu32_resolved", "path=%s missing=%d", path, missing);
    return 1;
}

// CoE5.exe's WinMain calls SDL_SetMainReady once, on the main thread, right before SDL_main: before
// any game code, and before the game first looks at the mods. Windows holds its loader lock while it
// runs this DLL's DllMain, and loading DLLs, showing the approval box, or starting plugins under it
// risks a deadlock, so DllMain only points the exe's import slot at startAtMainReady. The patch
// finds the import by name, so it needs no game address. If the exe doesn't import it, the loader
// starts in DllMain instead.
static void (*realSetMainReady)(void);

static void startAtMainReady(void) {
    writeLog("loader_deferred_start", "at=SDL_SetMainReady");
    runLoader(LOADER_STARTED_AT_MAIN_READY, GLU32_VERSION);
    realSetMainReady();
}

// Points the exe's import slot for SDL_SetMainReady, from a DLL whose name starts with SDL (any
// case), at startAtMainReady. The function's name is matched exactly. Returns 0, changing nothing,
// if the exe doesn't import it by name.
static int deferToMainReady(void) {
    unsigned char *base = (unsigned char *)GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_DATA_DIRECTORY imports = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!imports.VirtualAddress) return 0;
    for (IMAGE_IMPORT_DESCRIPTOR *d = (IMAGE_IMPORT_DESCRIPTOR *)(base + imports.VirtualAddress); d->Name; d++) {
        if (!isSdlImportName((const char *)(base + d->Name)) || !d->OriginalFirstThunk) continue;
        IMAGE_THUNK_DATA64 *name = (IMAGE_THUNK_DATA64 *)(base + d->OriginalFirstThunk);
        IMAGE_THUNK_DATA64 *slot = (IMAGE_THUNK_DATA64 *)(base + d->FirstThunk);
        for (; name->u1.AddressOfData; name++, slot++) {
            if (IMAGE_SNAP_BY_ORDINAL64(name->u1.Ordinal)) continue;
            if (strcmp((const char *)((IMAGE_IMPORT_BY_NAME *)(base + name->u1.AddressOfData))->Name, "SDL_SetMainReady")) continue;
            DWORD old;
            if (!VirtualProtect(&slot->u1.Function, sizeof slot->u1.Function, PAGE_READWRITE, &old)) return 0;
            realSetMainReady = (void (*)(void))(uintptr_t)slot->u1.Function;
            slot->u1.Function = (ULONGLONG)(uintptr_t)startAtMainReady;
            VirtualProtect(&slot->u1.Function, sizeof slot->u1.Function, old, &old);
            return 1;
        }
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(dll);
    startStubLog(GLU32_VERSION);
    if (!resolveRealGlu32()) return FALSE;
    int deferred = deferToMainReady();
    writeLog("loader_deferred", "ok=%d%s", deferred, deferred ? " at=SDL_SetMainReady" : " reason=no_sdl_import");
    if (!deferred) runLoader(LOADER_STARTED_IN_DLLMAIN, GLU32_VERSION);
    return TRUE;
}
