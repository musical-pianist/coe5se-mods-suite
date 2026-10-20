// ---- The map editor: scripts' toolbox lines, the map view, and drawing over the map --------------
// The map editor ("--mapeditor") has a toolbox of lines on the left of its map. A script adds its own
// line with coe5.addMapEditorLine: CoE5SE draws it right under "Create Fancy Terrain", so the
// editor's Tab hides it with the rest of the toolbox, gives it an id the editor itself never uses,
// and runs the script's onClick when the line (or a key bound to its id) is clicked.
// MAP_EDITOR_DRAW lets a script draw over the editor's map each frame, with coe5.ui.drawLine and
// coe5.ui.drawText at UI coordinates, and coe5.mapView says where each square is.
#include <math.h>

// ---- Toolbox lines --------------------------------------------------------------------------------
// Line n gets the id MAP_EDITOR_FREE_ID_FIRST + n: the editor handles ids 1 to 43 and the plane
// buttons' 1000 and up, so 44 to 999 do nothing in it but end the frame.
#define MAP_EDITOR_LINE_LABEL_MAX 63
#define MAP_EDITOR_LINE_TIP_MAX   255
typedef struct {
    char label[MAP_EDITOR_LINE_LABEL_MAX + 1], tip[MAP_EDITOR_LINE_TIP_MAX + 1];
    int lua;
} MapEditorLine;
static MapEditorLine *mapEditorLines;
static int mapEditorLineCount, mapEditorLineCapacity;

// The id of toolbox line `index`, or -1 past the editor's free ids.
static int mapEditorLineId(int index) {
    if (index < 0 || index > MAP_EDITOR_FREE_ID_LAST - MAP_EDITOR_FREE_ID_FIRST) return -1;
    return MAP_EDITOR_FREE_ID_FIRST + index;
}

// The index of the toolbox line with this id, or -1 for an id that is none of them.
static int mapEditorLineIndex(int id) {
    if (id < MAP_EDITOR_FREE_ID_FIRST || id - MAP_EDITOR_FREE_ID_FIRST >= mapEditorLineCount) return -1;
    return id - MAP_EDITOR_FREE_ID_FIRST;
}

static u64 (*nextToolboxLine)(int, const char *);
static int (*nextMapEditorWait)(int, int);
static void (*nextMapEditorDraw)(void);
static int mapEditorWaiting;
static int mapEditorDrawing;

// The game's "Create Fancy Terrain" line, then each shown script line, in the order they were added.
static u64 drawMapEditorToolboxLines(int id, const char *text) {
    u64 result = nextToolboxLine(id, text);
    for (int i = 0; i < mapEditorLineCount; i++) {
        if (!runLuaVisible(mapEditorLines[i].lua)) continue;
        textButton(mapEditorLineId(i), mapEditorLines[i].label);
        if (mapEditorLines[i].tip[0]) tooltip(mapEditorLines[i].tip);
    }
    return result;
}

// CoE5SE's outermost wrapper of the editor's frame wait.
static int dispatchMapEditorLineClick(int a, int b) {
    mapEditorWaiting++;
    int id = nextMapEditorWait(a, b);
    mapEditorWaiting--;
    int i = mapEditorLineIndex(id);
    if (i < 0 || !runLuaVisible(mapEditorLines[i].lua)) return id;
    writeLog("map_editor_line_clicked", "label=%s id=%d", mapEditorLines[i].label, id);
    runLuaCallback(mapEditorLines[i].lua, 0, 0);
    return id;
}

// MAP_EDITOR_DRAW's gate, outside every wrapper: the game calls its draw pass on every frame of every
// screen, and the site's wrappers run only in the editor's map frames. Elsewhere the draw pass runs
// at once.
static void gateMapEditorDraw(void) {
    if (mapEditorWaiting <= 0) {
        ((void (*)(void))siteGameTarget(SITE_MAP_EDITOR_DRAW))();
        return;
    }
    mapEditorDrawing++;
    nextMapEditorDraw();
    mapEditorDrawing--;
}

// Adds a toolbox line for the script whose callback is `lua`.
static int addMapEditorLineFor(void *caller, const char *label, const char *tip, int lua) {
    if (!label || !label[0]) {
        refuseApiCall(caller, "addMapEditorLine", "no_label", "tooltip=%s", tip ? tip : "");
        return -1;
    }
    if (!nextToolboxLine || !nextMapEditorWait) {
        refuseApiCall(caller, "addMapEditorLine", "site_unavailable", "label=%s", label);
        return -1;
    }
    int id = mapEditorLineId(mapEditorLineCount);
    if (id < 0) {
        refuseApiCall(caller, "addMapEditorLine", "no_free_id", "label=%s lines=%d", label, mapEditorLineCount);
        return -1;
    }
    if (!growList((void **)&mapEditorLines, &mapEditorLineCapacity, mapEditorLineCount + 1, sizeof mapEditorLines[0])) {
        refuseApiCall(caller, "addMapEditorLine", "out_of_memory", "label=%s", label);
        return -1;
    }
    MapEditorLine *line = &mapEditorLines[mapEditorLineCount++];
    noteApiCut(caller, "addMapEditorLine", "label", label, MAP_EDITOR_LINE_LABEL_MAX);
    noteApiCut(caller, "addMapEditorLine", "tooltip", tip ? tip : "", MAP_EDITOR_LINE_TIP_MAX);
    snprintf(line->label, sizeof line->label, "%.63s", label);
    snprintf(line->tip, sizeof line->tip, "%.255s", tip ? tip : "");
    line->lua = lua;
    char owner[MAX_PATH];
    const char *ownerField = apiCallerName(caller, owner, sizeof owner);
    writeLog("map_editor_line_added", "%s=%s label=%s id=%d", ownerField, owner, line->label, id);
    return id;
}

// CoE5SE's outermost wrapper of the whole editor.
// The --mapeditor launch option opens the editor before the first main menu frame, so the scripts
// load here first, as the menu would load them; an editor opened from the menu finds them loaded.
static void (*nextMapEditor)(void);
static void loadScriptsBeforeMapEditor(void) {
    if (!startupFinished) {
        writeLog("map_editor_before_menu", "menu_frames=%d", menuFrames);
        finishStartup();
    }
    nextMapEditor();
}

// At startup: the toolbox line site, and CoE5SE's outermost wrappers of the editor, its wait and
// MAP_EDITOR_DRAW.
static void installMapEditorHooks(void) {
    char owner[64];
    moduleFileName((void *)gateMapEditorDraw, owner, sizeof owner);
    const WrapPlace outermost = {.priority = WRAP_PRIORITY_DEFAULT, .layer = WRAP_LAYER_OUTERMOST};
    int line = wrapSite(SITE_MAP_EDITOR_TOOLBOX_LINE, (void *)drawMapEditorToolboxLines, (void **)&nextToolboxLine);
    int wait = insertWrap(SITE_MAP_EDITOR_WAIT, (void *)dispatchMapEditorLineClick, (void **)&nextMapEditorWait, outermost, owner, NULL);
    int draw = insertWrap(SITE_MAP_EDITOR_DRAW, (void *)gateMapEditorDraw, (void **)&nextMapEditorDraw, outermost, owner, NULL);
    int editor = insertWrap(SITE_MAP_EDITOR, (void *)loadScriptsBeforeMapEditor, (void **)&nextMapEditor, outermost, owner, NULL);
    writeLog("map_editor_hooks_installed", "toolbox_line=%d wait=%d draw=%d editor=%d", line, wait, draw, editor);
}

// ---- The map view ---------------------------------------------------------------------------------
// Where the map screen's and the map editor's squares are on the screen, in UI units (x from the
// left, y up from the bottom, screenW x screenH). World square (x, y) is a square of `zoom` UI
// units with its top left corner at (originX + x * zoom, originY - y * zoom). The first and last
// x and y are the squares of the shown plane that are at least partly on the screen; first is
// above last when none is.
typedef struct {
    double zoom, cameraX, cameraY, originX, originY;
    int screenW, screenH, plane, planeX, planeY, planeW, planeH, firstX, lastX, firstY, lastY;
} MapView;

// The view for these numbers. Returns 0 for a zoom or screen size the game never has (0 or below).
static int mapViewFor(double zoom, double cameraX, double cameraY, int screenW, int screenH, int planeX, int planeY,
                      int planeW, int planeH, MapView *out) {
    if (!(zoom > 0) || !isfinite(zoom) || !isfinite(cameraX) || !isfinite(cameraY) || screenW <= 0 || screenH <= 0) return 0;
    MapView v = {.zoom = zoom, .cameraX = cameraX, .cameraY = cameraY, .screenW = screenW, .screenH = screenH,
                 .planeX = planeX, .planeY = planeY, .planeW = planeW, .planeH = planeH};
    v.originX = screenW / 2 - cameraX * zoom;
    v.originY = screenH / 2 + (cameraY + 1) * zoom;
    double firstX = floor(-v.originX / zoom), lastX = ceil((screenW - v.originX) / zoom) - 1;
    double firstY = floor((v.originY - screenH) / zoom), lastY = ceil(v.originY / zoom) - 1;
    v.firstX = (int)fmin(fmax(firstX, planeX), (double)planeX + planeW);
    v.lastX = (int)fmax(fmin(lastX, (double)planeX + planeW - 1), (double)planeX - 1);
    v.firstY = (int)fmin(fmax(firstY, planeY), (double)planeY + planeH);
    v.lastY = (int)fmax(fmin(lastY, (double)planeY + planeH - 1), (double)planeY - 1);
    *out = v;
    return 1;
}

// The game's map view now. Returns 0 when the shown plane isn't one of the game's planes.
static int currentMapView(MapView *out) {
    int plane = G32(RVA_MAP_VIEW_PLANE), last = G32(RVA_LAST_PLANE);
    if (plane < 0 || plane > last || plane >= PLANE_TABLE_MAX) return 0;
    const unsigned char *record = GAME_PTR(RVA_PLANES) + (size_t)plane * PLANE_SIZE;
    int ok = mapViewFor(*(float *)GAME_PTR(RVA_MAP_ZOOM), *(float *)GAME_PTR(RVA_MAP_CAMERA_X), *(float *)GAME_PTR(RVA_MAP_CAMERA_Y),
                        G32(RVA_SCREEN_W), G32(RVA_SCREEN_H), *(const int *)(record + PLANE_X), *(const int *)(record + PLANE_Y),
                        *(const int *)(record + PLANE_W), *(const int *)(record + PLANE_H), out);
    out->plane = plane;
    return ok;
}

// ---- Drawing at UI coordinates ----------------------------------------------------------------------
// The GUI's own calls, as the game's line and text helpers make them (RVA_DRAW_LINE_UI, RVA_DRAW_TEXT):
// UI units scaled to window pixels, the GL state through the game's cache, so the draw pass that
// follows finds the state it expects.
static void *glImport(uintptr_t rva) {
#ifdef __APPLE__
    return GAME_PTR(rva);
#else
    return *(void **)GAME_PTR(rva);
#endif
}
#define GL_IMPORT(type, rva) ((type)glImport(rva))
#define GL_VERTEX_ARRAY 0x8074
#define GL_FLOAT_TYPE   0x1406
#define GL_LINES_MODE   1

// UI units to window pixels, along x and along y.
static float uiScaleX(void) { return G32(RVA_SCREEN_W) > 0 ? (float)G32(RVA_WINDOW_W) / (float)G32(RVA_SCREEN_W) : 1.0f; }
static float uiScaleY(void) { return G32(RVA_SCREEN_H) > 0 ? (float)G32(RVA_WINDOW_H) / (float)G32(RVA_SCREEN_H) : 1.0f; }

// One line from (x1, y1) to (x2, y2), `width` pixels wide, in r, g, b at `alpha`.
static void drawUiLine(float x1, float y1, float x2, float y2, float r, float g, float b, float alpha, float width) {
    float sx = uiScaleX(), sy = uiScaleY();
    float points[4] = {x1 * sx, y1 * sy, x2 * sx, y2 * sy};
    GAME_FN(void (*)(int, int), RVA_GL_STATE)(0, GL_STATE_LINES);
    GL_IMPORT(void (*)(float), RVA_GL_LINE_WIDTH)(width);
    GL_IMPORT(void (*)(float, float, float, float), RVA_GL_COLOR4F)(r, g, b, alpha);
    GL_IMPORT(void (*)(unsigned), RVA_GL_ENABLE_CLIENT_STATE)(GL_VERTEX_ARRAY);
    GL_IMPORT(void (*)(int, unsigned, int, const void *), RVA_GL_VERTEX_POINTER)(2, GL_FLOAT_TYPE, 0, points);
    GL_IMPORT(void (*)(unsigned, int, int), RVA_GL_DRAW_ARRAYS)(GL_LINES_MODE, 0, 2);
    GL_IMPORT(void (*)(unsigned), RVA_GL_DISABLE_CLIENT_STATE)(GL_VERTEX_ARRAY);
}

// One line of text with its start at (x, y), `size` UI units high, in the GUI font.
static void drawUiText(float x, float y, float size, const char *text, float r, float g, float b, float alpha) {
    float sx = uiScaleX(), sy = uiScaleY();
    GAME_FN(void (*)(const char *, float, float, float, int, float, float, float, float, const char *), RVA_RENDER_TEXT)(
        (const char *)GAME_PTR(RVA_STR_GUIFONT), x * sx, y * sy, size * sy * DRAW_TEXT_SIZE_SCALE, G32(RVA_TEXT_RENDER_FLAGS), r, g, b,
        alpha, text);
}
