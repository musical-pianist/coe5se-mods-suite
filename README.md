# Conquest of Elysium 5 Script Extender (CoE5SE)

[![Latest release](https://img.shields.io/github/v/release/musical-pianist/coe5se_mods_suite?label=latest)](https://github.com/musical-pianist/coe5se_mods_suite/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/musical-pianist/coe5se_mods_suite/total)](https://github.com/musical-pianist/coe5se_mods_suite/releases)
[![Docs](https://img.shields.io/badge/docs-coe5se--docs-blue)](https://musical-pianist.github.io/coe5se-docs/)

CoE5SE is the script extender for Conquest of Elysium 5. It raises the game's limits, runs Lua scripts, and adds Mod Settings to the main menu, so mods can do what the game alone can't.

## Install

One command installs CoE5SE and every mod in this repo, and copies CoE5SE's file into the game folder. Close the game first. Run it again any time to update.

**Windows:** open the Start menu, type PowerShell, open Windows PowerShell, then paste this and press Enter:

```
irm https://raw.githubusercontent.com/musical-pianist/coe5se_mods_suite/main/install.ps1 | iex
```

**Mac:** open Terminal (in Applications, Utilities), then paste this and press Return:

```
curl -fsSL https://raw.githubusercontent.com/musical-pianist/coe5se_mods_suite/main/install.sh | bash
```

On Mac, the first install leaves one step: in Steam, click the gear on Conquest of Elysium 5's page, then Properties, General, and set Launch Options to `/usr/bin/env DYLD_INSERT_LIBRARIES=coe5se.dylib %command%`. The script prints this step only while Launch Options still need it.

Then start the game. The first time, CoE5SE turns itself on and tells you so. Turn on the Jazzy mods you want on the Mods screen.

You can read both scripts before running them: [install.ps1](install.ps1), [install.sh](install.sh).

### Install by Hand

1. Download a mod's zip from [Releases](https://github.com/musical-pianist/coe5se_mods_suite/releases) and copy its folder into your mods folder: `%APPDATA%\coe5\mods\` on Windows, `~/.coe5/mods/` on Mac.
2. For CoE5SE, open the game folder (in Steam: right-click Conquest of Elysium 5, Manage, Browse local files) and copy one file into it from the `CoE5SE` folder: `glu32.dll` on Windows, next to `CoE5.exe`, or `coe5se.dylib` on Mac, next to `coe5_mac`. On Mac, also set the Launch Options above.
3. Start the game. If CoE5SE doesn't say it turned itself on, enable it on the Mods screen, click Exit, and restart the game.

> [!NOTE]
> `how-to-install.html` in the `CoE5SE` folder has GIFs of these steps, if you'd like to see them done.

## CoE5SE Features

### For Players

CoE5SE makes all of this possible:

- You can have way more mods before running into limits.
- Set each AI anywhere from Piss Boy to Emperor in a game you're already playing, and turn battle reports, score graphs, and common cause on or off.
- Save your whole new-game setup (map, society, every player slot, every AI's difficulty) as a preset, and stop forgetting your settings between campaigns.
- Restart from the Game Menu with the same settings. No more resign, wait 10 seconds, Ok, repick every campaign detail, repeat.
- Change any key of the game's Keyboard Shortcuts list: 196 actions on 10 screens.
- Various UI improvements. Delete saves from Load Game, and see what a ritual needs to cast.
- Each mod can have its own settings to customize your experience. A mod's update doesn't reset your picks.
- More and better error reports. Know when potentially save-breaking changes occur, like mods moving or replacing spells.

### For Modders

Write Lua scripts that read units, battles, players, classes, the world, rituals, spells and planes, and spawn and move units. Add main menu buttons, Game Menu lines, screens and key bindings, open Load Game and act when the game saves or loads, and remember a setup between games. Every function is in the Lua reference, [`CoE5SE/coe5.lua`](CoE5SE/coe5.lua).

Go past the game's limits, in single player and network games alike:

| Mods can add | Without CoE5SE | With CoE5SE |
| --- | --- | --- |
| Rituals | 2,250 | 10,000 |
| Spells | none | 3,000 |
| Magic paths | none | 58 |
| Custom planes<br>(like the Chaos Wastes) | none | 8 |
| Weapons | 3,799 | 5,799 |
| Monsters | 3,999 | 4,999 |
| Magic items | 699 | 1,699 |
| Classes | 169 | 219 |

Use new commands in your .c5se:

- Write your own [spells](https://musical-pianist.github.io/coe5se-docs/commands/spells/), with their own path, level and effect, and give your units new magic to cast.
- Add your own [magic paths](https://musical-pianist.github.io/coe5se-docs/commands/spell-paths/), with their own names (Chaos, Runecraft, Adeptus Mechanicus Battle Hymns), and share them with other mods by name.
- Give your spells the [limits and effects](https://musical-pianist.github.io/coe5se-docs/commands/spell-modifiers/) the game's own spells have: a slow cast, no casting at sea, a bonus for the army of whoever knows it.
- Add your own [planes](https://musical-pianist.github.io/coe5se-docs/commands/planes/) to the world. Build one the way the game builds its planes, read it from your own map file, or fill it with your own terrain and monsters.
- Give your [events](https://musical-pianist.github.io/coe5se-docs/commands/events/) if-then logic, so a single event can react to each situation in its own way.
- Give a monster [Small or Combat Caster](https://musical-pianist.github.io/coe5se-docs/commands/monster-traits/), and take off any one trait while keeping the rest.
- Write your own lists of [commander names](https://musical-pianist.github.io/coe5se-docs/commands/commander-names/), so your Dwarf Captains go by Gimli and Balin.
- Give your rituals [additional effects](https://musical-pianist.github.io/coe5se-docs/commands/ritual-effects/) not supported by the base game.
- Point at another monster or ritual [by its name](https://musical-pianist.github.io/coe5se-docs/commands/monster-names/), so that, as example, you can setup you mod to affect evolutions and units that don't have to worry about other mods.

Tie your mod's variables to [settings](https://musical-pianist.github.io/coe5se-docs/commands/mod-settings/), and you can craft a more dynamic experience around each player's preferences.

## Documentation

Every command, with examples, is in the [CoE5SE docs](https://musical-pianist.github.io/coe5se-docs/). Each mod below has its own Readme.

Terms of use: [LICENSE.txt](LICENSE.txt).

<!-- mods start -->
## Mods

Click a mod for its Readme. All these mods need CoE5SE to run.

- [CoE5SE](CoE5SE/README.md): The whole reason we be here.
- [Jazzy Game Presets](JazzyGamePresets/README.md): Save your whole new-game setup (map, society, every player slot, every AI's difficulty) as a preset, load it next time you want to start a new campaign, and change the AI and game rules in a game you're already playing.
- [Jazzy Hotkeys](JazzyHotkeys/README.md): Change any key of the game's Keyboard Shortcuts list: 196 actions on 10 screens, picked in Mod Settings.
- [Jazzy Restart](JazzyRestart/README.md): Restart in the Game Menu. It ends the game and starts a new world with the same settings, no relaunch. Also adds a "Load Game".
- [Jazzy UI Tweaks](JazzyUITweaks/README.md): Delete saves from Load Game, hide the Read Tip button, shrink or hide the mod banners on the menu screens, and see what a ritual needs to cast. Did the modders description forget to mention the baron had to be in a fairy circle IN SUMMER for the ritual to work? No worries. This mod gots you. You can turn each part of the mod back to vanilla in Mod Settings, Jazzy UI Tweaks.
- [Jazzy Cheats](JazzyCheats/README.md): Give yourself resources, drop units anywhere you've seen, get your movement back, and see every plane for a turn. No Cheat Engine needed. Works in multiplayer if folks want to have a cheat fest together for some reason.
<!-- mods end -->
