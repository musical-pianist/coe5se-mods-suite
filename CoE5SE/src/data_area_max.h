// maxdata <megabytes>: a mod's .c5se sets its own data area's cap, 1 to 100 MB.
#pragma once
#include "mod_storage.h"

#define MAXDATA_WORD "maxdata"

typedef struct {
    char mod[MOD_NAME_MAX];
    int megabytes;
} DataAreaMax;

static DataAreaMax *dataAreaMaxes;
static int dataAreaMaxCount, dataAreaMaxCapacity;

// The level-2 reading going on: whose, and what its sidecar declared so far.
static struct {
    int mod;
    int lines;
    int megabytes;
} maxdataReading = {-1, 0, 0};

// The cap of mod `mod`'s area in MB: its latest reading's maxdata, or the default.
static int storageAreaMegabytes(const char *mod) {
    for (int i = 0; i < dataAreaMaxCount; i++)
        if (!_stricmp(dataAreaMaxes[i].mod, mod)) return dataAreaMaxes[i].megabytes;
    return STORAGE_AREA_DEFAULT_MB;
}

// The same in bytes.
static long long storageAreaMax(const char *mod) {
    return (long long)storageAreaMegabytes(mod) * 1024 * 1024;
}

// Keeps `megabytes` as mod `mod`'s cap (0: the default) and logs data_area_max when it changed.
static void setStorageAreaMegabytes(const char *mod, int megabytes) {
    int i = 0, was = storageAreaMegabytes(mod), now = megabytes ? megabytes : STORAGE_AREA_DEFAULT_MB;
    while (i < dataAreaMaxCount && _stricmp(dataAreaMaxes[i].mod, mod)) i++;
    if (now == STORAGE_AREA_DEFAULT_MB) {
        if (i < dataAreaMaxCount) dataAreaMaxes[i] = dataAreaMaxes[--dataAreaMaxCount];
    } else if (i < dataAreaMaxCount) {
        dataAreaMaxes[i].megabytes = now;
    } else if (growList((void **)&dataAreaMaxes, &dataAreaMaxCapacity, dataAreaMaxCount + 1, sizeof *dataAreaMaxes)) {
        snprintf(dataAreaMaxes[dataAreaMaxCount].mod, sizeof dataAreaMaxes[0].mod, "%s", mod);
        dataAreaMaxes[dataAreaMaxCount++].megabytes = now;
    } else {
        writeLog("data_area_max_dropped", "mod=%s mb=%d reason=out_of_memory", mod, now);
        return;
    }
    if (now != was) writeLog("data_area_max", "mod=%s mb=%d", mod, now);
}

// Called around the reading of mod `mod`'s .c5m and sidecar at level 2.
static void maxdataReadingAt(int mod, int start) {
    if (start == 1) {
        maxdataReading.mod = mod;
        maxdataReading.lines = maxdataReading.megabytes = 0;
        return;
    }
    if (start < 0) {
        maxdataReading.mod = -1;
        return;
    }
    setStorageAreaMegabytes(modName(mod), maxdataReading.mod == mod ? maxdataReading.megabytes : 0);
    maxdataReading.mod = -1;
}

// Parses maxdata's argument into *megabytes, or returns the reason word it can't.
static const char *maxdataProblem(const char *args, int *megabytes) {
    const char *p = args;
    skipSpaces(&p);
    int negative = *p == '-';
    if (negative) p++;
    if (*p < '0' || *p > '9') return "not_a_number";
    long value = 0;
    for (; *p >= '0' && *p <= '9'; p++)
        if (value <= STORAGE_AREA_CEILING_MB) value = value * 10 + (*p - '0');
    if (!isLineEnd(p)) return "not_a_number";
    if (negative || value < 1 || value > STORAGE_AREA_CEILING_MB) return "out_of_range";
    *megabytes = (int)value;
    return NULL;
}

// maxdata <megabytes>, at level 2 in the sidecar: the mod's area cap.
static void maxdataLine(const char *args, int mod, int level) {
    if (level != 2) return;
    const char *name = spellModName(mod);
    int line = G32(RVA_MOD_LINE), megabytes = 0;
    if (!readingSidecarOf(mod) && !readingPrefixedLine()) {
        refuseC5mLine(MAXDATA_WORD, name, "maxdata_in_c5m", "line=%d", line);
        return;
    }
    if (maxdataReading.mod != mod) return;
    if (++maxdataReading.lines > 1) {
        refuseC5mLine(MAXDATA_WORD, name, "duplicate_line", "line=%d", line);
        return;
    }
    const char *problem = maxdataProblem(args, &megabytes);
    if (problem) {
        refuseC5mLine(MAXDATA_WORD, name, problem, "line=%d text=%.40s min=1 max=%d", line, args, STORAGE_AREA_CEILING_MB);
        return;
    }
    maxdataReading.megabytes = megabytes;
}

// Adds maxdata.
static void maxdataWordInit(void) {
    int ok = apiAddModCommand("maxdata", maxdataLine);
    writeLog("maxdata_word_installed", "ok=%d", ok);
}
