-- Note 1: Jazzy UI Tweaks makes small changes to the game's own screens. Each change has its own
-- setting on the mod's Mod Settings page, and turning a setting off puts that part of the game back
-- the way it was:
--   - Load Game shows <del> at the end of every save's row. Clicking it runs the game's own delete,
--     the one its Delete key runs, so the game still asks "Are you sure" (Notes 22 to 29).
--   - The column of mod banners on the right of the menu screens is half size (the default), full
--     size, or hidden (Notes 15 to 20).
--   - The main menu hides its Read Tip button (on by default). The buttons below it move up
--     (Notes 12 and 13).
--   - The mod banners stay on screen instead of fading out (on by default; Notes 18 and 21). The
--     menu buttons' fade is a separate setting, "Fade effect" in the game's own Preferences.
--   - On the ritual screen, a ritual's hover text lists what it needs to cast once the pointer has
--     rested on its row for 2 seconds, each line it can judge drawn white when met and grey when
--     not (off by default; Notes 30 to 46 and 50 to 55).
--   - The map editor's toolbox gets Show grid, and the x key does the same: faint square edges with
--     each square's map coordinates (on by default; Notes 57 to 65).
--
-- Note 2: This file is a Lua script. CoE5SE runs it; the mod has no DLL. CoE5SE reads the file once
-- per game launch, on the first main menu frame, and runs the lines that sit outside any function
-- from top to bottom. Most of those lines don't act yet: they hand CoE5SE a function to run later,
-- when the game reaches a certain step (coe5.wrap, Note 14). After you edit this file, restart the
-- game.
--
-- Note 3: A line or line end that starts with two dashes (--) is a comment. Lua skips it, so you can
-- change or delete any comment without changing what the mod does.
--
-- Note 4: The settings are declared in JazzyUITweaks.c5se, next to this file, with newsetting
-- blocks; that file holds their names, labels, and defaults. This script only reads them, with
-- coe5.setting(<setting name>). It reads a setting each time it matters, not once at the start, so a
-- change in Mod Settings counts at once, with no restart.

-- Note 5: `local` makes a name that only this file can see. Each mod's script runs on its own, so
-- another mod's script can't read or change these. Writing a name in CAPITALS is a common habit for
-- a value that never changes after this line; to Lua it is a name like any other. These numbers
-- describe the game's own layout, so change them only together with what the game draws.
local DELETE_FIRST_ID = 20000   -- Note 6: our <del> on row n gets the id 20000 + n, apart from the row's own id (10000 + n, Note 10)
local DELETE_COLUMN_X = 400     -- Note 7: where <del> starts; rows are 450 wide and the date starts at 225
local BANNER_ROW_STEP = 60      -- Note 8: the game draws banner row n at y = screen height - 60 - 60 * n
-- Note 9: One `local` line can make several names at once: the values after = go to the names before
-- it, in order, so BANNERS_HALF is 1 and BANNERS_HIDDEN is 2. They are the option numbers of the
-- mod_banners setting in JazzyUITweaks.c5se (0 Full size, 1 Half size, 2 Hidden).
local BANNERS_HALF, BANNERS_HIDDEN = 1, 2   -- mod_banners' options (0 is full size)

-- Note 10: coe5.screen is a table of the game's own numbers; coe5.lua lists them. The dot reads the
-- value stored in a table under a name: coe5.screen.fileBrowserRowFirst is 10000, and the file
-- browser gives save row n the id 10000 + n. A short local name for it keeps the lines below short.
local rowFirst = coe5.screen.fileBrowserRowFirst
-- Note 11: These two names are the script's memory. A `local` at the top of the file keeps its value
-- between calls until the game closes, so what one wrap stores, another one reads. true and false
-- work as on/off switches.
local inLoadGame = false   -- true while the file browser on screen belongs to Load Game
local deleteReady = false  -- true once all 3 wraps <del> needs are in; with fewer, a click could delete the wrong save

-- Note 12: coe5.hideMainMenuButton hides the main menu's Read Tip button on every frame where the
-- function you give it returns true. `function() ... end` with no name makes a function right inside
-- the call, a common Lua way to hand over code to run later. CoE5SE calls it on every main menu frame,
-- so it is kept to one short line.
--
-- Note 13: For an on/off setting, coe5.setting gives true or false, and nil (Lua's "nothing") when
-- the mod declares no setting by that name. `== true` asks "is it exactly true?", so the function
-- always returns true or false, as hideMainMenuButton expects, even for a nil. The call itself
-- returns true when CoE5SE took the request; `readTip` keeps that for the log line (Note 49).
local readTip = coe5.hideMainMenuButton(coe5.mainMenu.readTip, function()
  return coe5.setting("hide_read_tip") == true
end)

-- Note 14: coe5.wrap(<site name>, function ... end) runs code at a step of the game. A site is a call
-- in the game's own code; coe5.lua lists every site and what it is for. The function you pass runs
-- each time the game makes that call, and it gets `next` first, then the call's inputs. Calling
-- next(...) lets the game's own step go ahead with the inputs you hand it, and `return` hands the
-- game the result. Not calling next skips the game's step. coe5.wrap returns true when the wrap is in
-- place, and false when this platform doesn't have the site (coe5mod.log says why).
--
-- Note 15: MOD_BANNER_DRAW draws one enabled mod's banner. Its inputs are the banner's picture file
-- (path), where it goes (x, y), its width and height (w, h), and its opacity (alpha, 0 to 1). The
-- game draws row n at x = screen width - 210 and y = screen height - 60 - 60 * n, 200 wide. At half
-- size the column keeps its right edge and the rows sit half as far apart.
local banners = coe5.wrap("MOD_BANNER_DRAW", function(next, path, x, y, w, h, alpha)
  local size = coe5.setting("mod_banners")   -- Note 16: a choice setting gives its option's number (Note 9)
  -- Note 17: A plain `return` ends the function at once. next is never called, so the game never draws
  -- this banner: that is all Hidden needs.
  if size == BANNERS_HIDDEN then return end
  -- Note 18: "bannerFade" is the game's opacity for the whole banner column (Note 21), and alpha is
  -- this one banner's. The function's inputs are its own local names, so giving alpha a new value
  -- here changes what goes to next below: this banner is drawn fully opaque.
  if coe5.setting("keep_mod_banners") then
    coe5.set("bannerFade", 1)
    alpha = 1
  end
  -- Note 19: The game doesn't say which row it is drawing, so the script works it out from y. UI y
  -- counts up from the bottom, so screenH is the top. y = screenH - 60 - 60 * row, turned around, is
  -- row = (screenH - 60 - y) / 60. In Lua, / can give a number with a fractional part (5 / 2 is 2.5),
  -- which is fine for screen positions. Then the banner moves right by half its width and shrinks to
  -- half, so its right edge stays put, and its distance from the top halves to 30 + 30 * row.
  if size == BANNERS_HALF then
    local screenH = coe5.get("screenH")
    local row = (screenH - BANNER_ROW_STEP - y) / BANNER_ROW_STEP
    x = x + w / 2
    y = screenH - BANNER_ROW_STEP / 2 - row * BANNER_ROW_STEP / 2
    w = w / 2
    h = h / 2
  end
  -- Note 20: next draws the banner with the values as they now stand. This site gives no result, so
  -- the return passes on nothing.
  return next(path, x, y, w, h, alpha)
end)

-- Note 21: The game draws the banners only while their fade value is above 0, and lowers it every
-- frame. Setting it inside the banner draw (Note 18) stops working once the banners have faded out,
-- because the draw no longer runs. So while "Keep mod banners on screen" is on, every main menu frame
-- sets it back to full. MAIN_MENU_WAIT runs once for each frame (each redraw) of the main menu, and
-- its next returns the id of what was clicked; returning that id unchanged leaves the click to the
-- game.
local keep = coe5.wrap("MAIN_MENU_WAIT", function(next, block, mode)
  if coe5.setting("keep_mod_banners") then coe5.set("bannerFade", 1) end
  return next(block, mode)
end)

-- Note 22: Wraps Load Game's call to the file browser, so the other wraps know the browser is Load
-- Game's (other screens use the browser too). The switch goes on, next runs the whole browser and
-- returns only when it closes, and the switch goes off. Every frame and row of the browser happens
-- inside that next, so the wraps below see inLoadGame as true exactly while Load Game is open.
local browser = coe5.wrap("LOAD_GAME_BROWSER", function(next, title)
  inLoadGame = true
  local result = next(title)
  inLoadGame = false
  return result
end)

-- Note 23: Draws a save row's date, then <del> at the end of the row. next(id, text) draws the date
-- first; lines drawn after it go onto the same row. coe5.ui.columnX moves the next line to 400 from
-- the row's left edge, and coe5.ui.text draws a clickable line there with its own id (Note 6).
local rowDate = coe5.wrap("FILE_BROWSER_ROW_DATE", function(next, id, text)
  local result = next(id, text)
  -- Note 24: `and` needs every test to pass, and Lua stops at the first one that fails. In order: the
  -- browser is Load Game's (Note 22), all three wraps are in (Note 48), the id is a save row's (ids
  -- from 10000 up, Note 10), and the save_delete setting is on. `>=` means "at least".
  if inLoadGame and deleteReady and id >= rowFirst and coe5.setting("save_delete") then
    coe5.ui.columnX(DELETE_COLUMN_X)
    coe5.ui.text(DELETE_FIRST_ID + id - rowFirst, "<del>")   -- Note 25: row n's id is 10000 + n, so this is 20000 + n
  end
  return result
end)

-- Note 26: Turns a click on <del> into the browser's own delete action, aimed at that row. Right after
-- the wait, the browser deletes the row whose id is in "hoveredId", which it reads directly, and
-- nothing changes it in between, so the row's
-- id goes straight into it. FILE_BROWSER_WAIT runs once for each frame of the browser, and its next
-- returns the id clicked in that frame (0 for none).
local wait = coe5.wrap("FILE_BROWSER_WAIT", function(next, block, mode)
  local id = next(block, mode)
  -- Note 27: Only a <del> has an id from 20000 to just under 20000 + 10000 (Note 6). `<` means "less
  -- than".
  if inLoadGame and id >= DELETE_FIRST_ID and id < DELETE_FIRST_ID + rowFirst then
    local row = id - DELETE_FIRST_ID
    -- Note 28: `..` joins two pieces of text. Lua turns the number in `row` into text on its own.
    coe5.log("ui_tweaks_save_delete_clicked row=" .. row)
    coe5.set("hoveredId", rowFirst + row)
    -- Note 29: The game gets the Delete key's id in place of the click, so it runs its own delete, with
    -- its own "Are you sure" question, on the row just written into "hoveredId". Changing what a wrap
    -- returns is how it changes what the game does next.
    return coe5.screen.fileBrowserDelete
  end
  return id
end)

-- Note 30: The ritual screen readout. Hold the pointer on a ritual's row for RITUAL_HOVER_MS and its
-- hover text, at the bottom of the screen, gets a list of what the ritual needs to cast under the
-- game's own reason. The "ritual_needs" setting picks Off, the plain list (the default), or the plain
-- list plus the raw data a modder wants ("Advanced"); "ritual_needs_rows" picks greyed rows only (the
-- default) or every row. Every name in it comes from the game's own tables while the game runs
-- (resourceName, ritualTerrainName, planeName, monsterName, effectName), so a mod's resources,
-- terrains, and rituals show by their own names with no list kept in this file.
local RITUAL_HOVER_MS = 2000        -- how long the pointer stays on a row before the readout shows
local NEEDS_OFF, NEEDS_ADVANCED = 0, 2  -- ritual_needs' options (1 is the plain list)
local ROWS_ALL = 1                    -- ritual_needs_rows' options (0 is greyed rows only)
local BOX_BOTTOM, BOX_DROP, BOX_PAD = 16, 2, 5   -- the hover box's place in UI units (Note 43)
local EFFECT_MAXCAST, EFFECT_PLANEREQ = 1057, 939   -- the effect ids of the .c5m words maxcast and planereq
local EFFECT_NEWRIT, EFFECT_NOT_IMMORTAL = 904, 1088 -- newrit, and the effect the game's "immortal beings" reason checks
local EFFECT_NONEARBY1 = 1003        -- nonearby1req; nonearby3req to 99req follow, then nearby1req to 99req (1008-1012)
local NEARBY_TILES = {1, 3, 5, 7, 99} -- each rule's distance, in that order (99: anywhere on the plane)
local reasons = coe5.ritual   -- the game's reason codes (reasonMirror, ...) and row id bases, from coe5.lua

-- Note 31: What the game's two checks said about the row it is about to draw. For each ritual the
-- screen calls its location check, then its cast check, then draws the row, all inside the same
-- frame, so the row's wrap (Note 45) reads what these two observers stored just before it. Plain
-- assignments to locals cost almost nothing, which matters: the screen runs them for every row on
-- every frame, about 60 times a second.
local checkCommander, checkSlot, checkLocation, checkCast = -1, -1, 0, 0
local locationObserved = coe5.observe("RITUAL_SCREEN_LOCATION_CHECK", nil, function(result, commander, slot)
  checkCommander, checkSlot, checkLocation = commander, slot, result
end)
local castObserved = coe5.observe("RITUAL_SCREEN_CAN_CAST", nil, function(result, _, slot)   -- `_`: an input this function skips
  if slot == checkSlot then checkCast = result end
end)

-- Note 32: The hover timer. hoverRow is the row id the pointer was last seen on, hoverSince the
-- clock reading ("ticksMs", the game's millisecond clock) when it got there, and hoverSeen the last
-- reading while it stayed. Only the difference of two readings means anything (coe5.lua).
local hoverRow, hoverSince, hoverSeen = nil, 0, 0

-- Note 33: Built readouts, kept per ritual slot: { key = what it was built from, text = the result }.
-- A table indexed by numbers works like an array with holes, so slot 2999 needs no slots before it.
-- The readout is rebuilt only when its key changes (another turn, caster, cost, what the caster has
-- of a cost, or reason), so a pointer resting on a row costs one string comparison a frame instead
-- of a rebuild.
local readouts = {}
-- The readout on screen now: its marks (Note 37) and the row id it belongs to, or nil for none. The
-- row's wrap sets them (Note 45) and the hover box's line wrap reads them (Note 50).
local shownMarks, shownRow = nil, nil

-- Note 34: Returns whether the pointer has rested on row `id` for RITUAL_HOVER_MS. The clock is read
-- only for the hovered row, never for the others. A gap of more than half a second since the row was
-- last seen (the pointer left the screen, or the screen closed and opened again) restarts the wait,
-- and so does a reading that went backwards (the clock wraps around after about 49 days).
local function heldOn(id)
  local now = coe5.get("ticksMs")
  if hoverRow ~= id or now - hoverSeen > 500 or now < hoverSeen then
    hoverRow, hoverSince = id, now
  end
  hoverSeen = now
  return now - hoverSince >= RITUAL_HOVER_MS
end

-- Note 35: Adds one line to `lines`. table.insert(t, v) puts v after the last entry; collecting lines
-- in a table and joining them once with table.concat (Note 37's last line) is the usual Lua way to
-- build text, because each `..` makes a whole new string. A line the game decides (met is true or
-- false) is also kept in `marks`, a table indexed by the line's text, for its colour (Note 50); met
-- nil means the script can't tell, and the line keeps the game's colour.
local function needLine(lines, marks, text, met)
  local line = "- " .. text
  table.insert(lines, line)
  if met ~= nil then marks[line] = met end
end

-- The game's name for a cost type: resource 0 to 13 from its resource names, 14 to 18 (corpses,
-- gems, expendable units, goblins) from its income names without " Income" (string.gsub replaces
-- the pattern; `$` anchors it at the end), and any other by its number. `or` gives its right side
-- when the left is nil, so a name the game's table lacks never stops the script on a nil in `..`.
local function resourceName(kind)
  if kind <= 13 then return coe5.get("resourceName", kind) or ("Resource " .. kind) end
  if kind <= 18 then return (string.gsub(coe5.get("incomeName", kind) or ("Resource " .. kind), " Income$", "")) end
  return "Cost type " .. kind
end

-- Note 36: Reads one effect of a ritual by its id: the value of the first effect with that id, or
-- nil when it has none. A ritual holds up to 14 effects (0 to 13), and coe5.get gives nil past the
-- last one, which ends the loop. `for i = 0, 13 do` counts i from 0 to 13, both included.
local function effectValue(slot, effect)
  for i = 0, 13 do
    local id = coe5.get("ritualEffect", slot, i)
    if id == nil then return nil end
    if id == effect then return coe5.get("ritualEffectValue", slot, i) end
  end
  return nil
end

-- What the caster's side has of each of the row's four costs, in a list of four: a number, or false
-- when it can't tell (no cost there, or no known caster for a type past 13). "commanderCostAvailable"
-- counts what the game counts when the ritual is cast: the stock, the gems, and the corpses,
-- expendable units, and goblins on the caster's square. Without a known caster, types 0 to 13 use
-- the player's stock ("slotResource"). The list goes into the readout's key (Note 44), so casting a
-- ritual that uses up thralls or gold changes the marks of the rows still on screen.
local function costsHave(slot, row)
  local player = coe5.get("activePlayer")
  local caster = (checkSlot == slot and checkCommander >= 0) and checkCommander or -1
  local haves = {}
  for i = 1, 4 do
    local kind, have = row.costs[i * 2 - 1], nil
    if caster >= 0 and kind >= 0 and kind <= 18 then have = coe5.get("commanderCostAvailable", caster, kind) end
    if have == nil and player >= 0 and kind >= 0 and kind <= 13 then have = coe5.get("slotResource", player, kind) end
    haves[i] = have or false
  end
  return haves
end

-- Note 53: The monsters a ritual's unit rules name, from its strings, read as the game reads them:
-- "(&)Name" lines are the units a rule needs, "(-)" the ones it rules out, "(+)" the ones that lift
-- that ("unless"), and "(|)" a caster the rules don't apply to. The name is what follows the ")",
-- cut before " & ", then after a "*". string.match returns the pieces a pattern's brackets capture:
-- `%(` is a plain "(", `(.)` any one character, `(.*)` the rest; `.-` takes as little as it can.
local function ritualMonsters(slot)
  local lists = {["&"] = {}, ["-"] = {}, ["+"] = {}, ["|"] = {}}
  for i = 0, 47 do
    local text = coe5.get("ritualString", slot, i)
    if text == nil then break end
    local mark, rest = string.match(text, "^%((.)%)(.*)$")
    if mark and lists[mark] then
      local name = string.match(rest, "^(.-) & ") or rest
      name = string.match(name, "%*(.*)$") or name
      table.insert(lists[mark], name)
    end
  end
  return lists
end

-- The names in one of those lists, joined with " or " (the game adds their counts together).
local function monsterNames(list)
  return #list > 0 and table.concat(list, " or ") or "(none listed)"
end

-- "No <what>" when the rule allows none, or "At most N <what>"; `fewerThan` is the game's limit.
local function fewerThan(limit, what)
  return limit <= 1 and ("No " .. what) or ("At most " .. (limit - 1) .. " " .. what)
end

-- What lifts a "No ..." rule: a "(+)" monster there, and for most rules a "(|)" caster.
local function unlessText(lists, casterToo)
  local text = ""
  if #lists["+"] > 0 then text = text .. ", unless " .. table.concat(lists["+"], " or ") end
  if casterToo and #lists["|"] > 0 then text = text .. ", unless caster is " .. table.concat(lists["|"], " or ") end
  return text
end

-- Names joined as a list: "A", "A or B", "A, B or C".
local function orList(names)
  if #names <= 1 then return names[1] or "(none listed)" end
  return table.concat(names, ", ", 1, #names - 1) .. " or " .. names[#names]
end

-- Note 56: The promotion rule's line. The game reads the ritual's strings in pairs, the unit first,
-- then the monster it becomes, and skips a lone "(-)" line (one the unit rules read). The ritual can
-- be cast while an owned unit named by a pair's first line stands on the caster's square, the caster
-- too. A first line with another "(...)" before the name matches no unit there, so it gets no name.
-- `into` groups the units by what they become, in the order the pairs come; `count` is how many the
-- ritual promotes (-1: one, made a commander).
local function promotionText(slot, count)
  local into, order = {}, {}
  local i = 0
  while i <= 57 do
    local from = coe5.get("ritualString", slot, i)
    if from == nil then break end
    if string.sub(from, 1, 3) ~= "(-)" then
      local to = coe5.get("ritualString", slot, i + 1) or "?"
      if string.sub(from, 1, 1) ~= "(" then
        if not into[to] then into[to] = {}; table.insert(order, to) end
        local seen = false
        for _, name in ipairs(into[to]) do seen = seen or name == from end
        if not seen then table.insert(into[to], from) end
      end
      i = i + 1
    end
    i = i + 1
  end
  local upTo = count > 1 and ("up to " .. count .. " ") or ""
  if #order == 1 then
    return "Owned " .. orList(into[order[1]]) .. " here (" .. upTo .. (count > 1 and "become " or "becomes ") .. order[1] .. ")"
  end
  local groups = {}
  for _, to in ipairs(order) do table.insert(groups, orList(into[to]) .. " (becomes " .. to .. ")") end
  return "Owned " .. orList(groups) .. " here" .. (count > 1 and (", up to " .. count) or "")
end

-- Note 68: The fort parts in a fortreq or nofortreq value, each by the terrain word that sets its
-- bit (walls, irongate, ...), which CoE5SE reads from the game's terrain reader; a bit no word sets
-- shows its number.
local function fortPartWords(value)
  local words = {}
  for bit = 0, 31 do
    if value & (1 << bit) ~= 0 then
      table.insert(words, coe5.get("fortPartWord", bit) or ("fort part " .. (1 << bit)))
    end
  end
  return words
end

-- A plane by its name on this map, or by number when it has none.
local function planeCalled(plane)
  local name = plane >= 0 and plane <= 11 and coe5.get("planeName", plane) or ""
  return name ~= "" and name or ("plane " .. plane)
end

-- Note 54: The line for each of a ritual's other rules, by the effect id of its .c5m word. Each
-- function gets the effect's value, the monster lists (Note 53), and the slot, and returns nil where
-- the game reads no rule from that value. `[905] = function ...` puts the function in the table
-- under the number 905. Each function's test is the range of values the game reads as a rule
-- (levelreq above 0, eventvarreq 1 to 9999, noplanereq anything but 999, and so on); whether the
-- rule is met comes from ritualEffectMet, the game's own test (Note 39).
local RULE_LINES = {
  [905] = function(v) if v > 0 then return "Mastery Level exactly " .. v .. ", none higher" end end,   -- levelreq
  [912] = function(v) if v ~= 0 then return "Caster without flags " .. v end end,                      -- resrestrict
  [913] = function(v) if v ~= 0 then return "Caster with flags " .. v end end,                         -- resreq
  [924] = function(v) if v ~= 0 then return "Caster without benefits " .. v end end,                   -- benrestrict
  [962] = function(v) if v ~= 0 then return "Caster with ability " .. v end end,                       -- fxreq
  [973] = function(v) if v > 0 then return "No unit with ability " .. v .. " here" end end,            -- nofxherereq
  [974] = function(v, _, slot) if v ~= 0 then return promotionText(slot, v) end end,                     -- promotion
  [980] = function(v) if v > 0 then return "Enchantment " .. v .. " here" end end,                     -- enchherereq
  [986] = function(v) if v ~= 0 then return "No " .. orList(fortPartWords(v)) .. " here" end end,       -- nofortreq
  [987] = function(v) if v ~= 0 then return table.concat(fortPartWords(v), " and ") .. " here" end end, -- fortreq
  [993] = function(v) if v ~= 0 then return "Owned portal" end end,                                    -- hasportalreq
  [998] = function(v) if v > 0 then return "No portal here" end end,                                   -- noportalreq
  [1034] = function(v) if v ~= 999 then return "Not in " .. planeCalled(v) end end,                     -- noplanereq
  [1060] = function(v)                                                                                  -- homereq
    if v > 0 then return "In home province" elseif v < 0 then return "Not in home province" end
  end,
  [1074] = function(v)                                                                                  -- homecitreq
    if v > 0 then return "In home citadel" elseif v < 0 then return "Not in home citadel" end
  end,
  [1072] = function(v) if v >= 1 and v <= 9999 then return "Event variable " .. v .. " set" end end,     -- eventvarreq
  [1073] = function(v) if v >= 1 and v <= 9999 then return "Event variable " .. v .. " not set" end end, -- noeventvarreq
  [1085] = function(v)                                                                                  -- seasonreq
    if v >= 0 then return "In " .. (v <= 3 and coe5.get("seasonName", v) or ("season " .. v)) end
  end,
  [1071] = function(v, m) if v > 0 then return monsterNames(m["&"]) .. " here (at least " .. v .. ")" end end,   -- minmonreq
  [1062] = function(v, m) if v > 0 then return monsterNames(m["&"]) .. " in the world (at least " .. v .. ")" end end,   -- monworldreq
  [1063] = function(v, m) if v > 0 then return "Owned " .. monsterNames(m["&"]) .. " (at least " .. v .. ")" end end,   -- monplayerreq
  [946] = function(v, m) if v > 0 then return fewerThan(v, monsterNames(m["-"]) .. " here") .. unlessText(m, false) end end,   -- nomonreq
  [1061] = function(v, m, slot)                                                                         -- fewmonreq, read only without a nomonreq
    local nomon = effectValue(slot, 946)
    if v > 0 and (nomon == nil or nomon == 0) then return fewerThan(v + 1, monsterNames(m["-"]) .. " here") .. unlessText(m, false) end
  end,
  [947] = function(v, m) if v > 0 then return fewerThan(v, monsterNames(m["-"]) .. " with a home here") .. unlessText(m, true) end end,   -- nomonhomereq
  [1015] = function(v, m) if v > 0 then return fewerThan(v, monsterNames(m["-"]) .. " in the world") .. unlessText(m, true) end end,   -- nomonworldreq
  [1016] = function(v, m) if v > 0 then return fewerThan(v, "owned " .. monsterNames(m["-"]) .. " in the world") .. unlessText(m, true) end end,   -- nomonplayerreq
  [1018] = function(v, m)                                                                               -- nomonelysiumreq: planes 0, 2, 3
    if v > 0 then
      local planes = planeCalled(0) .. ", " .. planeCalled(2) .. ", or " .. planeCalled(3)
      return fewerThan(v, monsterNames(m["-"]) .. " in " .. planes) .. unlessText(m, true)
    end
  end,
  [1017] = function(v, m, slot)                                                                         -- nomonmasteryreq
    if v > 0 then
      return "No owned " .. monsterNames(m["-"]) .. ", or Mastery Level above " .. coe5.get("ritualLevel", slot) .. unlessText(m, true)
    end
  end,
}

-- Note 37: The plain list: each thing the ritual needs, one line each, marked met or not met where
-- the game already decides it; the mark shows only as the line's colour (Note 50). `row` holds the
-- row's own arguments (Note 45): the costs there are what this caster pays, after the game's
-- discounts. The marks come from what the caster's side has (commanderCostAvailable), the game's
-- terrain test (ritualTerrainAt, which works as the ritual screen's own), the caster's mastery
-- (unitAbility), the game's own test of each other rule (ritualEffectMet), and the reason codes the
-- game's two checks returned (Note 31). It returns the text and the marks.
local function plainNeeds(slot, row, haves)
  local lines, marks = {}, {}
  local player = coe5.get("activePlayer")
  -- The caster's unit, from the commander the location check named for this row; -1 when unknown.
  local unit = (checkSlot == slot and checkCommander >= 0) and coe5.get("commanderUnit", checkCommander) or -1
  for i = 1, 4 do
    local kind, amount, have = row.costs[i * 2 - 1], row.costs[i * 2], haves[i]
    -- Note 38: `~=` means "not equal". A cost type below 0 is no cost. A cost is met when what the
    -- caster's side has of it (costsHave, below) covers the amount; false there means unknown, and
    -- the line gets no mark.
    if kind >= 0 and amount ~= 0 then
      local met
      if have then met = have >= amount end
      needLine(lines, marks, resourceName(kind) .. " " .. amount, met)
    end
  end
  if coe5.get("ritualTerrain", slot) ~= -1 then
    local met
    if unit >= 0 then
      local x, y = coe5.get("unitX", unit), coe5.get("unitY", unit)
      if x >= 0 and x <= 999 and y >= 0 and y <= 159 then met = coe5.get("ritualTerrainAt", slot, x, y) end
    end
    needLine(lines, marks, "Terrain: " .. coe5.get("ritualTerrainName", slot), met)
  end
  -- Note 40: The screen lists a known ritual only while the caster's mastery level in its school
  -- (the ability numbered by its ritpow) is at least the ritual's level, so a listed one meets it.
  -- The line reads "Mastery Level 1(2)": the ritual's level, then the caster's in brackets.
  local level = coe5.get("ritualLevel", slot)
  local ritpow = coe5.get("ritualRitpow", slot)
  if unit >= 0 and ritpow >= 1 and ritpow <= 1501 then
    local mastery = coe5.get("unitAbility", unit, ritpow)
    needLine(lines, marks, "Mastery Level " .. level .. "(" .. mastery .. ")", mastery >= level)
  else
    needLine(lines, marks, "Mastery Level " .. level)
  end
  local maxcast = effectValue(slot, EFFECT_MAXCAST)
  if maxcast and maxcast > 0 then
    local cast = player >= 0 and coe5.get("slotRitualCasts", player, slot) or 0
    needLine(lines, marks, "Cast at most " .. maxcast .. (maxcast == 1 and " time" or " times") .. ", cast " .. cast .. " so far",
      checkCast ~= reasons.reasonMaxcast)
  end
  -- Note 41: The game passes the mirror the ritual needs as the row's last argument, already in words
  -- ("any type of empty mirror under the command of the mage"), and an empty text when it needs none.
  if row.mirrorText and row.mirrorText ~= "" then
    needLine(lines, marks, "Mirror: " .. row.mirrorText, checkCast ~= reasons.reasonMirror)
  end
  local plane = effectValue(slot, EFFECT_PLANEREQ)
  if plane and plane >= 0 and plane <= 11 then
    needLine(lines, marks, "Plane: " .. planeCalled(plane), checkCast ~= reasons.reasonPlane)
  elseif checkCast == reasons.reasonPlane then
    needLine(lines, marks, "Another plane", false)
  end
  if effectValue(slot, EFFECT_NOT_IMMORTAL) or checkCast == reasons.reasonImmortal then
    needLine(lines, marks, "A caster that isn't immortal", checkCast ~= reasons.reasonImmortal)
  end
  if effectValue(slot, EFFECT_NEWRIT) or checkCast == reasons.reasonMastery then
    needLine(lines, marks, "A ritual left to learn on this mastery level", checkCast ~= reasons.reasonMastery)
  end
  -- The location check's own reasons. Only a failed one says which need it was, so these show only
  -- then; their numbers are the game's (notes in coe5.lua's RITUAL_SCREEN_LOCATION_CHECK).
  if checkLocation == -3 then needLine(lines, marks, "More trade points than established guilds here", false) end
  if checkLocation == -4 then needLine(lines, marks, "A guild here", false) end
  -- Note 52: The terrain distance rules, one line each: nearby1req to nearby99req need the terrain
  -- within 1, 3, 5, 7 tiles or anywhere on the plane, and nonearby1req to nonearby99req need none
  -- there. ritualEffectTerrainName is nil for any other effect, so that skips them; ritualEffectMet is
  -- the game's own check at the caster's square.
  for i = 0, 13 do
    local id = coe5.get("ritualEffect", slot, i)
    if id == nil then break end
    local terrain = coe5.get("ritualEffectTerrainName", slot, i)
    if terrain then
      local rule = id - EFFECT_NONEARBY1
      local tiles = NEARBY_TILES[rule % 5 + 1]
      local text
      if tiles == 99 then
        text = rule < 5 and ("No " .. terrain .. " on this plane") or (terrain .. " on this plane")
      else
        text = rule < 5 and ("At least " .. (tiles + 1) .. " tiles away from " .. terrain) or ("Within " .. tiles .. " tiles of " .. terrain)
      end
      local met
      if checkSlot == slot and checkCommander >= 0 then met = coe5.get("ritualEffectMet", slot, i, checkCommander) end
      needLine(lines, marks, text, met)
    end
  end
  -- Note 39: Every other rule the ritual has, one line each, in the order of its effects. The game
  -- reads only the first effect with each id, so `seen` skips a repeat. RULE_LINES (above) gives the
  -- line's text, or nil where the game reads no rule from that value. The action point cost has no
  -- line: the game casts while the caster has any action point left, so the cost carries over like
  -- exhaustion and never stops a cast.
  local monsters = ritualMonsters(slot)
  local seen = {}
  for i = 0, 13 do
    local id = coe5.get("ritualEffect", slot, i)
    if id == nil then break end
    local rule = RULE_LINES[id]
    local text = rule and not seen[id] and rule(coe5.get("ritualEffectValue", slot, i), monsters, slot)
    seen[id] = true
    if text then
      local met
      if checkSlot == slot and checkCommander >= 0 then met = coe5.get("ritualEffectMet", slot, i, checkCommander) end
      needLine(lines, marks, text, met)
    end
  end
  return "Needs to cast:\n" .. table.concat(lines, "\n"), marks
end

-- Note 42: The raw block for modders ("Advanced"): the reason codes and the ritual's record as the
-- game holds it, with the costs shown both as charged and as the base. string.format fills %d with a
-- whole number, %s with text, and %x with a number in hexadecimal; %q would quote, so the strings
-- get their quotes by hand. The ritual's strings come back as a list of their own, for
-- readoutFor to fit (Note 44). A function can return several values: `return a, b`.
local function rawNeeds(slot, row)
  local lines = {}
  table.insert(lines, string.format("slot %d  ritpow %d  lvl %d  flags 0x%x  codes cast:%d loc:%d", slot,
    coe5.get("ritualRitpow", slot), coe5.get("ritualLevel", slot), coe5.get("ritualFlags", slot), checkCast, checkLocation))
  table.insert(lines, string.format('terr %d "%s"', coe5.get("ritualTerrain", slot), coe5.get("ritualTerrainName", slot)))
  local cost = "cost"
  for i = 0, 3 do
    local kind = coe5.get("ritualCostType", slot, i)
    if kind == nil then break end
    local name = "/" .. resourceName(kind)
    cost = cost .. string.format("  %d%s %d (base %d)", kind, name, row.costs[i * 2 + 2], coe5.get("ritualCost", slot, i))
  end
  table.insert(lines, cost .. "  ap " .. row.apCost)
  local effects, maxcast = "fx", nil
  for i = 0, 13 do
    local id = coe5.get("ritualEffect", slot, i)
    if id == nil then break end
    local value = coe5.get("ritualEffectValue", slot, i)
    if id == EFFECT_MAXCAST then maxcast = value end
    effects = effects .. "  " .. (coe5.get("effectName", id) or tostring(id)) .. "=" .. value
  end
  table.insert(lines, effects)
  local player = coe5.get("activePlayer")
  if player >= 0 then
    table.insert(lines, string.format("cast p%d %d%s", player, coe5.get("slotRitualCasts", player, slot), maxcast and ("/" .. maxcast) or ""))
  end
  -- string.sub(s, 1, 120) keeps a string's first 120 characters, so each one stays a single line.
  local strings = {}
  for i = 0, 47 do
    local text = coe5.get("ritualString", slot, i)
    if text == nil then break end
    table.insert(strings, string.format('str%d "%s"', i, string.sub(text, 1, 120)))
  end
  return table.concat(lines, "\n"), strings
end

-- Note 43: Where the game puts the hover box, in UI units, y counting up from the bottom of the
-- screen: for a text of n lines its bottom is at 16 less 2 for each line, and its lines are
-- "tooltipLineHeight" apart (14), drawn from the last one up. So past 8 lines the last ones start
-- below the screen. Blank lines at the end of the text take those places and lift the rest:
-- `pad` of them keep the last line 5 units above the bottom (the box's own edge) when
-- pad * (height - 2) is at least 2n - 16 + 5 (math.ceil rounds up). The box's top is then at
-- 16 + 5 + (n + pad) * (height - 2), which must stay on the screen ("screenH" units tall), so
-- `fits` says whether n lines fit at all.
local function boxPad(n, height)
  return math.max(0, math.ceil((BOX_DROP * n - BOX_BOTTOM + BOX_PAD) / (height - BOX_DROP)))
end
local function fits(n, height)
  return BOX_BOTTOM + BOX_PAD + (n + boxPad(n, height)) * (height - BOX_DROP) <= coe5.get("screenH")
end

-- Counts a text's lines: string.gsub returns the new text and how many it replaced; select(2, ...)
-- keeps the second of those values, the count of line breaks, and a text has one more line than that.
local function lineCount(text) return select(2, string.gsub(text, "\n", "\n")) + 1 end

-- Note 44: The row's whole hover text, built once and kept (Note 33): the game's own reason first,
-- then a blank line (the box shows it as a paragraph break), the plain list, and for Advanced the raw
-- block and as many of the ritual's strings as fit on the screen (Note 43), the rest counted
-- ("+5 more strings"); then the blank lines that lift the box. The key joins everything the text
-- depends on into one string; table.concat with a separator puts "," between the pieces. The cache
-- is per slot, so moving between two rows doesn't rebuild either one. It returns the text and the
-- plain list's marks (Note 37).
local function readoutFor(slot, row, mode)
  local height = coe5.get("tooltipLineHeight")
  local haves = costsHave(slot, row)
  -- tostring turns each entry, a number or false, into text for the key.
  local key = table.concat({coe5.get("turn"), mode, checkCommander, checkCast, checkLocation, row.apCost, row.hover or "",
    row.mirrorText or "", table.concat(row.costs, ","), tostring(haves[1]), tostring(haves[2]), tostring(haves[3]),
    tostring(haves[4]), height, coe5.get("screenH")}, ",")
  local kept = readouts[slot]
  if kept and kept.key == key then return kept.text, kept.marks end
  local needs, marks = plainNeeds(slot, row, haves)
  local text = (row.hover or "") .. "\n\n" .. needs
  if mode == NEEDS_ADVANCED then
    local raw, strings = rawNeeds(slot, row)
    text = text .. "\n\n" .. raw
    -- `#strings` is how many entries the list has. Each string is shown while it fits with room for
    -- the "+n more" line, and the first that doesn't stops the loop (`break`).
    local shown = 0
    for i, line in ipairs(strings) do
      local room = i == #strings and 1 or 2   -- the last string needs no "+n more" line after it
      if not fits(lineCount(text) + room, height) then break end
      text = text .. "\n" .. line
      shown = i
    end
    if shown < #strings then text = text .. "\n+" .. (#strings - shown) .. " more strings" end
  end
  text = text .. string.rep("\n", boxPad(lineCount(text), height))
  readouts[slot] = {key = key, text = text, marks = marks}
  return text, marks
end

-- Note 45: RITUAL_SCREEN_ROW draws one row of the ritual screen; the game calls it for every ritual
-- the caster knows, on every frame. Its 17 inputs are, in order: the row's id (the slot plus
-- coe5.ritual.rowCastable, or plus rowGreyed when greyed out), two numbers to pass on unchanged, the
-- ritual's name, four pairs of cost type and charged amount, the hover text, the action point cost,
-- whether it can be cast (1 or 0), the mirror monster, and the mirror's text. Most rows leave at
-- once: when the setting is Off, or the pointer isn't on this row, the wrap only hands the inputs
-- on, which keeps 30 rows cheap.
local ritualRow = coe5.wrap("RITUAL_SCREEN_ROW", function(next, id, a, b, name, t1, c1, t2, c2, t3, c3, t4, c4, hover, apCost,
                                                           castable, mirrorMonster, mirrorText)
  local mode = coe5.setting("ritual_needs")
  if mode ~= nil and mode ~= NEEDS_OFF and coe5.get("hoveredId") == id
      and (castable == 0 or coe5.setting("ritual_needs_rows") == ROWS_ALL) and heldOn(id) then
    local slot = castable == 0 and id - reasons.rowGreyed or id - reasons.rowCastable
    -- Note 46: A table built with { name = value } groups the row's inputs under names, so the
    -- functions above take one argument instead of fourteen.
    local row = {costs = {t1, c1, t2, c2, t3, c3, t4, c4}, apCost = apCost, hover = hover, mirrorText = mirrorText}
    hover, shownMarks = readoutFor(slot, row, mode)
    shownRow = id
  elseif shownRow == id then
    shownMarks, shownRow = nil, nil   -- the readout's row no longer shows it
  end
  return next(id, a, b, name, t1, c1, t2, c2, t3, c3, t4, c4, hover, apCost, castable, mirrorMonster, mirrorText)
end)

-- Note 50: The marks show as colour, like the ritual screen's own rows: a castable row's name is in
-- the game's text colour and a greyed one's in the same colour at half opacity. The hover box draws
-- in white, so a line that is met stays the game's white and a line that is not met gets the box's
-- white at the greyed rows' opacity, a grey. TOOLTIP_LINE_DRAW runs for each line of the hover box on
-- every frame, on every screen, with the line's text, its place in the box (0 the top), and its colour
-- (r, g, b, alpha, each 0 to 1). Most calls leave at the first test: no readout on screen. The game
-- cuts a long line to the box's width, and lineMark (Note 51) finds the mark of such a piece.
local GREYED_ALPHA = 0.5   -- the ritual screen's greyed rows: the text colour at this opacity
local function lineMark(marks, text)
  local mark = marks[text]
  -- Note 51: A line the box cut is drawn as pieces, none of them the whole line, so the first time a
  -- piece shows, the marked lines are searched for one holding it (string.find with `true` reads the
  -- piece as plain text, not a pattern). What was found is kept under the piece, "none" when nothing
  -- holds it, so each piece is searched once. An empty line is never searched: every line holds it.
  if mark == nil and text ~= "" then
    mark = "none"
    for line, met in pairs(marks) do
      if met ~= "none" and string.find(line, text, 1, true) then mark = met break end
    end
    marks[text] = mark
  end
  return mark
end
-- Note 55: A CoE5SE older than the TOOLTIP_LINE_DRAW site stops the whole script at a coe5.wrap that
-- names it, so this wrap goes through pcall. pcall(f, ...) runs f with the inputs after it and
-- returns true and f's result, or false instead of stopping the script when f fails. Under such a
-- CoE5SE, lineColour ends up false: the readout shows in the game's white, and the rest of the mod
-- works.
local wrapped, lineColour = pcall(coe5.wrap, "TOOLTIP_LINE_DRAW", function(next, text, index, r, g, b, alpha)
  if shownMarks and lineMark(shownMarks, text) == false and coe5.get("hoveredId") == shownRow then
    alpha = GREYED_ALPHA
  end
  return next(text, index, r, g, b, alpha)
end)
lineColour = wrapped and lineColour

-- Note 57: The map editor's grid. "Show grid" in the editor's toolbox, right under Create Fancy
-- Terrain, or the grid key (x unless the player picks another), draws faint lines on the squares'
-- edges and each square's map coordinates, x,y, in its bottom right corner; "Hide grid" and the key
-- take it away. Zoomed far out, where a label in every square would be too small to read and too
-- many to draw, the coordinates run along the top and left edges of the map instead, like a table's
-- headers. The numbers are the ones a map file uses: x and y counted from the plane's own first
-- square. The grid is off each time the editor opens. Everything it looks like is set here:
local GRID_R, GRID_G, GRID_B = 1, 1, 1   -- the lines' and labels' colour, white, each 0 to 1
local GRID_ALPHA = 0.3                   -- the labels' opacity, and the lines' at first: 0 invisible, 1 solid
local LINE_ALPHA_STEP = 0.1              -- each click of "Grid lines +" or "Grid lines -" moves the lines' opacity this much...
local LINE_ALPHA_MIN, LINE_ALPHA_MAX = 0.1, 1   -- ...within these
local GRID_LINE_WIDTH = 1                -- the lines' width in screen pixels
local LABEL_MIN_ZOOM = 36                -- squares at least this many UI units wide get a label each; smaller ones get the edge headers
local LABEL_SIZE_PER_ZOOM = 0.2          -- a square's label is this share of the square's width high...
local LABEL_SIZE_MIN, LABEL_SIZE_MAX = 9, 13   -- ...but never smaller or bigger than these
local HEADER_SIZE = 9                    -- the edge headers' height
local LABEL_PAD = 2                      -- the gap between a label and its square's edges
local CHAR_WIDTH = 0.35                  -- a character's width as a share of the text size, to right-align the labels
local PLANE_BAR = 25                     -- the plane buttons along the top of the screen, which the headers stay under
local TOOLBOX_RIGHT, TOOLBOX_BOTTOM = 160, 240   -- the toolbox's corner (UI x, y); row headers move right of it while it shows

-- Note 58: The script's memory for the grid: whether it is on, and whether the toolbox shows (the
-- editor tells no script that, so the script follows Tab and Hide Toolbox itself, Note 61).
local gridOn, toolboxShown = false, true
local function gridAllowed() return coe5.setting("map_editor_grid") == true end

-- Note 59: Two toolbox lines in one place: "Show grid" while the grid is off, "Hide grid" while it is
-- on. `visible` decides each frame whether a line shows. coe5.addMapEditorLine returns the id CoE5SE
-- gave the line (the key in Note 61 clicks it), or false when it couldn't add it.
local showGrid = coe5.addMapEditorLine{label = "Show grid", tooltip = "Square edges and map coordinates, x,y",
  visible = function() return gridAllowed() and not gridOn end, onClick = function() gridOn = true end}
local hideGrid = coe5.addMapEditorLine{label = "Hide grid", tooltip = "Square edges and map coordinates, x,y",
  visible = function() return gridAllowed() and gridOn end, onClick = function() gridOn = false end}

-- Note 66: The lines' opacity, which the player sets with "Grid lines +" and "Grid lines -" and which
-- stays from one editor and one launch to the next in the data file editor_grid.ini. It is kept in
-- tenths, a whole number, so ten clicks up and ten down land exactly where they started.
local LINE_ALPHA_FILE = "editor_grid"
local lineTenths = math.floor(GRID_ALPHA * 10 + 0.5)
local saved = coe5.readIni(LINE_ALPHA_FILE)
local savedTenths = saved and saved.grid and tonumber(saved.grid.line_alpha_tenths)
if savedTenths then lineTenths = savedTenths end
local function lineAlpha()
  return math.max(LINE_ALPHA_MIN, math.min(LINE_ALPHA_MAX, lineTenths * LINE_ALPHA_STEP))
end
local function changeLineAlpha(tenths)
  local wanted = math.floor(math.max(LINE_ALPHA_MIN, math.min(LINE_ALPHA_MAX, (lineTenths + tenths) * LINE_ALPHA_STEP)) * 10 + 0.5)
  if wanted == lineTenths then return end
  lineTenths = wanted
  coe5.writeIni(LINE_ALPHA_FILE, {grid = {line_alpha_tenths = lineTenths}})
end

-- Note 67: The two opacity lines, right under "Hide grid" (lines show in the order they were added),
-- only while the grid is on.
local linesUp = coe5.addMapEditorLine{label = "Grid lines +", tooltip = "Grid lines more opaque",
  visible = function() return gridAllowed() and gridOn end, onClick = function() changeLineAlpha(1) end}
local linesDown = coe5.addMapEditorLine{label = "Grid lines -", tooltip = "Grid lines less opaque",
  visible = function() return gridAllowed() and gridOn end, onClick = function() changeLineAlpha(-1) end}

-- Note 60: MAP_EDITOR runs the whole editor: next returns when the player quits it. The grid starts
-- off on every open and is off again after.
local editorOpen = coe5.wrap("MAP_EDITOR", function(next)
  gridOn, toolboxShown = false, true
  next()
  gridOn = false
end)

-- Note 61: Each frame of the editor. Before next, the grid key is bound to whichever of the two
-- lines shows, so pressing it clicks that line, toolbox hidden or not. After next, a Tab or Hide
-- Toolbox click flips the script's own note of the toolbox.
local editorKey = coe5.wrap("MAP_EDITOR_WAIT", function(next, block, mode)
  local key = coe5.setting("map_editor_grid_key")
  -- type(...) == "number" skips a line CoE5SE didn't add (false) and a missing setting (nil).
  local line = showGrid
  if gridOn then line = hideGrid end
  if gridAllowed() and type(line) == "number" and type(key) == "number" and key ~= coe5.key.none then
    coe5.ui.bindKey(line, key)
  end
  local id = next(block, mode)
  if id == coe5.mapEditor.hideToolbox then toolboxShown = not toolboxShown end
  return id
end)

-- Note 62: The label for one number pair, right-aligned so its last digit ends at `right`.
local function drawRight(text, right, y, size)
  coe5.ui.drawText(right - #text * size * CHAR_WIDTH, y, size, text, GRID_R, GRID_G, GRID_B, GRID_ALPHA)
end

-- Note 63: Draws the grid. coe5.mapView says where the squares are: square (x, y) is `zoom` UI units
-- wide, its top left corner at (originX + x * zoom, originY - y * zoom), and firstX to lastX, firstY
-- to lastY are the squares of the shown plane on the screen. UI y counts up from the bottom.
local function drawGrid()
  local v = coe5.mapView()
  if not v or v.firstX > v.lastX or v.firstY > v.lastY then return end
  local s = v.zoom
  local left, right = v.originX + v.firstX * s, v.originX + (v.lastX + 1) * s
  local top, bottom = v.originY - v.firstY * s, v.originY - (v.lastY + 1) * s
  local alpha = lineAlpha()
  for x = v.firstX, v.lastX + 1 do
    local ux = v.originX + x * s
    coe5.ui.drawLine(ux, bottom, ux, top, GRID_R, GRID_G, GRID_B, alpha, GRID_LINE_WIDTH)
  end
  for y = v.firstY, v.lastY + 1 do
    local uy = v.originY - y * s
    coe5.ui.drawLine(left, uy, right, uy, GRID_R, GRID_G, GRID_B, alpha, GRID_LINE_WIDTH)
  end
  if s >= LABEL_MIN_ZOOM then
    -- A label in each square's bottom right corner.
    local size = math.max(LABEL_SIZE_MIN, math.min(LABEL_SIZE_MAX, s * LABEL_SIZE_PER_ZOOM))
    for x = v.firstX, v.lastX do
      local squareRight = v.originX + (x + 1) * s - LABEL_PAD
      for y = v.firstY, v.lastY do
        drawRight((x - v.planeX) .. "," .. (y - v.planeY), squareRight, v.originY - (y + 1) * s + LABEL_PAD, size)
      end
    end
    return
  end
  -- Note 64: The edge headers: x along the top, y down the left, each over its column or row, just
  -- outside the map where the screen has room, else on the map's own edge squares. A number goes on
  -- every `step`th column or row, so they never overlap.
  local digits = #tostring(math.max(v.planeW, v.planeH))
  local step = math.ceil((digits * HEADER_SIZE * CHAR_WIDTH + LABEL_PAD) / s)
  local headerY = math.min(top + LABEL_PAD, v.screenH - PLANE_BAR - HEADER_SIZE)
  for x = v.firstX, v.lastX do
    if (x - v.planeX) % step == 0 then
      drawRight(tostring(x - v.planeX), v.originX + (x + 1) * s - 1, headerY, HEADER_SIZE)
    end
  end
  local rowStep = math.ceil((HEADER_SIZE + 1) / s)
  for y = v.firstY, v.lastY do
    if (y - v.planeY) % rowStep == 0 then
      local rowBottom = v.originY - (y + 1) * s
      local text = tostring(y - v.planeY)
      local edge = math.max(left - LABEL_PAD, #text * HEADER_SIZE * CHAR_WIDTH)
      if toolboxShown and rowBottom + s > TOOLBOX_BOTTOM then edge = math.max(edge, TOOLBOX_RIGHT + #text * HEADER_SIZE * CHAR_WIDTH) end
      drawRight(text, edge, rowBottom + (s - HEADER_SIZE) / 2, HEADER_SIZE)
    end
  end
end

-- Note 65: MAP_EDITOR_DRAW runs once a frame while the editor shows its map, after the map and before
-- the toolbox: what is drawn before next lies over the map and under the toolbox.
local editorDraw = coe5.wrap("MAP_EDITOR_DRAW", function(next)
  if gridOn and gridAllowed() then drawGrid() end
  return next()
end)

-- Note 47: `ok and 1 or 0` is a common Lua way to write "1 if ok, else 0". `and` gives its right side
-- when the left side counts as yes, and `or` gives its right side when what came before counts as
-- no. In Lua only nil and false count as no. The log line below needs whole numbers, not true and
-- false, so each wrap's result goes through flag.
local function flag(ok) return ok and 1 or 0 end

-- Note 48: A wrap CoE5SE couldn't add returned false and was logged (api_refused or lua_refused).
-- <del> draws only with all three. With true and false values, `a and b and c` is true only when all
-- three are true. This line runs once, while the script loads, after all three wraps above are in.
-- The wraps' functions run later, each time the game makes its call, and read deleteReady as it is
-- then (Note 11), so they see this value.
deleteReady = browser and rowDate and wait
-- Note 49: coe5.log writes a line to coe5mod.log, which shows what the script did, while CoE5SE's
-- "Enable copious logs" setting is on (off by default); otherwise it writes nothing. string.format
-- fills each %d with a whole number, in order. ritual_needs counts the ritual readout's row wrap,
-- its two observers, and the hover box's line wrap (Notes 31, 45, and 50). editor_grid counts the
-- grid's four toolbox lines and three wraps (Notes 59 to 67).
coe5.log(string.format("ui_tweaks_installed save_delete=%d of 3 banners=%d read_tip=%d keep_banners=%d ritual_needs=%d of 4 editor_grid=%d of 7",
  flag(browser) + flag(rowDate) + flag(wait), flag(banners), flag(readTip), flag(keep),
  flag(ritualRow) + flag(locationObserved) + flag(castObserved) + flag(lineColour),
  flag(showGrid) + flag(hideGrid) + flag(linesUp) + flag(linesDown) + flag(editorOpen) + flag(editorKey) + flag(editorDraw)))
