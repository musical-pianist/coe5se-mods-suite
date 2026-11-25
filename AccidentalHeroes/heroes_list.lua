-- Heroes list in the Game Menu

local Text = require("heroes_text")

local M = {}

local MENU_SECTION = "Accidental Heroes"
local BACK, PREVIOUS_PAGE, NEXT_PAGE, SWITCH_VIEW = 9, 10, 11, 12
local SEARCH, CLEAR_SEARCH = 23, 24
local FIRST_HERO_ROW = 1000
local HEROES_PER_PAGE = 8
local ROW_WIDTH = 300
local HERO_ROW_HEIGHT, LINE_HEIGHT = 30, 16.45
local SEARCH_TEXT_MOST = 32
local SEARCH_HOVER = "Type a name or a trait. Esc clears the search, then goes back"

local shownLast = {viewer = nil, fallen = false, page = 0}

-- Search
local function newSearch()
  return {text = "", focused = false, changed = false, live = coe5.ui.setTextFocus ~= nil and coe5.ui.takeTextEvents ~= nil}
end

local function readSearch(search)
  if not search.live then return false end
  local typed = coe5.ui.takeTextEvents()
  local back = false
  for _, e in ipairs(typed and typed.events or {}) do
    local before = search.text
    if e.type == "text" then
      local longer = search.text .. e.text
      if (utf8.len(longer) or SEARCH_TEXT_MOST + 1) <= SEARCH_TEXT_MOST then search.text = longer end
    elseif e.type == "backspace" then
      search.text = search.text:gsub("[%z\1-\127\194-\244][\128-\191]*$", "")
    elseif e.type == "clear" then
      search.text = ""
    elseif e.type == "cancel" then
      if search.text == "" then back = true else search.text = "" end
    end
    if search.text ~= before then search.changed = true end
  end
  search.focused = typed ~= nil and typed.focused
  if not search.focused and not back then
    search.focused = coe5.ui.setTextFocus(true) == true
    search.live = search.focused
  end
  return back
end

local function endSearch(search)
  if search.live then coe5.ui.setTextFocus(false) end
end

local function searchWords(search)
  local text = search.text:match("^%s*(.-)%s*$")
  return text ~= "" and text:lower() or nil
end

local function drawSearch(search)
  coe5.ui.sectionBegin("Search")
  if search.live then
    coe5.ui.text(0, "Search: " .. search.text .. "_")
    coe5.ui.tooltip(SEARCH_HOVER)
  else
    local words = searchWords(search)
    coe5.ui.text(SEARCH, words and "Search: " .. search.text or "Search")
    coe5.ui.tooltip("Find heroes by name or trait")
    if words then coe5.ui.text(CLEAR_SEARCH, "Clear search") end
  end
  coe5.ui.sectionEnd()
end

-- Which heroes show
local function heroMatches(hero, words)
  if not words then return true end
  for _, text in ipairs({hero.title or "", hero.traitLabel or "", hero.legendTraitLabel or ""}) do
    if text:lower():find(words, 1, true) then return true end
  end
  return false
end

local function heroesShownTo(viewer, showEveryPlayers, fallen, words)
  local entries, inView, inOtherView = {}, 0, 0
  for heroNumber, hero in ipairs(coe5.gameData.heroes or {}) do
    local isViewers = hero.owner == viewer
    if isViewers or showEveryPlayers then
      if (hero.deathTurn ~= nil) ~= fallen then
        inOtherView = inOtherView + 1
      else
        inView = inView + 1
        if heroMatches(hero, words) then
          entries[#entries + 1] = {heroNumber = heroNumber, hero = hero, isViewers = isViewers}
        end
      end
    end
  end
  table.sort(entries, function(a, b)
    if a.isViewers ~= b.isViewers then return a.isViewers end
    if fallen and a.hero.deathTurn ~= b.hero.deathTurn then return a.hero.deathTurn > b.hero.deathTurn end
    return a.heroNumber > b.heroNumber
  end)
  return entries, inView, inOtherView
end

-- Drawing
local function squareOfLivingHero(entry, heroAffliction)
  local unit = entry.hero.unit
  local unitIsStillThisHero = coe5.afflictionValue(unit, heroAffliction) == entry.heroNumber
  if entry.hero.deathTurn or not unitIsStillThisHero then return nil end
  return coe5.get("unitX", unit), coe5.get("unitY", unit)
end

local function drawHeroRow(rowId, entry, monsterNameOf)
  local hero = entry.hero
  local monster = hero.currentMonster or hero.monster
  local monsterName = monsterNameOf(monster)
  local ownerName = not entry.isViewers and (coe5.get("playerName", hero.owner) or "") or nil
  local canGoTo = entry.isViewers and not hero.deathTurn
  coe5.ui.monsterRow(canGoTo and rowId or 0, monster, Text.listTitle(hero, ownerName), Text.listLine(hero, monsterName))
  if canGoTo then coe5.ui.tooltip("Go to " .. hero.short .. " the " .. monsterName) end
end

local function drawEmptyLines(rowsMissing, text)
  local lines = math.floor(rowsMissing * HERO_ROW_HEIGHT / LINE_HEIGHT + 0.5)
  for line = 1, lines do
    if line % 2 == 1 then coe5.ui.rowBegin(0, ROW_WIDTH, 0.85, 0.55, 0.35, 0.25) else coe5.ui.rowBegin(0, ROW_WIDTH) end
    coe5.ui.text(0, line == 1 and text or "")
    coe5.ui.rowEnd()
  end
end

local function emptyListText(fallen, words)
  if words then return "No heroes match" end
  return fallen and "No fallen heroes" or "No heroes yet"
end

local function drawList(fallen, words, entries, inView, page, monsterNameOf)
  local title = fallen and "Fallen heroes" or "Heroes"
  local count = #entries < inView and string.format("%d of %d", #entries, inView) or tostring(inView)
  coe5.ui.sectionBegin(string.format("%s (%s)", title, count))
  local first = page * HEROES_PER_PAGE
  local shown = math.max(0, math.min(HEROES_PER_PAGE, #entries - first))
  for k = 1, shown do drawHeroRow(FIRST_HERO_ROW + first + k, entries[first + k], monsterNameOf) end
  if shown < HEROES_PER_PAGE then
    drawEmptyLines(HEROES_PER_PAGE - shown, shown == 0 and emptyListText(fallen, words) or nil)
  end
  coe5.ui.sectionEnd()
end

local function centerMapOnHero(entry, heroAffliction)
  local x, y = squareOfLivingHero(entry, heroAffliction)
  return x ~= nil and y ~= nil and x >= 0 and coe5.centerMap(x, y)
end

local function searchBoxMonster(viewer)
  local class = coe5.get("playerClass", viewer)
  return coe5.findMonster(coe5.get("classLeader", class) or "")
end

local function openList(env)
  local viewer = coe5.get("activePlayer")
  if shownLast.viewer ~= viewer then shownLast = {viewer = viewer, fallen = false, page = 0} end
  local view = shownLast
  local search = newSearch()
  local words, entries, inView, inOtherView, pageCount
  local function applyView()
    words = searchWords(search)
    entries, inView, inOtherView = heroesShownTo(viewer, env.showAll(), view.fallen, words)
    pageCount = math.max(1, (#entries + HEROES_PER_PAGE - 1) // HEROES_PER_PAGE)
    if view.page >= pageCount then view.page = 0 end
  end
  local function logSearch()
    coe5.log(string.format("heroes_search text=%s fallen=%s matches=%d", words or "", tostring(view.fallen), #entries))
  end
  applyView()
  while true do
    coe5.ui.screenBegin("Heroes")
    if readSearch(search) then endSearch(search) return false end
    if search.changed then
      search.changed, search.logged, view.page = false, false, 0
      applyView()
    elseif search.logged == false then
      search.logged = true
      logSearch()
    end
    drawSearch(search)
    drawList(view.fallen, words, entries, inView, view.page, env.monsterName)
    coe5.ui.buttonRow()
    coe5.ui.text(0, string.format("Page %d of %d", view.page + 1, pageCount))
    coe5.ui.text(PREVIOUS_PAGE, "Previous page")
    coe5.ui.text(NEXT_PAGE, "Next page")
    coe5.ui.text(SWITCH_VIEW, string.format("%s (%d)", view.fallen and "Living heroes" or "Fallen heroes", inOtherView))
    coe5.ui.text(BACK, "Back")
    coe5.ui.bindEscape(BACK)
    local clicked = coe5.ui.wait()
    if clicked == BACK then endSearch(search) return false end
    if clicked == PREVIOUS_PAGE then view.page = (view.page - 1) % pageCount end
    if clicked == NEXT_PAGE then view.page = (view.page + 1) % pageCount end
    if clicked == SWITCH_VIEW then
      view.fallen, view.page = not view.fallen, 0
      applyView()
    end
    if clicked == SEARCH and not search.live then
      local monster = searchBoxMonster(viewer)
      local text = monster and coe5.ui.nameBox("Search heroes", "Name or trait:", coe5.get("monsterSprite", monster), SEARCH_TEXT_MOST)
      if text then
        search.text, view.page = text, 0
        applyView()
        logSearch()
      end
    end
    if clicked == CLEAR_SEARCH then
      search.text, view.page = "", 0
      applyView()
    end
    local entry = entries[clicked - FIRST_HERO_ROW]
    if clicked > FIRST_HERO_ROW and entry and entry.isViewers and centerMapOnHero(entry, env.affliction) then
      endSearch(search)
      return true
    end
  end
end

function M.install(env)
  coe5.addGameMenuLine{
    label = "Heroes",
    tooltip = "Every hero risen from the ranks",
    section = MENU_SECTION,
    visible = function()
      local someonePlaysHere = coe5.get("activePlayer") >= 0 and coe5.netRole() ~= coe5.net.host
      return someonePlaysHere
    end,
    onClick = function()
      if openList(env) then return coe5.gameMenu.close end
      return coe5.gameMenu.stay
    end,
  }
end

return M
