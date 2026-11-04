# Jazzy Game Presets

[All mods](../README.md#mods) | [CoE5SE](https://github.com/musical-pianist/coe5se) | [Jazzy Hotkeys](../JazzyHotkeys/README.md) | [Jazzy Restart](../JazzyRestart/README.md) | [Jazzy UI Tweaks](../JazzyUITweaks/README.md) | [Jazzy Cheats](https://github.com/musical-pianist/coe5se-modding-tools/blob/main/JazzyCheats/README.md) | [Jazzy AI Observer](https://github.com/musical-pianist/coe5se-modding-tools/blob/main/JazzyAIObserver/README.md)

## Set up your perfect game once. Load it in one click. Tweak it mid-game.

Save your whole new-game setup (map, society, every player slot, every AI's difficulty) as a preset, load it next time, and change the AI and game rules in a game you're already playing.  
Needs CoE5SE. You can install it from here: [github.com/musical-pianist/coe5se](https://github.com/musical-pianist/coe5se)

## Features

### Campaign Presets

- Hit Ok on Setup Participants and your setup auto-saves as *Previous Game Settings*. Instant rematch.
- Want to keep a setup? Click *Save as preset* above Ok and give it a name (up to 32 characters).
- Saving with a name you already used overwrites that preset. No warning.

### Loading a Preset

![](https://staticdelivery.nexusmods.com/mods/10486/images/3/3-1791869147-951036108.png)

- Start New Game now asks *Load a preset?* Pick one, hit Ok, and both setup screens fill in. Change anything you like before you start.
- Delete takes two clicks, so you can't nuke a preset by accident. *Previous Game Settings* can't be deleted.
- The list shows up to 20 presets.

**Presets save what?**

![](https://staticdelivery.nexusmods.com/mods/10486/images/3/3-1791869147-820032930.png)

- Every map setting: society, common cause, score graphs, battle reports, start positions, wilder, map size, and north/south.
- Every player slot: human or AI, class, AI difficulty, team, plus *unique random players*. Player names aren't saved.
- Random stays Random, so you get a fresh roll every game.
- A class from a mod you turned off becomes Random.

Presets live in **%APPDATA%\coe5\coe5se_data\JazzyGamePresets\jazzy_campaign_presets\** (Mac: **~/.coe5/coe5se_data/JazzyGamePresets/jazzy_campaign_presets/**), one .ini file each.

### Extended Game Settings

![](https://staticdelivery.nexusmods.com/mods/10486/images/3/3-1791869151-1556027463.png)

![](https://staticdelivery.nexusmods.com/mods/10486/images/3/3-1791869152-430555734.png)

- Mid-game, open the settings menu and pick *Extended Game Settings* under Preferences.
- Turn battle reports, score graphs, and common cause on or off, and set each AI anywhere from Piss Boy to Emperor.
- Adjust ai difficulty mid campaign if you wish.
- Changes kick in from the next battle or turn.

## Limits

Single player only. No network games.

## For Modders

Jazzy Game Presets is a Lua mod. The script, JazzyGamePresets.lua, is in the mod's folder to read and learn from.

## Check Out My Other Mods!

- [Jazzy Restart](../JazzyRestart/README.md): Restart in the Game Menu. Single player.
- [Jazzy Hotkeys](../JazzyHotkeys/README.md): change any key of the game's Keyboard Shortcuts list, on every screen.
- [Jazzy UI Tweaks](../JazzyUITweaks/README.md): small changes to the game's own screens, like deleting saves from Load Game.
- [Jazzy Cheats](https://github.com/musical-pianist/coe5se-modding-tools/blob/main/JazzyCheats/README.md): give yourself resources and units, teleport a commander, and reveal the map.
