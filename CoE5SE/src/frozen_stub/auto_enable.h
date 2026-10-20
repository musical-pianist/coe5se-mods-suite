// Turning the CoE5SE mod on once, for a player who copied the stub and never ticked CoE5SE. The
// stub runs before the game reads coe5config, so a name it adds there is enabled on that same
// launch, with no restart. Its files, in CoE5SE's user folder, are below: the done file keeps it to
// once, so a player who unticks CoE5SE keeps it off. The stub writes the done file before it changes
// coe5config and deletes it only when the change failed, so a crash or a full disk part way leaves
// it marked: a later launch never turns CoE5SE on after the player turned it off.

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
