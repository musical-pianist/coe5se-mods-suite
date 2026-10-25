#!/bin/bash
# Builds this mod's DLLs from this folder's code alone into the mod folder above, over the ones
# there, with llvm-mingw 20260922 (x86_64-w64-mingw32-clang, from
# https://github.com/mstorsjo/llvm-mingw/releases/download/20260922/). Set CC to its path if it isn't on PATH.
# That compiler gives byte-identical DLLs on any machine: compare `shasum -a 256 ../*.dll` with
# ../SHA256.txt. ../windows-verify.cmd (Windows) and ../mac-verify.command (Mac) download it and do
# the whole check.
set -e
cd "$(dirname "$0")"
CC=${CC:-x86_64-w64-mingw32-clang}
"$CC" -shared -O2 -Wall -Wextra -s -Wl,--no-insert-timestamp -o ../glu32.dll frozen_stub/glu32_stub.c frozen_stub/glu32.def -lbcrypt -luser32
WINDRES=${WINDRES:-x86_64-w64-mingw32-windres}
RES=$(mktemp -d)
trap 'rm -r "$RES"' EXIT
"$WINDRES" -O coff -o "$RES/coe5se_loader.rc.o" coe5se_loader.rc
"$CC" -shared -O2 -Wall -Wextra -s -Wl,--no-insert-timestamp -o ../coe5se_loader.dll coe5se_loader.c -lbcrypt -luser32 "$RES/coe5se_loader.rc.o"
"$WINDRES" -O coff -o "$RES/coe5se.c5plugin.rc.o" coe5se.c5plugin.rc
"$CC" -shared -O2 -Wall -Wextra -s -Wl,--no-insert-timestamp -o ../coe5se.c5plugin.dll coe5se_plugin.c fanpatch/fix_feeblemind.c fanpatch/fix_item_reset.c fanpatch/fix_token_sprites.c lua/lapi.c lua/lauxlib.c lua/lbaselib.c lua/lcode.c lua/lcorolib.c lua/lctype.c lua/ldebug.c lua/ldo.c lua/ldump.c lua/lfunc.c lua/lgc.c lua/llex.c lua/lmathlib.c lua/lmem.c lua/lobject.c lua/lopcodes.c lua/lparser.c lua/lstate.c lua/lstring.c lua/lstrlib.c lua/ltable.c lua/ltablib.c lua/ltm.c lua/lundump.c lua/lutf8lib.c lua/lvm.c lua/lzio.c -I. -Ilua -lbcrypt "$RES/coe5se.c5plugin.rc.o"
