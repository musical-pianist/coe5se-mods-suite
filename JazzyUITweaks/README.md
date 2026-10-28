# Jazzy UI Tweaks

[All mods](../README.md#mods) | [CoE5SE](../CoE5SE/README.md) | [Jazzy Game Presets](../JazzyGamePresets/README.md) | [Jazzy Hotkeys](../JazzyHotkeys/README.md) | [Jazzy Restart](../JazzyRestart/README.md) | [Jazzy Cheats](../JazzyCheats/README.md) | [Jazzy AI Observer](../JazzyAIObserver/README.md)

Delete saves from Load Game, hide the Read Tip button, shrink or hide the mod banners on the menu screens, and see what a ritual needs to cast. You can turn each one back to vanilla in Mod Settings, Jazzy UI Tweaks.  
**Needs [CoE5SE](../CoE5SE/README.md) from Nexus Mods.**

## What It Adds

### Delete saves from Load Game

**Default: on.**

- A <del> at the end of every save in Load Game.
- Click it and the game asks *Are you sure you want to delete*. Yes deletes the save permanently.

### No Read Tip button

**Default: on.**

- The main menu without Read Tip. The buttons below it move up.

### Mod banners

**Default: half size, kept on screen.**

- The column of enabled mods' banners on the right of the menu screens at half size, full size, or hidden.
- With *Keep mod banners on screen* the column stays instead of fading away.

### What a ritual needs

**Default: off.**

- On the ritual screen, hold the pointer on a ritual for 2 seconds. Under the game's own hover text, a list shows what the ritual needs to cast: its costs, terrain, level, cast limit, mirror, plane, the units it needs or rules out, and its other rules, each in white when met and grey when not.
- *Plain needs* shows that list. *Advanced* adds the ritual's raw data for modders: reason codes, slot, school, level, flags, terrain code, base and charged costs, effects by their mod command, casts, and its strings.
- Greyed rituals only, or every ritual on the screen.
- Names come from the game while it runs, so mods' resources, terrains, and rituals show by their own names.

## Limits

In network games the mod banners, <del>, and the ritual list are vanilla.

## For Modders

Jazzy UI Tweaks is a Lua mod. The script, JazzyUITweaks.lua, is in the mod's folder to read and learn from, and its settings are declared in JazzyUITweaks.c5se beside it.

## Check Out My Other Mods!

- [CoE5SE](../CoE5SE/README.md): loads mods' Lua scripts and adds Mod Settings to the main menu.
- [Jazzy Game Presets](../JazzyGamePresets/README.md): save a new-game setup as a preset.
- [Jazzy Restart](../JazzyRestart/README.md): Restart in the Game Menu. Single player.
- [Jazzy Hotkeys](../JazzyHotkeys/README.md): change any key of the game's Keyboard Shortcuts list, on every screen.
