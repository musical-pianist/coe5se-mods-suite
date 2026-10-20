-- Note 1: Jazzy Restart adds "Load Game" and "Restart" lines to the in-game Game Menu (F10) in
-- single player games. Load Game picks a save from the game's own list, then ends the game in
-- progress and loads it. Restart ends the game in progress and starts a new world with the same
-- settings: the same map size, society, options, players, classes, teams, colors, and names. Only
-- what the player left on Random (a Random class, a Random society) is rolled again.
--
-- Note 2: This file is a Lua script. CoE5SE runs it; the mod has no DLL. CoE5SE reads the file
-- once per game launch, on the first main menu frame, and runs the lines that sit outside any
-- function from top to bottom. Most of those lines don't act yet: they hand CoE5SE a function to
-- run later, when the game reaches a certain step (coe5.wrap, Note 28) or when the player opens
-- the Game Menu (coe5.addGameMenuLine, Note 43). After you edit this file, restart the game.
--
-- Note 3: A line or line end that starts with two dashes (--) is a comment. Lua skips it, so you
-- can change or delete any comment without changing what the mod does. Three dashes (---) mark a
-- note for your code editor (Note 10).
--
-- Note 4: What happens, in order, and which part of this file runs at each step:
--   1. The player sets up a new game from the main menu. When Ok is clicked on Random Map
--      Settings, the script notes the society picked (Note 33). At "Creating World" it copies
--      every setting into `captured` (Notes 15 and 40).
--   2. During play the player opens the Game Menu and clicks Restart (Note 46). CoE5SE ends the
--      game the way Resign does, the Game Over screen shows on plain black (Note 41), and CoE5SE
--      then starts a new game's setup instead of going back to the main menu.
--   3. In that new setup the script skips both setup screens and puts the captured settings back
--      (Notes 31, 35 and 40). The game then rolls Random classes as it always does.
--   Load Game (Note 44) needs none of this: CoE5SE shows the list, ends the game, and loads the
--   save. A loaded game has no captured settings (Note 29), so it shows no Restart.

-- Note 5: `local` makes a name that only this file can see. Each mod's script runs on its own, so
-- another mod's script can't read or change these. Writing a name in CAPITALS is a common habit
-- for a value that never changes after this line; to Lua it is a name like any other.
local RANDOM_SOCIETY = 0                -- Note 6: society 0 is Random, the value Random Map Settings' Random button sets
-- Note 7: Curly braces {} make a table, Lua's one way to group values. A table of plain values
-- separated by commas, like the two below, is a list. These lists hold the names of the game
-- values (read with coe5.get, written with coe5.set) that the script copies and puts back.
-- coe5.lua describes every name. A list may run over several lines until its closing brace.
local MAP_VARS = {"society", "commonCause", "scoreGraphs", "battleReports", "startFlags", "wilder",
                  "mapW", "mapH", "mapDepth", "northPart", "southPart"}   -- Note 8: the values Random Map Settings sets
-- Note 9: the values Setup Participants sets for each player slot, human or AI. A player's color
-- (the square at the start of each row) is three numbers, its red, green, and blue, so it takes
-- three names. A CoE5SE older than those names stops any script that asks for one, so they join
-- the list only when CoE5SE knows them. pcall(f, ...) runs f and returns false, instead of
-- stopping the script, when f fails. table.insert(list, x) adds x at the end of the list. Under an
-- older CoE5SE, a Restart brings back everything but the colors.
local SLOT_VARS = {"slotController", "slotClass", "slotAi", "slotTeam"}
if pcall(coe5.get, "slotColorRed", 0) then
  table.insert(SLOT_VARS, "slotColorRed")
  table.insert(SLOT_VARS, "slotColorGreen")
  table.insert(SLOT_VARS, "slotColorBlue")
end

-- Note 10: The four names below are the script's memory. A `local` at the top of the file keeps
-- its value between calls until the game closes, so what one step stores, a later step reads.
-- nil means "nothing stored"; true and false work as on/off switches. The `---@type table?` line
-- tells a code editor with the Lua extension (LuaLS) that `captured` holds a table or nil (the ?
-- means "or nil"), so it can warn about mistakes. Lua skips that line like any comment.
---@type table?
local captured = nil           -- Note 11: the settings of the game in progress, or nil when there is nothing to restart
local restarting = false       -- Note 12: true only while the new game that Restart started is being set up
local societyChoice = nil      -- Note 13: the society picked on Random Map Settings (0 for Random), read before Ok rolls Random
local autoOk = false           -- Note 14: true while the script clicks Random Map Settings' Ok by itself (a restart with a Random society)

-- Note 15: `local function name()` makes a function: a named group of steps that runs only when
-- something calls it, written as `captureSetup()`. Everything up to the matching `end` belongs to
-- it. This one runs at "Creating World" of a game the player set up by hand (Note 40). Both setup
-- screens are done by then, and Random classes are not rolled yet, so a Random class is still
-- stored as Random.
local function captureSetup()
  -- Note 16: `c` starts as a table holding three empty tables, stored under the names map, slots,
  -- and names. `c.map` means "the value stored in c under the name map".
  local c = {map = {}, slots = {}, names = {}}
  -- Note 17: A `for ... in ipairs(list) do ... end` loop runs the part between do and end once for
  -- each item of the list, in order, with `name` set to that item. The `_` would hold the item's
  -- position; `_` is the usual name for a value the loop doesn't use. `c.map[name] = ...` stores a
  -- value under the name held in `name`, so afterwards c.map.society holds the society, c.map.mapW
  -- the map width, and so on.
  for _, name in ipairs(MAP_VARS) do c.map[name] = coe5.get(name) end
  c.uniqueRandom = coe5.get("newWorldUniqueRandom")   -- Note 18: Setup Participants' "unique random players" box
  c.societyChoice = societyChoice
  -- Note 19: `for i = 0, 31 do` counts i from 0 to 31, the game's 32 player slots. Lua lists
  -- usually start at 1, but these tables are numbered from 0 to match the game's slot numbers.
  -- Each slot gets its own small table of the SLOT_VARS values. In coe5.get(name, i), the second
  -- input picks the slot.
  for i = 0, 31 do
    c.slots[i] = {}
    for _, name in ipairs(SLOT_VARS) do c.slots[i][name] = coe5.get(name, i) end
  end
  captured = c   -- Note 20: the copy is stored only now, once every value is read
  -- Note 21: coe5.log writes a line to coe5mod.log, which shows what the script did, while CoE5SE's
  -- "Enable copious logs" setting is on (off by default); otherwise it writes nothing.
  -- string.format fills each %d with a whole number and each %s with text, in order. tostring turns
  -- any value, nil included, into text, so values that may be nil go through it. A nil given to %d
  -- would be an error, and an error stops the whole script until the game restarts.
  coe5.log(string.format("setup captured: society=%d chosen=%s map=%dx%d unique_random=%s",
    c.map.society, tostring(c.societyChoice), c.map.mapW, c.map.mapH, tostring(c.uniqueRandom)))
end

-- Note 22: Copies the human players' names. The game fills the names in during the new game's
-- setup, and they are only in place once that setup has finished, so this runs after it (Note 32)
-- rather than inside captureSetup.
local function captureNames()
  for i = 0, 31 do
    -- Note 23: `if test then ... end` runs the middle part only when the test is true. `==` asks
    -- "are these equal?"; a single = stores a value instead. coe5.controller.human is the game's
    -- number for a human player, written by name so the line says what it means.
    if captured.slots[i].slotController == coe5.controller.human then captured.names[i] = coe5.get("slotName", i) end
  end
end

-- Note 24: Puts back every value Random Map Settings set in the captured game, except the one
-- named in `skip`. A function's inputs are named between its brackets. A call may leave an input
-- out, as `applyMapSettings()` does in Note 37, and the input is then nil, which matches no name.
local function applyMapSettings(skip)
  -- Note 25: `pairs` visits every name and value stored in a table, here captured.map. Unlike
  -- ipairs it follows no fixed order, which is fine here because each coe5.set writes one value
  -- on its own. `~=` means "is not equal", so every name but the skipped one is written.
  for name, value in pairs(captured.map) do
    if name ~= skip then coe5.set(name, value) end
  end
end

-- Note 26: At "Creating World" of Restart's new game, puts every player slot back as the captured
-- game had it, with the human players' names, so the game doesn't ask for the names again. A slot
-- whose class was Random still holds Random (a class below 1), and the game rolls it right after
-- this step, so that player gets a new random class, as in a fresh game. Colors need this step
-- too: each new game's setup starts every slot on the game's default color for it, and nothing
-- from "Creating World" on changes a color, so the one written here is the one the game keeps.
local function applySlots()
  for i = 0, 31 do
    for name, value in pairs(captured.slots[i]) do coe5.set(name, i, value) end
    -- Note 27: `if captured.names[i] then` passes when a name was stored for slot i. In Lua only
    -- nil and false count as "no"; every other value counts as "yes", even 0 and empty text.
    if captured.names[i] then coe5.set("slotName", i, captured.names[i]) end
  end
  if captured.uniqueRandom then coe5.set("newWorldUniqueRandom", captured.uniqueRandom) end
end

-- Note 28: coe5.wrap(<site name>, function ... end) runs code at a step of the game. A site is a
-- call in the game's own code; coe5.lua lists every site and what it is for. The function you pass
-- runs each time the game makes that call, and it gets `next` first, then the call's inputs.
-- Calling next(...) lets the game's own step go ahead: code before it runs before the step, code
-- after it runs once the step is done, and `return` hands the game the result. The function has no
-- name because it is written right inside the call, a common Lua way to pass code to run later.
-- (Lua also has a built-in function called next; inside these functions the name means the wrap's
-- next instead, which is fine because this script never uses the built-in one.)
--
-- Note 29: MAIN_MENU_PLAY_GAME is Start New Game or Load Game (`arg` is 0 in single player), and
-- also the new game CoE5SE starts after a restart. Its next returns only when that game is over.
-- A game started from the menu drops the last game's settings, since coe5.restartPhase() is
-- "starting" only for the game a restart began; that game keeps them. A loaded game never
-- captures settings, because it skips the setup, so Restart stays hidden after Load Game.
coe5.wrap("MAIN_MENU_PLAY_GAME", function(next, arg)
  if coe5.restartPhase() ~= "starting" then captured = nil end
  return next(arg)
end)

-- Note 30: PLAY_GAME_NEW_WORLD is a new game's whole setup: both setup screens, "Creating World",
-- and building the world. Its next returns 0 when the player clicks Cancel. The function takes one
-- of two paths.
coe5.wrap("PLAY_GAME_NEW_WORLD", function(next, mode)
  -- Note 31: Path one, the new game Restart started. `and` needs both tests to pass; `captured` on
  -- its own passes when a copy is stored (Note 27). The script turns on `restarting`, which its
  -- other wraps check (Notes 35 and 40), and runs the setup in skipParticipants mode, which leaves
  -- out Setup Participants. setSetupAutomated(true) tells mods such as Jazzy Game Presets that
  -- this script fills the setup in, so they leave the screens alone. Both switches go off again
  -- once next returns; every mod shares the setSetupAutomated switch, so it must always go off.
  if coe5.restartPhase() == "starting" and captured then
    coe5.log(string.format("new game: map=%dx%d", captured.map.mapW, captured.map.mapH))
    restarting = true
    coe5.setSetupAutomated(true)
    local result = next(coe5.newGame.skipParticipants)
    coe5.setSetupAutomated(false)
    restarting = false
    return result
  end
  -- Note 32: Path two, a game the player sets up by hand. The old settings are dropped first, and
  -- the NEW_WORLD_CREATING_WORLD wrap (Note 40) stores new ones if the player gets that far. A
  -- result other than 0 means the player didn't cancel; only then are the names copied.
  captured = nil
  societyChoice = nil
  local result = next(mode)
  if result ~= 0 and captured then captureNames() end
  return result
end)

-- Note 33: MAP_SETTINGS_WAIT runs once for each frame (each redraw) of Random Map Settings, and its
-- next returns the id of what was clicked in that frame. When Ok is clicked, the script reads the
-- society at once, because the game's Ok then turns Random into a rolled society, and this frame
-- is the last moment that shows whether the player picked Random. While autoOk is on (Note 36),
-- the script returns Ok without calling next, so Ok is clicked on the screen's first frame.
--
-- Note 34: `local canRollSociety = coe5.wrap(...)` keeps what coe5.wrap returns: true when the wrap
-- is in place, false when the CoE5SE in use doesn't have the site on this platform. An older
-- CoE5SE has it on Windows only; on Mac canRollSociety is then false, and a Restart there keeps
-- the society the first game rolled.
local canRollSociety = coe5.wrap("MAP_SETTINGS_WAIT", function(next, block, mode)
  if autoOk then return coe5.newGame.mapSettingsOk end
  local id = next(block, mode)
  if id == coe5.newGame.mapSettingsOk then societyChoice = coe5.get("society") end
  return id
end)

-- Note 35: NEW_WORLD_MAP_SCREEN is Random Map Settings as a whole. Outside a restart the script
-- lets the screen run as usual: `return next()` ends the function there and passes the screen's
-- result on. During a restart it puts the captured values back and tells the game Ok was pressed.
coe5.wrap("NEW_WORLD_MAP_SCREEN", function(next)
  if not restarting then return next() end
  if captured.societyChoice == RANDOM_SOCIETY and canRollSociety then
    -- Note 36: The player had picked Random. The script sets the society to Random, turns autoOk
    -- on, and runs the real screen; the MAP_SETTINGS_WAIT wrap (Note 33) clicks Ok on its first
    -- frame, and the game's own Ok rolls a new society. Ok also writes the map size again with a
    -- small random change, so applyMapSettings("society") then puts back every value but the new
    -- society.
    coe5.set("society", RANDOM_SOCIETY)
    autoOk = true
    next()
    autoOk = false
    applyMapSettings("society")
  else
    -- Note 37: Any other society, or a Restart without the site (Note 34): every value goes back,
    -- the society included, and the screen never shows.
    applyMapSettings()
  end
  -- Note 38: another log line (Note 21), with the values as they now stand.
  coe5.log(string.format("restart settings: society=%d chosen=%s map=%dx%d",
    coe5.get("society"), tostring(captured.societyChoice), coe5.get("mapW"), coe5.get("mapH")))
  return coe5.newGame.mapScreenOk   -- Note 39: as if Ok was pressed
end)

-- Note 40: NEW_WORLD_CREATING_WORLD is the "Creating World" step: both setup screens are done, and
-- Random classes are about to be rolled. During a restart the script puts the slots back. In any
-- other new game it captures the settings for a later Restart, but only in a game that isn't a
-- network game (networkGame is false) and runs on a random map (loadedMapName is "", empty text;
-- it holds the file name when the game started on a map file). `elseif` is a second test, tried
-- only when the first one fails. next runs last, so the game creates the world after the script's
-- work. This site gives no result, so the function returns none.
coe5.wrap("NEW_WORLD_CREATING_WORLD", function(next, kind, text)
  if restarting then applySlots()
  elseif not coe5.get("networkGame") and coe5.get("loadedMapName") == "" then captureSetup() end
  next(kind, text)
end)

-- Note 41: GAME_OVER_WAIT runs once for each frame of the Game Over screen. While a restart is
-- ending, the script changes the screen's `mode` input on its way to next: coe5.gameOverMode.plain
-- draws nothing behind the screen, a plain black background, in place of the game's own fog and
-- falling particles (coe5.gameOverMode.fog), which are slow to draw. This Game Over screen is only
-- a step on the way to the new world; every other Game Over keeps the game's background. Changing
-- an input before passing it to next is how a wrap changes what the game does. Under a CoE5SE
-- without this site on the platform in use, coe5.wrap returns false (Note 34) and the fog stays.
coe5.wrap("GAME_OVER_WAIT", function(next, block, mode)
  if coe5.restartPhase() == "ending" then return next(block, coe5.gameOverMode.plain) end
  return next(block, mode)
end)

-- Note 42: Counts the human players. Load Game and Restart are for single player: with another
-- human left, a resign doesn't end the game, so coe5.loadGame and coe5.restartGame refuse.
-- `humans = humans + 1` adds one (Lua has no shorter += form), and `return humans` hands the count
-- back to the line that called this.
local function humanPlayers()
  local humans = 0
  for i = 0, 31 do
    if coe5.get("slotController", i) == coe5.controller.human then humans = humans + 1 end
  end
  return humans
end

-- Note 43: coe5.addGameMenuLine puts a line at the end of the Game Menu's Saving & Exiting section,
-- in the order the calls run, so Load Game comes first and Restart under it. Its one input is a
-- table in curly braces (Note 7), and when a call's only input is a table, Lua lets you leave out
-- the round brackets. Inside, each `name = value` sets one part of the line, with a comma after
-- each. label and tooltip are plain text: edit them to rename the line or change its hover text.
-- visible and onClick are functions CoE5SE calls later.
coe5.addGameMenuLine{
  label = "Load Game",
  tooltip = "Ends this game and loads a save from the list.",
  -- Note 44: `coe5.loadGame ~= nil` hides the line under a CoE5SE older than coe5.loadGame, where
  -- calling it would stop the script with an error.
  visible = function() return coe5.loadGame ~= nil and not coe5.get("networkGame") and humanPlayers() == 1 end,
  onClick = function()
    -- coe5.loadGame() shows the game's own Load Game list, with every mod's changes to it
    -- (Jazzy UI Tweaks' <del>). Cancel returns false and the game goes on. A picked save returns
    -- true; CoE5SE then ends this game once this function returns and loads the save, as the main
    -- menu's Load Game does.
    if coe5.loadGame() then coe5.log("load game clicked on turn " .. coe5.turn()) end
    return coe5.gameMenu.stay
  end,
}

coe5.addGameMenuLine{
  label = "Restart",
  tooltip = "Ends this game and starts a new world with the same settings.",
  -- Note 45: visible runs on every frame the menu is drawn, and shows the line while it returns
  -- true: settings are captured (`captured ~= nil`), the game isn't a network game, and exactly one
  -- human plays. Each test gives true or false. Return true or false here, never a number, since in
  -- Lua 0 counts as true (Note 27).
  visible = function() return captured ~= nil and not coe5.get("networkGame") and humanPlayers() == 1 end,
  onClick = function()
    -- Note 46: Runs when the player clicks Restart. coe5.restartGame() returns true when CoE5SE
    -- agrees; CoE5SE then ends the game once this function returns, and the menu stays open if it
    -- refused. `..` joins two pieces of text, here the message and the turn number. The line
    -- returns coe5.gameMenu.stay rather than resigning, because CoE5SE ends the game itself.
    if coe5.restartGame() then coe5.log("restart clicked on turn " .. coe5.turn()) end
    return coe5.gameMenu.stay
  end,
}
