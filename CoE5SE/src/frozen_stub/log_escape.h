// How coe5mod.log writes a byte that would break a line.
#pragma once
#include <stddef.h>

// How many bytes `c` takes in the log.
static __attribute__((unused)) size_t logByteSize(unsigned char c) {
    if (c >= 0x20 && c != 0x7f) return 1;
    return c == '\n' || c == '\r' || c == '\t' ? 2 : 4;
}

// What logByteSize adds over text[0..length).
static __attribute__((unused)) size_t logEscapeGrowth(const char *text, size_t length) {
    size_t extra = 0;
    for (size_t i = 0; i < length; i++) extra += logByteSize((unsigned char)text[i]) - 1;
    return extra;
}

// Escapes text[0..length) in place, from the back.
static __attribute__((unused)) void escapeLogText(char *text, size_t length, size_t extra) {
    static const char hex[] = "0123456789abcdef";
    for (size_t i = length, j = length + extra; i != j;) {
        unsigned char c = (unsigned char)text[--i];
        size_t size = logByteSize(c);
        if (size == 1) {
            text[--j] = (char)c;
        } else if (size == 2) {
            text[--j] = c == '\n' ? 'n' : c == '\r' ? 'r' : 't';
            text[--j] = '\\';
        } else {
            text[--j] = hex[c & 15];
            text[--j] = hex[c >> 4];
            text[--j] = 'x';
            text[--j] = '\\';
        }
    }
}
