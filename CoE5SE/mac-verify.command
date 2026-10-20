#!/bin/bash
# Checks that every DLL and dylib in this mod was built from the code in its src folder:
# glu32.dll, coe5se_loader.dll, coe5se.c5plugin.dll, coe5se.dylib, coe5se_loader.dylib, coe5se.c5plugin.dylib.
# It downloads the two compilers they were built with (llvm-mingw for the DLLs, zig for the dylibs,
# each one pinned archive checked by its SHA-256), builds src into a temporary folder, and compares
# each rebuilt file with the one in this folder: OK means byte-identical. Nothing is installed, and
# the temporary folder is deleted at the end. windows-verify.cmd does the same on Windows, with the
# same compilers' Windows builds, which give the same bytes. Needs an Apple Silicon Mac; it uses only
# curl, shasum, and tar, which every Mac has, and never Xcode or its Command Line Tools.
# Run it by double-clicking (macOS opens it in Terminal), or from a terminal: ./mac-verify.command [nopause]
# shellcheck disable=SC2086,SC2034,SC2043  # flag lists split on purpose; the build lines fill in the template

MOD=$(cd "$(dirname "$0")" && pwd)
TC_URL="https://github.com/mstorsjo/llvm-mingw/releases/download/20260922/llvm-mingw-20260922-ucrt-macos-universal.tar.xz"
TC_SHA256="52e5f5a7b131021d0c39a37a38fa380a1da7885cd04bd61afd0cd4ecfb8bc1f3"
TC_DIR="llvm-mingw-20260922-ucrt-macos-universal"
ZIG_URL_1="https://ziglang.freetls.fastly.net/zig-aarch64-macos-0.16.0.tar.xz"
ZIG_URL_2="https://ziglang.org/download/0.16.0/zig-aarch64-macos-0.16.0.tar.xz"
ZIG_SHA256="b23d70deaa879b5c2d486ed3316f7eaa53e84acf6fc9cc747de152450d401489"
ZIG_DIR="zig-aarch64-macos-0.16.0"
FLAGS="-shared -O2 -Wall -Wextra -s -Wl,--no-insert-timestamp"
MAC_FLAGS="-target aarch64-macos.11.0 -dynamiclib -O2 -Wall -Wextra -fstack-protector -g0 -s"
WORK=$(mktemp -d "${TMPDIR:-/tmp}/coe5_verify.XXXXXX") || exit 1
OUT="$WORK/out"
BUILD_FAILED=""
FAILED=""
PAUSE_AT_END=""
# A double-click opens a Terminal window: pause at the end then (whenever input is a terminal), so
# the result stays readable.
[ "${1:-}" != nopause ] && [ -t 0 ] && PAUSE_AT_END=1
# The temporary folder goes even when the run is stopped with Ctrl-C.
trap 'cd /; rm -rf "$WORK"' EXIT

# Prints the SHA-256 of the file $1 in lowercase, or nothing if it's missing.
hashOf() {
  [ -f "$1" ] && shasum -a 256 "$1" | cut -d' ' -f1
}

# Downloads the URL $2 into the file $3 and checks it against the SHA-256 $4; $1 names the compiler.
# Fails when the download fails or gives another file.
fetch() {
  local got
  echo "    from:      $2"
  echo "    expected:  $4"
  curl -fL --retry 3 -sS -o "$3" "$2" || { echo "    download failed"; return 1; }
  got=$(hashOf "$3")
  if [ "$got" != "$4" ]; then
    echo "    got:       $got  MISMATCH, not used"
    return 1
  fi
  echo "    got:       $got  OK"
}

# Compares the shipped file $1 with its rebuild and with its line in SHA256.txt: OK only when all
# three name the same SHA-256.
compare() {
  local shipped rebuilt listed=no
  shipped=$(hashOf "$MOD/$1")
  rebuilt=$(hashOf "$OUT/$1")
  grep -qx "$shipped  $1" "$MOD/SHA256.txt" 2>/dev/null && listed=yes
  if [ -n "$shipped" ] && [ "$shipped" = "$rebuilt" ] && [ $listed = yes ]; then
    echo "  OK    $1  $rebuilt"
    return
  fi
  FAILED=1
  echo "  FAIL  $1"
  echo "        shipped:            $shipped"
  echo "        rebuilt:            $rebuilt"
  echo "        listed in SHA256.txt: $listed"
}

verify() {
  mkdir -p "$OUT" || return 1
  echo "Step 1 of 4: download the two pinned compilers and check their SHA-256"
  echo "  llvm-mingw 20260922 (builds the Windows DLLs)"
  fetch llvm-mingw "$TC_URL" "$WORK/tc.tar.xz" "$TC_SHA256" || return 1
  echo "  zig 0.16.0 (builds the Mac dylibs)"
  fetch zig "$ZIG_URL_1" "$WORK/zig.tar.xz" "$ZIG_SHA256" || fetch zig "$ZIG_URL_2" "$WORK/zig.tar.xz" "$ZIG_SHA256" || return 1
  tar -xf "$WORK/tc.tar.xz" -C "$WORK" || return 1
  tar -xf "$WORK/zig.tar.xz" -C "$WORK" || return 1
  CC="$WORK/$TC_DIR/bin/x86_64-w64-mingw32-clang"
  ZIG="$WORK/$ZIG_DIR/zig"
  # zig keeps its cache in the temporary folder too, so nothing stays behind.
  export ZIG_GLOBAL_CACHE_DIR="$WORK/zig-cache" ZIG_LOCAL_CACHE_DIR="$WORK/zig-cache"

  echo
  echo "Step 2 of 4: the compilers' own version lines"
  echo "    $("$CC" --version | head -1)"
  echo "    zig $("$ZIG" version), $("$ZIG" cc --version | head -1)"

  echo
  echo "Step 3 of 4: rebuild each file from src/ into the temporary folder $WORK"
  cd "$MOD/src" || return 1
  echo "  glu32.dll"
  echo "    > \"$CC\" $FLAGS -o \"$OUT/glu32.dll\" frozen_stub/glu32_stub.c frozen_stub/glu32.def -lbcrypt -luser32"
  "$CC" $FLAGS -o "$OUT/glu32.dll" frozen_stub/glu32_stub.c frozen_stub/glu32.def -lbcrypt -luser32 || BUILD_FAILED=1
  echo "  coe5se_loader.dll"
  echo "    > \"$CC\" $FLAGS -o \"$OUT/coe5se_loader.dll\" coe5se_loader.c -lbcrypt -luser32"
  "$CC" $FLAGS -o "$OUT/coe5se_loader.dll" coe5se_loader.c -lbcrypt -luser32 || BUILD_FAILED=1
  echo "  coe5se.c5plugin.dll"
  echo "    > \"$CC\" $FLAGS -o \"$OUT/coe5se.c5plugin.dll\" coe5se_plugin.c lua/lapi.c lua/lauxlib.c lua/lbaselib.c lua/lcode.c lua/lcorolib.c lua/lctype.c lua/ldebug.c lua/ldo.c lua/ldump.c lua/lfunc.c lua/lgc.c lua/llex.c lua/lmathlib.c lua/lmem.c lua/lobject.c lua/lopcodes.c lua/lparser.c lua/lstate.c lua/lstring.c lua/lstrlib.c lua/ltable.c lua/ltablib.c lua/ltm.c lua/lundump.c lua/lutf8lib.c lua/lvm.c lua/lzio.c -Ilua -lbcrypt"
  "$CC" $FLAGS -o "$OUT/coe5se.c5plugin.dll" coe5se_plugin.c lua/lapi.c lua/lauxlib.c lua/lbaselib.c lua/lcode.c lua/lcorolib.c lua/lctype.c lua/ldebug.c lua/ldo.c lua/ldump.c lua/lfunc.c lua/lgc.c lua/llex.c lua/lmathlib.c lua/lmem.c lua/lobject.c lua/lopcodes.c lua/lparser.c lua/lstate.c lua/lstring.c lua/lstrlib.c lua/ltable.c lua/ltablib.c lua/ltm.c lua/lundump.c lua/lutf8lib.c lua/lvm.c lua/lzio.c -Ilua -lbcrypt || BUILD_FAILED=1
  echo "  coe5se.dylib"
  echo "    > \"$ZIG\" cc $MAC_FLAGS -install_name coe5se.dylib -o \"$OUT/coe5se.dylib\" frozen_stub/mac_stub.c -isystem frozen_stub/mac_sdk frozen_stub/mac_sdk/CoreFoundation.tbd"
  "$ZIG" cc $MAC_FLAGS -install_name coe5se.dylib -o "$OUT/coe5se.dylib" frozen_stub/mac_stub.c -isystem frozen_stub/mac_sdk frozen_stub/mac_sdk/CoreFoundation.tbd || BUILD_FAILED=1
  echo "  coe5se_loader.dylib"
  echo "    > \"$ZIG\" cc $MAC_FLAGS -install_name coe5se_loader.dylib -o \"$OUT/coe5se_loader.dylib\" coe5se_loader.c -isystem mac_sdk mac_sdk/CoreFoundation.tbd"
  "$ZIG" cc $MAC_FLAGS -install_name coe5se_loader.dylib -o "$OUT/coe5se_loader.dylib" coe5se_loader.c -isystem mac_sdk mac_sdk/CoreFoundation.tbd || BUILD_FAILED=1
  echo "  coe5se.c5plugin.dylib"
  echo "    > \"$ZIG\" cc $MAC_FLAGS -install_name coe5se.c5plugin.dylib -o \"$OUT/coe5se.c5plugin.dylib\" coe5se_plugin.c lua/lapi.c lua/lauxlib.c lua/lbaselib.c lua/lcode.c lua/lcorolib.c lua/lctype.c lua/ldebug.c lua/ldo.c lua/ldump.c lua/lfunc.c lua/lgc.c lua/llex.c lua/lmathlib.c lua/lmem.c lua/lobject.c lua/lopcodes.c lua/lparser.c lua/lstate.c lua/lstring.c lua/lstrlib.c lua/ltable.c lua/ltablib.c lua/ltm.c lua/lundump.c lua/lutf8lib.c lua/lvm.c lua/lzio.c -Ilua -isystem mac_sdk mac_sdk/CoreFoundation.tbd"
  "$ZIG" cc $MAC_FLAGS -install_name coe5se.c5plugin.dylib -o "$OUT/coe5se.c5plugin.dylib" coe5se_plugin.c lua/lapi.c lua/lauxlib.c lua/lbaselib.c lua/lcode.c lua/lcorolib.c lua/lctype.c lua/ldebug.c lua/ldo.c lua/ldump.c lua/lfunc.c lua/lgc.c lua/llex.c lua/lmathlib.c lua/lmem.c lua/lobject.c lua/lopcodes.c lua/lparser.c lua/lstate.c lua/lstring.c lua/lstrlib.c lua/ltable.c lua/ltablib.c lua/ltm.c lua/lundump.c lua/lutf8lib.c lua/lvm.c lua/lzio.c -Ilua -isystem mac_sdk mac_sdk/CoreFoundation.tbd || BUILD_FAILED=1
  cd "$MOD" || return 1
  [ -z "$BUILD_FAILED" ] || return 1

  echo
  echo "Step 4 of 4: compare each shipped file with its rebuild and with SHA256.txt"
  for file in glu32.dll coe5se_loader.dll coe5se.c5plugin.dll coe5se.dylib coe5se_loader.dylib coe5se.c5plugin.dylib; do compare "$file"; done
}

echo "CoE5SE build check"
echo "Rebuilds the 6 DLLs and dylibs in this folder from the source in src/ and compares them byte"
echo "for byte. SECURITY.md explains each step."
echo
if ! verify; then
  FAILED=1
  echo "Verification could not finish."
fi
cd "$MOD" || true
rm -rf "$WORK"
echo
if [ -n "$FAILED" ]; then echo "Result: FAIL"; else echo "Result: OK, every DLL and dylib matches its source."; fi
echo "The temporary folder is deleted. Nothing was installed."
if [ -n "$PAUSE_AT_END" ]; then read -r -p "Press Enter to close." _; fi
[ -z "$FAILED" ]
