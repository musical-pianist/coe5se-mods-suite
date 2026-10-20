// A test hook into CoE5SE's answers to the event triggers' finders.
#pragma once

#define EVENT_FINDER_SHADOW_EXPORT "coe5seEventFinderShadow"

// (player, x, y, name) -> a unit, or -1.
typedef int (*EventFinderFn)(int player, int x, int y, const char *name);

#define EVENT_FINDER_UNIT      0
#define EVENT_FINDER_ITEM      1
#define EVENT_FINDER_KINDS     2

typedef int (*EventFinderShadowFn)(int kind, int player, int x, int y, const char *name, EventFinderFn game,
                                   EventFinderFn answer);

typedef struct {
    unsigned long long answered;
    unsigned long long passed;
    unsigned long long builds;
} EventFinderCounts;

typedef const EventFinderCounts *(*EventFinderShadowSetFn)(EventFinderShadowFn shadow);
