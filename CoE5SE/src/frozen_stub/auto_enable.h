// Turning the CoE5SE mod on once, for a player who copied the stub and never ticked CoE5SE.

#pragma once
#include <stdio.h>

#define AUTO_ENABLE_DONE_FILE   "auto_enable_done.txt"
#define AUTO_ENABLE_NOTICE_FILE "auto_enable_notice.txt"
#define AUTO_ENABLE_BACKUP_FILE "coe5config_before_auto_enable"
// What each file says, for a player who finds it.
#define AUTO_ENABLE_DONE_TEXT   "The CoE5SE stub found CoE5SE enabled, or turned it on. While this file exists it never turns it on again."
#define AUTO_ENABLE_NOTICE_TEXT "The CoE5SE stub turned CoE5SE on. CoE5SE tells the player on the main menu, then deletes this file."

// 1 if `path` exists.
static __attribute__((unused)) int autoEnableFileExists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

// Creates `path` with one line saying what it is. Returns 0 if the line didn't reach the file.
static __attribute__((unused)) int writeAutoEnableFile(const char *path, const char *line) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    int ok = fprintf(f, "%s\n", line) > 0;
    return fclose(f) == 0 && ok;
}
