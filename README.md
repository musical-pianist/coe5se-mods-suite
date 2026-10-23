# CoE5SE and Jazzy mods

This repo is temporary: it will be deleted once Nexus Mods approves the mods' pages, which will be their home.

## Install with one command

This installs the latest CoE5SE, Jazzy Game Presets, Jazzy Hotkeys, Jazzy Restart, Jazzy UI Tweaks and Jazzy Cheats, and copies CoE5SE's file into the game folder. Close the game first. Running it again later updates them.

**Windows:** open the Start menu, type PowerShell, open Windows PowerShell, then paste this and press Enter:

```
irm https://raw.githubusercontent.com/musical-pianist/coe5se_mods_suite/main/install.ps1 | iex
```

**Mac:** open Terminal (in Applications, Utilities), then paste this and press Return:

```
curl -fsSL https://raw.githubusercontent.com/musical-pianist/coe5se_mods_suite/main/install.sh | bash
```

On Mac, the first install leaves one step: in Steam, click the gear on Conquest of Elysium 5's page, then Properties, General, and set Launch Options to `/usr/bin/env DYLD_INSERT_LIBRARIES=coe5se.dylib %command%`. The script prints this step only while Launch Options still need it, so an update doesn't ask again.

Then start the game. The first time, CoE5SE turns itself on and tells you so. Turn on the Jazzy mods you want on the Mods screen.

You can read both scripts before running them: [install.ps1](install.ps1), [install.sh](install.sh).

## Install by hand

Each mod is also a zip under Releases. Copy its folder into your mods folder:

- Windows: `%APPDATA%\coe5\mods\`
- Mac: `~/.coe5/mods/`

CoE5SE needs one more step, copying one file into the game folder: open `CoE5SE/how-to-install.html` for the steps.

Terms of use: [LICENSE.txt](LICENSE.txt).

<!-- mods start -->
## Mods

Each mod's folder has its full description, the same text as its Nexus Mods page.

- [CoE5SE](CoE5SE/README.md): CoE5SE extends mod tools with higher limits and Lua scripting (v5.40, Windows and Mac) and adds Mod Settings to the main menu. Mods built on it can add buttons, screens, Game Menu lines, and key bindings, and go past the game's limits. Up
- [Jazzy Game Presets](JazzyGamePresets/README.md): Save your whole new-game setup (map, society, every player slot, every AI's difficulty) as a preset, load it next time, and change the AI and game rules in a game you're already playing. Needs CoE5SE.
- [Jazzy Hotkeys](JazzyHotkeys/README.md): Change any key of the game's Keyboard Shortcuts list: 196 actions on 10 screens, picked in Mod Settings. Needs CoE5SE.
- [Jazzy Restart](JazzyRestart/README.md): Restart in the Game Menu. It ends the game and starts a new world with the same settings, no relaunch. Needs CoE5SE.
- [Jazzy UI Tweaks](JazzyUITweaks/README.md): Delete saves from Load Game, hide the Read Tip button, shrink or hide the mod banners on the menu screens, and see what a ritual needs to cast. You can turn each one back to vanilla in Mod Settings, Jazzy UI Tweaks. Needs CoE5SE.
<!-- mods end -->
