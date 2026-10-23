// coe5se.dylib, the Mac stub and the counterpart of glu32.dll (glu32_stub.c): the file the player
#define PLATFORM_NO_DLLMAIN
#include "platform_mac.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LOG_TAG "stub"
#define STARTUP_LOG_PID   // every line ends with pid=
#include "startup_log.h"
#include "approve_dll.h"
#include "auto_enable.h"
#include "coe5config.h"
#include "loader_start.h"
#include "stub.h"
#include "stub_version.h"   // STUB_VERSION

// coe5_mac's main calls srand once, right after it reads /dev/urandom for the seed: before any game
static void startAtSrand(unsigned int seed) {
    static int started;
    if (!started) {
        started = 1;
        writeLog("loader_deferred_start", "at=srand");
        runLoader(LOADER_STARTED_AT_SRAND, STUB_VERSION);
    }
    srand(seed);
}

// Runs when dyld inserts this library, before the game's main.
__attribute__((constructor)) static void stubStart(void) {
    startStubLog(STUB_VERSION);
    int deferred = pointGameImportAt("_srand", (void *)startAtSrand, NULL) > 0;
    writeLog("loader_deferred", "ok=%d%s", deferred, deferred ? " at=srand" : " reason=no_srand_import");
    if (!deferred) runLoader(LOADER_STARTED_IN_CONSTRUCTOR, STUB_VERSION);
}
