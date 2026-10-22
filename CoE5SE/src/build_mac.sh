#!/bin/bash
# Builds this mod's Mac dylibs from this folder's code alone into the mod folder above, over the ones
# there, for arm64 and macOS 11.0 like the game, with zig 0.16.0 (zig-aarch64-macos-0.16.0.tar.xz
# from https://ziglang.org/download/, or zig-x86_64-windows-0.16.0.zip on Windows). Set ZIG to
# its path if it isn't on PATH. It needs no Xcode: zig brings its own macOS headers, and mac_sdk/
# declares the few it lacks.
# That compiler gives byte-identical dylibs on any machine: compare `shasum -a 256 ../*.dylib` with
# ../SHA256.txt. ../mac-verify.command (Mac) and ../windows-verify.cmd (Windows) download it and do
# the whole check.
set -e
cd "$(dirname "$0")"
ZIG=${ZIG:-zig}
"$ZIG" cc -target aarch64-macos.11.0 -dynamiclib -O2 -Wall -Wextra -fstack-protector -g0 -s -install_name coe5se.dylib -o ../coe5se.dylib frozen_stub/mac_stub.c -isystem frozen_stub/mac_sdk frozen_stub/mac_sdk/CoreFoundation.tbd
"$ZIG" cc -target aarch64-macos.11.0 -dynamiclib -O2 -Wall -Wextra -fstack-protector -g0 -s -install_name coe5se_loader.dylib -o ../coe5se_loader.dylib coe5se_loader.c -isystem mac_sdk mac_sdk/CoreFoundation.tbd
"$ZIG" cc -target aarch64-macos.11.0 -dynamiclib -O2 -Wall -Wextra -fstack-protector -g0 -s -install_name coe5se.c5plugin.dylib -o ../coe5se.c5plugin.dylib coe5se_plugin.c fanpatch/fix_feeblemind.c fanpatch/fix_item_reset.c fanpatch/fix_token_sprites.c lua/lapi.c lua/lauxlib.c lua/lbaselib.c lua/lcode.c lua/lcorolib.c lua/lctype.c lua/ldebug.c lua/ldo.c lua/ldump.c lua/lfunc.c lua/lgc.c lua/llex.c lua/lmathlib.c lua/lmem.c lua/lobject.c lua/lopcodes.c lua/lparser.c lua/lstate.c lua/lstring.c lua/lstrlib.c lua/ltable.c lua/ltablib.c lua/ltm.c lua/lundump.c lua/lutf8lib.c lua/lvm.c lua/lzio.c -I. -Ilua -isystem mac_sdk mac_sdk/CoreFoundation.tbd
