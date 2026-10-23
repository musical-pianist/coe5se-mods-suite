// Checks that the game's own code is still what CoE5SE's copies and patches of it were made for.

// Logs `event` for game code CoE5SE copies or changes, with its fields after `head` ("" or "function=<name> ").
static int checkGameFunction(const char *event, const char *head) {
    if (!gameBuild) {
        writeLog(event, "%schecked=0 reason=unknown_game_build", head);
        return 0;
    }
    if (!gameCodeMatches) {
        writeLog(event, "%smatch=0 reason=game_code_differs", head);
        return 0;
    }
    writeLog(event, "%smatch=1 build=%s", head, gameBuild->gameVersion);
    return 1;
}

// The startup check of name_box, logged as name_box_check.
static void checkNameBoxCode(void) { checkGameFunction("name_box_check", ""); }

// The startup check of the event triggers' two finders, one event_finders_check line each.
static int checkEventFinderCode(void) {
    int unit = checkGameFunction("event_finders_check", "function=find_unit_at ");
    int item = checkGameFunction("event_finders_check", "function=find_item_holder_at ");
    return unit && item;
}
