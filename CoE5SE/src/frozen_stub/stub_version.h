// The stubs' versions, each logged as loader_version on every launch's first line (stub.h). Raise
// one with any change to its stub's bytes or a header it includes: players copy the stub once, so
// a new one means every player copies it again, or says Yes when CoE5SE offers it (stub_update.h).
// CoE5SE reads them as the versions its release ships.

#pragma once

#define GLU32_VERSION 8   // glu32.dll, the Windows stub
#define STUB_VERSION 9    // coe5se.dylib, the Mac stub
