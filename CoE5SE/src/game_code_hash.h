// The SHA-256 of the game's code section in memory, the check every patch of game code stands on.
#pragma once
#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
#include <mach-o/loader.h>
#else
#include <bcrypt.h>
#endif

#define GAME_CODE_SHA256_HEX_LEN 64

// The game's code section: its start in memory and its size.
static int findGameCode(const unsigned char *image, const unsigned char **code, size_t *size) {
#ifdef __APPLE__
    const struct mach_header_64 *header = (const struct mach_header_64 *)image;
    const struct load_command *lc = (const struct load_command *)(header + 1);
    for (uint32_t i = 0; i < header->ncmds; i++, lc = (const void *)((const char *)lc + lc->cmdsize)) {
        if (lc->cmd != LC_SEGMENT_64) continue;
        const struct segment_command_64 *seg = (const struct segment_command_64 *)lc;
        if (strncmp(seg->segname, "__TEXT", sizeof seg->segname)) continue;
        const struct section_64 *sect = (const struct section_64 *)(seg + 1);
        for (uint32_t k = 0; k < seg->nsects; k++, sect++) {
            if (strncmp(sect->sectname, "__text", sizeof sect->sectname)) continue;
            *code = image + (sect->addr - seg->vmaddr);
            *size = (size_t)sect->size;
            return 1;
        }
    }
    return 0;
#else
    const IMAGE_NT_HEADERS64 *nt = (const IMAGE_NT_HEADERS64 *)(image + ((const IMAGE_DOS_HEADER *)image)->e_lfanew);
    const IMAGE_SECTION_HEADER *sect = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, sect++) {
        if (strncmp((const char *)sect->Name, ".text", sizeof sect->Name)) continue;
        *code = image + sect->VirtualAddress;
        *size = sect->Misc.VirtualSize;
        return 1;
    }
    return 0;
#endif
}

// Writes the SHA-256 of `size` bytes at `data` to `hex` as lowercase hex.
static int hashMemorySha256(const void *data, size_t size, char hex[GAME_CODE_SHA256_HEX_LEN + 1]) {
    unsigned char digest[32];
#ifdef __APPLE__
    CC_SHA256(data, (CC_LONG)size, digest);
    int ok = 1;
#else
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_HASH_HANDLE hash = NULL;
    int ok = size <= 0xffffffffu && BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0)) &&
             BCRYPT_SUCCESS(BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0)) &&
             BCRYPT_SUCCESS(BCryptHashData(hash, (PUCHAR)(uintptr_t)data, (ULONG)size, 0)) &&
             BCRYPT_SUCCESS(BCryptFinishHash(hash, digest, sizeof digest, 0));
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
#endif
    for (int i = 0; ok && i < 32; i++) snprintf(hex + 2 * i, 3, "%02x", digest[i]);
    return ok;
}
