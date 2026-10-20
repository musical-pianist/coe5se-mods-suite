// The handover of CoE5SE's spell paths to a plugin that raises them further.
#pragma once

typedef int (*SpellPathInstallFn)(int *count, const char ***names);

#define SPELL_PATH_TAKEOVER_EXPORT "coe5seTakeOverSpellPaths"
#define SPELL_PATH_COUNT_EXPORT    "coe5seSpellPathCount"
