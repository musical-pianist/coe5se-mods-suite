-- Note 1: Jazzy Hotkeys lets the player change every key of the game's Keyboard Shortcuts list in
-- Mod Settings. Each key is a key setting in JazzyHotkeys.c5se, next to this file, named after its
-- key action (map_end_turn for coe5.keyAction.mapEndTurn), on a page per screen. The script hands
-- each pick to CoE5SE with coe5.rebindKey (Note 24), and CoE5SE puts the key in the game's
-- shortcuts, in every game text that names it, and in the Keyboard Shortcuts list. No two lines of
-- one screen hold the same key (the key settings' group): the key list greys out a key another line
-- of the screen has, and a click on it asks whether to move the key here and leave that line with
-- no key (a fixed line's key never moves). Different screens may share a key.
--
-- Note 2: No player can pick Esc. Every line whose game key is Esc is a fixed key line that always
-- holds Esc, and the script leaves those actions to the game (Note 23). The map's Game Menu line is
-- the one that adds a key instead: the game gives the map no Esc, and the script adds Esc beside
-- F10 (coe5.addActionKey, Note 22). F10 and Esc are both fixed lines on the Map page, so no other
-- map key can have them.
--
-- Note 3: This file is a Lua script. CoE5SE runs it; the mod has no DLL. CoE5SE reads the file once
-- per game launch, on the first main menu frame, and runs the lines that sit outside any function
-- from top to bottom. After you edit this file, restart the game.
--
-- Note 4: A line or line end that starts with two dashes (--) is a comment. Lua skips it, so you can
-- change or delete any comment without changing what the mod does.
--
-- Note 5: What happens, in order, and which part of this file runs at each step:
--   1. CoE5SE runs the script on the first main menu frame. It makes one row per key action (Notes
--      6 to 11), hands CoE5SE every key the settings hold (Note 27), and asks CoE5SE to run a
--      function on each frame of the main menu (Note 28).
--   2. The player opens Mod Settings from the main menu and changes keys.
--   3. Back on the main menu, that function runs sendPicks (Note 16), which hands CoE5SE only the
--      keys that changed, so a change counts from the next game screen.
-- In network games no script runs, so the game's own keys hold there.

-- Note 6: `local` makes a name that only this file can see. Each mod's script runs on its own, so
-- another mod's script can't read or change these. `local function name(...)` makes a function: a
-- named group of steps that runs only when something calls it. Everything up to the matching `end`
-- belongs to it.
--
-- Note 7: settingName turns a key action's Lua name into its setting's name: mapEndTurn becomes
-- map_end_turn, and a digit after a letter gets its own _ (mapEditorPencil1 becomes
-- map_editor_pencil_1). s:gsub(pattern, replacement) replaces every match of the pattern in s. In a
-- pattern, %u matches one capital letter, %a any letter, and %d any digit; %0 in the replacement is
-- the whole match, and %1 and %2 are the parts in the first and second ( ). So the first gsub puts
-- _ before each capital, the second puts _ between a letter and a digit, and :lower() makes the
-- result lowercase. gsub also returns how many it replaced; the outer ( ) keep only the text.
local function settingName(luaName)
  return (luaName:gsub("%u", "_%0"):gsub("(%a)(%d)", "%1_%2"):lower())
end

-- Note 8: Curly braces {} make a table, Lua's one way to group values. Here `hotkeys` starts as an
-- empty list, and the loop below adds one row (itself a table) per key action.
local hotkeys = {}
-- Note 9: coe5.keyAction is a table of every key action, stored under its Lua name: 196 in all.
-- `for name, action in pairs(t)` runs the loop once for each name stored in t, with `action` set to
-- the value under that name, its number. #hotkeys is the list's length, so hotkeys[#hotkeys + 1]
-- adds a row at the end. Each row holds:
--   setting  the key setting's name (Note 7)
--   action   the key action's number, for coe5.rebindKey
--   sent     the key last handed to CoE5SE; -1 is a number no key has, so the first sendPicks
--            (Note 16) hands over every key
for name, action in pairs(coe5.keyAction) do
  hotkeys[#hotkeys + 1] = {setting = settingName(name), action = action, sent = -1}
end
-- Note 10: pairs visits the names in no fixed order. table.sort puts the rows in the order of the
-- function's answer: it returns true when row a goes before row b, here by action number, the
-- Keyboard Shortcuts list's own order.
table.sort(hotkeys, function(a, b) return a.action < b.action end)

-- Note 11: coe5.setting gives a key setting's key value, and nil (Lua's "nothing") when the mod
-- declares no setting by that name, which it logs as lua_refused. JazzyHotkeys.c5se declares one
-- setting for each name in coe5.keyAction, so the shipped file declares every name.
-- A setting removed from the .c5se leaves its action with no key (coe5.key.none): the row stores
-- `missing`, so the script asks for it only this once and logs it once. `== nil` asks "is it
-- nothing?"; `if ... then ... end` runs the part between then and end only when the test passes.
for _, h in ipairs(hotkeys) do
  if coe5.setting(h.setting) == nil then h.missing = true end
end

-- Note 12: The key a row's setting holds now. `h.missing and coe5.key.none or coe5.setting(...)` is
-- a common Lua way to write "if missing then none, else the setting": `and` gives its right side
-- only when its left side is true, and `or` gives its right side only when its left side is false
-- or nil.
local function pickedKey(h)
  return h.missing and coe5.key.none or coe5.setting(h.setting)
end

-- Note 13: A short local name for the Game Menu's action and for Esc keeps the lines below short.
-- coe5.key holds the game's key values; coe5.lua lists them.
local GAME_MENU = coe5.keyAction.mapGameMenu
local ESC = coe5.key.esc

-- Note 14: Every pick goes out once at the start (Note 27), then only a pick that changed. Reading
-- 196 settings on each main menu frame is quick; handing them over again would rewrite every key
-- each frame for nothing.
--
-- Note 15: string.format fills each %d in its first text with the next whole number given, in
-- order, so the log line reads "hotkeys_picks_sent changed=1 of 196".
--
-- Note 16: sendPicks hands every setting that changed since the last call to CoE5SE.
local function sendPicks()
  local changed = 0   -- Note 17: counts the keys handed over this time, for the log line (Note 26)
  -- Note 18: `for _, h in ipairs(list)` runs the loop once for each row of the list, in order, with
  -- h set to the row. The `_` would hold the row's position; `_` is the usual name for a value the
  -- loop doesn't use.
  for _, h in ipairs(hotkeys) do
    local key = pickedKey(h)
    -- Note 19: `~=` asks "are these different?". When the key is the one last sent, nothing changed,
    -- and the row is skipped. Lua has no `continue`, so the rest of the pass sits inside this if.
    if key ~= h.sent then
      h.sent = key   -- Note 20: a single = stores a value, so the row remembers what it sends
      -- Note 21: if, elseif runs at most one of the two parts: the first whose test passes, or
      -- neither. The order matters: the Game Menu's setting holds Esc too (Note 2), so it is tested
      -- first, and only the other fixed Esc lines reach the Esc test.
      --
      -- Note 22: addActionKey gives the Game Menu a second key beside F10, its game key, and the
      -- game's texts name both, "Esc/F10".
      if h.action == GAME_MENU then
        coe5.addActionKey(h.action, key)
        changed = changed + 1
      -- Note 23: A fixed Esc line keeps the game's own Esc: neither part runs for it, so nothing is
      -- sent, and the line doesn't count as sent.
      elseif key ~= ESC then
        -- Note 24: rebindKey sets the action's key. CoE5SE writes it into the game's shortcuts on
        -- every frame of the action's screen, so one call lasts the whole launch. Nothing is saved
        -- here: Mod Settings saves the player's picks, and the next launch sends them again.
        coe5.rebindKey(h.action, key)
        changed = changed + 1   -- Note 25: Lua has no ++; this adds one to changed
      end
    end
  end
  -- Note 26: coe5.log adds a line to coe5mod.log, which shows what the script did, as
  -- "event=lua_log mod=JazzyHotkeys text=hotkeys_picks_sent changed=1 of 196" (Note 15), while
  -- CoE5SE's "Enable copious logs" setting is on (off by default); otherwise it writes nothing. In
  -- Lua only false and nil count as false, and 0 counts as true, so the test is `changed > 0`.
  if changed > 0 then coe5.log(string.format("hotkeys_picks_sent changed=%d of %d", changed, #hotkeys)) end
end

-- Note 27: Every row's sent is -1 (Note 9), so this first call hands over every key the settings
-- hold, before any game screen opens.
sendPicks()

-- Note 28: coe5.wrap asks CoE5SE to run the function given on each frame of the main menu
-- (MAIN_MENU_WAIT), around the game's own call there, which draws one frame and returns what was
-- clicked. The function gets `next` first, then the call's two numbers. It hands over any changed
-- picks, then calls next with the same numbers so the game's frame goes on, and returns the game's
-- result unchanged. `function(...) ... end` with no name makes a function right inside the call, a
-- common Lua way to hand over code to run later.
--
-- Note 29: coe5.wrap returns true when the wrap is in place and false when it isn't; `wrapped`
-- keeps that for the log line (Note 30).
local wrapped = coe5.wrap("MAIN_MENU_WAIT", function(next, block, mode)
  sendPicks()
  return next(block, mode)
end)
-- Note 30: logs "hotkeys_installed menu_wait=1 settings=196". `wrapped and 1 or 0` (Note 12) gives
-- 1 for true and 0 for false.
coe5.log(string.format("hotkeys_installed menu_wait=%d settings=%d", wrapped and 1 or 0, #hotkeys))
