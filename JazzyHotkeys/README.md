# Jazzy Hotkeys

[All mods](../README.md#mods) | [CoE5SE](../CoE5SE/README.md) | [Jazzy Game Presets](../JazzyGamePresets/README.md) | [Jazzy Restart](../JazzyRestart/README.md) | [Jazzy UI Tweaks](../JazzyUITweaks/README.md) | [Jazzy Cheats](../JazzyCheats/README.md) | [Jazzy AI Observer](../JazzyAIObserver/README.md)

Change any key of the game's Keyboard Shortcuts list: 196 actions on 10 screens, picked in Mod Settings.  
**Needs [CoE5SE](../CoE5SE/README.md) from Nexus Mods.**

## What It Adds

### Every key of every screen

**Default: the game's own keys.**

- Mod Settings, Jazzy Hotkeys, has one page per screen: Map, Transfer troops, Battle, Map editor, Battle map editor, Recruitment, Setup participants, Army on the battlefield, Battle unit list, and Load game.
- Every key line of the game's Keyboard Shortcuts list is on its screen's page.
- Buttons, tooltips, and the Keyboard Shortcuts list show your picks.
- Reset to defaults, on the mod's first page, puts every key back to the game's own.

### The key picker

- Click a key and the picker opens: modifier tabs, a grid of keys, No key, and Default.
- A key another action on that screen uses can't be picked, and the game says which action has it. Set that action to No key first. Different screens can share a key.

### Game Menu

**Esc and F10, on the map.**

- Esc and F10 always open the Game Menu on the map. Neither can change, and no other map key can have them.
- Esc can't be picked for any action. Where the game gives an action Esc (Cancel, Exit screen), that line keeps it.

## Limits

Network games keep the game's own keys. The list's mouse lines aren't keys, so they stay as they are.

## For Modders

Jazzy Hotkeys is a Lua mod. The script, JazzyHotkeys.lua, is in the mod's folder to read and learn from, and its settings are declared in JazzyHotkeys.c5se beside it.

## Check Out My Other Mods!

- [CoE5SE](../CoE5SE/README.md): loads mods' Lua scripts and adds Mod Settings to the main menu.
- [Jazzy Game Presets](../JazzyGamePresets/README.md): save a new-game setup as a preset.
- [Jazzy Restart](../JazzyRestart/README.md): Restart in the Game Menu. Single player.
- [Jazzy UI Tweaks](../JazzyUITweaks/README.md): small changes to the game's own screens, like deleting saves from Load Game.
