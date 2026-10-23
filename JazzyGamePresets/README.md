# Jazzy Game Presets

[All mods](../README.md#mods) | [CoE5SE](../CoE5SE/README.md) | [Jazzy Hotkeys](../JazzyHotkeys/README.md) | [Jazzy Restart](../JazzyRestart/README.md) | [Jazzy UI Tweaks](../JazzyUITweaks/README.md) | [Jazzy Cheats](../JazzyCheats/README.md)

## Set up your perfect game once. Load it in one click. Tweak it mid-game.

Save your whole new-game setup (map, society, every player slot, every AI's difficulty) as a preset, load it next time, and change the AI and game rules in a game you're already playing.  
**Needs [CoE5SE](../CoE5SE/README.md) from Nexus Mods.**

## What It Adds

### Presets

- Hit Ok on Setup Participants and your setup auto-saves as *Last game*. Instant rematch.
- Want to keep a setup? Click *Save as preset* above Ok and give it a name (up to 32 characters).
- Saving with a name you already used overwrites that preset. No warning.

### Loading a Preset

![](https://staticdelivery.nexusmods.com/mods/10486/images/3/3-1791869147-951036108.png)

- Start New Game now asks *Load a preset?* Pick one, hit Ok, and both setup screens fill in. Change anything you like before you start.
- *No preset* plays like vanilla.
- Delete takes two clicks, so you can't nuke a preset by accident. *Last game* can't be deleted.
- The list shows up to 20 presets.

### What's in a Preset

![](https://staticdelivery.nexusmods.com/mods/10486/images/3/3-1791869147-820032930.png)

- Every map setting: society, common cause, score graphs, battle reports, start positions, wilder, map size, and north/south.
- Every player slot: human or AI, class, AI difficulty, team, plus *unique random players*. Player names aren't saved.
- Random stays Random, so you get a fresh roll every game.
- A class from a mod you turned off becomes Random.

Presets live in **%APPDATA%\coe5\coe5se_data\JazzyGamePresets\jazzy_campaign_presets\** (Mac: **~/.coe5/coe5se_data/JazzyGamePresets/jazzy_campaign_presets/**), one .ini file each.

### Random Classes

- Under *unique random players*, the *random classes* line shows how many classes an AI on Random can roll. Click it to untick the ones you never want to face. At least one always stays ticked.
- An AI left on Random gets one of the ticked classes, picked by the game's own rules. An unticked class is never picked for an AI.
- With *unique random players* on and more AIs than ticked classes, the extra AIs share ticked classes.
- A class you pick for a slot yourself always goes in, and your own Random slot still rolls from every class.
- The list saves with the preset and with *Last game*. A class from a mod you turned off is remembered for when the mod comes back.

### Extended Game Settings

![](https://staticdelivery.nexusmods.com/mods/10486/images/3/3-1791869151-1556027463.png)

![](https://staticdelivery.nexusmods.com/mods/10486/images/3/3-1791869152-430555734.png)

- Mid-game, press F10 and pick *Extended Game Settings* under Preferences.
- Turn battle reports, score graphs, and common cause on or off, and set each AI anywhere from Piss Boy to Emperor.
- AI rolling over? Crank it to Emperor and it earns about 4 times the income from its next turn. Getting stomped? Turn it down.
- Changes kick in from the next battle or turn, and your save keeps them.
- Map size, society, and the rest only matter when the world is made, so they aren't here.

## Limits

Single player only. No network games.

## For Modders

Jazzy Game Presets is a Lua mod. The script, JazzyGamePresets.lua, is in the mod's folder to read and learn from.

## Check Out My Other Mods!

- [CoE5SE](../CoE5SE/README.md): loads mods' Lua scripts and adds Mod Settings to the main menu.
- [Jazzy Restart](../JazzyRestart/README.md): Restart in the Game Menu. Single player.
- [Jazzy Hotkeys](../JazzyHotkeys/README.md): change any key of the game's Keyboard Shortcuts list, on every screen.
- [Jazzy UI Tweaks](../JazzyUITweaks/README.md): small changes to the game's own screens, like deleting saves from Load Game.

[All mods](../README.md#mods) | [CoE5SE](../CoE5SE/README.md) | [Jazzy Hotkeys](../JazzyHotkeys/README.md) | [Jazzy Restart](../JazzyRestart/README.md) | [Jazzy UI Tweaks](../JazzyUITweaks/README.md) | [Jazzy Cheats](../JazzyCheats/README.md)
