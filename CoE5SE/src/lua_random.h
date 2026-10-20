// The randomness behind every script's Lua state, from the system's random source.
#pragma once
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <bcrypt.h>
#else
#include <stdlib.h>
#endif

// The 64-bit finalizer of MurmurHash3: every input bit changes about half of the output bits.
static uint64_t mixLuaHash(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdull;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ull;
    return x ^ (x >> 33);
}

// A private build of CoE5SE may give the random bytes itself.
static void fillLuaRandom(void *buf, size_t n) {
#ifdef COE5SE_LUA_RANDOM_HOOK
    if (COE5SE_LUA_RANDOM_HOOK(buf, n)) return;
#endif
#ifdef _WIN32
    if (BCRYPT_SUCCESS(BCryptGenRandom(NULL, buf, (ULONG)n, BCRYPT_USE_SYSTEM_PREFERRED_RNG))) return;
    static uint64_t calls;
    static int logged;
    if (!logged++) writeLog("lua_random_fallback", "error=bcrypt_gen_random_failed");
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    for (size_t i = 0; i < n; i += sizeof(uint64_t)) {
        uint64_t x = mixLuaHash((uint64_t)now.QuadPart ^ (++calls << 32) ^ GetCurrentProcessId());
        memcpy((char *)buf + i, &x, n - i < sizeof x ? n - i : sizeof x);
    }
#else
    arc4random_buf(buf, n);
#endif
}

// luai_makeseed.
unsigned coe5seMakeLuaSeed(void) {
    unsigned seed;
    fillLuaRandom(&seed, sizeof seed);
    return seed;
}

// l_hashfloat.
unsigned coe5seHashLuaFloat(LUA_NUMBER n) {
    static uint64_t keys[2];
    static int haveKeys;
    if (!haveKeys) {
        fillLuaRandom(keys, sizeof keys);
        haveKeys = 1;
    }
    if (n == 0) n = 0;
    uint64_t bits;
    memcpy(&bits, &n, sizeof bits);
    uint64_t h = mixLuaHash(mixLuaHash(bits ^ keys[0]) ^ keys[1]);
    return (unsigned)(h ^ (h >> 32));
}
