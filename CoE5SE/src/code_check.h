// Checks that the game's own code is still what CoE5SE's copies of it were made from.
#define GAME_ADDRESSES_CODE_TABLE
#include "game_addresses.h"

#define CODE_ARCH_X64   0
#define CODE_ARCH_ARM64 1
#ifdef __aarch64__
#define CODE_ARCH       CODE_ARCH_ARM64
#define GAME_LINK_BASE  0x100000000ULL
#else
#define CODE_ARCH       CODE_ARCH_X64
#define GAME_LINK_BASE  0x140000000ULL
#endif

typedef struct {
    unsigned int offset;
    const char *reason;
    unsigned long long found, expected;
} CodeDifference;

// The RVA the call at code[offset] goes to.
static int codeCallTarget(const unsigned char *code, size_t size, unsigned int rva, unsigned int offset, int arch,
                          unsigned long long *target) {
    if (arch == CODE_ARCH_X64) {
        if (offset >= size || size - offset < 5 || code[offset] != 0xe8) return 0;
        uint32_t raw = (uint32_t)code[offset + 1] | (uint32_t)code[offset + 2] << 8 | (uint32_t)code[offset + 3] << 16 |
                       (uint32_t)code[offset + 4] << 24;
        *target = (unsigned long long)((long long)rva + offset + 5 + (int32_t)raw);
        return 1;
    }
    if (offset >= size || size - offset < 4 || offset % 4) return 0;
    uint32_t word = (uint32_t)code[offset] | (uint32_t)code[offset + 1] << 8 | (uint32_t)code[offset + 2] << 16 |
                    (uint32_t)code[offset + 3] << 24;
    if ((word & 0xfc000000u) != 0x94000000u) return 0;
    long long imm = (long long)(word & 0x3ffffffu);
    if (imm & 0x2000000) imm -= 0x4000000;
    *target = (unsigned long long)((long long)rva + offset + imm * 4);
    return 1;
}

static int codeHexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// Keeps the difference at the lowest offset.
static void codeDifferenceAt(CodeDifference *d, unsigned int offset, const char *reason, unsigned long long found,
                             unsigned long long expected) {
    if (d->reason && d->offset <= offset) return;
    d->offset = offset;
    d->reason = reason;
    d->found = found;
    d->expected = expected;
}

// Compares the game's code at rva with a byte pattern, and checks where each call goes.
static int checkCode(const unsigned char *code, size_t size, unsigned int rva, const char *pattern,
                     const CodeCall *calls, int callCount, int arch, CodeDifference *d) {
    memset(d, 0, sizeof *d);
    size_t n = 0;
    for (const char *p = pattern;; n++) {
        while (*p == ' ') p++;
        if (!*p) break;
        int wild = p[0] == '?' && p[1] == '?';
        int hi = codeHexDigit(p[0]), lo = hi < 0 ? -1 : codeHexDigit(p[1]);
        if (!wild && lo < 0) {
            codeDifferenceAt(d, (unsigned int)n, "bad_pattern", 0, 0);
            return 0;
        }
        if (!wild && n < size && code[n] != (unsigned char)(hi << 4 | lo))
            codeDifferenceAt(d, (unsigned int)n, "byte", code[n], (unsigned long long)(hi << 4 | lo));
        p += 2;
    }
    if (n != size) codeDifferenceAt(d, (unsigned int)(n < size ? n : size), "size", size, n);
    for (int i = 0; i < callCount; i++) {
        unsigned long long target;
        if (!codeCallTarget(code, size, rva, calls[i].offset, arch, &target))
            codeDifferenceAt(d, calls[i].offset, "not_a_call", 0, calls[i].targetRva);
        else if (target != calls[i].targetRva)
            codeDifferenceAt(d, calls[i].offset, "call_target", target, calls[i].targetRva);
    }
    return d->reason == NULL;
}

// Checks the game's function at rva..end against its row of the code table and logs `event`.
static int checkGameFunction(const char *event, const char *head, unsigned int rva, unsigned int end, const char *code,
                             const CodeCall *calls, int callCount) {
    if (!gameBuild) {
        writeLog(event, "%schecked=0 reason=unknown_game_build", head);
        return 0;
    }
    size_t size = end - rva;
    CodeDifference d;
    if (checkCode(GAME_PTR(rva), size, rva, code, calls, callCount, CODE_ARCH, &d)) {
        writeLog(event, "%smatch=1 bytes=%zu calls=%d", head, size, callCount);
        return 1;
    }
    writeLog(event, "%smatch=0 offset=0x%x va=0x%llx reason=%s found=0x%llx expected=0x%llx", head, d.offset,
             GAME_LINK_BASE + rva + d.offset, d.reason, d.found, d.expected);
    return 0;
}

// The startup check of name_box, logged as name_box_check.
static void checkNameBoxCode(void) {
    checkGameFunction("name_box_check", "", RVA_NAME_BOX, RVA_NAME_BOX_END, NAME_BOX_CODE, NAME_BOX_CALLS,
                      (int)(sizeof NAME_BOX_CALLS / sizeof NAME_BOX_CALLS[0]));
}

// The startup check of the event triggers' two finders, one event_finders_check line each.
static int checkEventFinderCode(void) {
    int unit = checkGameFunction("event_finders_check", "function=find_unit_at ", RVA_FIND_UNIT_AT, RVA_FIND_UNIT_AT_END,
                                 FIND_UNIT_AT_CODE, FIND_UNIT_AT_CALLS, (int)(sizeof FIND_UNIT_AT_CALLS / sizeof FIND_UNIT_AT_CALLS[0]));
    int item = checkGameFunction("event_finders_check", "function=find_item_holder_at ", RVA_FIND_ITEM_HOLDER_AT,
                                 RVA_FIND_ITEM_HOLDER_AT_END, FIND_ITEM_HOLDER_AT_CODE, FIND_ITEM_HOLDER_AT_CALLS,
                                 (int)(sizeof FIND_ITEM_HOLDER_AT_CALLS / sizeof FIND_ITEM_HOLDER_AT_CALLS[0]));
    return unit && item;
}
