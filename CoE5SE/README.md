# CoE5SE

[All mods](../README.md#mods) | [Jazzy Game Presets](../JazzyGamePresets/README.md) | [Jazzy Hotkeys](../JazzyHotkeys/README.md) | [Jazzy Restart](../JazzyRestart/README.md) | [Jazzy UI Tweaks](../JazzyUITweaks/README.md) | [Jazzy Cheats](../JazzyCheats/README.md)

I debated and decided to *release* this mod unhidden. Please give your critique and offer your support! Most script extenders aren't built in a couple of days and take teams. It takes time to get this right.

CoE5SE lets modders write Lua scripts for Conquest of Elysium 5 and adds Mod Settings to the main menu. Mods on it also get new .c5se words, their own settings pages, and room for more rituals, new spells, and new spell paths.

With it, a mod can add buttons and screens, put lines in the Game Menu, read keys, remember a setup between games, and go past 3000 rituals.

Built for game version 5.41. Still on 5.40? Download "CoE5SE for CoE5 5.40" from the Files tab.

## Functionality Out-of-the-Box

- Raises the ritual limit so mods can add up to 10000 rituals, so more big mods can be enabled together without running out of rituals.
- Lets modders add up to 3000 new spells.
- Lets modders add up to 58 new spell paths, with their own names (Chaos, Runecraft, Adeptus Mechanicus Battle Hymns), and rename the game's own.
- Tells a user when potentially save-breaking bugs occur like mods moving/replacing spells. This might be removed if troublesome.

## Mods Built on CoE5SE

- [Jazzy Game Presets](../JazzyGamePresets/README.md): save a new-game setup as a preset.
- [Jazzy Restart](../JazzyRestart/README.md): Restart in the Game Menu. Single player.
- [Jazzy Hotkeys](../JazzyHotkeys/README.md): change any key of the game's Keyboard Shortcuts list, on every screen. This... probably is a mistake. But here you go!
- [Jazzy UI Tweaks](../JazzyUITweaks/README.md): small changes to the game's own screens, like deleting saves from Load Game.

I'm a pianist ok? Aren't we all weird?

## Mod Settings

A button on the main menu, after Mods. Seeing it means CoE5SE is running.

Every enabled mod with settings gets a page. A change applies from then on, in new games and loaded saves. A mod's update doesn't reset your picks. A setting that needs a restart offers one. Reset to defaults on a mod's page puts all its settings back.

## Use at Your Own Risk

CoE5SE rewrites the game's code while it runs. That is how it does what a .c5m can't, and how it can crash, ruin saves, or just cause you a generally unpleasant time. A game update can break it, and every mod on it, until I update it. Also just... many games with mod support period are vulnerable, which compares to many many mods on nexus. There is no warranty. I will not help recover save files.

## Install

### Windows

1. Download CoE5SE from the Files tab and copy its CoE5SE folder into %APPDATA%\coe5\mods (paste that path into File Explorer's address bar).
1. Open the game folder (Steam: right-click Conquest of Elysium 5, Manage, Browse local files).
1. Copy glu32.dll from this mod's folder into the game folder, next to CoE5.exe. This mod's folder is:

   ```
   %APPDATA%\coe5\mods\CoE5SE\
   ```

   ![](https://staticdelivery.nexusmods.com/mods/10486/images/1/1-1792611690-493736941.gif)

1. Start the game. The first time, CoE5SE turns itself on and tells you so:

   ![](https://staticdelivery.nexusmods.com/mods/10486/images/1/1-1791869124-1713456765.png)

   If you don't see this message, enable CoE5SE on the Mods screen, click Exit, and restart the game.

### Mac

1. Download CoE5SE from the Files tab and copy its CoE5SE folder into ~/.coe5/mods (Finder: Go, Go to Folder).
1. Open the game folder (Steam: right-click Conquest of Elysium 5, Manage, Browse local files).
1. Copy coe5se.dylib from this mod's folder into the game folder, next to coe5_mac. Hold Option while you drag it, so Finder copies it instead of moving it. This mod's folder is:

   ```
   ~/.coe5/mods/CoE5SE/
   ```

   ![](https://staticdelivery.nexusmods.com/mods/10486/images/1/1-1792611724-1681850184.gif)

1. In Steam, click the gear on Conquest of Elysium 5's page, then Properties, General, and set Launch Options to:

   ```
   /usr/bin/env DYLD_INSERT_LIBRARIES=coe5se.dylib %command%
   ```

   ![](https://staticdelivery.nexusmods.com/mods/10486/images/1/1-1792611739-990505162.gif)

1. Start the game. The first time, CoE5SE turns itself on and tells you so. If it doesn't, enable CoE5SE on the Mods screen, click Exit, and restart the game.

## Uninstall

Disable CoE5SE on the Mods screen. To remove it fully, also delete glu32.dll from the game folder.

## Reporting a Problem

In the comments, with what happened, the enabled mods, and we might progress to looking at logs if the issue isn't obvious.

## For Modders

I'll admit, this will be rough until October. I have made all design decisions so backwards compatibility should be a thing, but you might be better leaving me until the first week of October to bake.

That said, here is a growing list of guides!

- [CoE5SE: Mod Command Tables](https://steamcommunity.com/sharedfiles/filedetails/?id=3812423193): An ever-updating list of commands
- [CoE5SE: A guide to adding spells](https://steamcommunity.com/sharedfiles/filedetails/?id=3810911368): Coding examples
- [CoE5SE: Small Modding Tidbits](https://steamcommunity.com/sharedfiles/filedetails/?id=3811708236): Short notes on CoE5SE that don't have a home elsewhere
- CoE5SE: Lua Command Tables: Coming soon!
- CoE5SE: A guide to game settings: Coming soon!
- CoE5SE: A guide to extended monster traits: Coming soon!

No Github for now and source is shipped with the mod. Have fun!

## Safety

Let me start with a cop out. All other script extenders I can find didn't have good safety standards and trusted mods explicitly. SKSE, Cyberpunk, whole teams. When you donload this base mod and then other mods on top, you DO carry some risk. For now, it's just me, but this might change.  
That said, I'm actively working on measures for those of us security minded.

As of right now, here are the measures applied:
- Source is available in the folder: You can build and verify hash from a "windows-verify.cmd" (Windows) or "mac-verify.command" (Mac) that compiles binaries against what it built to the coe5se .dll file already present.
- This script extender only supports lua, I tried very hard to make it so exploints cannot happen by other mods, including reading factorio white papers(my extender is inspired by their architecture).

## Not Affiliated with Illwinter

Illwinter makes Conquest of Elysium 5. CoE5SE is not their work; I'm one overly enthusiastic fan. A problem with CoE5SE goes in the comments here, not to Illwinter.

Script extenders have been around in PC modding for a long time (SKSE, F4SE; the name is a nod to them), and studios have mostly left them alone. That doesn't mean Illwinter has to. This project continues as long as Illwinter is fine with it. If they aren't, this be all but smoke.

## Why Did I Make This Mod

At a base level, I made this for myself. All the "Jazzy" mods listed above I've been wishing for since I first played this game! That said, I'm not a traditional modder. I hate creative writing and I hate sprites. So I decided my contribution to the community would be make other's stories a little bit easier to write and play. Hopefully, I'm not 5 years late to the table on this. If I am and there's no adoption, well, at least I made the mods I need to have a good time lol.

[All mods](../README.md#mods) | [Jazzy Game Presets](../JazzyGamePresets/README.md) | [Jazzy Hotkeys](../JazzyHotkeys/README.md) | [Jazzy Restart](../JazzyRestart/README.md) | [Jazzy UI Tweaks](../JazzyUITweaks/README.md) | [Jazzy Cheats](../JazzyCheats/README.md)
