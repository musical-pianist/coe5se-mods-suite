// The part of MinHook's interface the fan patch uses, given by CoE5SE's entry hooks.
#pragma once

#include "../../fanpatch_bridge.h"

typedef enum { MH_OK = 0, MH_ERROR_NOT_EXECUTABLE, MH_ERROR_UNSUPPORTED_FUNCTION } MH_STATUS;

static inline MH_STATUS MH_CreateHook(void *target, void *detour, void **original)
{
    return fanpatchHookCreate(target, detour, original) ? MH_OK : MH_ERROR_UNSUPPORTED_FUNCTION;
}
static inline MH_STATUS MH_EnableHook(void *target)  { return fanpatchHookEnable(target, 1) ? MH_OK : MH_ERROR_NOT_EXECUTABLE; }
static inline MH_STATUS MH_DisableHook(void *target) { return fanpatchHookEnable(target, 0) ? MH_OK : MH_ERROR_NOT_EXECUTABLE; }
static inline MH_STATUS MH_RemoveHook(void *target)  { return fanpatchHookRemove(target) ? MH_OK : MH_ERROR_NOT_EXECUTABLE; }
static inline const char *MH_StatusToString(MH_STATUS status)
{
    return status == MH_OK ? "MH_OK" : status == MH_ERROR_NOT_EXECUTABLE ? "MH_ERROR_NOT_EXECUTABLE" : "MH_ERROR_UNSUPPORTED_FUNCTION";
}
