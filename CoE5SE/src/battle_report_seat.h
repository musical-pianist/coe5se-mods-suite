// coe5.reportBattlesTo(seat): one human seat gets the game's Battle Report message for every AI faction's battle it isn't in.

static int battleReportSeat = -1;
// Every new or loaded game starts with no seat.
static void clearBattleReportSeat(void) { battleReportSeat = -1; }

#ifndef __APPLE__
#define BATTLE_REPORT_STUB_SIZE 0x100
#define FIRST_INDEPENDENT_SLOT 24
static unsigned char *battleReportStub;

static void emitBattleReportRel32(unsigned char **p, const unsigned char *target) {
    int rel = (int)(target - (*p + 4));
    memcpy(*p, &rel, 4);
    *p += 4;
}

// Writes the stub and the jump to it, once. Returns 1 when in place.
static int installBattleReportStub(void) {
    static int done;
    if (done) return done > 0;
    done = -1;
    if (!gameCodeMatches) {
        writeLog("battle_report_seat_installed", "ok=0 reason=game_code_unknown");
        return 0;
    }
    unsigned char *at = GAME_PTR(RVA_FIGHT_SHOWN_STORE);
    unsigned char *page = allocNearBytes(at, BATTLE_REPORT_STUB_SIZE);
    if (!page) {
        writeLog("battle_report_seat_installed", "ok=0 reason=no_near_page");
        return 0;
    }
    unsigned long long seat = (unsigned long long)(uintptr_t)&battleReportSeat;
    unsigned long long reports = (unsigned long long)(uintptr_t)GAME_PTR(RVA_BATTLE_REPORTS);
    unsigned long long controllers = (unsigned long long)(uintptr_t)GAME_PTR(RVA_SLOT_CONTROLLER);
    unsigned char *p = page, *done1, *done2, *done3, *done4, *done5;
    *p++ = 0x48; *p++ = 0xb8; memcpy(p, &seat, 8); p += 8;
    *p++ = 0x48; *p++ = 0x63; *p++ = 0x00;
    *p++ = 0x85; *p++ = 0xc0;
    *p++ = 0x78; done1 = p++;
    *p++ = 0x48; *p++ = 0xb9; memcpy(p, &reports, 8); p += 8;
    *p++ = 0x66; *p++ = 0x83; *p++ = 0x39; *p++ = 0x00;
    *p++ = 0x74; done2 = p++;
    *p++ = 0x48; *p++ = 0xb9; memcpy(p, &controllers, 8); p += 8;
    *p++ = 0x66; *p++ = 0x83; *p++ = 0x3c; *p++ = 0x41; *p++ = CONTROLLER_HUMAN;
    *p++ = 0x75; done4 = p++;
    *p++ = 0x31; *p++ = 0xd2;
    unsigned char *sideLoop = p, *nextSide, *aiSide;
    *p++ = 0x44; *p++ = 0x8b; *p++ = 0x5c; *p++ = 0x95; *p++ = (unsigned char)-DO_ALL_FIGHTS_ROLES;
    *p++ = 0x41; *p++ = 0xff; *p++ = 0xcb;
    *p++ = 0x41; *p++ = 0x83; *p++ = 0xfb; *p++ = 1;
    *p++ = 0x77; nextSide = p++;
    *p++ = 0x66; *p++ = 0x83; *p++ = 0x3c; *p++ = 0x51; *p++ = CONTROLLER_AI;
    *p++ = 0x74; aiSide = p++;
    *nextSide = (unsigned char)(p - (nextSide + 1));
    *p++ = 0xff; *p++ = 0xc2;
    *p++ = 0x83; *p++ = 0xfa; *p++ = FIRST_INDEPENDENT_SLOT;
    *p++ = 0x72; *p = (unsigned char)(sideLoop - (p + 1)); p++;
    *p++ = 0xeb; done5 = p++;
    *aiSide = (unsigned char)(p - (aiSide + 1));
    *p++ = 0x83; *p++ = 0x7c; *p++ = 0x85; *p++ = (unsigned char)-DO_ALL_FIGHTS_ROLES; *p++ = 0x00;
    *p++ = 0x75; done3 = p++;
    *p++ = 0xc7; *p++ = 0x44; *p++ = 0x85; *p++ = (unsigned char)-DO_ALL_FIGHTS_ROLES;
    *p++ = 3; *p++ = 0; *p++ = 0; *p++ = 0;
    *p++ = 0x41; *p++ = 0xbf; *p++ = 1; *p++ = 0; *p++ = 0; *p++ = 0;
    *done1 = (unsigned char)(p - (done1 + 1));
    *done2 = (unsigned char)(p - (done2 + 1));
    *done3 = (unsigned char)(p - (done3 + 1));
    *done4 = (unsigned char)(p - (done4 + 1));
    *done5 = (unsigned char)(p - (done5 + 1));
    *p++ = 0x44; *p++ = 0x89; *p++ = 0x7c; *p++ = 0x24;
    *p++ = DO_ALL_FIGHTS_SHOWN;
    *p++ = 0xe9; emitBattleReportRel32(&p, GAME_PTR(RVA_FIGHT_SHOWN_STORE_NEXT));
    makePagesReadExecute(page, BATTLE_REPORT_STUB_SIZE);
    unsigned char jump[5] = {0xe9};
    int rel = (int)(page - (at + 5));
    memcpy(jump + 1, &rel, 4);
    if (!writeGameCode(at, jump, sizeof jump)) {
        writeLog("battle_report_seat_installed", "ok=0 reason=code_unwritable");
        return 0;
    }
    battleReportStub = page;
    done = 1;
    writeLog("battle_report_seat_installed", "ok=1 address=%p stub=%p bytes=%d", (void *)at, (void *)page, (int)(p - page));
    return 1;
}
#endif

// coe5.reportBattlesTo(seat): the seat, while human, gets a Battle Report for every AI faction's battle it isn't in; nil or -1 for none.
static int lReportBattlesTo(lua_State *L) {
    lua_Integer seat = luaL_optinteger(L, 1, -1);
    const char *reason = isNetworkGame() ? "network_game" : NULL;
    if (!reason && seat != -1 && (seat < 0 || seat >= PLAYER_SLOTS)) reason = "bad_player";
#ifdef __APPLE__
    if (!reason) reason = "windows_only";
#else
    if (!reason && seat >= 0 && !installBattleReportStub()) reason = "not_installed";
#endif
    LuaScript *s = luaScriptOf(L);
    if (reason) {
        refuseLuaCall(s ? s->mod : "?", "coe5.reportBattlesTo", reason, "seat=%lld", (long long)seat);
        lua_pushboolean(L, 0);
        return 1;
    }
    battleReportSeat = (int)seat;
    writeLog("battle_report_seat", "mod=%s seat=%lld", s ? s->mod : "?", (long long)seat);
    lua_pushboolean(L, 1);
    return 1;
}
