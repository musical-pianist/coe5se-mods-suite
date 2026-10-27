// ---- The commander panel: scripts' lines --------------------------------------------------------
// Scripts' lines at the end of the selected commander's order column on the map.
#define COMMANDER_LINE_FIRST_ID  2000
#define COMMANDER_LINE_LAST_ID   9997
#define COMMANDER_PREVIOUS_ID    9998
#define COMMANDER_NEXT_ID        9999
#define COMMANDER_LINE_LABEL_MAX 63
#define COMMANDER_LINE_TIP_MAX   255
#define COMMANDER_LINE_NO_ICON   -1
#define COMMANDER_PAGE_LINES     16
#define COMMANDER_BOX_ROWS       4
#define COMMANDER_BOTTOM_ROWS    1
#define COMMANDER_TEXT_SIZE      13.2f
#define COMMANDER_ROW_STEP       1.1f
#define COMMANDER_ICON_SIZE      0.75f
#define COMMANDER_ICON_INSET     5.0f
#define COMMANDER_LABEL_INSET    15.0f
#define COMMANDER_ICON_RAISE     1.0f
typedef struct {
    char label[COMMANDER_LINE_LABEL_MAX + 1], tip[COMMANDER_LINE_TIP_MAX + 1];
    int icon;
    int lua;
    int coloured;
    float colour[4];
} CommanderLine;
static CommanderLine *commanderLines;
static int commanderLineCount, commanderLineCapacity;
static int commanderLineSite;

// The id of commander line `index`, or -1 past the map's free ids.
static int commanderLineId(int index) {
    if (index < 0 || index > COMMANDER_LINE_LAST_ID - COMMANDER_LINE_FIRST_ID) return -1;
    return COMMANDER_LINE_FIRST_ID + index;
}

// The index of the commander line with this id, or -1 for an id that is none of them.
static int commanderLineIndex(int id) {
    if (id < COMMANDER_LINE_FIRST_ID || id - COMMANDER_LINE_FIRST_ID >= commanderLineCount) return -1;
    return id - COMMANDER_LINE_FIRST_ID;
}

// One line of the column at (x, y), drawn as the game draws its own.
static void drawCommanderLine(const CommanderLine *line, int id, float x, float y, float scale) {
    float size = COMMANDER_TEXT_SIZE * scale, iconX = x + COMMANDER_ICON_INSET;
    if (line->icon != COMMANDER_LINE_NO_ICON) {
#ifndef __APPLE__
        GAME_FN(void (*)(int, int, int, int, float, float, float, float), RVA_ICON_BUTTON_AT)(
            id, SPRITE_KIND_ICON, line->icon, PLANE_ICON_FLAGS, iconX, y + COMMANDER_ICON_RAISE, COMMANDER_ICON_SIZE * size,
            COMMANDER_ICON_SIZE * size);
#else
        // @platform-split icon_button_at: Mac's fixed-place icon has the kind and flags built in
        GAME_FN(void (*)(int, int, float, float, float, float), RVA_ICON_BUTTON_AT)(
            id, line->icon, iconX, y + COMMANDER_ICON_RAISE, COMMANDER_ICON_SIZE * size, COMMANDER_ICON_SIZE * size);
#endif
    }
    float *textColour = (float *)GAME_PTR(RVA_TEXT_COLOUR), saved[4] = {0};
    if (line->coloured) memcpy(saved, textColour, sizeof saved), memcpy(textColour, line->colour, sizeof saved);
    GAME_FN(void (*)(int, float, float, float, const char *, ...), RVA_DRAW_BUTTON_PLAIN)(
        id, iconX + COMMANDER_LABEL_INSET * scale, y, size, "%s", line->label);
    if (line->coloured) memcpy(textColour, saved, sizeof saved);
    if (line->tip[0]) GAME_FN(GameTextFn, RVA_TEXT)("%s", line->tip);
}

// Which shown lines a page draws.
typedef struct {
    int first, count;
    int page, pages;
} CommanderPage;

// The page `page` of `shown` lines when `fit` lines fit under the column (none below 0).
static CommanderPage commanderPageLayout(int shown, int fit, int page) {
    CommanderPage p = {0, shown, 0, 1};
    int size = fit < COMMANDER_PAGE_LINES ? fit : COMMANDER_PAGE_LINES;
    if (size < 0) size = 0;
    if (shown <= size) return p;
    if (fit - COMMANDER_BOX_ROWS < size) size = fit - COMMANDER_BOX_ROWS;
    if (size < 1) size = 1;
    p.pages = (shown + size - 1) / size;
    p.page = page < 0 ? 0 : page >= p.pages ? p.pages - 1 : page;
    p.first = p.page * size;
    p.count = shown - p.first < size ? shown - p.first : size;
    return p;
}

static int *shownLines, shownLineCapacity;
static int commanderPage, commanderPageFor = -1, commanderPagesShown = 1;

// The shown lines' count, each one's index in shownLines.
static int shownCommanderLines(void) {
    if (!growList((void **)&shownLines, &shownLineCapacity, commanderLineCount, sizeof shownLines[0])) return 0;   // No log: out of memory draws no line this frame
    int n = 0;
    for (int i = 0; i < commanderLineCount; i++)
        if (runLuaVisible(commanderLines[i].lua)) shownLines[n++] = i;
    return n;
}

// One line of the page box: a label at the column's labels' x.
static void drawCommanderBoxLine(int id, const char *label, float x, float y, float scale) {
    GAME_FN(void (*)(int, float, float, float, const char *, ...), RVA_DRAW_BUTTON_PLAIN)(
        id, x + COMMANDER_ICON_INSET + COMMANDER_LABEL_INSET * scale, y, COMMANDER_TEXT_SIZE * scale, "%s", label);
}

// The panel's frame one line lower and taller for each shown line, then the lines in that room.
#ifndef __APPLE__
typedef int (*PanelFrameFn)(int, float, float, float, float, float, float, float, float, float, float, float, const char *, ...);
static int commanderPanelFrameHook(int id, float x, float y, float w, float h, float a5, float a6, float a7, float a8, float a9,
                                   float a10, float a11, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const char *dataDir = va_arg(args, const char *);
    va_end(args);
#else
typedef int (*PanelFrameFn)(int, float, float, float, float, float, float, float, float, float, float, float, long, long, long, long,
                            long, long, long, const char *);
static int commanderPanelFrameHook(int id, float x, float y, float w, float h, float a5, float a6, float a7, float a8, float a9,
                                   float a10, float a11, long r1, long r2, long r3, long r4, long r5, long r6, long r7,
                                   const char *dataDir) {
#endif
    int selected = G32(RVA_SELECTED_COMMANDER);
    if (selected != commanderPageFor) commanderPage = 0, commanderPageFor = selected;
    int shown = commanderLineCount ? shownCommanderLines() : 0;
    float scale = *(const float *)GAME_PTR(RVA_UI_TEXT_SCALE), step = COMMANDER_ROW_STEP * COMMANDER_TEXT_SIZE * scale;
    int fit = (int)floorf((y - step * (0.5f + COMMANDER_BOTTOM_ROWS)) / step) + 1;
    CommanderPage p = commanderPageLayout(shown, fit, commanderPage);
    commanderPage = p.page, commanderPagesShown = p.pages;
    float n = (float)p.count;
    PanelFrameFn frame = GAME_FN(PanelFrameFn, RVA_DRAW_FRAME_IMAGE);
#ifndef __APPLE__
#define DRAW_PANEL_FRAME(fy, fh) frame(id, x, fy, w, fh, a5, a6, a7, a8, a9, a10, a11, fmt, dataDir)
#else
#define DRAW_PANEL_FRAME(fy, fh) frame(id, x, fy, w, fh, a5, a6, a7, a8, a9, a10, a11, r1, r2, r3, r4, r5, r6, r7, dataDir)
#endif
    int result = DRAW_PANEL_FRAME(y - step * n, h + step * n);
    for (int k = 0; k < p.count; k++) {
        int i = shownLines[p.first + k];
        drawCommanderLine(&commanderLines[i], commanderLineId(i), x, y - step * (0.5f + (float)k), scale);
    }
    if (p.pages > 1) {
        int rows = 1 + (p.page > 0) + (p.page < p.pages - 1);
        float top = y - step * n - step * 0.5f, firstY = top - step * 1.25f, bottom = firstY - step * ((float)rows - 1 + 0.5f);
        DRAW_PANEL_FRAME(bottom, top - bottom);
        char label[48];
        snprintf(label, sizeof label, "Page %d of %d", p.page + 1, p.pages);
        drawCommanderBoxLine(0, label, x, firstY, scale);
        float lineY = firstY - step;
        if (p.page > 0) drawCommanderBoxLine(COMMANDER_PREVIOUS_ID, "Previous", x, lineY, scale), lineY -= step;
        if (p.page < p.pages - 1) drawCommanderBoxLine(COMMANDER_NEXT_ID, "Next", x, lineY, scale);
    }
#undef DRAW_PANEL_FRAME
    return result;
}

// Runs a clicked commander line's onClick; 1 when `id` was a shown line's.
static int dispatchCommanderLineClick(int id) {
    if ((id == COMMANDER_PREVIOUS_ID || id == COMMANDER_NEXT_ID) && commanderPagesShown > 1 && G32(RVA_SELECTED_COMMANDER) >= 0) {
        commanderPage += id == COMMANDER_NEXT_ID ? 1 : -1;
        writeLog("commander_lines_page", "page=%d pages=%d", commanderPage + 1, commanderPagesShown);
        return 1;
    }
    int i = commanderLineIndex(id);
    if (i < 0 || G32(RVA_SELECTED_COMMANDER) < 0 || !runLuaVisible(commanderLines[i].lua)) return 0;   // No log: the game's own id, or a hidden line's
    writeLog("commander_line_clicked", "label=%s id=%d commander=%d", commanderLines[i].label, id, G32(RVA_SELECTED_COMMANDER));
    runLuaCallback(commanderLines[i].lua, 0, 0);
    return 1;
}

// Adds a commander line for the script whose callback is `lua`.
static int addCommanderLineFor(void *caller, const char *label, const char *tip, int icon, const float *colour, int lua) {
    if (!label || !label[0]) {
        refuseApiCall(caller, "addCommanderLine", "no_label", "tooltip=%s", tip ? tip : "");
        return -1;
    }
    if (icon != COMMANDER_LINE_NO_ICON && (icon < 0 || icon >= SPRITES_PER_KIND)) {
        refuseApiCall(caller, "addCommanderLine", "bad_icon", "label=%s icon=%d", label, icon);
        return -1;
    }
    for (int c = 0; colour && c < 4; c++)
        if (!(colour[c] >= 0.0f && colour[c] <= 1.0f)) {
            refuseApiCall(caller, "addCommanderLine", "bad_color", "label=%s part=%d value=%g", label, c + 1, (double)colour[c]);
            return -1;
        }
    if (!commanderLineSite) {
        refuseApiCall(caller, "addCommanderLine", "site_unavailable", "label=%s", label);
        return -1;
    }
    int id = commanderLineId(commanderLineCount);
    if (id < 0) {
        refuseApiCall(caller, "addCommanderLine", "no_free_id", "label=%s lines=%d", label, commanderLineCount);
        return -1;
    }
    if (!growList((void **)&commanderLines, &commanderLineCapacity, commanderLineCount + 1, sizeof commanderLines[0])) {
        refuseApiCall(caller, "addCommanderLine", "out_of_memory", "label=%s", label);
        return -1;
    }
    CommanderLine *line = &commanderLines[commanderLineCount++];
    noteApiCut(caller, "addCommanderLine", "label", label, COMMANDER_LINE_LABEL_MAX);
    noteApiCut(caller, "addCommanderLine", "tooltip", tip ? tip : "", COMMANDER_LINE_TIP_MAX);
    snprintf(line->label, sizeof line->label, "%.63s", label);
    snprintf(line->tip, sizeof line->tip, "%.255s", tip ? tip : "");
    line->icon = icon;
    line->lua = lua;
    line->coloured = colour != NULL;
    if (colour) memcpy(line->colour, colour, sizeof line->colour);
    char owner[MAX_PATH];
    const char *ownerField = apiCallerName(caller, owner, sizeof owner);
    writeLog("commander_line_added", "%s=%s label=%s id=%d icon=%d", ownerField, owner, line->label, id, icon);
    return id;
}

// At startup: the panel's frame call points at CoE5SE.
static void installCommanderLineHooks(void) {
    commanderLineSite = pointCustomPlaneCall(RVA_COMMANDER_PANEL_FRAME_CALL, RVA_DRAW_FRAME_IMAGE, (void *)commanderPanelFrameHook,
                                             "commander_panel_frame");
    writeLog("commander_line_hooks_installed", "frame=%d", commanderLineSite);
}
