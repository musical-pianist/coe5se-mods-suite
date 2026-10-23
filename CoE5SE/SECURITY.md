# CoE5SE security notes

CoE5SE (the CoE5 Script Extender) adds Lua scripting, Mod Settings, and extra mod commands to
Conquest of Elysium 5. It does this with native code that runs inside the game's process. This file
lists every way that code affects the system, where each behavior lives in `src/`, and how to check
that the shipped binaries were built from `src/`.

Report a security problem through CoE5SE's Nexus Mods page:
https://www.nexusmods.com/mods/1?game_id=10486

## The binaries

| File | Platform | Installed in | What it is | Source |
|---|---|---|---|---|
| `glu32.dll` | Windows | the game folder (the player copies it once) | The stub. CoE5.exe imports `glu32.dll` and ships none, so Windows loads this file. Every one of its 52 exports is a one-instruction jump into the real `System32\glu32.dll`. | `src/frozen_stub/glu32_stub.c`, `src/frozen_stub/stub.h` |
| `coe5se.dylib` | Mac | the game folder (the player copies it once) | The stub. The player adds the Steam launch option `/usr/bin/env DYLD_INSERT_LIBRARIES=coe5se.dylib %command%`; dyld runs its constructor before the game's `main`. The constructor only points the game's import of `srand` at the stub, so the stub's steps run when the game first calls `srand`, near the top of `main`, on its main thread. | `src/frozen_stub/mac_stub.c`, `src/frozen_stub/stub.h` |
| `coe5se_loader.dll`, `coe5se_loader.dylib` | both | CoE5SE's mod folder | The loader. Picks which plugin DLLs load, checks each one's SHA-256 first. | `src/coe5se_loader.c`, `src/approve_dll.h` |
| `coe5se.c5plugin.dll`, `coe5se.c5plugin.dylib` | both | CoE5SE's mod folder | CoE5SE itself: the game patches, the Lua host, Mod Settings, and the mod commands. | `src/coe5se_plugin.c` (its commented `#include` list at the bottom names the file of each feature) |

Their SHA-256 hashes are in `SHA256.txt`, in the format `shasum -a 256 -c SHA256.txt` and `sha256sum -c SHA256.txt` read.

## How it starts

1. The game starts and the OS loads the stub (Windows: through the game's own import of `glu32.dll`; Mac: through the launch option above).
2. The stub (`src/frozen_stub/stub.h`) finds the game's user folder by the game's own rule (`%APPDATA%\coe5`, `~/.coe5`, or `COE5_SAVE`), finds CoE5SE's mod folder, and reads the game's `coe5config` to see whether the CoE5SE mod is enabled. If it isn't, nothing more loads and the game runs unmodified.
3. Once per player, if CoE5SE has never been enabled, the stub enables it in `coe5config` (`src/frozen_stub/auto_enable.h`). It records that in `auto_enable_done.txt` first, so a player who later unticks CoE5SE keeps it off.
4. The stub loads `coe5se_loader` from CoE5SE's mod folder and calls its one export.
5. The loader (`src/coe5se_loader.c`) builds the list of enabled mods in load order and loads one plugin: CoE5SE's own. Before loading, it opens the file so it can't be swapped, hashes it, and checks the hash against `coe5se/approved.txt` in the user folder (`src/approve_dll.h`, `src/approvals.h`). Every other mod's plugin DLL is refused and logged (`setAsideModPlugins`), with no exception.
6. CoE5SE checks the game build. It knows each supported build by the exe's link timestamp and image size (Windows) or its Mach-O UUID (Mac) (`KNOWN_BUILDS` in `src/coe5se_plugin.c`). On any other build it asks the player with a Yes/No box, No being the default, and on No it installs nothing.

If any step fails, the stub or loader writes `loader_inactive` with the reason to the log and the game runs unmodified.

## What it changes in the game's process

- **The game code check.** Before its first change, CoE5SE takes the SHA-256 of the game's code section in memory (Windows `.text`, Mac `__TEXT,__text`) and compares it with the hash built in for that game build (`src/game_code_hash.h`, `KNOWN_BUILDS` in `src/coe5se_plugin.c`; logged as `game_code_check`). It only reads. Every patch below but the call-site patches runs only when the hash matches; otherwise it changes nothing and logs `reason=game_code_differs`. CoE5SE carries no copy of the game's code to compare with: each write is built from the instruction in memory.
- **Call-site patches.** CoE5SE redirects individual call instructions in the game's code to its own functions. On Windows each patch rewrites one 5-byte `call`, or one 6-byte `call` through an import slot into a 5-byte `call` and a `nop` (`VirtualProtect` around the write; `src/common.h`, `src/coe5se_plugin.c`). On Mac each patch rewrites one `bl` to point at a trampoline placed in the zero padding of the game's own `__TEXT` (`mach_vm_protect` around the write; `src/patch_arm64.h`). A site whose bytes don't match what CoE5SE expects is skipped and logged as `site_skipped`.
- **Trace filter (Windows).** Only after a mod's script calls `coe5.keepTraceLines` with a list: CoE5SE replaces the first instruction of the game's four debug trace functions with a jump to its own filter, in one aligned 8-byte write each (`VirtualProtect` around the write), and only when the game code check matched (logged as `trace_filter_check` and `trace_filter_installed`). The filter decides which debug lines reach the game's own `log.txt`; it writes nothing itself (`src/trace_filter.h`).
- **The AI army split's minimum (Windows).** Only while a mod's Lua wrap of `AI_SPLIT_ARMY` passes another minimum to `next`: CoE5SE writes it into the two 4-byte immediates of the game's split that hold the minimum (a `mov` and a `cmp`, `VirtualProtect` around each write) for that call and writes the game's own back after it; the site goes in only when the game code check matched and both instructions hold the same minimum (logged as `ai_split_min_units_check`; `src/lua_sites.h`, `callSplitArmyNext`).
- **Game start test (Windows).** When CoE5SE points the game's start call at its `GAME_START` site: since CoE5 5.41 the game's start preparation runs once a launch, and the game's code tests a done flag right before the call, jumping over it once the preparation has run. CoE5SE replaces that 2-byte `jne` with a 2-byte `nop` (`VirtualProtect` around the write), so the site runs in every game, and its own function makes the same test before calling the game's preparation, which still runs once a launch. Only when the game code check matched (logged as `game_start_once_check` when it didn't, and `game_start_once_removed`; `src/coe5se_plugin.c`, "GAME_START in every game").
- **Seal event step (Windows).** Only when a mod first wraps `SEAL_EVENT`: CoE5SE writes a small stub in a page it allocates near the game (one call, three jumps back into the game's own code, and a `mov eax, 1; ret` standing in for the game), registers the game function's own unwind codes for it (`RtlAddFunctionTable`), and replaces 9 bytes of the game's month-end seal step with a jump to it and four `nop`s (`VirtualProtect` around the write); only when the game code check matched (logged as `seal_event_step_check` and `seal_event_patched`; `src/seal_events.h`).
- **How code is written (Windows).** Every write CoE5SE makes into the game's code goes through one function, `writeGameCode` in `src/common.h`: it opens only the pages the bytes are on with `VirtualProtect`, checks that the change was allowed (a refusal writes nothing and is logged as `code_unwritable`), copies the bytes, puts the pages' protection back, and clears the instruction cache. A patch of many writes (the table moves, the Ritual Expansion, the spell paths) opens the game's code section once for all of them (`openGameCode`, checked the same way) and puts its protection back after (`closeGameCode`). A patch that needs several writes puts back the ones already made when a later one is refused, so the game's code is all patched or all its own. The Mac build has a function of the same name (`src/patch_arm64.h`).
- **Executable memory.** CoE5SE allocates small executable pages for its stubs and the gates between the game and its functions: near the game's image on Windows (`allocNear` and `allocNearBytes` in `src/common.h`, the gate pages in `src/coe5se_plugin.c`) and with `mmap` then `mprotect` to read-execute on Mac (`src/patch_arm64.h`). On Windows each page is set read-execute as soon as its code is written (`makePagesReadExecute`), and a later change to it goes through `writeGameCode`; what the stubs and gates store and read while they run sits on separate read-write pages that are not executable. A page whose protection can't be changed stays as it was allocated and is logged as `page_protection_kept`.
- **Moved tables.** To raise the game's ritual and spell limits, CoE5SE moves those tables to memory it allocates (read-write, not executable) and repoints the game's references (`src/ritual_expansion.h`, `src/spell_table_move.h`). On Windows it does the same for the weapon, item, class and monster tables and the arrays the game sizes by the monster table, raises their bounds and sizes in place (one immediate each), and makes the stack frames of the four game functions that keep per-monster arrays bigger: their frame size, the stack offsets in them, and their unwind data in the game's read-only data (each page opened for its own bytes and put back). Each table moves only when the game code check matched, all or nothing per table (logged as `table_not_moved` when it doesn't), and the old tables' whole pages become unreadable (`src/table_move.h`, `src/*_table_sites.h`, `src/monster_frame_sites.h`). Only when a mod declares a plane, it does the same for the plane table, its camera arrays and the map draw's per-plane image arrays, and raises the game's plane-number bounds in place (one immediate or one instruction word each, also only when the game code check matched; `src/plane_table_move.h`).
- **Terrain looks.** Only when a mod declares a terrain's looks (`terrainlooks`, `coe5.setTerrainLooks`): CoE5SE replaces the instructions where the game's ground pass reads a square's terrain with a jump to a stub that reads the number through its own table first; with a ground picture, it replaces the first instruction of the game's ground mixing step with a jump to its own copy of that step, and points the plane image builder's call at a stub that notes the plane; with decorations, it points the map's decoration call at its own function. Each write waits for the game code check (logged as `hook_skipped` when it didn't match) and made with `VirtualProtect` (Windows) or into the padding of the game's own `__TEXT` (Mac); its stubs sit in the pages above (`src/terrain_looks.h`). It reads the mod's pictures as files in that mod's folder.
- **The fan patch's game fixes.** CoE5SE carries three fixes from a modder's fan patch (`src/fanpatch/`: the item reset, Feebleminded casters, battle token item pictures). They hook seven game functions at their start: on Windows the first 5 to 8 bytes become a jump to a stub in a page near the game, which jumps to the fix; on Mac the first 16 bytes become a jump through `x16`, with the replaced instructions copied into the padding of the game's own `__TEXT` to run the game's function. Each hook waits for the game code check (logged as `entry_hook_refused` when it didn't match, `entry_hook_set` when written; `src/fanpatch/fanpatch_host.h`, `src/fanpatch/entry_hook_sites.h`). The item reset keeps a copy of the item table in memory it allocates. In a network game the fixes leave the game's behaviour as it is.
- **Crash line (Windows).** When the game crashes, CoE5SE writes one line to the log and then lets the crash continue as it would without CoE5SE (`src/crash_log.h`).

## Files it reads and writes

| Where | File | Written when | Source |
|---|---|---|---|
| Game folder | `coe5mod.log`, `coe5mod.1.log` to `coe5mod.4.log` (100 MB each at most) | every launch | `src/log_files.h`, `src/startup_log.h` |
| Game folder | `glu32.dll` (Windows) or `coe5se.dylib` (Mac) | only when the player answers Yes to the main-menu offer of a newer stub. Only a file whose SHA-256 is built into CoE5SE (`src/stub_hash.h`) is copied. On Windows the running stub is renamed `glu32.dll.old` first and deleted at the next launch. | `src/stub_update.h` |
| User folder | `coe5config` (the game's own settings file) | once per player, to enable CoE5SE (step 3 above); a copy of the original is kept as `coe5se/coe5config_before_auto_enable` | `src/frozen_stub/auto_enable.h`, `src/coe5config.h` |
| User folder | `coe5se/approved.txt`, `coe5se/auto_enable_done.txt`, `coe5se/stub_update_offered.txt` | when a plugin is approved, at the one-time enable, after a stub offer | `src/approvals.h`, `src/frozen_stub/auto_enable.h`, `src/stub_update.h` |
| User folder | `coe5seModMenu/<ModName>.ini` | when the player changes a Mod Settings choice | `src/settings_files.h` |
| User folder | `coe5se_data/<ModName>/` and its subfolders | when a mod's Lua script saves data. Each mod writes only its own folder: 1 MB per file, 10 MB per mod unless the mod's `.c5se` sets up to 100 MB with `maxdata`, names checked (no path separators, no `..`, no device names) | `src/mod_storage.h`, `src/data_area_max.h`, `src/lua_data_files.h` |
| User folder | `oldworkshop` (Mac only) | at launch: the Mac game can't ask Steam for its Workshop items, so the loader writes the list the game would have written | `src/workshop_cache.h` |

The user folder is `%APPDATA%\coe5` on Windows, `~/.coe5` on Mac, or the folder `COE5_SAVE` names.
Every rewrite of a settings or list file goes to a new file first and replaces the old one in one
step, so a crash never leaves half a file.

## What it never does

- **No network access.** No file in `src/` calls a socket, HTTP, or DNS function.
- **No other programs.** The only process CoE5SE starts is the game itself: it restarts CoE5 (same executable, arguments, and environment) after the stub update, and when the set of enabled mods with code changes during a session (`relaunchGame` in `src/restart_prompt.h`).
- **No registry access.** No file in `src/` calls a registry function.
- **No code from other mods.** Other mods' plugin DLLs are refused (step 5). Mods extend the game with Lua scripts, which run in the sandbox below.

## The Lua sandbox

Each enabled mod's `<ModName>.lua` runs in its own Lua 5.5.1 state (`src/lua_loading.h`, `newLuaSandbox`).

- Only the base, string, table, math, utf8, and coroutine libraries are opened. `io`, `os`, `package`, and `debug` are never opened, and `dofile`, `loadfile`, `load`, and `collectgarbage` are removed.
- `require` is CoE5SE's own (`lRequire`): it reads a `.lua` file only from inside the mod's own folder, named by letters, digits, and `_` with a dot for each subfolder, stored under exactly that name, and never through a symbolic link, junction, or other reparse point.
- Scripts, and the files they require, load as text only.
- A script reaches the game only through the `coe5` table (`coe5.lua` lists every function). Values a script passes are checked against the game's own ranges; a refused call logs `lua_refused` with the reason.
- One call into a script may run at most 20,000,000 Lua steps (loops inside the string and table libraries count too). A script that spends that, or raises an error, is turned off for the rest of the session and the game continues.
- Memory: 1.5 GB per script and 6 GB for all scripts together. Typically scripts should call at most 10s of MB of ram, but I found these limits useful to allow for some modder inefficency that can be easily ran on modern systems. As example, a game that nears the 6gb limit for scripts would at most cohesively take 8gb of ram even in the late-game.
- `%p` in a format string prints an id, never an address.
- In network games no script runs.

Vendored Lua is in `src/lua/`. CoE5SE's changes to it are marked `CoE5SE:` in the code.

## Checking that the binaries match the source

`windows-verify.cmd` (Windows 10 1803 or later) and `mac-verify.command` (any Apple Silicon Mac) each
rebuild all six binaries from `src/` and compare them with the shipped files byte for byte. Either
script checks both platforms' files. Double-click it, or run it from a terminal with `nopause`.

What each script does, in the order it prints:

1. Downloads two pinned compilers into a temporary folder and checks each download's SHA-256 before
   using it. A mismatch stops the run.
2. Prints each compiler's own version line.
3. Rebuilds every file from `src/` into the temporary folder, printing the exact command for each.
4. Compares each shipped file with its rebuild and with its line in `SHA256.txt`. A file is OK only
   when all three name the same SHA-256.

Then it prints `Result: OK, every DLL and dylib matches its source.` or `Result: FAIL`, deletes the
temporary folder, and exits. It installs nothing and changes nothing outside the temporary folder.

### The pinned compilers

| Compiler | Builds | Archive | SHA-256 |
|---|---|---|---|
| llvm-mingw 20260922 (clang for Windows targets) | the 3 DLLs | Windows: https://github.com/mstorsjo/llvm-mingw/releases/download/20260922/llvm-mingw-20260922-ucrt-x86_64.zip | `e3ad77d117a4bea19a7a3b333341824d79a5a371004a10e25b8504e7b3047666` |
| | | Mac: https://github.com/mstorsjo/llvm-mingw/releases/download/20260922/llvm-mingw-20260922-ucrt-macos-universal.tar.xz | `52e5f5a7b131021d0c39a37a38fa380a1da7885cd04bd61afd0cd4ecfb8bc1f3` |
| zig 0.16.0 (`zig cc`, clang with its own macOS headers) | the 3 dylibs | Windows: https://ziglang.freetls.fastly.net/zig-x86_64-windows-0.16.0.zip | `68659eb5f1e4eb1437a722f1dd889c5a322c9954607f5edcf337bc3684a75a7e` |
| | | Mac: https://ziglang.freetls.fastly.net/zig-aarch64-macos-0.16.0.tar.xz | `b23d70deaa879b5c2d486ed3316f7eaa53e84acf6fc9cc747de152450d401489` |

Both are the upstream projects' own release archives. llvm-mingw comes from its GitHub releases. zig
comes from the Zig Software Foundation's mirror (ziglang.freetls.fastly.net), with ziglang.org as the
fallback. Each compiler's Windows and Mac archives produce the same bytes, which is why either
script can check all six files.

### Why the builds are reproducible

The commands are in `src/build.sh` (DLLs) and `src/build_mac.sh` (dylibs).

- DLL flags: `-shared -O2 -Wall -Wextra -s -Wl,--no-insert-timestamp`. `--no-insert-timestamp` leaves the PE timestamp at zero. `-s` drops
  the symbol table and debug sections, which would otherwise hold the compiler package's build
  folder.
- dylib flags: `-target aarch64-macos.11.0 -dynamiclib -O2 -Wall -Wextra -fstack-protector -g0 -s`, plus `-install_name <file name>`. `-g0 -s` keep object paths and times
  out of the file. `-install_name` replaces the default, which would be the build folder's path. The
  zig target is named, so zig uses only its own macOS headers and never looks for Xcode or an SDK.
  The few declarations those headers lack (four CoreFoundation calls, `mach_vm.h`, OpenGL) are in
  `src/mac_sdk/`, written by us, with `.tbd` link stubs. zig's linker adds the ad-hoc code signature
  arm64 requires, and that signature is the same on every run.
- Source files are passed in a fixed order (sorted in the C locale), because link order changes the
  bytes.

### What a matching result shows

A match shows that the shipped binaries are exactly what `src/` produces with these two compilers.
It doesn't show that the code is harmless; that is what the sections above and the source are for.
It relies on the compiler archives being the upstream releases; their hashes are pinned, and anyone
can rebuild with a copy of the same releases from another source.

### Checking by hand

- Hashes only: `shasum -a 256 -c SHA256.txt` (Mac, Linux) or
  `Get-FileHash -Algorithm SHA256 <file>` (Windows PowerShell), compared with `SHA256.txt`.
- A full rebuild without the scripts: download the archives above, check their SHA-256, then run
  `src/build.sh` with `CC` set to llvm-mingw's `x86_64-w64-mingw32-clang`, and `src/build_mac.sh`
  with `ZIG` set to zig. Both write over the binaries in this folder, so run them on a copy.
