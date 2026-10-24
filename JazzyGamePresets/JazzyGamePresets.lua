-- Note 1: Jazzy Game Presets saves new game setups and fills them back in. Before Random Map Settings
-- it asks "Load a preset?", and a chosen preset fills in both setup screens. Setup Participants gets
-- a "Save as preset" button above Ok (Note 93), and every Ok there saves the setup as "Previous Game Settings".
-- Under "unique random players" it adds a "random classes" line: a checklist of the classes an AI
-- player's Random can roll, kept with the preset (Note 139). The script also logs setup clicks and
-- setting changes while CoE5SE's "Enable copious logs" setting is on (off by default).
-- Presets are data files in the game's user folder,
-- coe5se_data\JazzyGamePresets\jazzy_campaign_presets\<name>.ini (%APPDATA%\coe5 on Windows,
-- ~/.coe5 on Mac). Each file has a [RandomMapSettings] section, a [SetupParticipants] section, and
-- one [PlayerN] section per player.
--
-- In a game, "Extended Game Settings" in the Game Menu changes the setup options the game reads
-- during play: battle reports, score graphs, common cause, and each AI player's difficulty. The
-- game's own save file keeps all four, so a change stays after saving and loading.
--
-- Note 2: This file is a Lua script. CoE5SE runs it; the mod has no DLL. CoE5SE reads the file once
-- per game launch, on the first main menu frame, and runs the lines that sit outside any function
-- from top to bottom. Most of those lines don't act yet: they hand CoE5SE a function to run later,
-- when the game reaches a certain step (coe5.wrap, Note 77) or when the player opens the Game Menu
-- (coe5.addGameMenuLine, Note 136). After you edit this file, restart the game.
--
-- Note 3: A line or line end that starts with two dashes (--) is a comment. Lua skips it, so you can
-- change or delete any comment without changing what the mod does. Three dashes (---) mark a note
-- for your code editor (Note 17).
--
-- Note 4: What happens, in order, and which part of this file runs at each step:
--   1. The player clicks Start New Game. Before Random Map Settings the script shows its own "Load a
--      preset?" screen (Notes 61 to 75) and reads the chosen preset's file (Note 41).
--   2. Random Map Settings opens with the preset's values already in place (Notes 76 to 91).
--   3. Setup Participants opens with the preset's players (Notes 52 and 92). Its "Save as preset"
--      button saves a named preset (Note 96), and its Ok saves "Previous Game Settings" (Note 119). Its "random
--      classes" line opens the checklist (Notes 139 to 152).
--   4. At "Creating World", each AI player left on Random gets a class from the checked ones, by
--      the game's own rules where one fits, never an unchecked one (Notes 147, 148, and 152).
--   5. During play, "Extended Game Settings" in the Game Menu opens a screen of the four options
--      above (Notes 121 to 136).
--   On every frame of the setup screens, the script also writes what changed to coe5mod.log while
--   CoE5SE's "Enable copious logs" setting is on (Notes 102 to 110).

-- Note 5: `local` makes a name that only this file can see. Each mod's script runs on its own, so
-- another mod's script can't read or change these. Writing a name in CAPITALS is a common habit for
-- a value that never changes after this line; to Lua it is a name like any other.
local LAST_GAME = "last_game"      -- Note 6: the preset every Ok on Setup Participants rewrites
local MAX_SLOTS = 24               -- Note 7: the participant screen's add and remove code works on 24 slots
local MAX_LISTED = 20              -- Note 8: the most presets the "Load a preset?" screen lists
local AI_LEVEL_FIRST = 1           -- Note 9: the game's Difficulty Level popup offers levels 1-10 (0 is no level)
local AI_LEVEL_LAST = 10           -- Note 10: the AI level names are 0-10
local AI_LEVEL_DEFAULT = 2         -- Note 11: Jester, the game's default
local CLASS_NAME_MAX = 63          -- Note 12: a class name is stored up to 63 characters

-- Note 13: The "Load a preset?" screen's ids: the "No preset" line is
-- PRESET_CHOICE_BASE, preset i's line PRESET_CHOICE_BASE + i. Every clickable line on a screen has
-- an id, a number the script picks when it draws the line. coe5.ui.wait hands back the id of the line clicked, which is how the
-- script knows what the player did.
local PRESET_CHOICE_BASE = 100
-- Note 14: One `local` line can make several names at once: the values after = go to the names
-- before it, in order, so PROMPT_OK is 9, PROMPT_CANCEL 8, and PROMPT_DELETE 7.
local PROMPT_OK, PROMPT_CANCEL, PROMPT_DELETE = 9, 8, 7
local SAVE_PRESET_ID = 900         -- Note 15: above every id the participants screen uses (the highest is 600 + 31)

-- Note 16: coe5.dataFolder names the folder the script's data files live in (Note 1). Without this
-- line the folder would be named after the mod. It must come before the first data file call
-- (coe5.readIni, coe5.writeIni, coe5.listIni, coe5.deleteIni).
coe5.dataFolder("jazzy_campaign_presets")

-- Note 17: The ---@ lines below are for a code editor with the Lua extension (LuaLS); Lua skips them
-- like any comment. They describe the shape of a preset, so the editor can complete field names and
-- warn about typos. ---@class names a kind of table, and each ---@field one value it holds and that
-- value's type: integer is a whole number, PresetSlot[] a list of PresetSlot tables. A PresetSlot is
-- one player; a Preset is one whole setup.
---@class PresetSlot
---@field controller integer
---@field cls integer
---@field ai integer
---@field team integer
---@field red number? the player's color, 0 to 1 each; nil when the preset has no color for the slot
---@field green number?
---@field blue number?

---@class Preset
---@field society integer
---@field commonCause integer
---@field scoreGraphs integer
---@field battleReports integer
---@field startPositions integer 0 normal, 1 clustered, 2 west/east
---@field wilder integer 1 on, 0 off
---@field mapSize integer the size button 0..4 (Small..Enormous), or -1 for exactly mapWidth x mapHeight
---@field mapWidth integer
---@field mapHeight integer
---@field north integer
---@field south integer
---@field uniqueRandom integer the "unique random players" tick box on Setup Participants
---@field randomOff string[] the names of the classes an AI's Random doesn't roll (Note 139)
---@field slots PresetSlot[]

-- Note 18: The six names below are the script's memory. A `local` at the top of the file keeps its
-- value between calls until the game closes, so what one step stores, a later step reads. nil means
-- "nothing stored". `---@type Preset?` tells the editor the name holds a Preset or nil (the ? means
-- "or nil").
---@type Preset?
local armedPreset = nil     -- Note 19: the preset the next new game gets
---@type Preset?
local slotsPreset = nil     -- Note 20: set once the map screen got the preset, while the participants still need it
local pendingMapClick = nil -- Note 21: the button id to hand the map screen as its first click
local mapSizeChoice = 1     -- Note 22: the size button last clicked on the map screen in this setup (vanilla starts at Medium)
local societyChoice = nil   -- Note 23: the society picked when Random Map Settings' Ok was clicked; nil before that
local randomOff = {}        -- the classes an AI's Random doesn't roll in this setup, by name (Note 139)

-- Note 24: A short helper for log lines. `...` in a function's inputs means "any number of further
-- inputs", and `...` in the call passes them all on. So log("x=%d", 5) is the same as
-- coe5.log(string.format("x=%d", 5)). string.format fills each %d with a whole number and each %s
-- with text, in order. coe5.log writes the line to coe5mod.log, which shows what the script did,
-- while CoE5SE's "Enable copious logs" setting is on (off by default); otherwise it writes nothing.
local function log(fmt, ...) coe5.log(string.format(fmt, ...)) end

-- ---- Classes ------------------------------------------------------------------------------------
-- Note 25: A mod class's index in the class table changes with the enabled mods, so presets store
-- the class name and look the index up again when they load. (A dashed line like the one above is
-- only a comment that marks a part of the file.)

-- Note 26: Returns class c's name, or nil if c isn't a class in the table: c runs from 1 to the
-- last class in use, classLastIndex. `or` passes when either test passes.
local function classNameAt(c)
  if c < 1 or c > coe5.get("classLastIndex") then return nil end
  return coe5.get("className", c)
end

-- Note 27: Returns the index of the class with this name, or nil if no enabled mod has it. A colon
-- calls a text function on the text before it: name:sub(1, 63) is the same as string.sub(name, 1, 63),
-- the first 63 characters. Saved names were cut to 63 (Note 37), so each class name is cut the same
-- way before the two are compared. `return c` ends the loop and the function at once.
local function findClassByName(name)
  for c = 1, coe5.get("classLastIndex") do
    if coe5.get("className", c):sub(1, CLASS_NAME_MAX) == name then return c end
  end
  return nil
end

-- ---- Presets ------------------------------------------------------------------------------------

-- Note 28: Reads the live settings into a preset. It is called from the participants screen (its Ok
-- click or the save line), before the game replaces "Random" classes with rolled ones, so random
-- slots stay random. A player's color (the square at the start of each row) is three numbers, its
-- red, green, and blue, read with three names. A CoE5SE older than those names stops any script
-- that asks for one, so HAS_COLORS below is true only when CoE5SE knows them. pcall(f, ...) runs f
-- and returns false, instead of stopping the script, when f fails. Under an older CoE5SE, presets
-- keep everything but the colors.
local HAS_COLORS = pcall(coe5.get, "slotColorRed", 0)
---@return Preset
local function capturePreset()
  local flags = coe5.get("startFlags")
  -- Note 29: Curly braces {} make a table, Lua's one way to group values. Inside, each
  -- `name = value,` stores one value under that name, so p.society is the society and so on. The
  -- values are worked out right here, as the table is made.
  local p = {
    -- Note 30: Ok turns a Random society (0) into a rolled one, so the preset keeps what the player
    -- picked, and a preset with Random stays Random. If there was no click (the screen was skipped),
    -- it takes the live value: `or` gives its right side when the left side is nil.
    society = societyChoice or coe5.get("society"),
    commonCause = coe5.get("commonCause"),
    scoreGraphs = coe5.get("scoreGraphs"),
    battleReports = coe5.get("battleReports"),
    -- Note 31: startFlags keeps on/off switches in single bits of one number: 8 is clustered,
    -- 16 west/east (0x10 and 0x8 are the same numbers written in hexadecimal). `&` keeps only the bits
    -- two numbers share, so (flags & 0x10) ~= 0 asks "is the 16 switch on?". `~=` means "is not
    -- equal". `a and x or y` gives x when a passes and y when it doesn't, and two of them in a row
    -- read as "2 if west/east, else 1 if clustered, else 0", the numbering startDefault uses.
    startPositions = (flags & 0x10) ~= 0 and 2 or (flags & 0x8) ~= 0 and 1 or 0,
    wilder = coe5.get("wilder") == 2 and 1 or 0,   -- Note 32: the game's value 2 is stored as 1 (on), anything else as 0
    mapSize = mapSizeChoice,
    mapWidth = coe5.get("mapW"),
    mapHeight = coe5.get("mapH"),
    north = coe5.get("northPart"),
    south = coe5.get("southPart"),
    -- Note 33: This value is nil outside a wrap of a setup site, and `or 0` turns a nil into 0 before
    -- the test, so the line never compares nil.
    uniqueRandom = (coe5.get("newWorldUniqueRandom") or 0) ~= 0 and 1 or 0,
    randomOff = {},
    slots = {},
  }
  -- The classes Random doesn't roll go in by name, sorted so a preset's file keeps one order (Note 140).
  for name in pairs(randomOff) do p.randomOff[#p.randomOff + 1] = name end
  table.sort(p.randomOff)
  -- Note 34: Goes through player slots 0 to 23 and keeps only those in use, a human or an AI.
  -- `#p.slots` is how many items the list holds, so p.slots[#p.slots + 1] = ... adds one at the end.
  -- Lua lists start at 1, so the players in use become items 1, 2, 3, ... with no gaps, whatever slots
  -- they sat in. Each player's color goes with the player, so it moves to the slot the player
  -- lands in when the preset loads.
  for i = 0, MAX_SLOTS - 1 do
    local controller = coe5.get("slotController", i)
    if controller == coe5.controller.human or controller == coe5.controller.ai then
      local s = {controller = controller, cls = coe5.get("slotClass", i),
                 ai = coe5.get("slotAi", i), team = coe5.get("slotTeam", i)}
      if HAS_COLORS then
        s.red, s.green, s.blue = coe5.get("slotColorRed", i), coe5.get("slotColorGreen", i), coe5.get("slotColorBlue", i)
      end
      p.slots[#p.slots + 1] = s
    end
  end
  return p
end

-- Note 35: Saves a preset as <name>.ini. writeIni replaces the whole file, so a preset with fewer
-- players leaves no stale player sections behind.
---@param p Preset
---@param name string
---@return boolean
local function savePreset(p, name)
  -- Note 36: coe5.writeIni takes a table of tables: each name in t becomes a [section] of the file,
  -- and each name = value inside it one line of that section. A table may run over several lines,
  -- and one line may hold several values, separated by commas.
  local t = {
    RandomMapSettings = {
      Society = p.society, CommonCause = p.commonCause, ScoreGraphs = p.scoreGraphs,
      BattleReports = p.battleReports, StartPositions = p.startPositions, Wilder = p.wilder,
      MapSize = p.mapSize, MapWidth = p.mapWidth, MapHeight = p.mapHeight, North = p.north, South = p.south,
    },
    SetupParticipants = {UniqueRandom = p.uniqueRandom},
  }
  -- Note 37: ipairs goes through a list in order, giving each item's position (i) and the item (s).
  -- t["Player" .. i] stores under a name built while the script runs: `..` joins text, so the names
  -- are Player1, Player2, and so on. Square brackets are needed for a built name; the dot only works
  -- for a name written out in the code. A Random class has no name, so `or ""` writes empty text.
  -- The color's three numbers go in as ColorRed, ColorGreen, and ColorBlue (writeIni writes a number
  -- with enough digits to read back the same); a slot without them (s.red is nil) writes none.
  for i, s in ipairs(p.slots) do
    t["Player" .. i] = {Controller = s.controller, Class = s.cls,
                        ClassName = (classNameAt(s.cls) or ""):sub(1, CLASS_NAME_MAX), AI = s.ai, Team = s.team,
                        ColorRed = s.red, ColorGreen = s.green, ColorBlue = s.blue}
  end
  -- A [RandomClasses] section with Off1, Off2, ... holds the classes Random doesn't roll (Note 140).
  if #p.randomOff > 0 then
    t.RandomClasses = {}
    for i, className in ipairs(p.randomOff) do t.RandomClasses["Off" .. i] = className end
  end
  return coe5.writeIni(name, t)   -- Note 38: true when the file was written, false (and logged) when it wasn't
end

-- Note 39: Reads an integer, or returns `fallback` if the key is missing or doesn't start with a
-- number. Presets store -1 (no team, north and south unset).
--
-- Note 40: The file's values come back as text. `t[section] and t[section][key]` gives nil when the
-- section is missing, rather than an error from looking inside nil. text:match(...) uses a Lua
-- pattern: ^ is the start of the text, %s* any spaces, [+-]? an optional sign, %d+ one or more
-- digits, and the round brackets mark the part to hand back. tonumber turns that text into a number,
-- and math.tointeger makes sure it is a whole number.
local function readInt(t, section, key, fallback)
  local text = t[section] and t[section][key]
  local digits = text and text:match("^%s*([+-]?%d+)")
  return digits and math.tointeger(tonumber(digits)) or fallback
end

-- Note 41: Loads a preset from its file. Returns nil if the file is missing or has no players.
---@param name string
---@return Preset?
local function loadPreset(name)
  local t = coe5.readIni(name)
  if not t then return nil end   -- Note 42: readIni gives nil when there is no such file; `not nil` is true
  -- Note 43: A missing key gets the fallback given here. For the map size and north and south, -1
  -- means "leave as is".
  local map = "RandomMapSettings"   -- Note 44: the section's name, kept in a short name so each line below stays short
  local p = {
    society = readInt(t, map, "Society", 0),
    commonCause = readInt(t, map, "CommonCause", 0),
    scoreGraphs = readInt(t, map, "ScoreGraphs", 0),
    battleReports = readInt(t, map, "BattleReports", 0),
    startPositions = readInt(t, map, "StartPositions", 0),
    wilder = readInt(t, map, "Wilder", 0),
    mapSize = readInt(t, map, "MapSize", -1),
    mapWidth = readInt(t, map, "MapWidth", 0),
    mapHeight = readInt(t, map, "MapHeight", 0),
    north = readInt(t, map, "North", -1),
    south = readInt(t, map, "South", -1),
    uniqueRandom = readInt(t, "SetupParticipants", "UniqueRandom", 0),
    randomOff = {},
    slots = {},
  }
  -- Off1, Off2, ... up to the first one missing; a preset saved before random classes has none (Note 140).
  local off, n = t.RandomClasses or {}, 1
  while off["Off" .. n] do
    p.randomOff[n] = off["Off" .. n]
    n = n + 1
  end
  -- Note 45: Read players from [Player1] on, and stop at the first section without a Controller.
  -- `break` leaves the loop at once, and the lines after the loop's end run next.
  for i = 1, MAX_SLOTS do
    local section = "Player" .. i
    local controller = readInt(t, section, "Controller", -1)
    if controller < 0 then break end
    -- Note 46: The controller is a human (0) or an AI (1): math.min turns any number above 1 into 1, an
    -- AI.
    local s = {controller = math.min(controller, coe5.controller.ai), cls = readInt(t, section, "Class", 0)}
    -- Note 47: Look the class up by name, since a mod's class may have another index now. Only the
    -- game's Random choices (values below 1) have no name; an index above 0 without one counts as
    -- Random. t[section] is safe to look inside here, because its Controller was just found. `elseif`
    -- is a second test, tried only when the first one fails.
    local className = t[section].ClassName or ""
    if className ~= "" then
      s.cls = findClassByName(className)
      if not s.cls then
        log("preset_class_missing preset=%s player=%d class=%s", name, i, className)
        s.cls = 0   -- Note 48: Random
      end
    elseif s.cls > 0 then
      log("preset_class_unnamed preset=%s player=%d class=%d", name, i, s.cls)
      s.cls = 0   -- Note 49: Random
    end
    -- Note 50: Setup Participants indexes the AI level names with `ai` and doesn't check it, so a
    -- hand-edited or shared preset must not give it an out-of-range value. A value below 0 or above
    -- 10 becomes Jester. A color is kept only when all three of its numbers are there: tonumber turns
    -- text into a number, or gives nil for text that isn't one (and for the empty text that `or ""`
    -- gives a missing key). Each number is held to 0 to 1, the range a game color takes. A preset saved
    -- before colors has none, so its players keep the colors the game gives them.
    s.ai = readInt(t, section, "AI", 0)
    if s.ai < 0 or s.ai > AI_LEVEL_LAST then s.ai = AI_LEVEL_DEFAULT end
    s.team = readInt(t, section, "Team", -1)
    local red, green, blue = tonumber(t[section].ColorRed or ""), tonumber(t[section].ColorGreen or ""),
                             tonumber(t[section].ColorBlue or "")
    if red and green and blue then
      s.red, s.green, s.blue = math.max(0, math.min(1, red)), math.max(0, math.min(1, green)), math.max(0, math.min(1, blue))
    end
    p.slots[#p.slots + 1] = s
  end
  if #p.slots == 0 then return nil end   -- Note 51: `#` counts a list's items; a file with no players is no preset
  return p
end

-- Note 52: Writes the preset's players into the participant slots. It runs right after the game sets
-- its default participants and before the screen draws, so the first frame already shows the preset.
-- It returns true when it wrote them, and false when there was nothing to write.
local function applySlots()
  local p = slotsPreset
  if not p then return false end
  -- Note 53: Game slots count from 0 and Lua lists from 1, so slot i takes the list's item i + 1.
  -- `if s then` passes when there is such an item: in Lua only nil and false count as "no". A color
  -- written here stays: each new game's setup puts every slot on the game's own colors before this
  -- runs, and after it only the player changes one (the color square, or key r). A player the
  -- preset has no color for keeps the game's. `colored` counts the colors written, for the log line.
  local colored = 0
  for i = 0, MAX_SLOTS - 1 do
    local s = p.slots[i + 1]
    if s then
      coe5.set("slotController", i, s.controller)
      coe5.set("slotClass", i, s.cls)
      coe5.set("slotAi", i, s.ai)
      coe5.set("slotTeam", i, s.team)
      if s.red and HAS_COLORS then
        coe5.set("slotColorRed", i, s.red)
        coe5.set("slotColorGreen", i, s.green)
        coe5.set("slotColorBlue", i, s.blue)
        colored = colored + 1
      end
    else
      -- Note 54: The preset has fewer players than this slot, so a default player still sitting in it
      -- is removed.
      local controller = coe5.get("slotController", i)
      if controller == coe5.controller.human or controller == coe5.controller.ai then
        coe5.set("slotController", i, coe5.controller.removed)   -- Note 55: as the screen's delete button does
      end
    end
  end
  -- The preset's classes that Random doesn't roll come with its players (Note 140).
  randomOff = {}
  for _, className in ipairs(p.randomOff) do randomOff[className] = true end
  slotsPreset = nil   -- Note 56: done, so later calls (Note 113) return at once
  log("preset_applied_slots slots=%d colors=%d", #p.slots, colored)
  return true
end

-- ---- "Load a preset?" screen --------------------------------------------------------------------
-- Note 57: The screen uses the same widgets as Random Map Settings, so it looks like a vanilla
-- screen.

-- Note 58: The saved presets' names, "last_game" first and the rest in name order, at most
-- MAX_LISTED. coe5.listIni gives the names already sorted. table.insert(names, 1, x) puts x first and
-- moves the rest down one. table.remove(names) with no position removes the last item, and the
-- `while` loop repeats that as long as its test passes.
---@return string[]
local function listPresets()
  local names, hasLastGame = {}, false
  for _, name in ipairs(coe5.listIni()) do   -- Note 59: `_` is the usual name for a value the loop doesn't use
    if name == LAST_GAME then hasLastGame = true else names[#names + 1] = name end
  end
  if hasLastGame then table.insert(names, 1, LAST_GAME) end
  while #names > MAX_LISTED do table.remove(names) end
  return names
end

-- Note 60: The name the prompt shows for a preset file: "Previous Game Settings" for last_game, and underscores
-- shown as spaces for the rest. gsub gives back two values, the new text and how many replacements
-- it made. The extra round brackets keep only the first, so the function returns just the text.
local function presetDisplayName(name)
  if name == LAST_GAME then return "Previous Game Settings" end
  return (name:gsub("_", " "))
end

-- Note 61: Shows the "Load a preset?" screen just before Random Map Settings, and arms the chosen
-- preset. Returns true to go on to the map screen, or false to go back to the menu.
local function promptPreset()
  armedPreset = nil
  local names = listPresets()
  if #names == 0 then return true end   -- Note 62: nothing is saved yet, so the game goes on as vanilla
  local choice = 0              -- Note 63: 0 for no preset, i for names[i]
  local confirmDelete = false   -- Note 64: true after the first Delete click; the second click deletes
  coe5.ui.screenMusic()   -- Note 65: the menu screens' music, as the game starts it before a screen
  log("preset_prompt_opened presets=%d", #names)
  -- Note 66: A screen is a loop. Each pass draws the whole screen once, then coe5.ui.wait shows it
  -- and returns the id clicked (0 when nothing was). `while true do` repeats forever, so the loop only
  -- ends at a `return`, here on Ok or Cancel.
  while true do
    -- Note 67: "Previous Game Settings" is rewritten every game, so only named presets can be deleted.
    local deletable = choice > 0 and names[choice] ~= LAST_GAME
    coe5.ui.screenBegin("New Game Preset")
    coe5.ui.sectionBegin("Load a preset?")
    -- Note 68: coe5.ui.choice draws a radio line; its second input says whether it shows as picked.
    -- `choice == i` gives true for the picked line only. The screen remembers nothing itself: the
    -- script redraws the pick from `choice` on every pass.
    coe5.ui.choice(PRESET_CHOICE_BASE, choice == 0, "No preset")
    for i, name in ipairs(names) do coe5.ui.choice(PRESET_CHOICE_BASE + i, choice == i, presetDisplayName(name)) end
    coe5.ui.sectionEnd()
    coe5.ui.buttonRow()
    coe5.ui.text(PROMPT_OK, "Ok")
    -- Note 69: The `a and x or y` choice again (Note 31): after one Delete click the line asks again.
    if deletable then coe5.ui.text(PROMPT_DELETE, confirmDelete and "Really delete?" or "Delete") end
    coe5.ui.text(PROMPT_CANCEL, "Cancel")
    coe5.ui.bindKey(PROMPT_OK, coe5.key.enter)   -- Note 70: Enter clicks Ok, and Esc clicks Cancel
    coe5.ui.bindEscape(PROMPT_CANCEL)
    local id = coe5.ui.wait()
    -- Note 71: Any click but Delete takes back a first Delete click.
    if id ~= 0 and id ~= PROMPT_DELETE then confirmDelete = false end
    if id == PROMPT_DELETE and deletable then
      -- Note 72: The second click deletes the file, picks "No preset", and reads the list
      -- again.
      if confirmDelete then
        local deleted = coe5.deleteIni(names[choice])
        log("%s name=%s", deleted and "preset_deleted" or "preset_delete_failed", names[choice])
        confirmDelete = false
        choice = 0
        names = listPresets()
      else
        confirmDelete = true
      end
    elseif id == PROMPT_CANCEL then
      log("preset_prompt_closed result=cancel")
      return false
    elseif id == PROMPT_OK then
      -- Note 73: Ok reads the picked preset's file (Note 41). A file that can't be read leaves
      -- armedPreset nil, and the game goes on with no preset.
      if choice > 0 then
        armedPreset = loadPreset(names[choice])
        if not armedPreset then log("preset_load_failed name=%s", names[choice]) end
      end
      log("preset_prompt_closed result=ok preset=%s armed=%d", choice > 0 and names[choice] or "none", armedPreset and 1 or 0)
      return true
    -- Note 74: A click on one of the preset lines (ids 100 to 100 + the number of presets, Note 13)
    -- picks that line.
    elseif id >= PRESET_CHOICE_BASE and id <= PRESET_CHOICE_BASE + #names then
      choice = id - PRESET_CHOICE_BASE
    end
    -- Note 75: Any other id (0 for no click) changes nothing, and the loop draws the next frame.
  end
end

-- Note 76: Runs Random Map Settings. In single player it first asks "Load a preset?". The screen
-- copies its start values into locals on entry, so the preset has to be in place before next(). The
-- preset borrows some values the screen opens with (the command line's exact map size, the start
-- positions, Wilder), and this puts them back afterwards, so later new games without a preset behave
-- as vanilla.
--
-- Note 77: coe5.wrap(<site name>, function ... end) runs code at a step of the game. A site is a call
-- in the game's own code; coe5.lua lists every site and what it is for. The function you pass runs
-- each time the game makes that call, and it gets `next` first, then the call's inputs. Calling
-- next(...) lets the game's own step go ahead: code before it runs before the step, code after it
-- runs once the step is done, and `return` hands the game the result. coe5.wrap returns true when the
-- wrap is in place and false when this platform doesn't have the site; mapScreenWrapped keeps that
-- for the last log line (Note 138). NEW_WORLD_MAP_SCREEN is the whole Random Map Settings screen,
-- and its next returns 1 on Ok and 0 on Cancel.
local mapScreenWrapped = coe5.wrap("NEW_WORLD_MAP_SCREEN", function(next)
  -- Note 78: Another mod (Restart) is running this setup itself, so there is no prompt and no preset.
  if coe5.setupAutomated() then return next() end
  societyChoice = nil
  randomOff = {}   -- a new setup starts with every class rolled; a preset brings its own (Note 140)
  -- Note 79: Vanilla starts on Medium, or on the exact command-line size when one was given. The
  -- `a and x or y` choice (Note 31) works with -1 because -1 is a value: only nil and false count as
  -- "no" in Lua.
  mapSizeChoice = (coe5.get("cliMapW") > 0 or coe5.get("cliMapH") > 0) and -1 or 1
  -- Note 80: Cancel goes back to the main menu, like the map screen's Cancel. Lua stops an `and` at
  -- the first test that fails, so in a network game's setup ("setupNetwork" true) promptPreset never
  -- runs, and no prompt shows. Returning 0 tells the game Cancel was clicked.
  if not coe5.get("setupNetwork") and not promptPreset() then return 0 end
  local p = armedPreset
  if not p then return next() end   -- Note 81: no preset picked, so the screen runs as usual

  -- Note 82: Keeps the four values the preset borrows, so they can go back after the screen (Note 88).
  local savedW, savedH = coe5.get("cliMapW"), coe5.get("cliMapH")
  local savedStart, savedWilder = coe5.get("startDefault"), coe5.get("wilderDefault")
  coe5.set("society", p.society)
  coe5.set("commonCause", p.commonCause)
  coe5.set("scoreGraphs", p.scoreGraphs)
  coe5.set("battleReports", p.battleReports)
  coe5.set("startDefault", p.startPositions)
  coe5.set("wilderDefault", p.wilder ~= 0)   -- Note 83: "wilderDefault" takes only true or false, and `~= 0` gives one
  if p.mapSize >= 0 and p.mapSize <= 4 then
    -- Note 84: Click the size button for the player on the screen's first frame, so the button shows
    -- as selected and the game rolls the size the same way it does for a real click.
    pendingMapClick = coe5.newGame.mapSizeFirst + p.mapSize
  else
    -- Note 85: Any other size (-1) opens the screen on exactly the saved width and height, as the
    -- command line's --mapw and --maph do.
    coe5.set("cliMapW", p.mapWidth)
    coe5.set("cliMapH", p.mapHeight)
    mapSizeChoice = -1
  end
  coe5.set("northPart", p.north)
  coe5.set("southPart", p.south)
  -- Note 86: The test only asks whether the value exists here at all: 0 counts as "yes" in Lua, so
  -- even an unticked box (0) passes, and only nil skips the line.
  if coe5.get("newWorldUniqueRandom") then coe5.set("newWorldUniqueRandom", p.uniqueRandom) end
  log("preset_applied_map society=%d map_size=%d map=%dx%d start_positions=%d wilder=%d unique_random=%d",
    p.society, p.mapSize, p.mapWidth, p.mapHeight, p.startPositions, p.wilder, p.uniqueRandom)

  -- Note 87: The real screen now runs with the preset's values in place, and returns when the player
  -- clicks Ok or Cancel.
  local result = next()

  -- Note 88: The borrowed values go back (Note 82). pendingMapClick is cleared too, in case the
  -- screen never got to use it.
  coe5.set("cliMapW", savedW)
  coe5.set("cliMapH", savedH)
  coe5.set("startDefault", savedStart)
  coe5.set("wilderDefault", savedWilder)
  pendingMapClick = nil
  -- Note 89: On Ok (a result other than 0) the preset waits for Setup Participants (Note 52); on
  -- Cancel it is dropped. This `a and x or y` (Note 31) works because a preset is a table, never nil
  -- or false.
  slotsPreset = result ~= 0 and p or nil
  armedPreset = nil   -- Note 90: a chosen preset applies to one new game only
  return result
end)   -- Note 91: `end)` closes the function and then coe5.wrap's round bracket, since the function sits inside the call

-- Note 92: The game's default single-player participants; once next has set them, the preset's
-- players are written over them (Note 52). next returns nothing here, so neither does the function.
-- This site is Windows only; on Mac the fallback in setupWait puts the players in and has the
-- screen laid out again before its first frame shows (Note 113).
local initWrapped = coe5.wrap("NEW_WORLD_INIT_PARTICIPANTS", function(next, arg)
  next(arg)
  applySlots()
end)

-- Note 93: Draws "Save as preset" as the first button of the participants screen's Ok row, above Ok,
-- with an empty line before the row. coe5.ui.text with id 0 draws plain text that can't be clicked,
-- and empty text makes it a blank line. Lines drawn before next go above the row, and lines drawn
-- after it go into the row. It works the same on Mac, where CoE5SE makes the row itself.
local saveLineWrapped = coe5.wrap("PARTICIPANTS_BUTTON_ROW", function(next, style)
  coe5.ui.text(0, "")
  local result = next(style)
  coe5.ui.text(SAVE_PRESET_ID, "Save as preset")
  return result
end)

-- Note 94: Replaces every character but letters, digits, spaces, and - _ ( ) with _, and trims spaces
-- at both ends, so the name is always a valid file name.
--
-- Note 95: In a Lua pattern, [...] matches any one character listed inside, and a ^ right after the [
-- turns it around: any character NOT listed. a-z means every letter from a to z, and %- is a plain
-- dash (the % stops - from meaning something else in a pattern). gsub replaces each match with _.
-- Then match("^ *(.-) *$") hands back the text between any spaces at the start (^ *) and at the end
-- ( *$); .- is the shortest run of any characters that fits.
local function sanitizePresetName(name)
  return (name:gsub("[^a-zA-Z0-9 _()%-]", "_"):match("^ *(.-) *$"))
end

-- Note 96: Asks for a name in the game's name box and saves the current setup under it, when the save
-- line is clicked on the participants screen.
local function saveNamedPreset()
  local p = capturePreset()
  -- Note 97: The game's name box always shows a monster picture. This uses the leader of the first
  -- chosen class (class 1, the Baron, if every slot is Random), the picture the game shows when naming
  -- a player. `break` leaves the loop at the first slot with a real class.
  local cls = 1
  for _, s in ipairs(p.slots) do
    if classNameAt(s.cls) then cls = s.cls break end
  end
  -- Note 98: "classLeader" is the leader's monster name, and coe5.findMonster turns a name into the
  -- monster's index, or nil. The `and` tests check there is an index and that it is inside the
  -- monster table before its picture is read; otherwise the picture is 0.
  local leader = coe5.findMonster(coe5.get("classLeader", cls))
  local sprite = leader and leader <= coe5.get("monsterLastIndex") and coe5.get("monsterSprite", leader) or 0

  -- Note 99: coe5.ui.nameBox runs the game's own "Enter name" box until Ok or Cancel. 32 is the most
  -- characters the player can type. It returns the typed text (Enter clicks Ok), or nil on Cancel or
  -- Esc.
  local typed = coe5.ui.nameBox("Save Preset", "Preset name", sprite, 32)
  if not typed then
    log("preset_save_cancelled reason=cancel")
    return
  end
  -- Note 100: A name that is empty once cleaned, or the name Last game uses, is not saved: the second
  -- would be overwritten by the next Ok.
  local name = sanitizePresetName(typed)
  if name == "" or name == LAST_GAME then
    log("preset_save_cancelled reason=bad_name")
    return
  end
  local saved = savePreset(p, name)
  -- Note 101: The `a and x or y` choice (Note 31) picks the log line's first word.
  log("%s name=%s slots=%d unique_random=%d", saved and "preset_saved" or "preset_save_failed", name, #p.slots, p.uniqueRandom)
end

-- ---- New-game setup logging ---------------------------------------------------------------------
-- Note 102: This part writes the setup screens' clicks and values to coe5mod.log, so the log shows
-- what each screen did, while CoE5SE's "Enable copious logs" setting is on (off by default). It
-- changes nothing in the game.

-- Note 103: A class's name for the log, or "random" for an index that isn't a class.
local function classNameForLog(c) return classNameAt(c) or "random" end

-- Note 104: An AI level's name, or "?" out of range. The `or "?"` also covers a name the game's table
-- leaves empty, for which coe5.get gives nil.
local function aiLevelName(level)
  if level < 0 or level > AI_LEVEL_LAST then return "?" end
  return coe5.get("aiLevelName", level) or "?"
end

local function flag(on) return on and 1 or 0 end   -- Note 105: true becomes 1, false or nil 0, for %d in the log lines

-- Note 106: The state logged last, so each frame logs only what changed. prevSlotState is a table
-- with one text per slot number, empty at first.
local prevMapState, prevSlotState, prevScreen = "", {}, nil

-- Note 107: Logs the settings that changed since the last frame of any setup screen. Entering another
-- screen logs the whole state again: clearing the stored state makes every value look changed.
local function logSetupChanges(screen)
  if screen ~= prevScreen then
    log("setup_screen_entered screen=%s", screen)
    prevScreen, prevMapState, prevSlotState = screen, "", {}
  end
  -- Note 108: All the map values go into one text, so one comparison tells whether any changed. The
  -- format is one long text built from three pieces joined with `..`. %x writes a number in
  -- hexadecimal, and %.60s writes at most 60 characters of a text. `& 0xffff` keeps the low 16 bits
  -- of startFlags (Note 31).
  local mapState = string.format(
    "society=%d common_cause=%d score_graphs=%d start_flags=0x%x wilder=%d battle_reports=%d " ..
    "map=%dx%d north=%d south=%d loaded_map='%.60s' cli_map=%dx%d cli_society=%d " ..
    "start_default=%d wilder_default=%d subsoc=%d",
    coe5.get("society"), coe5.get("commonCause"), coe5.get("scoreGraphs"), coe5.get("startFlags") & 0xffff,
    coe5.get("wilder"), coe5.get("battleReports"), coe5.get("mapW"), coe5.get("mapH"),
    coe5.get("northPart"), coe5.get("southPart"), coe5.get("loadedMapName"),
    coe5.get("cliMapW"), coe5.get("cliMapH"), coe5.get("cliSociety"), coe5.get("startDefault"),
    flag(coe5.get("wilderDefault")), flag(coe5.get("subSociety")))
  if mapState ~= prevMapState then
    log("setup_map_state screen=%s %s", screen, mapState)
    prevMapState = mapState
  end
  -- Note 109: The same for each of the game's 32 player slots (0 to 31), one text per slot.
  for i = 0, 31 do
    local controller, cls, ai = coe5.get("slotController", i), coe5.get("slotClass", i), coe5.get("slotAi", i)
    local slot = string.format("controller=%d class=%d(%s) ai=%d(%s) team=%d name='%.24s'",
      controller, cls, classNameForLog(cls), ai, aiLevelName(ai), coe5.get("slotTeam", i), coe5.get("slotName", i))
    -- Note 110: Log only the slots the screen shows (a human or an AI), plus any slot that just stopped
    -- being shown. prevSlotState[i] is nil for a slot never shown, so a slot once shown keeps being
    -- watched.
    local shown = controller == coe5.controller.human or controller == coe5.controller.ai
    if slot ~= prevSlotState[i] and (shown or prevSlotState[i]) then
      log("setup_slot_state screen=%s slot=%d %s", screen, i, slot)
    end
    if shown or prevSlotState[i] then prevSlotState[i] = slot end
  end
end

-- Note 111: Runs one frame of a setup screen. `screen` is map_settings, participants, or
-- participants_2 (the AI difficulty popup). The three wraps at the end of the file (Note 137) all
-- call this one function, each with its own screen name, and pass on its result.
local function setupWait(screen, next, block, mode)
  -- Note 112: setupWait isn't a wrap itself. Each wrap's function hands it the wrap's next as an
  -- input, so calling next here still runs the game's own frame.
  --
  -- Note 113: A fallback only: the NEW_WORLD_INIT_PARTICIPANTS wrap normally applies the slots before
  -- the first draw. On Mac, which has no such site (Note 92), this call is the one that puts the
  -- preset's players in. Once applied, applySlots returns at once (Note 56). The game laid this frame
  -- out with its default players, so when the players went in here the script returns 0 without
  -- calling next: the frame is never shown, and the game lays the screen out again with the
  -- preset's players before it draws one (0 is no button's id, so the game does nothing else with
  -- it, as with Note 118).
  -- @platform-split new_world_init_participants: on Mac this wait puts the preset's players in
  local applied = screen == "participants" and applySlots()
  logSetupChanges(screen)
  if applied then return 0 end
  local clicked   -- Note 114: a local with no value yet starts as nil; one of the two branches below sets it
  if pendingMapClick and screen == "map_settings" then
    -- Note 115: The preset's size button id stands in for the player's click: the script returns it
    -- without calling next, so the game acts on it as if the player had clicked it (Note 84).
    clicked = pendingMapClick
    pendingMapClick = nil
    log("preset_clicked_for_player screen=%s id=%d", screen, clicked)
  else
    clicked = next(block, mode)
  end
  if clicked ~= 0 then log("setup_click screen=%s id=%d", screen, clicked) end
  if screen == "map_settings" then
    -- Note 116: The size buttons are ids 100 to 104 (Small to Enormous); a click on one notes its size
    -- for the next preset saved.
    local first = coe5.newGame.mapSizeFirst
    if clicked >= first and clicked <= first + 4 then mapSizeChoice = clicked - first end
    -- Note 117: Read before the game rolls a Random society.
    if clicked == coe5.newGame.mapSettingsOk then societyChoice = coe5.get("society") end
  elseif screen == "participants" then
    if clicked == SAVE_PRESET_ID then
      saveNamedPreset()
      return 0   -- Note 118: nothing for the game to do; the screen redraws
    end
    if clicked == coe5.newGame.participantsOk then
      -- Note 119: Ok saves "Previous Game Settings" before the game rolls the random classes.
      local p = capturePreset()
      local saved = savePreset(p, LAST_GAME)
      log("%s name=%s slots=%d", saved and "preset_saved" or "preset_save_failed", LAST_GAME, #p.slots)
    end
  end
  return clicked   -- Note 120: every other click goes to the game unchanged
end

-- ---- "Extended Game Settings" in the Game Menu --------------------------------------------------
-- Note 121: The game reads battle reports, score graphs, common cause, and each AI's difficulty while
-- it runs, and save_game writes them, so a change counts from the next battle, turn, or check and
-- stays in the save. The other setup options only shape the world as it is built, so they aren't
-- offered.

local SETUP_AI_BASE = 100          -- Note 122: the Extended Game Settings screen's line for AI player slot i is SETUP_AI_BASE + i
local LEVEL_CHOICE_BASE = 100      -- Note 123: the Difficulty Level screen's line for level n is LEVEL_CHOICE_BASE + n
local SLOT_COUNT = 32              -- Note 124: the game's player slots, 0 to 31

-- Note 125: The on/off options, in the order Random Map Settings lists them, with the game's own help
-- texts. SETUP_TOGGLES is a list of tables, one per option, each holding the same named values: the
-- line's id, its name in the log, the game value it changes, its label, and its tooltip. Code below
-- reads them as t.id, t.var, and so on.
local SETUP_TOGGLES = {
  {id = 1, name = "common_cause", var = "commonCause", label = "Common cause",
   tip = "A player only loses if no one on the team has a citadel or commander. This setting has no effect if there are no teams."},
  {id = 2, name = "score_graphs", var = "scoreGraphs", label = "Score graphs",
   tip = "Enables you to view score graphs of all players during an ongoing game. If disabled they will only be shown when the game is over."},
  {id = 3, name = "battle_reports", var = "battleReports", label = "Battle reports",
   tip = "Enabling this will prevent battles from being shown when they occur. Instead you will receive a battle report in the monthly messages. A replay of the battle can be viewed from the message."},
}

-- Note 126: An AI player's name on the screen. The screen leaves out classes: the game only shows
-- other players' classes with score graphs on. A slot without a name shows as "Player <slot + 1>",
-- so slot 0 is Player 1. Lua adds before it joins text, so the round brackets aren't needed, but
-- they make the order plain.
local function aiPlayerName(slot)
  local name = coe5.get("slotName", slot)
  if name == "" then return "Player " .. (slot + 1) end
  return name
end

-- Note 127: Asks for an AI player's difficulty, as Setup Participants' Difficulty Level popup does.
-- Returns the level picked, or nil on Cancel. It is a screen loop like the one in Note 66, with one
-- radio line per level from 1 to 10 and the current level shown as picked.
local function pickAiLevel(slot, level)
  while true do
    coe5.ui.screenBegin("Difficulty Level")
    coe5.ui.sectionBegin(aiPlayerName(slot))
    for n = AI_LEVEL_FIRST, AI_LEVEL_LAST do coe5.ui.choice(LEVEL_CHOICE_BASE + n, n == level, aiLevelName(n)) end
    coe5.ui.sectionEnd()
    coe5.ui.buttonRow()
    coe5.ui.text(PROMPT_CANCEL, "Cancel")
    coe5.ui.bindEscape(PROMPT_CANCEL)
    local id = coe5.ui.wait()
    if id == PROMPT_CANCEL then return nil end
    if id >= LEVEL_CHOICE_BASE + AI_LEVEL_FIRST and id <= LEVEL_CHOICE_BASE + AI_LEVEL_LAST then
      return id - LEVEL_CHOICE_BASE
    end
  end
end

-- Note 128: The Extended Game Settings screen. Clicks change the screen's copy; Ok writes the
-- changes into the game.
local function openGameSetup()
  -- Note 129: `on` is the screen's copy of the three options, true or false under each game value's
  -- name, so on.commonCause is Common cause. The game keeps them as 0 or 1, and `~= 0` turns that into
  -- true or false, because in Lua 0 itself would count as "yes".
  local on = {}
  for _, t in ipairs(SETUP_TOGGLES) do on[t.var] = coe5.get(t.var) ~= 0 end
  -- Note 130: `ais` lists the AI players: each item is a small table with the slot and a copy of its
  -- level.
  local ais = {}
  for i = 0, SLOT_COUNT - 1 do
    if coe5.get("slotController", i) == coe5.controller.ai then
      ais[#ais + 1] = {slot = i, level = coe5.get("slotAi", i)}
    end
  end
  log("game_setup_opened ai_players=%d", #ais)
  -- Note 131: A screen loop (Note 66). Each option is a clickable "Label: On" or "Label: Off" line with
  -- the game's help text as its tooltip, and the Difficulty Level box shows only when there are AI
  -- players.
  while true do
    coe5.ui.screenBegin("Extended Game Settings")
    coe5.ui.sectionBegin("Advanced Options")
    for _, t in ipairs(SETUP_TOGGLES) do
      coe5.ui.text(t.id, t.label .. ": " .. (on[t.var] and "On" or "Off"))
      coe5.ui.tooltip(t.tip)
    end
    coe5.ui.sectionEnd()
    if #ais > 0 then
      coe5.ui.sectionBegin("Difficulty Level")
      for _, a in ipairs(ais) do coe5.ui.text(SETUP_AI_BASE + a.slot, aiPlayerName(a.slot) .. ": " .. aiLevelName(a.level)) end
      coe5.ui.sectionEnd()
    end
    coe5.ui.buttonRow()
    coe5.ui.text(PROMPT_OK, "Ok")
    coe5.ui.text(PROMPT_CANCEL, "Cancel")
    coe5.ui.bindKey(PROMPT_OK, coe5.key.enter)
    coe5.ui.bindEscape(PROMPT_CANCEL)
    local id = coe5.ui.wait()
    -- Note 132: Cancel leaves the screen, and the copy is dropped with it.
    if id == PROMPT_CANCEL then
      log("game_setup_closed result=cancel")
      return
    elseif id == PROMPT_OK then
      -- Note 133: Ok writes each value that differs from the game's, and logs it. The copy holds true
      -- or false; `on[t.var] and 1 or 0` turns it back into the game's 1 or 0.
      for _, t in ipairs(SETUP_TOGGLES) do
        local value = on[t.var] and 1 or 0
        if coe5.get(t.var) ~= value then
          coe5.set(t.var, value)
          log("game_setup_changed option=%s value=%d", t.name, value)
        end
      end
      for _, a in ipairs(ais) do
        if coe5.get("slotAi", a.slot) ~= a.level then
          coe5.set("slotAi", a.slot, a.level)
          log("game_setup_changed option=ai_level slot=%d value=%d", a.slot, a.level)
        end
      end
      log("game_setup_closed result=ok")
      return
    end
    -- Note 134: A click on an option's line flips it in the copy: `not` turns true into false and false
    -- into true.
    for _, t in ipairs(SETUP_TOGGLES) do
      if id == t.id then on[t.var] = not on[t.var] end
    end
    -- Note 135: A click on an AI player's line opens the level pick (Note 127). On Cancel it gives nil,
    -- and `or a.level` keeps the level as it was.
    for _, a in ipairs(ais) do
      if id == SETUP_AI_BASE + a.slot then a.level = pickAiLevel(a.slot, a.level) or a.level end
    end
  end
end

-- Note 136: coe5.addGameMenuLine puts the "Extended Game Settings" line in the Game Menu. Its one
-- input is a table in curly braces (Note 29), and when a call's only input is a table, Lua lets you
-- leave out the round brackets. label and tooltip are plain text: edit them to rename the line or
-- change its hover text. section = coe5.gameMenuSection.settings puts the line at the end of the
-- Game Menu's Settings section, under Preferences; without it the line would go at the end of Saving
-- & Exiting. A CoE5SE older than coe5.gameMenuSection has no such table, and `a and b` gives nil
-- there instead of an error, so the line still shows, in Saving & Exiting. The line has no visible
-- function, so it shows in every game the script runs in (scripts don't run in network games).
-- onClick runs when the player clicks the line, and coe5.gameMenu.stay keeps the Game Menu open once
-- the Extended Game Settings screen closes.
local gameSetupAdded = coe5.addGameMenuLine{
  label = "Extended Game Settings",
  tooltip = "This game's battle reports, score graphs, common cause, and AI difficulty.",
  section = coe5.gameMenuSection and coe5.gameMenuSection.settings,
  onClick = function()
    openGameSetup()
    return coe5.gameMenu.stay
  end,
}

-- Note 137: Wraps the three setup screens' frame sites with setupWait (Note 111). The table's names
-- are the site names and its values the screen names; `{MAP_SETTINGS_WAIT = "map_settings"}` is
-- short for a name in quotes and square brackets. pairs visits every name and value in a table, in no
-- fixed order, which is fine because each wrap stands on its own. Each pass makes a new function that
-- remembers that pass's `screen`. `waits` counts the wraps that went in: 3 of 3 on both builds
-- (on Mac 1 of 3 under an older CoE5SE, which has MAP_SETTINGS_WAIT and PARTICIPANTS_DIFFICULTY_WAIT
-- on Windows only). `waits = waits + 1` adds one (Lua has no shorter += form).
local waits = 0
for site, screen in pairs({MAP_SETTINGS_WAIT = "map_settings", PARTICIPANTS_WAIT = "participants",
                           PARTICIPANTS_DIFFICULTY_WAIT = "participants_2"}) do
  if coe5.wrap(site, function(next, block, mode) return setupWait(screen, next, block, mode) end) then
    waits = waits + 1
  end
end
-- Note 138: A log line says which parts went in, as 1 or 0 each (Note 105).
log("setup_hooks_installed waits=%d of 3 map_screen=%d init_participants=%d save_line=%d game_setup=%d",
  waits, flag(mapScreenWrapped), flag(initWrapped), flag(saveLineWrapped), flag(gameSetupAdded))

-- ---- Random classes -----------------------------------------------------------------------------
-- Note 139: "Unique random players" keeps two players from rolling the same class. This part adds a
-- line under that box, "<random classes: 27 of 31>", that opens a checklist of the classes an AI
-- player's Random can roll; at least one class always stays checked. At "Creating World" each AI
-- player still on Random gets one of the checked classes, by the same rules the game's own roll
-- uses where a checked class fits them (Note 148). An unchecked class never goes to an AI player:
-- when "unique random players" has used up the checked classes, the next AI shares one. Human
-- players on Random are rolled by the game as always, and a class picked for a slot by hand is
-- never changed. randomOff, the sixth name in the script's memory (Note 18), holds the unchecked
-- classes as a set: randomOff["Troll King"] is true when the Troll King is off, and nil otherwise.
-- A set keyed by name, not by index, matters because a mod's class moves to another index when the
-- enabled mods change (Note 25): a name still means the same class, and a class whose mod is turned
-- off simply isn't listed until the mod comes back.
--
-- Note 140: The set travels with the setup. Every preset saves it as a [RandomClasses] section,
-- Off1 = "Troll King", Off2 = ..., and "Previous Game Settings" saves it on every Ok (Note 119). Loading a
-- preset brings its set back with its players (Note 52), and a new setup with no preset starts
-- with every class on (Note 78's wrap).

local RANDOM_CLASSES_ID = 901      -- Note 141: the line's id on Setup Participants, beside SAVE_PRESET_ID (Note 15)
local CLASS_LINE_BASE = 100        -- Note 142: the checklist's line for class c is CLASS_LINE_BASE + c (c is at most 249)
local ALL_ON, ALL_OFF, NEXT_PAGE, PREVIOUS_PAGE = 10, 11, 12, 13   -- the checklist's other lines (Note 14's form)
local CLASSES_PER_PAGE = 18        -- Note 143: the game's screens are 600 high, so a long list is split into pages
local NAMES_IN_TOOLTIP = 8         -- Note 144: the line's hover text names at most this many unchecked classes

-- Note 145: The checklist needs CoE5SE's classAbility (Note 147). Under an older CoE5SE, which lacks
-- the coe5.classAbility table, `a and b` stops at the nil and the pcall never runs, so
-- HAS_CLASS_ABILITY is false: the line isn't drawn and Random works as in the game.
local HAS_CLASS_ABILITY = coe5.classAbility ~= nil and pcall(coe5.get, "classAbility", 1, coe5.classAbility.aiClass)

-- Note 146: The classes the checklist lists, in the class table's order: every class with a name
-- except those the game never gives an AI by random choice (aiclass -1, such as a test class), since
-- unchecking one of those would change nothing. Each item is a small table with the class's index
-- and its name, cut to 63 characters as presets store it (Note 37).
local function listedClasses()
  local list = {}
  for c = 1, coe5.get("classLastIndex") do
    local name = coe5.get("className", c)
    if name and name ~= "" and coe5.get("classAbility", c, coe5.classAbility.aiClass) >= 0 then
      list[#list + 1] = {index = c, name = name:sub(1, CLASS_NAME_MAX)}
    end
  end
  return list
end

-- Note 147: Whether the game's own roll could give class c to slot `slot`, read from the class's
-- abilities with coe5.get("classAbility", class, id), as the game reads them: aiClass above 0 means
-- only an AI may roll it and below 0 only a human; aiLevelMin and aiLevelMax bound the AI's
-- difficulty, 0 meaning no bound. With "unique random players" on, a class another human or AI
-- player already has is out too. Only AI slots come here, so the human-only test is all aiClass
-- needs. `levels` false skips the difficulty bounds (Note 148).
local function gameAllows(c, slot, unique, levels)
  if coe5.get("classAbility", c, coe5.classAbility.aiClass) < 0 then return false end
  local level = coe5.get("slotAi", slot)
  local min = coe5.get("classAbility", c, coe5.classAbility.aiLevelMin)
  local max = coe5.get("classAbility", c, coe5.classAbility.aiLevelMax)
  if levels and ((min > 0 and level < min) or (max > 0 and level > max)) then return false end
  if unique then
    for i = 0, 31 do
      local controller = coe5.get("slotController", i)
      if i ~= slot and (controller == coe5.controller.human or controller == coe5.controller.ai)
         and coe5.get("slotClass", i) == c then
        return false
      end
    end
  end
  return true
end

-- Note 148: Picks a class for one AI slot, only ever from the checked classes. It tries three
-- times, each with fewer of the game's rules, and stops at the first that has a candidate:
--   1. every rule (Note 147), "unique random players" included;
--   2. the same without unique, so the AI shares a checked class another player has;
--   3. without the difficulty bounds too.
-- It returns nil only when no checked class is listed at all, which a preset can cause when every
-- class it left checked comes from a mod that is now off. TRIES is a list of tables, one per try,
-- and `unique and t.unique` keeps the test off in every try when the box itself is off.
--
-- The game draws a class at random and draws again while the class doesn't fit; it also skips a
-- class with a randomSkip value that percent of the time. This does the same from the shorter
-- list: math.random(n) gives a whole number from 1 to n, and math.random(0, 99) one from 0 to 99.
-- The draws stop after 100 per candidate, so a list whose every class skips 100% of the time can't
-- loop forever (the game's own roll would); the last draw is taken then.
local TRIES = {{unique = true, levels = true}, {unique = false, levels = true}, {unique = false, levels = false}}
local function pickClass(slot, unique, classes)
  for try, t in ipairs(TRIES) do
    local candidates = {}
    for _, cls in ipairs(classes) do
      if not randomOff[cls.name] and gameAllows(cls.index, slot, unique and t.unique, t.levels) then
        candidates[#candidates + 1] = cls
      end
    end
    if #candidates > 0 then
      local pick
      for _ = 1, 100 * #candidates do
        pick = candidates[math.random(#candidates)]
        local skip = coe5.get("classAbility", pick.index, coe5.classAbility.randomSkip)
        if skip <= 0 or math.random(0, 99) >= skip then break end
      end
      return pick, #candidates, try
    end
  end
  return nil
end

-- Note 149: The count the line shows, checked classes of all those listed, and whether any class
-- is off at all. Lua's next(t) gives a table's first name, or nil for an empty table, so
-- anyClassOff is true when the set has an entry; with every class on, the game's roll is left alone
-- (Note 152). Inside a wrap's function, `next` names the wrap's input instead, which is why this
-- test sits out here.
local function anyClassOff() return next(randomOff) ~= nil end

local function countChecked(classes)
  local on = 0
  for _, cls in ipairs(classes) do
    if not randomOff[cls.name] then on = on + 1 end
  end
  return on
end

-- Note 150: The hover text for the line: what it does, then the unchecked classes by name, at most
-- NAMES_IN_TOOLTIP of them and a count of the rest. table.concat joins a list's texts with ", "
-- between them.
local function randomClassesTip(classes)
  local off = {}
  for _, cls in ipairs(classes) do
    if randomOff[cls.name] then off[#off + 1] = cls.name end
  end
  local tip = "Classes Random can roll for AI players."
  if #off == 0 then return tip end
  local shown = {table.unpack(off, 1, math.min(#off, NAMES_IN_TOOLTIP))}
  tip = tip .. " Not rolled: " .. table.concat(shown, ", ")
  if #off > NAMES_IN_TOOLTIP then tip = tip .. string.format(", and %d more", #off - NAMES_IN_TOOLTIP) end
  return tip
end

-- Note 151: The checklist screen. Like the Extended Game Settings screen (Note 128) it works on a
-- copy, `off`, so Cancel changes nothing and Ok writes the copy back. `page` counts from 0; the
-- page's classes are items page * CLASSES_PER_PAGE + 1 onward. math.ceil rounds up, so 31 classes
-- make 2 pages, and math.max keeps at least one page for an empty list. At least one class stays
-- checked: a click on the last checked class does nothing, and "<all off>" leaves the first class
-- in the list on.
local function openRandomClasses()
  local classes = listedClasses()
  local off = {}
  for name in pairs(randomOff) do off[name] = true end
  local page, pages = 0, math.max(1, math.ceil(#classes / CLASSES_PER_PAGE))
  log("random_classes_opened classes=%d", #classes)
  while true do
    coe5.ui.screenBegin("Random Classes")
    coe5.ui.sectionBegin("Classes Random can roll")
    for k = page * CLASSES_PER_PAGE + 1, math.min(#classes, (page + 1) * CLASSES_PER_PAGE) do
      local cls = classes[k]
      coe5.ui.checkbox(CLASS_LINE_BASE + cls.index, not off[cls.name], cls.name)
    end
    coe5.ui.sectionEnd()
    local on = 0
    for _, cls in ipairs(classes) do
      if not off[cls.name] then on = on + 1 end
    end
    coe5.ui.text(0, string.format("Rolled %d of %d", on, #classes))
    coe5.ui.text(ALL_ON, "<all on>")
    coe5.ui.text(ALL_OFF, "<all off>")
    if pages > 1 then
      coe5.ui.text(0, string.format("Page %d of %d", page + 1, pages))
      if page + 1 < pages then coe5.ui.text(NEXT_PAGE, "<next page>") end
      if page > 0 then coe5.ui.text(PREVIOUS_PAGE, "<previous page>") end
    end
    coe5.ui.buttonRow()
    coe5.ui.text(PROMPT_OK, "Ok")
    coe5.ui.text(PROMPT_CANCEL, "Cancel")
    coe5.ui.bindKey(PROMPT_OK, coe5.key.enter)
    coe5.ui.bindEscape(PROMPT_CANCEL)
    local id = coe5.ui.wait()
    if id == PROMPT_CANCEL then
      log("random_classes_closed result=cancel")
      return
    elseif id == PROMPT_OK then
      randomOff = off
      log("random_classes_closed result=ok rolled=%d of %d", on, #classes)
      return
    elseif id == ALL_ON then
      off = {}
    elseif id == ALL_OFF then
      for k, cls in ipairs(classes) do off[cls.name] = k > 1 or nil end
    elseif id == NEXT_PAGE and page + 1 < pages then
      page = page + 1
    elseif id == PREVIOUS_PAGE and page > 0 then
      page = page - 1
    else
      -- A class's line flips it: `not off[name] or nil` stores true for a class turning off and nil
      -- (no entry) for one turning back on, so the set only ever holds the unchecked classes. The
      -- last checked class (on is 1 and this one is on) stays on.
      for _, cls in ipairs(classes) do
        if id == CLASS_LINE_BASE + cls.index and not (on == 1 and not off[cls.name]) then
          off[cls.name] = not off[cls.name] or nil
        end
      end
    end
  end
end

-- Note 152: Three wraps put the feature in. The button row wrap draws the line, with its hover
-- text, above Note 93's blank line: a wrap added later runs first (outside the earlier one), so
-- this line comes before the earlier wrap's. The participants wait wrap runs the frame through
-- next (the earlier wrap and the game) and opens the checklist when the line was clicked;
-- returning 0 makes the game draw the screen again with the new count. The Creating World wrap
-- gives each AI player on Random a checked class before next lets the game roll the humans'
-- Random. Its priority, -1, puts it inside every other script's wrap of that step, so Jazzy
-- Restart has put its saved players back (still on Random) before this runs.
local randomLineWrapped, randomWaitWrapped, randomRollWrapped = false, false, false
if HAS_CLASS_ABILITY then
  randomLineWrapped = coe5.wrap("PARTICIPANTS_BUTTON_ROW", function(next, style)
    local classes = listedClasses()
    coe5.ui.text(RANDOM_CLASSES_ID, string.format("<random classes: %d of %d>", countChecked(classes), #classes))
    coe5.ui.tooltip(randomClassesTip(classes))
    return next(style)
  end)
  randomWaitWrapped = coe5.wrap("PARTICIPANTS_WAIT", function(next, block, mode)
    local clicked = next(block, mode)
    if clicked ~= RANDOM_CLASSES_ID then return clicked end
    openRandomClasses()
    return 0
  end)
  randomRollWrapped = coe5.wrap("NEW_WORLD_CREATING_WORLD", function(next, kind, text)
    if anyClassOff() then
      local classes = listedClasses()
      local unique = (coe5.get("newWorldUniqueRandom") or 0) ~= 0
      for slot = 0, MAX_SLOTS - 1 do
        if coe5.get("slotController", slot) == coe5.controller.ai and coe5.get("slotClass", slot) < 1 then
          local pick, candidates, try = pickClass(slot, unique, classes)
          if pick then
            coe5.set("slotClass", slot, pick.index)
            log("random_class_rolled slot=%d class=%s candidates=%d try=%d", slot, pick.name, candidates, try)
          else
            log("random_class_left_to_game slot=%d reason=no_checked_class_listed", slot)
          end
        end
      end
    end
    next(kind, text)
  end, -1)
end
log("random_classes_installed class_ability=%d line=%d wait=%d roll=%d",
  flag(HAS_CLASS_ABILITY), flag(randomLineWrapped), flag(randomWaitWrapped), flag(randomRollWrapped))
