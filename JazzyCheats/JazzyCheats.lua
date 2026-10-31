-- Jazzy Cheats: three lines in the Game Menu's own "Cheats" section, for the player whose turn
-- it is. "Resources" opens a screen that gives 10 or 100 of one resource per click. "Units" lists
-- the units of the player's class with their pictures; a click on one closes the menu, and the next
-- left click on a map square places 1 or 10 of it there. "Reveal Map" puts every plane in view
-- until the player's next turn. Two lines under the selected commander's orders on the map, each
-- turned off by its own setting: "Restore movement (cheat)" gives every unit on the next
-- left-clicked square its action points back, and "Teleport (cheat)" moves the commander and his
-- army to the next left-clicked square, as a teleport ritual does, for no action points. While
-- either waits for a square its line reads what it waits for, in PICKING_COLOR.

local CHEATS_SECTION = "Cheats"   -- the Game Menu section CoE5SE draws for these lines
local ID_BACK = 9
local ID_AMOUNT_FIRST = 10     -- AMOUNTS[i] is line ID_AMOUNT_FIRST + i - 1
local ID_RESOURCE_FIRST = 100  -- resource r is line ID_RESOURCE_FIRST + r
local RESOURCE_LAST = 13       -- resourceName's last index (0 Gold .. 13 Lifeforce)
local AMOUNTS = {10, 100}

local amount = AMOUNTS[1]      -- the picked amount, kept until the game closes

local function openResources()
  local player = coe5.get("activePlayer")
  while true do
    coe5.ui.screenBegin("Resources")
    coe5.ui.sectionBegin("Amount")
    for i, a in ipairs(AMOUNTS) do coe5.ui.choice(ID_AMOUNT_FIRST + i - 1, a == amount, tostring(a)) end
    coe5.ui.sectionEnd()
    coe5.ui.sectionBegin("Give to " .. coe5.get("slotName", player))
    for r = 0, RESOURCE_LAST do
      local name = coe5.get("resourceName", r) or ("Resource " .. r)
      coe5.ui.text(ID_RESOURCE_FIRST + r, string.format("%s: %d", name, coe5.get("slotResource", player, r)))
      coe5.ui.tooltip(string.format("Add %d %s", amount, name))
    end
    coe5.ui.sectionEnd()
    coe5.ui.buttonRow()
    coe5.ui.text(ID_BACK, "Back")
    coe5.ui.bindEscape(ID_BACK)
    local id = coe5.ui.wait()
    if id == ID_BACK then return end
    if id >= ID_AMOUNT_FIRST and id < ID_AMOUNT_FIRST + #AMOUNTS then amount = AMOUNTS[id - ID_AMOUNT_FIRST + 1] end
    if id >= ID_RESOURCE_FIRST and id <= ID_RESOURCE_FIRST + RESOURCE_LAST then
      local r = id - ID_RESOURCE_FIRST
      local total = coe5.get("slotResource", player, r) + amount
      coe5.set("slotResource", player, r, total)
      coe5.log(string.format("resource_given slot=%d resource=%d amount=%d total=%d", player, r, amount, total))
    end
  end
end

coe5.addGameMenuLine{
  label = "Resources",
  tooltip = "Give the player whose turn it is 10 or 100 of any resource.",
  section = CHEATS_SECTION,
  visible = function() return coe5.get("activePlayer") >= 0 end,
  onClick = function()
    openResources()
    return coe5.gameMenu.stay
  end,
}

-- ---- Units --------------------------------------------------------------------------------------
local UNIT_AMOUNTS = {1, 10}
local ID_UNIT_AMOUNT_FIRST = 10   -- UNIT_AMOUNTS[i] is line ID_UNIT_AMOUNT_FIRST + i - 1
local ID_PREVIOUS_PAGE = 20
local ID_NEXT_PAGE = 21
local ID_PICK_AGAIN = 22          -- the refusal screen's "Pick another square"
local ID_SEARCH = 23
local ID_CLEAR_SEARCH = 24
local ID_OK = 25                  -- the "Movement restored" screen's Ok
local SEARCH_LENGTH = 32          -- the search box's length (coe5.ui.nameBox takes 1 to 42)
local ID_UNIT_FIRST = 1000        -- the list's unit i is row ID_UNIT_FIRST + i
local UNITS_PER_PAGE = 8          -- two-line rows a page: they fit the 600-high UI under the Amount section
local RECRUIT_OFFERS = 160        -- classRecruitName's offers, 0-159
local START_ARMY_LINES = 16       -- classStartArmy's lines, 0-15
local KNOWN_RITUALS = 64          -- commanderRitual's slots, 0-63
local RITUAL_STRINGS = 58         -- ritualString's results, 0-57
local COMMANDER_RECORDS = 9990    -- commanderUnit's records, 0-9989
local UNIT_RECORDS = 99000        -- unitOwner's records, 0-98999
local NAMES_SHOWN = 2             -- sources, or commanders, a row's second line names before "and n more"
-- The game values one frame of building the list may read; past it the build goes on in the next
-- frame, behind a "Reading rituals" screen, so a list of any size never runs into the 20,000,000
-- steps a call may run. tests/test_lua_host.c (testJazzyCheatsUnits) builds the biggest list
-- the tables hold: 100,000 a frame still fit, and the whole list in one call doesn't.
local READS_PER_FRAME = 20000
-- The game holds the standard troops' names in its code, not in a table (classStandardTroops).
local STANDARD_TROOPS = {"Spearman", "Swordsman", "Archer", "Crossbowman", "Heavy Infantry", "Catapult"}
local SECTION_CLASS, SECTION_RITUALS = 1, 2
-- Esc on the map while placing clicks this id; no line of the map uses it.
local MAP_ID_CANCEL = 990000
local BUTTON_LEFT, BUTTON_RIGHT = 1, 2   -- mouseButtons' bits

local unitAmount = UNIT_AMOUNTS[1]   -- the picked amount, kept until the game closes
local unitPage, unitPageClass = 0, nil
---@class PickedUnit
---@field player integer
---@field monster integer
---@field name string
---@field commander boolean
---@field amount integer
---@type PickedUnit|nil
local placing = nil          -- the picked unit until a map square takes it
---@type {player: integer}|nil
local restoring = nil        -- set by Restore movement until a map square is picked
---@type {player: integer, commander: integer, name: string}|nil
local teleporting = nil      -- set by the commander's Teleport line until a map square is picked
local stopPicking            -- stops every pick for a square, below
local lastButtons = 0        -- the map frame's mouseButtons a frame before, to see a press start
local swallowing = false     -- a click the mod used: its button reads 0 until let go

-- What each ritual summons, by ritual slot, read once a game: a list of {name, commander}, empty for
-- a ritual that doesn't summon. Rituals change only when the game reads the mods again, at a new
-- game or a load, and GAME_START follows both.
local ritualSummonsCache = {}
coe5.observe("GAME_START", function() ritualSummonsCache = {} end)

-- The monsters a summoning string names, as the game reads it (popstr): parts joined by "&", each
-- "c*Name" (a commander), a count or dice and "*Name" ("1d3+1*Hellhound"), a cap letter and
-- "*Name" ("u*Master Thief"), or a bare name. A leading "(167)" (a terrain) or "(!)" (a familiar)
-- says when the string is picked and isn't part of the name.
local function summonedMonsters(text)
  local found = {}
  for piece in (text .. "&"):gmatch("([^&]*)&") do
    local part = piece:match("^%s*(.-)%s*$")
    while part:find("^%(") and part:find("^%b()") do part = part:match("^%b()%s*(.*)$") end
    local prefix, name = part:match("^([^*]*)%*%s*(.-)$")
    if not name then prefix, name = "", part end
    if name ~= "" then found[#found + 1] = {name = name, commander = prefix == "c"} end
  end
  return found
end

-- "a, b and 3 more": the first NAMES_SHOWN of `names`, then how many more.
local function shortList(names, separator)
  local shown = {}
  for i = 1, math.min(#names, NAMES_SHOWN) do shown[i] = names[i] end
  local text = table.concat(shown, separator)
  if #names > NAMES_SHOWN then text = string.format("%s and %d more", text, #names - NAMES_SHOWN) end
  return text
end

-- The units the player may place, each once (a name, as a commander or not): first the class's
-- (its recruitment list, its start army, the standard troops when it has stdtroops; a recruit count
-- of -1 or a start army line without a number is a commander), then what the summoning rituals of
-- the player's living commanders summon. Each unit keeps where it comes from for its second line.
-- The class leader is in none of them. It runs as a coroutine and yields after each READS_PER_FRAME
-- game values, so a long build spreads over frames.
local function buildUnitList(player, class)
  local reads = 0
  local function read(...)
    reads = reads + 1
    if reads >= READS_PER_FRAME then
      reads = 0
      coroutine.yield()
    end
    return coe5.get(...)
  end
  local units, byKey = {}, {}
  local function add(name, commander, section, source)
    local key = name:lower() .. (commander and "|commander" or "")
    local unit = byKey[key]
    if unit == false then return end   -- a name no monster has, logged once
    if not unit then
      local monster = name ~= "" and coe5.findMonster(name)
      if not monster then
        coe5.log(string.format("unit_unknown class=%d name=%s source=%s", class, name, source))
        byKey[key] = false
        return
      end
      unit = {name = name, monster = monster, commander = commander, section = section, sources = {}, sourceSeen = {}}
      byKey[key] = unit
      units[#units + 1] = unit
    end
    if not unit.sourceSeen[source] then
      unit.sourceSeen[source] = true
      unit.sources[#unit.sources + 1] = source
    end
  end
  for i = 0, RECRUIT_OFFERS - 1 do
    local name = read("classRecruitName", class, i)
    if not name then break end
    add(name, read("classRecruitCount", class, i) == -1, SECTION_CLASS, "Recruit")
  end
  for i = 0, START_ARMY_LINES - 1 do
    local line = read("classStartArmy", class, i)
    if not line then break end
    local name = line:match("^%d+ (.+)$")
    if name then add(name, false, SECTION_CLASS, "Start army") else add(line, true, SECTION_CLASS, "Start army") end
  end
  if read("classStandardTroops", class) then
    for _, name in ipairs(STANDARD_TROOPS) do add(name, false, SECTION_CLASS, "Standard troops") end
  end

  -- The rituals the player's living commanders know, each with the kinds of commander knowing it.
  local knownBy, rituals = {}, {}
  local lastRitual, lastMonster = read("ritualLastIndex"), read("monsterLastIndex")
  local lastCommander = math.min(read("commanderLastIndex"), COMMANDER_RECORDS - 1)
  for c = 0, lastCommander do
    local unit = read("commanderUnit", c)
    if unit >= 0 and unit < UNIT_RECORDS and read("unitOwner", unit) == player and read("unitHp", unit) > 0 then
      local monster = read("unitMonster", unit)
      local kind = monster >= 0 and monster <= lastMonster and read("monsterName", monster) or "?"
      for i = 0, KNOWN_RITUALS - 1 do
        local ritual = read("commanderRitual", c, i)
        if not ritual then break end
        if ritual <= lastRitual then
          local by = knownBy[ritual]
          if not by then
            by = {kinds = {}, seen = {}}
            knownBy[ritual] = by
            rituals[#rituals + 1] = ritual
          end
          if not by.seen[kind] then
            by.seen[kind] = true
            by.kinds[#by.kinds + 1] = kind
          end
        end
      end
    end
  end
  table.sort(rituals)
  for _, ritual in ipairs(rituals) do
    local summoned = ritualSummonsCache[ritual]
    if not summoned then
      summoned = {}
      if read("ritualSummons", ritual) then
        for i = 0, RITUAL_STRINGS - 1 do
          local text = read("ritualString", ritual, i)
          if not text then break end
          for _, found in ipairs(summonedMonsters(text)) do summoned[#summoned + 1] = found end
        end
      end
      ritualSummonsCache[ritual] = summoned
    end
    if #summoned > 0 then
      local source = string.format("%s (%s)", read("ritualName", ritual), shortList(knownBy[ritual].kinds, ", "))
      for _, found in ipairs(summoned) do add(found.name, found.commander, SECTION_RITUALS, source) end
    end
  end
  coe5.log(string.format("unit_list_built slot=%d class=%d units=%d rituals=%d", player, class, #units, #rituals))
  return units
end

-- Builds the list, a frame at a time when it is long, with a "Reading rituals" screen meanwhile.
-- Returns nil when the player leaves that screen with Back.
local function unitList(player, class)
  local build = coroutine.create(buildUnitList)
  local ok, result = coroutine.resume(build, player, class)
  while ok and coroutine.status(build) ~= "dead" do
    coe5.ui.screenBegin("Units")
    coe5.ui.sectionBegin("Reading rituals")
    coe5.ui.text(0, "Reading commanders' rituals...")
    coe5.ui.sectionEnd()
    coe5.ui.buttonRow()
    coe5.ui.text(ID_BACK, "Back")
    coe5.ui.bindEscape(ID_BACK)
    if coe5.ui.wait() == ID_BACK then return nil end
    ok, result = coroutine.resume(build)
  end
  if not ok then error(result, 0) end
  return result
end

-- The units whose name holds `text`, ignoring case; every unit when there is no text. Plain text: no
-- character in it is a pattern.
local function matchingUnits(units, text)
  if not text then return units end
  local wanted = text:lower()
  local found = {}
  for _, unit in ipairs(units) do
    if unit.name:lower():find(wanted, 1, true) then found[#found + 1] = unit end
  end
  return found
end

-- The Units screen. Returns true when the player picked a unit to place, so the Game Menu closes.
local function openUnits()
  local player = coe5.get("activePlayer")
  local class = coe5.get("slotClass", player)
  local allUnits = unitList(player, class)
  if not allUnits then return false end
  local hasRitualUnits = #allUnits > 0 and allUnits[#allUnits].section == SECTION_RITUALS
  local headers = {
    [SECTION_CLASS] = string.format("Class units (%s)", coe5.get("className", class)),
    [SECTION_RITUALS] = "From commanders' rituals",
  }
  -- The search box shows the class leader's picture, or the first unit's.
  local leader = coe5.findMonster(coe5.get("classLeader", class) or "")
  local searchMonster = leader or (allUnits[1] and allUnits[1].monster)
  local filter = nil   -- the search text, kept until the screen closes
  local units, pages
  local function applyFilter()
    units = matchingUnits(allUnits, filter)
    pages = math.max(1, (#units + UNITS_PER_PAGE - 1) // UNITS_PER_PAGE)
  end
  applyFilter()
  if unitPageClass ~= class or unitPage >= pages then unitPage, unitPageClass = 0, class end
  while true do
    coe5.ui.screenBegin("Units")
    coe5.ui.sectionBegin("Amount")
    for i, a in ipairs(UNIT_AMOUNTS) do coe5.ui.choice(ID_UNIT_AMOUNT_FIRST + i - 1, a == unitAmount, tostring(a)) end
    coe5.ui.sectionEnd()
    coe5.ui.sectionBegin("Search")
    coe5.ui.text(ID_SEARCH, filter and "Search: " .. filter or "Search")
    coe5.ui.tooltip("Filter units by name or source")
    if filter then coe5.ui.text(ID_CLEAR_SEARCH, "Clear search") end
    coe5.ui.sectionEnd()
    if filter and #units == 0 then
      coe5.ui.sectionBegin("Units")
      coe5.ui.text(0, "No units match")
      coe5.ui.sectionEnd()
    end
    local first, section = unitPage * UNITS_PER_PAGE, nil
    for i = first + 1, math.min(#units, first + UNITS_PER_PAGE) do
      local unit = units[i]
      if unit.section ~= section then
        if section then coe5.ui.sectionEnd() end
        section = unit.section
        coe5.ui.sectionBegin(headers[section])
      end
      coe5.ui.monsterRow(ID_UNIT_FIRST + i, unit.monster, unit.commander and unit.name .. " (commander)" or unit.name,
        shortList(unit.sources, "; "))
      coe5.ui.tooltip(string.format("Place %d %s: click, then a seen square", unitAmount, unit.name))
    end
    if section then coe5.ui.sectionEnd() end
    if unitPage == pages - 1 and not hasRitualUnits and not filter then
      coe5.ui.sectionBegin(headers[SECTION_RITUALS])
      coe5.ui.text(0, "No commander with a summoning ritual")
      coe5.ui.sectionEnd()
    end
    coe5.ui.buttonRow()
    if pages > 1 then
      coe5.ui.text(0, string.format("Page %d of %d", unitPage + 1, pages))
      coe5.ui.text(ID_PREVIOUS_PAGE, "Previous page")
      coe5.ui.text(ID_NEXT_PAGE, "Next page")
    end
    coe5.ui.text(ID_BACK, "Back")
    coe5.ui.bindEscape(ID_BACK)
    local id = coe5.ui.wait()
    if id == ID_BACK then return false end
    if id >= ID_UNIT_AMOUNT_FIRST and id < ID_UNIT_AMOUNT_FIRST + #UNIT_AMOUNTS then
      unitAmount = UNIT_AMOUNTS[id - ID_UNIT_AMOUNT_FIRST + 1]
    end
    if id == ID_PREVIOUS_PAGE then unitPage = (unitPage - 1) % pages end
    if id == ID_NEXT_PAGE then unitPage = (unitPage + 1) % pages end
    if id == ID_SEARCH and searchMonster then
      -- The game's own text box; Cancel or Esc (nil) keeps the search there was, empty text clears
      -- it. The prompt sits between the picture and the field, so it stays short to fit the box.
      local text = coe5.ui.nameBox("Search units", "Name:",
        coe5.get("monsterSprite", searchMonster), SEARCH_LENGTH)
      if text then
        text = text:match("^%s*(.-)%s*$")
        filter = text ~= "" and text or nil
        applyFilter()
        unitPage = 0
        coe5.log(string.format("unit_search text=%s matches=%d", filter or "", #units))
      end
    end
    if id == ID_CLEAR_SEARCH then
      filter = nil
      applyFilter()
      unitPage = 0
    end
    local unit = units[id - ID_UNIT_FIRST]
    if id > ID_UNIT_FIRST and unit then
      placing = {player = player, monster = unit.monster, name = unit.name, commander = unit.commander, amount = unitAmount}
      lastButtons = BUTTON_LEFT   -- the click that picked it may still be held when the map is back
      coe5.log(string.format("unit_picked slot=%d monster=%d amount=%d", player, unit.monster, unitAmount))
      return true
    end
  end
end

coe5.addGameMenuLine{
  label = "Units",
  tooltip = "Place 1 or 10 of a class unit or ritual summon on a seen square",
  section = CHEATS_SECTION,
  visible = function() return coe5.get("activePlayer") >= 0 end,
  onClick = function()
    if openUnits() then return coe5.gameMenu.close end
    return coe5.gameMenu.stay
  end,
}

local function stopPlacing(reason)
  coe5.log(string.format("unit_placing_stopped slot=%d monster=%d reason=%s", placing.player, placing.monster, reason))
  placing = nil
end

-- Why the player can't place a unit on (x, y), or nil: a square they haven't seen, or one holding
-- another player's army, unless that player is on their team. The game's own spawns check none of
-- this; the mod does, so the cheat stays within what the player could reach.
local function sameTeam(a, b)
  if a < 0 or a > 31 then return false end
  local team = coe5.get("slotTeam", a)
  return team >= 0 and team == coe5.get("slotTeam", b)
end
local function placeProblem(player, x, y)
  if coe5.get("squareSeen", player, x, y) <= 1 then return "not_seen", "Square not seen" end
  local army = coe5.get("squareArmy", x, y)
  if army == -1 then return "armies_at_war", "Two players' armies are on that square." end
  local owner = coe5.get("squareArmyOwner", x, y)
  if army ~= 0 and owner ~= player and not sameTeam(owner, player) then
    return "other_army", "Another player's army is on that square."
  end
end

-- The screen that says why a square was refused. Returns true to pick another square.
local function refusedScreen(title, header, text)
  while true do
    coe5.ui.screenBegin(title)
    coe5.ui.sectionBegin(header)
    coe5.ui.text(0, text)
    coe5.ui.sectionEnd()
    coe5.ui.buttonRow()
    coe5.ui.text(ID_PICK_AGAIN, "Pick another square")
    coe5.ui.text(ID_BACK, "Cancel")
    coe5.ui.bindKey(ID_PICK_AGAIN, coe5.key.enter)
    coe5.ui.bindEscape(ID_BACK)
    local id = coe5.ui.wait()
    if id == ID_PICK_AGAIN then return true end
    if id == ID_BACK then return false end
  end
end

-- Places unitAmount of the picked unit on (x, y), each a variant as the recruit screen picks it.
local function place(x, y)
  local p = placing
  local reason, text = placeProblem(p.player, x, y)
  local made = 0
  if not reason then
    for _ = 1, p.amount do
      local variant = coe5.monsterVariant(p.monster)
      local spawned = variant and (p.commander and coe5.spawnCommander(p.player, variant, x, y) or coe5.spawnUnit(p.player, variant, x, y))
      if not spawned then break end
      made = made + 1
    end
    if made == 0 then reason, text = "not_made", "The game has no room for more units." end
  end
  if reason then
    coe5.log(string.format("unit_refused slot=%d monster=%d x=%d y=%d reason=%s", p.player, p.monster, x, y, reason))
    if not refusedScreen("Units", string.format("Can't place %s there", p.name), text) then stopPlacing("cancelled") end
    lastButtons = BUTTON_LEFT   -- the screen's click may still be held when the map is back
    return
  end
  coe5.log(string.format("unit_spawned slot=%d monster=%d x=%d y=%d count=%d", p.player, p.monster, x, y, made))
  placing = nil
end

-- ---- The commander's cheat lines ---------------------------------------------------------------
-- Restore movement and Teleport are lines under the selected commander's orders on the map, each
-- while its setting is on (its default). While one waits for a square, its line is replaced by one
-- in PICKING_COLOR saying so; a click on that line stops the wait. A CoE5SE older than
-- coe5.addCommanderLine has no such lines.
local PICKING_COLOR = {1, 0.85, 0.3}   -- a waiting line's label: gold, beside the game's white
local RESTORE_ICON = 0x85    -- the game's Continue Moving icon in the commander panel
local TELEPORT_ICON = 0x7a   -- the game's Enter Gateway icon in the commander panel

-- The player whose commander is selected, or nil: the player whose turn it is, if the commander is his and alive.
local function ownCommanderPlayer(commander)
  local player = coe5.get("activePlayer")
  if commander < 0 or player < 0 then return nil end
  local unit = coe5.get("commanderUnit", commander)
  if unit < 0 or coe5.get("unitOwner", unit) ~= player or coe5.get("unitHp", unit) <= 0 then return nil end
  return player
end

-- ---- Restore movement ---------------------------------------------------------------------------
local function stopRestoring(reason)
  coe5.log(string.format("movement_pick_stopped slot=%d reason=%s", restoring.player, reason))
  restoring = nil
end

-- Whether the Restore movement lines may show: the setting on and the player's own commander selected.
local function restoreLinesShown()
  return coe5.setting("restore_movement") == true and ownCommanderPlayer(coe5.get("selectedCommander")) ~= nil
end

if coe5.addCommanderLine then
  coe5.addCommanderLine{
    label = "Restore movement (cheat)",
    tooltip = "Action points back for every unit on a clicked square",
    icon = RESTORE_ICON,
    visible = function() return restoring == nil and restoreLinesShown() end,
    onClick = function()
      local player = ownCommanderPlayer(coe5.get("selectedCommander"))
      if not player then return end
      stopPicking("other_pick")
      restoring = {player = player}
      lastButtons = BUTTON_LEFT   -- the click on this line may still be held
      coe5.log(string.format("movement_pick_started slot=%d", restoring.player))
    end,
  }
  coe5.addCommanderLine{
    label = "Restoring: pick a square",
    tooltip = "Left click a square to restore its units' movement. Right click cancels.",
    icon = RESTORE_ICON,
    color = PICKING_COLOR,
    visible = function() return restoring ~= nil and restoreLinesShown() end,
    onClick = function() if restoring then stopRestoring("line") end end,
  }
end

-- The screen after a restore: "Movement restored" and how many units got their action points back.
local function restoredScreen(count)
  while true do
    coe5.ui.screenBegin("Restore movement")
    coe5.ui.sectionBegin("Movement restored")
    coe5.ui.text(0, string.format("Units: %d", count))
    coe5.ui.sectionEnd()
    coe5.ui.buttonRow()
    coe5.ui.text(ID_OK, "Ok")
    coe5.ui.bindKey(ID_OK, coe5.key.enter)
    coe5.ui.bindEscape(ID_OK)
    if coe5.ui.wait() == ID_OK then return end
  end
end

-- Gives every living unit on (x, y), whoever owns it, its action points back (unitApUsed 0), then
-- shows how many. A square with no unit shows why, with "Pick another square".
local function restore(x, y)
  local player = restoring.player
  local count = 0
  for u = 0, math.min(coe5.get("unitLastIndex"), UNIT_RECORDS - 1) do
    if coe5.get("unitHp", u) > 0 and coe5.get("unitMonster", u) >= 0 and coe5.get("unitX", u) == x and coe5.get("unitY", u) == y then
      coe5.set("unitApUsed", u, 0)
      count = count + 1
    end
  end
  lastButtons = BUTTON_LEFT   -- the screen's click may still be held when the map is back
  if count == 0 then
    coe5.log(string.format("movement_refused slot=%d x=%d y=%d reason=no_units", player, x, y))
    if not refusedScreen("Restore movement", "Can't restore movement there", "No units on that square") then
      stopRestoring("cancelled")
    end
    return
  end
  coe5.log(string.format("movement_restored slot=%d x=%d y=%d units=%d", player, x, y, count))
  restoring = nil
  restoredScreen(count)
end

-- ---- Teleport ---------------------------------------------------------------------------------
-- A line under the selected commander's orders on the map, while the "teleport" setting is on (its
-- default): a click on it, then a left click on a map square, moves the commander and every unit
-- following him on his square there, on any plane, through coe5.teleportCommander, the teleport
-- ritual's own call. A free square costs no action points. On a square where the army meets another
-- at war the game spends all of them, as walking in does, and the battle is fought after the move.
-- The mod refuses a square the player hasn't seen, terrain the army couldn't walk onto (the game's
-- teleport checks none, so sea would drown it), and a commander already in a battle, so the cheat
-- never takes an army out of a fight.
local function stopTeleporting(reason)
  coe5.log(string.format("teleport_pick_stopped slot=%d commander=%d reason=%s", teleporting.player, teleporting.commander, reason))
  teleporting = nil
end

-- Whether the selected commander waits for a square to teleport to.
local function selectedTeleporting()
  return teleporting ~= nil and teleporting.commander == coe5.get("selectedCommander")
end

if coe5.addCommanderLine then
  coe5.addCommanderLine{
    label = "Teleport (cheat)",
    tooltip = "Teleport the commander and his troops to a seen square on any plane, for no action points. Landing on enemies starts a battle.",
    icon = TELEPORT_ICON,
    visible = function()
      return coe5.setting("teleport") == true and not selectedTeleporting() and ownCommanderPlayer(coe5.get("selectedCommander")) ~= nil
    end,
    onClick = function()
      local commander = coe5.get("selectedCommander")
      local player = ownCommanderPlayer(commander)
      if not player then return end
      stopPicking("other_pick")
      teleporting = {player = player, commander = commander, name = coe5.get("commanderName", commander)}
      lastButtons = BUTTON_LEFT   -- the click on this line may still be held
      coe5.log(string.format("teleport_pick_started slot=%d commander=%d", teleporting.player, commander))
    end,
  }
  coe5.addCommanderLine{
    label = "Teleporting: pick a square",
    tooltip = "Left click a seen square to teleport there. Right click cancels.",
    icon = TELEPORT_ICON,
    color = PICKING_COLOR,
    visible = function()
      return coe5.setting("teleport") == true and selectedTeleporting() and ownCommanderPlayer(coe5.get("selectedCommander")) ~= nil
    end,
    onClick = function() if teleporting then stopTeleporting("line") end end,
  }
end

-- Why the picked commander can't teleport to (x, y), or nil.
local function teleportProblem(t, x, y)
  if coe5.get("squareSeen", t.player, x, y) <= 1 then return "not_seen", "Square not seen" end
  local unit = coe5.get("commanderUnit", t.commander)
  if coe5.get("squareArmy", coe5.get("unitX", unit), coe5.get("unitY", unit)) == -1 then
    return "in_battle", "Commander is in a battle"
  end
  if not coe5.mayEnterSquare(t.commander, x, y) then return "no_entry", "The army can't enter that terrain" end
end

-- Teleports the picked commander's army to (x, y), or says why not, with "Pick another square".
local function teleport(x, y)
  local t = teleporting
  local reason, text = teleportProblem(t, x, y)
  local moved = not reason and coe5.teleportCommander(t.commander, x, y)
  if not reason and not moved then reason, text = "refused", "Can't teleport there" end   -- logged by CoE5SE as lua_refused
  lastButtons = BUTTON_LEFT   -- a screen's click may still be held when the map is back
  if reason then
    coe5.log(string.format("teleport_refused slot=%d commander=%d x=%d y=%d reason=%s", t.player, t.commander, x, y, reason))
    if not refusedScreen("Teleport", string.format("Can't teleport %s there", t.name), text) then stopTeleporting("cancelled") end
    return
  end
  coe5.log(string.format("teleported slot=%d commander=%d x=%d y=%d", t.player, t.commander, x, y))
  teleporting = nil
end

-- Stops picking a square for Units, Restore movement or Teleport.
function stopPicking(reason)
  if placing then stopPlacing(reason) end
  if restoring then stopRestoring(reason) end
  if teleporting then stopTeleporting(reason) end
end

-- Each map frame while a unit is picked or Restore movement or Teleport waits for a square: the first
-- frame of a left click on a square places the unit, restores movement or teleports there; Esc, a right click, the Game
-- Menu, or another player's turn stops. A click the mod uses never reaches the map (it would move
-- the selected commander): mouseButtons reads 0 until the button is let go.
coe5.wrap("MAP_SCREEN_WAIT", function(next, block, mode)
  local picking = placing or restoring or teleporting
  if not picking and not swallowing then return next(block, mode) end
  if picking then coe5.ui.bindKey(MAP_ID_CANCEL, coe5.key.esc) end
  local id = next(block, mode)
  local buttons = coe5.get("mouseButtons")
  local pressed = buttons ~= 0 and lastButtons == 0
  lastButtons = buttons
  if swallowing then
    if buttons ~= 0 then coe5.set("mouseButtons", 0) else swallowing = false end
  end
  if not picking then return id end
  if coe5.get("activePlayer") ~= picking.player then
    stopPicking("turn_changed")
    return id
  end
  if id == MAP_ID_CANCEL then
    stopPicking("esc")
    return 0
  end
  if id == coe5.mapAction.gameMenu then
    stopPicking("game_menu")
    return id
  end
  if not pressed then return id end
  if buttons & BUTTON_RIGHT ~= 0 then
    stopPicking("right_click")
    coe5.set("mouseButtons", 0)
    swallowing = true
    return 0
  end
  local x, y = coe5.get("mapPointerX"), coe5.get("mapPointerY")
  if buttons ~= BUTTON_LEFT or id ~= 0 or x < 0 or y < 0 then return id end   -- the map's own lines keep their clicks
  coe5.set("mouseButtons", 0)
  swallowing = true
  if placing then place(x, y) elseif restoring then restore(x, y) else teleport(x, y) end
  return 0
end)

-- While a unit is picked or Restore movement or Teleport waits, the map's menu button's hover text, "(F10)
-- Menu", says what a click on the map does. A line drawn in MAP_SCREEN_WAIT's wrap isn't shown: the
-- map lays its lines out before that wait.
coe5.wrap("MAP_MENU_BUTTON_TEXT", function(next, text)
  if placing then return next(string.format("Placing %s x%d (right click cancels)", placing.name, placing.amount)) end
  if restoring then return next("Restoring movement (right click cancels)") end
  if teleporting then return next(string.format("Teleporting %s (right click cancels)", teleporting.name)) end
  return next(text)
end)

-- ---- Reveal Map ---------------------------------------------------------------------------------
-- The Game Over screen's Reveal Map, for the player whose turn it is: every square of every plane,
-- the mods' planes included, in view until that player's next turn, when the game darkens them as
-- it darkens any square out of sight. The menu closes so the map shows them. A CoE5SE older than
-- coe5.revealMap leaves the line out.
coe5.addGameMenuLine{
  label = "Reveal Map",
  tooltip = "Every plane in view until the next turn",
  section = CHEATS_SECTION,
  visible = function() return coe5.revealMap ~= nil and coe5.get("activePlayer") >= 0 end,
  onClick = function()
    local player = coe5.get("activePlayer")
    if not coe5.revealMap(player) then return coe5.gameMenu.stay end   -- logged by CoE5SE as lua_refused
    coe5.log(string.format("map_revealed slot=%d turn=%d", player, coe5.turn()))
    return coe5.gameMenu.close
  end,
}
