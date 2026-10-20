// or, elseif and else in events.
#pragma once

#define EVENT_FAILED     -1
#define PLAYER_EVENT     1
#define SQUARE_EVENT     2

static struct {
    int active;
    int kind;
    int inActions;
    int hadOr;
    int anyHeld;
    int dropTriggers;
    int branchTaken;
    int afterElse;
    int targetX, targetY, player;
} eventLogic;
static unsigned char eventStartMarks[SQUARE_EVENT_A_SIZE];
static unsigned char eventHeldMarks[SQUARE_EVENT_A_SIZE];
static unsigned char eventTakenMarks[SQUARE_EVENT_A_SIZE];

static unsigned char *eventMarks(void) { return gameBase + RVA_SQUARE_EVENT_A; }

// The event line's first word is `word`.
static int eventWordIs(const char *line, const char *word) {
    size_t len = strlen(word);
    return !strncmp(line, word, len) && (line[len] == '\0' || line[len] == ' ' || line[len] == '\t' || line[len] == '\r' || line[len] == '\n');
}

static int isEventLogicWord(const char *line) {
    return eventWordIs(line, "or") || eventWordIs(line, "elseif") || eventWordIs(line, "else");
}

// Puts the event back as it was at its start, for the next group or branch.
static void restartEvent(const unsigned char *marks) {
    G32(RVA_EVENT_TARGET_X) = eventLogic.targetX;
    G32(RVA_EVENT_TARGET_Y) = eventLogic.targetY;
    G32(RVA_EVENT_PLAYER) = eventLogic.player;
    if (eventLogic.kind == PLAYER_EVENT) {
        G32(RVA_EVENT_STATE) = PLAYER_EVENT;
        return;
    }
    int any = 0;
    for (size_t i = 0; i < SQUARE_EVENT_A_SIZE; i++) any |= (eventMarks()[i] = marks[i]);
    G32(RVA_EVENT_STATE) = any ? SQUARE_EVENT : EVENT_FAILED;
}

// Ends a group of triggers at an `or`, or at the branch's end: notes whether it held and which squares
// it kept.
static void endTriggerGroup(void) {
    int held = G32(RVA_EVENT_STATE) >= 1;
    eventLogic.anyHeld |= held;
    if (eventLogic.kind == SQUARE_EVENT && held)
        for (size_t i = 0; i < SQUARE_EVENT_A_SIZE; i++) eventHeldMarks[i] |= eventMarks()[i];
}

// Ends a branch's triggers, at its first action or at its elseif or else: the branch holds when any of
// its groups held, and a squareevent keeps every square some group kept.
static void endBranchTriggers(void) {
    if (eventLogic.inActions) return;
    eventLogic.inActions = 1;
    if (eventLogic.hadOr && !eventLogic.dropTriggers) {
        endTriggerGroup();
        if (eventLogic.kind == SQUARE_EVENT) {
            memcpy(eventMarks(), eventHeldMarks, SQUARE_EVENT_A_SIZE);
            G32(RVA_EVENT_STATE) = eventLogic.anyHeld ? SQUARE_EVENT : EVENT_FAILED;
        } else if (!eventLogic.anyHeld) {
            G32(RVA_EVENT_STATE) = EVENT_FAILED;
        }
    }
    if (G32(RVA_EVENT_STATE) >= 1) {
        if (eventLogic.kind == PLAYER_EVENT) eventLogic.branchTaken = 1;
        else
            for (size_t i = 0; i < SQUARE_EVENT_A_SIZE; i++) eventTakenMarks[i] |= eventMarks()[i];
    }
}

static void startBranchTriggers(void) {
    eventLogic.inActions = eventLogic.hadOr = eventLogic.anyHeld = eventLogic.dropTriggers = 0;
    memset(eventHeldMarks, 0, sizeof eventHeldMarks);
}

// Refuses an or, elseif or else line where it can't stand.
static void refuseEventLogicLine(const char *line, const char *reason) {
    char word[8];
    snprintf(word, sizeof word, "%.*s", (int)strcspn(line, " \t\r\n"), line);
    refuseC5mLine(word, "-", reason, "line=%d file=%.200s", G32(RVA_MOD_LINE), (const char *)(gameBase + RVA_EVENT_FILE));
}

// An event line before the game's event runner gets it; returns 1 when CoE5SE took the line.
static int eventLogicLine(const char *line, int level) {
    int ours = isEventLogicWord(line);
    if (level != 2 && level != 3) return ours;
    if (eventWordIs(line, "endevent")) {
        eventLogic.active = 0;
        return 0;
    }
    if (!eventLogic.active) {
        if (ours) refuseEventLogicLine(line, "outside_event");
        return ours;
    }
    int trigger = line[0] == '+' || line[0] == '-';
    if (!ours) {
        if (trigger && !eventLogic.inActions) return eventLogic.dropTriggers;
        if (!trigger) endBranchTriggers();
        return 0;
    }
    if (eventWordIs(line, "or")) {
        if (eventLogic.inActions) {
            refuseEventLogicLine(line, "or_after_actions");
            return 1;
        }
        if (eventLogic.dropTriggers) return 1;
        eventLogic.hadOr = 1;
        endTriggerGroup();
        if (eventLogic.kind == PLAYER_EVENT && G32(RVA_EVENT_STATE) >= 1) eventLogic.dropTriggers = 1;
        else restartEvent(eventStartMarks);
        return 1;
    }
    if (eventLogic.afterElse) {
        refuseEventLogicLine(line, "after_else");
        G32(RVA_EVENT_STATE) = EVENT_FAILED;
        return 1;
    }
    endBranchTriggers();
    eventLogic.afterElse = eventWordIs(line, "else");
    startBranchTriggers();
    if (eventLogic.kind == PLAYER_EVENT) {
        if (eventLogic.branchTaken) G32(RVA_EVENT_STATE) = EVENT_FAILED;
        else restartEvent(NULL);
    } else {
        static unsigned char left[SQUARE_EVENT_A_SIZE];
        for (size_t i = 0; i < SQUARE_EVENT_A_SIZE; i++) left[i] = eventStartMarks[i] & (unsigned char)!eventTakenMarks[i];
        restartEvent(left);
    }
    return 1;
}

// After the game's runner read an event line: an event's start keeps its state to come back to.
static void eventLogicAfterLine(const char *line, int level) {
    if (level != 2 && level != 3) return;
    if (!eventWordIs(line, "playerevent") && !eventWordIs(line, "squareevent")) return;
    int state = G32(RVA_EVENT_STATE);
    memset(&eventLogic, 0, sizeof eventLogic);
    if (state != PLAYER_EVENT && state != SQUARE_EVENT) return;
    eventLogic.active = 1;
    eventLogic.kind = state;
    eventLogic.targetX = G32(RVA_EVENT_TARGET_X);
    eventLogic.targetY = G32(RVA_EVENT_TARGET_Y);
    eventLogic.player = G32(RVA_EVENT_PLAYER);
    memset(eventHeldMarks, 0, sizeof eventHeldMarks);
    memset(eventTakenMarks, 0, sizeof eventTakenMarks);
    if (state == SQUARE_EVENT) memcpy(eventStartMarks, eventMarks(), SQUARE_EVENT_A_SIZE);
}
