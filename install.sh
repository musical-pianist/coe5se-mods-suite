#!/bin/bash
# Installs CoE5SE and the Jazzy mods on a Mac, from this repo's main branch.
# Run it with:
#   curl -fsSL https://gitlab.com/musical_pianist/coe5se_mods_suite/-/raw/main/install.sh | bash
# It puts each mod folder in ~/.coe5/mods (replacing an older copy) and
# coe5se.dylib in the game folder, next to coe5_mac. It changes nothing else.
set -euo pipefail

ARCHIVE_URL="${COE5_SUITE_ZIP_URL:-https://gitlab.com/musical_pianist/coe5se_mods_suite/-/archive/main/coe5se_mods_suite-main.zip}"
MODS_DIR="${COE5_MODS_DIR:-$HOME/.coe5/mods}"
STEAM_DIR="$HOME/Library/Application Support/Steam"
APP_ID=1606340
MODS=(CoE5SE JazzyGamePresets JazzyHotkeys JazzyRestart JazzyUITweaks)
LAUNCH_OPTIONS='/usr/bin/env DYLD_INSERT_LIBRARIES=coe5se.dylib %command%'

fail() {
  echo "Install stopped: $1" >&2
  exit 1
}

# Each Steam library holds the game if it has the game's app manifest.
findGameDir() {
  local libraries=("$STEAM_DIR")
  local line library manifest installDir
  if [[ -f "$STEAM_DIR/steamapps/libraryfolders.vdf" ]]; then
    while IFS= read -r line; do
      libraries+=("$line")
    done < <(sed -n 's/^[[:space:]]*"path"[[:space:]]*"\(.*\)"[[:space:]]*$/\1/p' "$STEAM_DIR/steamapps/libraryfolders.vdf")
  fi
  for library in "${libraries[@]}"; do
    manifest="$library/steamapps/appmanifest_$APP_ID.acf"
    [[ -f "$manifest" ]] || continue
    installDir=$(sed -n '/"installdir"/{s/^[[:space:]]*"installdir"[[:space:]]*"\(.*\)"[[:space:]]*$/\1/p;q;}' "$manifest")
    if [[ -n "$installDir" && -f "$library/steamapps/common/$installDir/coe5_mac" ]]; then
      echo "$library/steamapps/common/$installDir"
      return 0
    fi
  done
  return 1
}

GAME_DIR="${COE5_GAME_DIR:-}"
if [[ -z "$GAME_DIR" ]]; then
  GAME_DIR=$(findGameDir) || fail "couldn't find Conquest of Elysium 5 in Steam. Is it installed?"
fi
[[ -f "$GAME_DIR/coe5_mac" ]] || fail "no coe5_mac in $GAME_DIR"

# ps's comm is the executable's path, so only this game folder's copy matches; pgrep -x would match any copy.
# shellcheck disable=SC2009
if ps -axo comm= | grep -Fx "$GAME_DIR/coe5_mac" >/dev/null; then
  fail "Conquest of Elysium 5 is running. Quit the game, then paste the command again."
fi

WORK_DIR=$(mktemp -d)
trap 'rm -rf -- "$WORK_DIR"' EXIT

echo "Downloading the mods..."
curl -fsSL -o "$WORK_DIR/suite.zip" "$ARCHIVE_URL" || fail "the download failed. Check your internet connection and try again."
unzip -q "$WORK_DIR/suite.zip" -d "$WORK_DIR/unzipped" || fail "the download was damaged. Try again."
SOURCE_DIR=$(find "$WORK_DIR/unzipped" -mindepth 1 -maxdepth 1 -type d -print -quit)
for mod in "${MODS[@]}"; do
  [[ -f "$SOURCE_DIR/$mod/$mod.c5m" ]] || fail "the download has no $mod folder. Try again later."
done
[[ -f "$SOURCE_DIR/CoE5SE/coe5se.dylib" ]] || fail "the download has no coe5se.dylib. Try again later."

mkdir -p "$MODS_DIR"
for mod in "${MODS[@]}"; do
  # Copy next to the old folder first, so a failed copy leaves the old one in place.
  rm -rf -- "$MODS_DIR/.$mod.installing"
  cp -R "$SOURCE_DIR/$mod" "$MODS_DIR/.$mod.installing"
  rm -rf -- "${MODS_DIR:?}/$mod"
  mv "$MODS_DIR/.$mod.installing" "$MODS_DIR/$mod"
  echo "Installed $mod in $MODS_DIR/$mod"
done

cp "$SOURCE_DIR/CoE5SE/coe5se.dylib" "$GAME_DIR/.coe5se.dylib.installing"
mv -f "$GAME_DIR/.coe5se.dylib.installing" "$GAME_DIR/coe5se.dylib"
echo "Installed coe5se.dylib in $GAME_DIR"

echo
if grep -Fqs 'DYLD_INSERT_LIBRARIES=coe5se.dylib' "$STEAM_DIR"/userdata/*/config/localconfig.vdf; then
  echo "Steam's Launch Options for the game are already set."
else
  echo "One step left. In Steam, click the gear on Conquest of Elysium 5's page,"
  echo "then Properties, General, and set Launch Options to:"
  echo
  echo "  $LAUNCH_OPTIONS"
fi
echo
echo "Then start the game. The first time, CoE5SE turns itself on and tells you so."
echo "Turn on the Jazzy mods you want on the Mods screen."
