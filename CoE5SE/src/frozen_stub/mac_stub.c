// coe5se.dylib, the Mac stub and the counterpart of glu32.dll (glu32_stub.c): the file the player
// copies into the game folder once. The game loads it through CoE5's Steam launch options,
//   /usr/bin/env DYLD_INSERT_LIBRARIES=coe5se.dylib %command%
// and dyld runs its constructor before the game's main, which runs the stub's steps (stub.h).

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

// Runs when dyld inserts this library, before the game's main.
__attribute__((constructor)) static void stubStart(void) {
    startStubLog(STUB_VERSION);
    runLoader(LOADER_STARTED_IN_CONSTRUCTOR, STUB_VERSION);
}
