-- Jazzy AI Observer: what each AI player did on its last turn and why.

local SECTION = "AI Inspector"
local OUTER, INNER = 2000000000, -2000000000   -- outside every other script's wrap; inside every one
local STEPS = {"rally", "friendly", "conquer", "explore", "far"}
local STEP_SITE = {rally = "AI_RALLY", friendly = "AI_FRIENDLY_MOVE", conquer = "AI_CONQUER", explore = "AI_EXPLORE",
                   far = "AI_FAR_EXPLORE"}
local STEP_NAME = {rally = "Rally", friendly = "Friendly move", conquer = "Conquer", explore = "Explore", far = "Far explore"}
local STEP_ORDER = {rally = 1, friendly = 2, conquer = 3, explore = 4, far = 5}
local EXPLORE_BELOW, FAR_EXPLORE_BELOW = 50, 10   -- coe5.aiMove.exploreBelow, farExploreBelow
local NO_SQUARE = -1
local HISTORY_TURNS = 5

-- Records: the record logic; Ins: the inspector's own helpers.
local Records = {}
local Ins = {}   -- the inspector's own helpers (the main chunk holds at most 200 locals)

-- ---- Text --------------------------------------------------------------------------------------
local function clean(text)   -- a value for a record: no line breaks, |, or outer spaces
  return (tostring(text or ""):gsub("[\r\n|]", " "):match("^%s*(.-)%s*$"))
end

local function turnLabel(turn) return string.format("Turn %d", turn) end
-- Who played or changed something, as a screen says it.
function Records.whoText(text)
  return (tostring(text or ""):gsub("by you%f[%A]", "by hand"):gsub("%f[%a]you%f[%A]", "human"))
end
-- The move flags the AI's loop skips, in words.
function Records.skipText(skip)
  local out, s = {}, " " .. tostring(skip or "") .. " "
  for _, w in ipairs({{" stupid ", "Stupid"}, {" loner ", "loner"}, {" hero for hire ", "adventurer"}, {" stationary ", "Stationary"}}) do
    if s:find(w[1], 1, true) then out[#out + 1] = w[2] end
  end
  return table.concat(out, ", ")
end
-- An answer {score, x, y} in words: "29 at 4,5", "0, no square".
function Records.answerWords(a)
  if not a then return "none" end
  if (a[2] or -1) < 0 then return string.format("%d, no square", a[1] or 0) end
  return string.format("%d at %d,%d", a[1] or 0, a[2], a[3])
end

-- What a player sees of a square: squareSeen 0 never explored, 1 not seen now, 2 seen, 3 scried.
local SEEN_WORD = {[0] = "never explored", [1] = "not seen now", [2] = "seen", [3] = "scried"}
-- A record's word as a screen says it, for `who` ("John", or "its"/"it" for the commander's own player).
local function seenPhrase(word, who, whoIt)
  if word == "seen" then return "in " .. who .. " sight" end
  if word == "not seen now" then return "out of " .. who .. " sight (seen before)" end
  if word == "never explored" then return "never seen by " .. whoIt end
  if word == "scried" then return "scried by " .. whoIt end
  return word or ""
end

-- "32,17 Stargazer's Keep (Elysium)": the square, its name or terrain, and its plane.
local function placeOf(x, y)
  if not x or x < 0 or not y or y < 0 then return "no square" end
  local p = coe5.get("squarePlane", x, y)
  local plane = p and p >= 0 and coe5.get("planeName", p) or "no plane"
  local name = coe5.get("squareName", x, y)
  if not name or name == "" then
    local t = coe5.get("squareTerrain", x, y)
    name = t and coe5.get("terrainName", t) or "?"
  end
  return string.format("%d,%d %s (%s)", x, y, Records.plainName(clean(name)), clean(plane))
end

local function commanderLabel(c)
  local unit = coe5.get("commanderUnit", c)
  local m = unit and unit >= 0 and coe5.get("unitMonster", unit) or -1
  local monster = m >= 0 and coe5.get("monsterName", m) or "?"
  return clean(coe5.get("commanderName", c)) .. " the " .. clean(monster)
end

local function playerLabel(s)
  local cls = coe5.get("slotClass", s)
  local level = coe5.get("aiLevelName", coe5.get("slotAi", s)) or tostring(coe5.get("slotAi", s))
  return clean(coe5.get("slotName", s)), cls >= 0 and clean(coe5.get("className", cls)) or "", clean(level)
end

local function isController(s, controller) return coe5.get("slotController", s) == controller end

-- The living commanders of player s: {[commander] = unit}.
local function commandersOf(s)
  local out = {}
  for c = 0, coe5.get("commanderLastIndex") do
    local unit = coe5.get("commanderUnit", c)
    if unit >= 0 and coe5.get("unitHp", unit) > 0 and coe5.get("unitOwner", unit) == s then out[c] = unit end
  end
  return out
end

-- The units following commander c on its square, its own unit left out.
local function followerCount(c)
  local unit = coe5.get("commanderUnit", c)
  if not unit or unit < 0 then return 0 end
  local n = 0
  for _, u in ipairs(coe5.squareUnits(coe5.get("unitX", unit), coe5.get("unitY", unit))) do
    if u ~= unit and coe5.get("unitCommander", u) == c then n = n + 1 end
  end
  return n
end

-- ---- Records: the logic ------------------------------------------------------------------------
-- One plan of a commander: each step's scores and squares.

-- The step whose square the game keeps, and the best score.
function Records.pick(plan, side)
  local best, winner = 0, nil
  for _, step in ipairs(STEPS) do
    local e = plan[step]
    local a = e and e[side]
    if a and a[1] then
      if (step == "rally" and a[1] > 0) or (step ~= "rally" and a[1] > best) then best, winner = a[1], step end
    end
  end
  return winner, best
end

-- The best score of the steps before `step` that ran (final answers), as the game's best then.
function Records.bestBefore(plan, step)
  local cut = {}
  for _, s in ipairs(STEPS) do
    if STEP_ORDER[s] >= STEP_ORDER[step] then break end
    cut[s] = plan[s]
  end
  local _, best = Records.pick(cut, "final")
  return best
end

-- The runner-up: the highest final score among the other steps that ran, and its step.
function Records.runnerUp(plan, winner)
  local score, step
  for _, s in ipairs(STEPS) do
    local e = plan[s]
    if s ~= winner and e and e.final and e.final[1] and (not score or e.final[1] > score) then score, step = e.final[1], s end
  end
  return score, step
end

-- Why a step has no answer: nil when it ran.
function Records.notRun(plan, step)
  local e = plan[step]
  if e and e.final and e.game then return nil end
  if e and e.outer and not e.game then
    return Records.whoText("skipped by " .. (tostring(e.by or "a script"):gsub(",", ", "))) .. ": " .. Records.answerWords(e.final)
  end
  if step == "explore" then
    local best = Records.bestBefore(plan, step)
    if best >= EXPLORE_BELOW then
      return string.format("not run: best so far %d, runs only below %d", best, EXPLORE_BELOW)
    end
  elseif step == "far" then
    local best = Records.bestBefore(plan, step)
    if best >= FAR_EXPLORE_BELOW then
      return string.format("not run: best so far %d, runs only below %d", best, FAR_EXPLORE_BELOW)
    end
  end
  return "not run"
end

-- Whether a game folder holds the game being played.
function Records.sameGame(live, stored)
  local both = 0
  for s, v in pairs(stored or {}) do
    if live[s] then
      if live[s] ~= v then return false end
      both = both + 1
    end
  end
  return both >= 3
end

-- The turns of a "last turns" list around `turn`: `n` turns, centred on it where the records allow.
function Records.window(turn, first, latest, n)
  local lo = math.max(first, turn - n // 2)
  local hi = math.min(latest, lo + n - 1)
  lo = math.max(first, hi - n + 1)
  local out = {}
  for t = hi, lo, -1 do out[#out + 1] = t end
  return out
end

-- The trade line: what the next turn start's trade step moves.
function Records.tradeText(items)
  local pays, gets = {}, {}
  for _, item in ipairs(items or {}) do
    local name, n = tostring(item):match("^(.-) (%-?%d+)$")
    n = tonumber(n)
    if n and n < 0 then pays[#pays + 1] = (-n) .. " " .. name elseif n and n > 0 then gets[#gets + 1] = n .. " " .. name end
  end
  if #pays == 0 and #gets == 0 then return "Trade at the next turn start: none." end
  local text = "Trade at the next turn start:"
  if #pays > 0 then text = text .. " pays " .. table.concat(pays, ", ") end
  if #gets > 0 then text = text .. (#pays > 0 and " for " or " gets ") .. table.concat(gets, ", ") end
  return text .. "."
end

-- The line of a drawn box under the point (x, y), or nil.
function Records.lineAt(box, x, y)
  if not (box and x and y) or x < box.left or x >= box.left + box.width then return nil end
  local i = math.floor((box.top + box.line - 3 - y) / box.line)
  if i < 1 or i > box.count then return nil end
  return i
end

-- A ritual candidate's record in words.
function Records.ritualText(line, count)
  local name, said, kept, by = line:match("^([^:]*):([^:]*):([^:]*):?(.*)$")
  if not name then return line end
  local who = by ~= "" and (by:gsub(",", ", ")) or "a script"
  local text
  if kept == "dropped" then
    text = said == "dropped" and "pointless" or Records.whoText("vetoed by " .. who)
  else
    text = said == "skipped" and Records.whoText("kept by " .. who) or "not pointless"
  end
  return string.format("%s (%s%s)", name, text, (count or 1) > 1 and string.format(", %d times", count) or "")
end

-- A text cut at spaces into lines of at most `width` characters (a word longer than that keeps its own line).
function Records.wrapText(text, width)
  local out, line = {}, ""
  for word in tostring(text or ""):gmatch("%S+") do
    if line ~= "" and #line + 1 + #word > width then
      out[#out + 1] = line
      line = word
    else
      line = line == "" and word or (line .. " " .. word)
    end
  end
  if line ~= "" or #out == 0 then out[#out + 1] = line end
  return out
end

-- Recording-off spans "20-21,30-30" as words: "turns 20-21, turn 30".
function Records.spansText(spans)
  local out = {}
  for from, to in tostring(spans or ""):gmatch("(%d+)%-(%d+)") do
    out[#out + 1] = from == to and ("turn " .. from) or ("turns " .. from .. "-" .. to)
  end
  return table.concat(out, ", ")
end

-- A list with repeats said once each with a count: "troop pick-up skipped by X (3 times); ...".
function Records.counted(items)
  local counts, order = {}, {}
  for _, item in ipairs(items) do
    if not counts[item] then order[#order + 1] = item end
    counts[item] = (counts[item] or 0) + 1
  end
  for i, item in ipairs(order) do
    if counts[item] > 1 then order[i] = string.format("%s (%d times)", item, counts[item]) end
  end
  return table.concat(order, "; ")
end

-- "1 turn", "3 turns".
function Records.count(n, word) return n .. " " .. word .. (n == 1 and "" or "s") end

-- The mods that made a call this turn, "ModA,ModB", or nil.
function Records.actorsOf(actions, call, commander)
  local out, seen = {}, {}
  for _, a in ipairs(actions or {}) do
    if a.call == call and (commander == nil or a.commander == commander) and not seen[a.mod] then
      seen[a.mod] = true
      out[#out + 1] = a.mod
    end
  end
  return #out > 0 and table.concat(out, ",") or nil
end

-- A square's name without the mark a name may start with.
function Records.plainName(name) return (tostring(name or ""):gsub("^![!%d]", "")) end
-- A place as records hold it ("31,20 !!Oak Road Castle (Elysium)") without that mark.
function Records.plainPlace(place)
  local xy, rest = tostring(place or ""):match("^(%-?%d+,%-?%d+ )(.*)$")
  if xy then return xy .. Records.plainName(rest) end
  return Records.plainName(place)
end

-- The square a record's "x y" (or a place's "x,y ...") names, nil for none (-1): where a click goes.
function Records.squareAt(text)
  local x, y = tostring(text or ""):match("^%s*(%-?%d+)[ ,](%-?%d+)")
  x, y = tonumber(x), tonumber(y)
  if x and y and x >= 0 and y >= 0 then return x, y end
end
-- A go line's hover text: "Click: go to 39,19 Silver Mine (Elysium)"; how: "Click" or "Right click".
function Records.goText(place, how) return string.format("%s: go to %s", how or "Click", tostring(place or "?")) end
-- The column of a table row under the pointer, or nil.
function Records.columnAt(columns, rowLeft, rowWidth, px)
  if not px then return nil end
  local x = px - rowLeft
  if x < 0 or x >= rowWidth then return nil end
  local at
  for i, cx in ipairs(columns) do if x >= cx then at = i end end
  return at
end
-- The commander header's sentence without its place.
function Records.headRest(text, from, to)
  local pre, post = text:sub(1, from - 1), text:sub(to + 1)
  local words = {["Heading to "] = "Heading there", ["Exploring toward "] = "Exploring toward it", ["On its target, "] = "On its target",
                 ["Moved to "] = "Moved there", ["Rested at "] = "Rested there"}
  if words[pre] then return words[pre] .. post end
  local before = pre:match("^(.-)%.? At $")
  if before then return before .. post end
  return pre .. "there" .. post
end

-- One map frame of a skip at the seat's turn: "wait", "end" (End Turn now) or "stop" (a key or click).
function Records.skipStep(sk, pressed, frames)
  if not sk.armed then
    if not pressed then sk.armed = true end
    return "wait"
  end
  if pressed then return "stop" end
  sk.frames = sk.frames + 1
  if sk.frames < frames then return "wait" end
  sk.frames, sk.armed = 0, false   -- the next turn arms again
  return "end"
end

-- Whose human turn a skip ends.
function Records.skipEnds(active, observer, played)
  if active == nil or active < 0 then return false end
  if active == observer then return true end
  return played ~= nil and (active == played.slot or active == played.seat)
end

-- Whether a run of ended turns for Play as AI stops here.
function Records.handoverArrived(mode, playedTurn)
  if mode == "pick" then return playedTurn == true end
  if mode == "return" then return not playedTurn end
  return false
end

-- A screen's rows held to a fixed count.
function Records.fixedRows(rows, n, none)
  local out = {}
  for i, r in ipairs(rows) do out[i] = r end
  if #out == 0 and none ~= nil then out[1] = none end
  while #out < n do out[#out + 1] = false end
  return out
end

-- `list` held to `n` entries at the right end, `fill` before it.
function Records.alignRight(list, n, fill)
  local out, from = {}, math.max(#list - n, 0)
  for i = 1, n - (#list - from) do out[i] = fill end
  for i = from + 1, #list do out[#out + 1] = list[i] end
  return out
end

-- One page of a list of `count` rows: always `perPage` entries, a row's index or false for an empty one.
function Records.pageRows(count, page, perPage)
  local out = {}
  for i = 1, perPage do
    local at = page * perPage + i
    out[i] = at <= count and at or false
  end
  return out
end

-- Play as AI's panel: its two lines from the played AI's last record.
function Records.partnerLines(r)
  if not r then return {"Commanders -", "Bought -"} end
  local counts, order, n = {}, {}, 0
  for _, com in pairs(r.coms) do
    if com.finish ~= "-1 -1" then
      n = n + 1
      local _, kind = Records.gridCell(com)
      kind = (STEP_NAME[kind] or kind or "?"):lower()
      if not counts[kind] then order[#order + 1] = kind end
      counts[kind] = (counts[kind] or 0) + 1
    end
  end
  table.sort(order)
  local parts = {}
  for _, k in ipairs(order) do parts[#parts + 1] = k .. " " .. counts[k] end
  return {string.format("Commanders %d: %s", n, #parts > 0 and table.concat(parts, ", ") or "-"),
    #r.buys > 0 and ("Bought " .. Records.whoText(table.concat(r.buys, ", ")))
      or string.format("Bought nothing: %s gold at recruiting", tostring(r.gold_at_buy or "?"))}
end

-- The game's resources: slotResource and slotAiIncome take 0 to 13 (0 gold, 1 iron).
Records.RESOURCES = 14
-- A resource and its income as the game's top bar writes them: "100 (+45)".
function Records.resText(stock, income) return string.format("%d (%+d)", stock or 0, income or 0) end
-- Resources ({{name =, stock =, income =}}) as a record list, written "Gold 736|Iron 101|Hand of Glory 31":
function Records.packRes(list, field, signed)
  local out = {}
  for _, e in ipairs(list) do out[#out + 1] = e.name .. " " .. (signed and string.format("%+d", e[field] or 0) or tostring(e[field] or 0)) end
  return out
end
-- A packed list, or its written text, back: {{name =, value =}} in its order.
function Records.unpackRes(text)
  local out = {}
  if type(text) == "table" then text = table.concat(text, "|") end
  for item in tostring(text or ""):gmatch("[^|]+") do
    local name, v = item:match("^(.-) ([%+%-]?%d+)$")
    if name then out[#out + 1] = {name = name, value = tonumber(v)} end
  end
  return out
end
-- A record's resources at its turn's end.
function Records.resFromRecord(r)
  local stocks, incomes = Records.unpackRes(r.res_end), Records.unpackRes(r.res_income)
  if #stocks == 0 and #incomes == 0 then
    return {{name = "Gold", stock = r.gold_end or 0, income = r.income_gold or 0}, {name = "Iron", stock = r.iron_end or 0, income = r.income_iron or 0}}
  end
  local out, at = {}, {}
  local function entry(name)
    if not at[name] then
      out[#out + 1] = {name = name, stock = 0, income = 0}
      at[name] = out[#out]
    end
    return at[name]
  end
  for _, e in ipairs(stocks) do entry(e.name).stock = e.value end
  for _, e in ipairs(incomes) do entry(e.name).income = e.value end
  return out
end
-- Resources from the `from`th on as "Gold 100 (+45), Hand of Glory 977 (+28)"; "" for none.
function Records.resJoin(list, from)
  local out = {}
  for i = from or 1, #list do
    local e = list[i]
    -- Trade is points spent at each turn start, never kept: the points, as the top bar shows them.
    out[#out + 1] = e.name == "Trade" and ("Trade " .. (e.income or 0)) or (e.name .. " " .. Records.resText(e.stock, e.income))
  end
  return table.concat(out, ", ")
end

-- A skip's count as typed: a whole number from 1 to 50, else nil.
function Records.skipCount(text)
  local n = tonumber(tostring(text or ""):match("^%s*(%d+)%s*$"))
  if not n or n < 1 or n > 50 then return nil end
  return n
end
-- The stop rule of a skip set to stop early.
function Records.skipLoss(before, now)
  for slot, b in pairs(before) do
    local n = now[slot]
    if b.ai and not (n and n.ai) then return b.name .. " has been eliminated" end
    if b.leader >= 0 and n and n.leader < 0 then return b.name .. "'s leader died" end
  end
  return nil
end

-- A score as a screen says it: -999 is the conquest step's "no square found".
function Records.scoreText(score)
  if score == -999 then return "-999 (no square found)" end
  return tostring(score)
end

-- Why a commander with plans ended its turn where it stood.
function Records.restReason(com)
  local plan = com.plans and com.plans[#com.plans]
  if not plan or (tonumber(com.kind) or 0) ~= 0 then return nil end
  if com.target and com.target ~= "-1 -1" and com.finish == com.target then return nil end
  local winner, best = Records.pick(plan, "final")
  local moved = com.start and com.finish and com.start ~= com.finish
  local from, to = tostring(com.start_place or ""):match("%(([^()]*)%)$"), tostring(com.finish_place or ""):match("%(([^()]*)%)$")
  if com.casts and com.casts ~= "" then return "cast " .. com.casts .. " this turn" end
  if moved and from and to and from ~= to then
    return string.format("went from %s to %s by a ritual or a portal, not by its move steps", from, to)
  end
  if winner then
    local e = plan[winner]
    return string.format("%s %d at %d,%d won, but the move %s (no route, or a dangerous step)",
      STEP_NAME[winner], best, e.final[2], e.final[3], moved and "stopped short" or "never left the square")
  end
  if moved then return "moved on an earlier plan, then no step scored enough" end
  return "no step scored enough (rally above 0, each later step above the best so far)"
end

-- One cell of the step grid for a commander's turn.
local GRID_STEP = {rally = "Rally", friendly = "Friendly", conquer = "Conquer", explore = "Explore", far = "Far expl."}
function Records.gridCell(com)
  if not com then return "-", nil end
  local changed = false
  for _, p in ipairs(com.plans or {}) do
    for _, step in ipairs(STEPS) do
      if p[step] and p[step].by then changed = true end
    end
  end
  local mark = changed and "*" or ""
  if com.played_by == "you" then return "human" .. mark, "you" end
  if com.follows then return "follows", "follows" end
  local plan = com.plans and com.plans[#com.plans]
  if not plan then
    if com.skip and com.skip ~= "" then return "stays", "stays" end
    return "no plan", "no plan"
  end
  local winner, best = Records.pick(plan, "final")
  if not winner then return "rested" .. mark, "rested" end
  return GRID_STEP[winner] .. " " .. best .. mark, winner
end

-- The step grid of one AI, and its totals by step.
function Records.grid(recordsByTurn, turns, commanders)
  local rows, totals = {}, {}
  for _, c in ipairs(commanders) do
    local row = {c = c, cells = {}}
    local seen = false
    for i, t in ipairs(turns) do
      local r = recordsByTurn[t]
      local com = r and r.coms[c]
      local cell, kind = Records.gridCell(com)
      if com and com.finish == "-1 -1" then cell, kind = "died", "died" end
      if r and not com and seen then cell, kind = "gone", "gone" end
      row.cells[i] = r and cell or "-"
      if com then seen = true end
      if r and kind then totals[kind] = (totals[kind] or 0) + 1 end
    end
    rows[#rows + 1] = row
  end
  return rows, totals
end

-- The runner-up for "next best": the highest kept score among the other steps that found a square.
function Records.nextBest(plan, winner)
  local score, step
  for _, s in ipairs(STEPS) do
    local e = plan[s]
    if s ~= winner and e and e.final and e.final[1] and (e.final[2] or -1) >= 0 and (not score or e.final[1] > score) then
      score, step = e.final[1], s
    end
  end
  return score, step
end

-- An answer as text, "s x y", and back; "-" for none.
function Records.answerText(a)
  if not a then return "-" end
  return string.format("%d %d %d", a[1] or 0, a[2] or NO_SQUARE, a[3] or NO_SQUARE)
end
function Records.answerOf(text)
  local s, x, y = tostring(text or ""):match("^(%-?%d+) (%-?%d+) (%-?%d+)$")
  if not s then return nil end
  return {tonumber(s), tonumber(x), tonumber(y)}
end

-- A turn's record as an ini table, and back.
local TURN_KEYS = {"turn", "slot", "name", "class", "level", "played_by", "gold_start", "gold_end", "iron_start", "iron_end",
                   "income_gold", "income_iron", "res_start", "res_end", "res_income", "land", "citadels", "commanders", "leader", "leader_name", "buy_step",
                   "buys", "offers_left", "gold_at_buy", "trade", "casts"}
local COM_KEYS = {"name", "start", "finish", "start_place", "finish_place", "strength", "follows", "follows_name", "kind",
                  "target", "target_place", "target_seen", "ritual_plan", "plan_set", "casts", "played_by", "skip"}

-- The [about] section every record starts with.
local RECORD_ABOUT = {
  ["1_file"] = "AI Inspector's record of one AI player's turn: tTTTT_pSS.ini, TTTT the AI turn, SS the player slot",
  ["2_turn"] = "[turn]: turn slot name class level played_by gold_start iron_start gold_end iron_end income_gold income_iron "
    .. "(at the turn's end, as the game pays this AI) res_start res_end res_income (every resource it has, as "
    .. "Gold 736|Iron 101|Hand of Glory 31; res_income as the game pays this AI, Gold +29) land citadels commanders leader leader_name buy_step gold_at_buy buys offers_left "
    .. "trade (what its next turn start's trade step moves: Gold -8|Sacrifices 4) casts (rituals cast this turn: Raise Dead x2); "
    .. "the end values after the turn's battles",
  ["3_commander"] = "[c<commander>]: name start finish start_place finish_place strength (its last plan's) follows follows_name "
    .. "kind (0 none, 1 target, 2 explore) target target_place target_seen ritual_plan plan_set (a plan a script set before the "
    .. "AI move) casts played_by skip plans troops rituals; finish -1 -1: died, finish_place when and where",
  ["4_step"] = "p<plan>_<step> (step rally friendly conquer explore far): <game score x y>|<kept score x y>|<place>|<seen>|"
    .. "<changed by>; game - when the game's step never ran; no key when the step never ran; -999 no square found",
  ["5_ritual"] = "rituals: <ritual>:<game kept, dropped or skipped>:<kept or dropped>[:<mods that changed it>], | between",
  ["6_cut"] = "a value holds at most 1000 bytes: a longer list ends with <N> more not recorded, a longer text with (cut)",
}

-- One record value cut to VALUE_MAX bytes.
local VALUE_MAX = 1000
function Records.fitList(items)
  local out, size = {}, 0
  for i, item in ipairs(items) do
    item = clean(item)
    local rest = #items - i + 1
    local tail = rest > 1 and ("|" .. rest .. " more not recorded") or ""
    if size + #item + (#out > 0 and 1 or 0) + (i < #items and #tail or 0) > VALUE_MAX then
      out[#out + 1] = rest .. " more not recorded"
      break
    end
    out[#out + 1] = item
    size = size + #item + 1
  end
  return table.concat(out, "|")
end
function Records.fitText(text)
  text = clean(text)
  if #text <= VALUE_MAX then return text end
  return text:sub(1, VALUE_MAX - 6) .. " (cut)"
end

function Records.toIni(rec)
  local out = {turn = {}, about = RECORD_ABOUT}
  for _, k in ipairs(TURN_KEYS) do
    local v = rec[k]
    if type(v) == "table" then
      if #v > 0 then out.turn[k] = Records.fitList(v) end
    elseif v ~= nil and v ~= "" then
      out.turn[k] = type(v) == "number" and v or Records.fitText(v)
    end
  end
  for c, com in pairs(rec.coms or {}) do
    local sec = {}
    for _, k in ipairs(COM_KEYS) do
      local v = com[k]
      if v ~= nil and v ~= "" then sec[k] = type(v) == "number" and v or Records.fitText(v) end
    end
    sec.plans = #com.plans
    for n, plan in ipairs(com.plans) do
      if plan.comstr then sec["p" .. n .. "_strength"] = plan.comstr end
      for _, step in ipairs(STEPS) do
        local e = plan[step]
        if e then
          sec["p" .. n .. "_" .. step] = Records.answerText(e.game) .. "|" .. Records.answerText(e.final) .. "|"
            .. clean(e.place or ""):sub(1, 200) .. "|" .. clean(e.seen or "") .. "|" .. clean(e.by or ""):sub(1, 600)
        end
      end
    end
    if #com.troops > 0 then sec.troops = Records.fitList(com.troops) end
    if #com.rituals > 0 then sec.rituals = Records.fitList(com.rituals) end
    out["c" .. c] = sec
  end
  return out
end

local function split(text)
  local out = {}
  for part in tostring(text or ""):gmatch("[^|]+") do out[#out + 1] = part end
  return out
end

function Records.fromIni(t)
  if not t or not t.turn then return nil end
  local rec = {coms = {}}
  for k, v in pairs(t.turn) do rec[k] = tonumber(v) or v end
  rec.buys, rec.offers_left, rec.trade, rec.casts = split(t.turn.buys), split(t.turn.offers_left), split(t.turn.trade), split(t.turn.casts)
  for name, sec in pairs(t) do
    local c = tonumber(name:match("^c(%d+)$"))
    if c then
      local com = {c = c, plans = {}, troops = split(sec.troops), rituals = split(sec.rituals)}   ---@type table<string, any>
      for _, k in ipairs(COM_KEYS) do
        local v = sec[k]
        if v ~= nil then com[k] = tonumber(v) or v end
      end
      for _, k in ipairs({"start_place", "finish_place", "target_place"}) do
        if com[k] then com[k] = Records.plainPlace(com[k]) end
      end
      for n = 1, tonumber(sec.plans) or 0 do
        local plan = {}   ---@type table
        plan.comstr = tonumber(sec["p" .. n .. "_strength"])
        for _, step in ipairs(STEPS) do
          local v = sec["p" .. n .. "_" .. step]
          if v then
            local game, final, place, seen, by = v:match("^([^|]*)|([^|]*)|([^|]*)|?([^|]*)|?(.*)$")
            plan[step] = {outer = final ~= "-", game = Records.answerOf(game), final = Records.answerOf(final), place = Records.plainPlace(place),
                          seen = seen, by = by ~= "" and by or nil}
          end
        end
        com.plans[n] = plan
      end
      rec.coms[c] = com
    end
  end
  return rec
end

-- ---- Data area -----------------------------------------------------------------------------------
-- In coe5se_data\JazzyAIObserver\ (CoE5SE's data area for this mod):
local index, game, pendingLoad
local sightShown   -- the AI whose sight the Observer's map shows (coe5.showSight), nil for its own
local viewGone     -- {turn =, text =}: the viewed AI was eliminated, so the map went back to the Observer's view
local seatKept = false   -- the seat's own sight is kept by coe5.showSight (an AI's shown, or revealed, now)
-- The inspector's settings and their defaults.
local settings = {recording = true, list = true, arrows = 1, topBar = true, tags = true, hoverCard = true, skipStop = false}

local function baseName(path) return (tostring(path or ""):match("([^\\/]+)$") or ""):lower() end

local function readIndex()
  coe5.dataFolder("ins")
  index = coe5.readIni("index") or {}
  index.ins, index.saves, index.playas = index.ins or {}, index.saves or {}, index.playas or {}
  local s = (coe5.readIni("settings") or {}).settings or {}
  settings.recording = s.recording ~= "0"
  settings.list = s.list ~= "0"
  settings.arrows = tonumber(s.arrows) or 1
  settings.topBar = s.top_bar ~= "0"
  settings.tags = s.tags ~= "0"
  settings.hoverCard = s.hover_card ~= "0"
  settings.skipStop = s.skip_stop == "1"
end
local function writeIndex()
  coe5.dataFolder("ins")
  coe5.writeIni("index", index)
end
local function writeSettings()
  coe5.dataFolder("ins")
  coe5.writeIni("settings", {settings = {recording = settings.recording and 1 or 0, list = settings.list and 1 or 0,
    arrows = settings.arrows, top_bar = settings.topBar and 1 or 0, tags = settings.tags and 1 or 0, hover_card = settings.hoverCard and 1 or 0, skip_stop = settings.skipStop and 1 or 0}})
end

local function recordName(turn, slot) return string.format("t%04d_p%02d", math.max(turn, 0), slot) end

-- The players of the game being played: {[slot] = "name|class"} for every slot with a class.
local function livePlayers()
  local out = {}
  for s = 0, 31 do
    local cls = coe5.get("slotClass", s)
    if cls and cls > 0 then out[s] = clean(coe5.get("slotName", s)) .. "|" .. clean(coe5.get("className", cls)) end
  end
  return out
end

-- A game folder's players.
local function storedPlayers(id)
  coe5.dataFolder(id)
  local info = coe5.readIni("game")
  if info and info.players then
    local out = {}
    for k, v in pairs(info.players) do out[tonumber(k:match("%d+")) or -1] = v end
    return out
  end
  local out = {}
  for _, name in ipairs(coe5.listIni()) do
    local s = tonumber(name:match("^t%d+_p(%d+)$"))
    if s and not out[s] then
      local r = coe5.readIni(name)
      if r and r.turn then out[s] = clean(r.turn.name) .. "|" .. clean(r.turn.class) end
    end
  end
  return out
end

-- The folder of a loaded save whose file name the index doesn't hold.
local function findLoadedGame(name)
  local base = name:match("^(aitestkit g%d+ t%-?%d+) .+$")
  if base and index.saves[base] then return index.saves[base] end
  local live = livePlayers()
  for n = (tonumber(index.ins.next) or 1) - 1, 1, -1 do
    local id = string.format("game_%04d", n)
    if Records.sameGame(live, storedPlayers(id)) then return id end
  end
end

-- A save loaded into a folder that holds later turns: those turns move out.
function Records.laterThan(name, turn, active)
  local t, s = name:match("^t(%d+)_p(%d+)$")
  t, s = tonumber(t), tonumber(s)
  return t ~= nil and (t > turn or (t == turn and s >= active))
end
function Records.offTurnsBefore(spans, turn)
  local out = {}
  for a, b in tostring(spans or ""):gmatch("(%d+)%-(%d+)") do
    local from, to = tonumber(a) or 0, tonumber(b) or 0
    if from < turn then out[#out + 1] = from .. "-" .. math.min(to, turn - 1) end
  end
  return table.concat(out, ",")
end
function Ins.setAsideLaterTurns(id)
  local turn = math.max(coe5.turn(), 0)
  local active = math.max(coe5.get("activePlayer") or 0, 0)
  coe5.dataFolder(id)
  local later = {}
  for _, name in ipairs(coe5.listIni()) do
    if Records.laterThan(name, turn, active) then later[#later + 1] = {name = name, t = coe5.readIni(name)} end
  end
  local info = coe5.readIni("game")
  if info and info.game then
    local kept = Records.offTurnsBefore(info.game.off_turns, turn)
    if (info.game.off_turns or "") ~= kept then
      info.game.off_turns = kept
      coe5.writeIni("game", info)
    end
  end
  if #later == 0 then return end
  local aside = string.format("%s_left_t%04d", id, turn)
  for n = 2, 99 do
    coe5.dataFolder(aside)
    if #coe5.listIni() == 0 then break end
    aside = string.format("%s_left_t%04d_%d", id, turn, n)
  end
  local moved = 0
  for _, f in ipairs(later) do
    if f.t and coe5.writeIni(f.name, f.t) then moved = moved + 1 end
  end
  coe5.dataFolder(id)
  for _, f in ipairs(later) do coe5.deleteIni(f.name) end
  coe5.log(string.format("inspector_later_turns_set_aside game=%s turn=%d from_slot=%d files=%d moved=%d folder=%s", id, turn, active,
    #later, moved, aside))
end

local function startGame()
  readIndex()
  local id = pendingLoad and index.saves[pendingLoad]
  if not id and pendingLoad then
    id = findLoadedGame(pendingLoad)
    if id then
      index.saves[pendingLoad] = id
      coe5.log(string.format("inspector_game_found save=%s game=%s", pendingLoad, id))
    end
  end
  if not id then
    local n = tonumber(index.ins.next) or 1
    id = string.format("game_%04d", n)
    index.ins.next = tostring(n + 1)
  end
  index.ins.current = id
  writeIndex()
  if pendingLoad then Ins.setAsideLaterTurns(id) end
  game = {id = id, cache = {}, lastTurnOf = {}}
  coe5.dataFolder(id)
  local info = coe5.readIni("game")
  if not info then
    info = {game = {id = id, loaded = pendingLoad or ""}}
    if settings.recording then info.game.first_recorded = tostring(math.max(coe5.turn(), 0)) end
  end
  if not info.players then   -- the players, so a later load under another file name finds this folder
    info.players = {}
    for s, v in pairs(livePlayers()) do info.players[string.format("p%02d", s)] = v end
    coe5.writeIni("game", info)
  end
  game.firstRecorded = tonumber(info.game and info.game.first_recorded)
  game.offTurns = info.game and info.game.off_turns or ""
  -- Each slot's newest recorded turn, eliminated AIs' too.
  for _, name in ipairs(coe5.listIni()) do
    local tt, s = name:match("^t(%d+)_p(%d+)$")
    tt, s = tonumber(tt), tonumber(s)
    if s and (game.lastTurnOf[s] or -1) < tt then game.lastTurnOf[s] = tt end
  end
  pendingLoad = nil
end

-- Recording turned off or on: the turns it was off go in game.ini (off_turns "20-21,30-30").
local function noteRecordingSwitch(on)
  if not game then return end
  local turn = math.max(coe5.turn(), 0)
  coe5.dataFolder(game.id)
  local info = coe5.readIni("game") or {game = {id = game.id}}
  if on and info.game.off_from then
    local from = tonumber(info.game.off_from) or turn
    if turn > from then
      game.offTurns = (game.offTurns ~= "" and game.offTurns .. "," or "") .. from .. "-" .. (turn - 1)
      info.game.off_turns = game.offTurns
    end
    info.game.off_from = nil
  elseif not on then
    info.game.off_from = tostring(turn)
  end
  coe5.writeIni("game", info)
end

local function noteRecordingStarts()
  if not game or game.firstRecorded then return end
  game.firstRecorded = math.max(coe5.turn(), 0)
  coe5.dataFolder(game.id)
  local info = coe5.readIni("game") or {game = {id = game.id}}
  info.game.first_recorded = tostring(game.firstRecorded)
  coe5.writeIni("game", info)
end

-- Player slot's record of `turn`, read once (nil: not recorded).
local function readRecord(turn, slot)
  if not game or turn < 0 then return nil end
  local key = recordName(turn, slot)
  local hit = game.cache[key]
  if hit == nil then
    coe5.dataFolder(game.id)
    hit = Records.fromIni(coe5.readIni(key)) or false
    game.cache[key] = hit
  end
  return hit or nil
end

-- The newest recorded turn of slot at or before `turn`, and its record.
local function latestRecord(slot, turn)
  turn = turn or coe5.turn()
  for t = turn, math.max(turn - 3, 0), -1 do
    local rec = readRecord(t, slot)
    if rec then return t, rec end
  end
  local last = game and game.lastTurnOf[slot]   -- an AI with no recent turn: eliminated, or recording off
  if last and last <= turn then
    local rec = readRecord(last, slot)
    if rec then return last, rec end
  end
end

-- ---- Play as AI ----------------------------------------------------------------------------------
-- Play as AI: the player plays an AI player's turns, the game's AI finishing each.
local ID_BACK, ID_CANCEL, ID_PLAYER_FIRST = 9, 8, 100
local runAiTurn = coe5.runAiTurn
local playAs   -- {slot =, seat =, active =, returning =} from the pick to Return to AI

local function nextMover(after)   -- the next slot after `after` to move this month, or nil
  for s = after + 1, 31 do
    local controller = coe5.get("slotController", s)
    if controller == coe5.controller.human or controller == coe5.controller.ai or controller == coe5.controller.independent then
      return s
    end
  end
end

-- X's stocks and its commanders' action points left, for the log: "Gold 120,Iron 3 ap=c12:3,c40:2".
function Ins.resNow(s)
  local out = {}
  for res = 0, Records.RESOURCES - 1 do
    local stock = coe5.get("slotResource", s, res) or 0
    local income = coe5.get("slotAiIncome", s, res) or coe5.get("slotIncome", s, res) or 0
    if res <= 1 or stock ~= 0 or income ~= 0 then
      out[#out + 1] = {name = clean(coe5.get("resourceName", res) or ("Resource " .. res)), stock = stock, income = income}
    end
  end
  return out
end
local function stocksAndAp(s)
  local stocks, ap = {}, {}
  for r = 0, 13 do
    local amount = coe5.get("slotResource", s, r)
    if amount ~= 0 then stocks[#stocks + 1] = clean(coe5.get("resourceName", r) or ("r" .. r)) .. " " .. amount end
  end
  for c = 0, coe5.get("commanderLastIndex") do
    local unit = coe5.get("commanderUnit", c)
    if unit >= 0 and coe5.get("unitHp", unit) > 0 and coe5.get("unitOwner", unit) == s then
      ap[#ap + 1] = "c" .. c .. ":" .. coe5.get("unitApLeft", unit)
    end
  end
  return table.concat(stocks, ","), table.concat(ap, ",")
end

-- The seat back to human, then X back to the AI unless the game took X out meanwhile.
local function endReview(reason)
  if playAs.active and isController(playAs.seat, coe5.controller.ai) then
    coe5.set("slotController", playAs.seat, coe5.controller.human)
  end
  if playAs.active and isController(playAs.slot, coe5.controller.human) then
    coe5.set("slotController", playAs.slot, coe5.controller.ai)
  end
  coe5.log(string.format("play_as_ended slot=%d seat=%d turn=%d reason=%s", playAs.slot, playAs.seat, coe5.turn(), reason))
  playAs = nil
end

local function addPlayedEntry(entry)
  if not game then return end
  coe5.dataFolder(game.id)
  local played = coe5.readIni("played") or {}
  local n = 1
  while played["review" .. n] do n = n + 1 end
  played["review" .. n] = entry
  coe5.writeIni("played", played)
end

-- At the end of the turn before X's first played one: X human, the seat the AI's.
local function maybeStartReview(after)
  if not playAs or playAs.returning then return end
  if not isController(playAs.slot, coe5.controller.ai) and not isController(playAs.slot, coe5.controller.human) then
    return endReview("not_in_game")
  end
  if playAs.active or nextMover(after) ~= playAs.slot then return end
  sightShown = nil   -- the seat's own sight is back since its turn ended (Ins.endObserverTurn)
  coe5.set("slotController", playAs.slot, coe5.controller.human)
  playAs.active = true
  if isController(playAs.seat, coe5.controller.human) then coe5.set("slotController", playAs.seat, coe5.controller.ai) end
  local name, class = playerLabel(playAs.slot)
  addPlayedEntry({slot = playAs.slot, player = name, class = class, turn = coe5.turn(), seat = playAs.seat})
  coe5.log(string.format("play_as_started slot=%d seat=%d turn=%d", playAs.slot, playAs.seat, coe5.turn()))
end

-- X's turn start: X is AI while the game starts its turn, so it gets an AI's turn start.
local turnStartAsAi, seatHumanMeanwhile = false, false

-- The seat human while X is an AI between its turns' human parts.
local function seatHumanWhile()
  if playAs and playAs.active and not playAs.returning and isController(playAs.seat, coe5.controller.ai) then
    coe5.set("slotController", playAs.seat, coe5.controller.human)
    seatHumanMeanwhile = true
  end
end
local function seatAiAgain()
  if seatHumanMeanwhile and playAs and not playAs.returning and isController(playAs.seat, coe5.controller.human) then
    coe5.set("slotController", playAs.seat, coe5.controller.ai)
  end
  seatHumanMeanwhile = false
end

local function reviewTurnStartBefore(player)
  if not (playAs and playAs.slot == player) then return end
  if playAs.active and isController(player, coe5.controller.human) then
    seatHumanWhile()
    coe5.set("slotController", player, coe5.controller.ai)
    turnStartAsAi = true
  end
  coe5.log(string.format("play_as_turn_start slot=%d turn=%d played=%s before=%s", player, coe5.turn(), tostring(turnStartAsAi),
    (stocksAndAp(player))))
end
local function reviewTurnStartAfter(player)
  if not (playAs and playAs.slot == player) then return end
  if turnStartAsAi then coe5.set("slotController", player, coe5.controller.human) end
  seatAiAgain()
  coe5.log(string.format("play_as_turn_start slot=%d turn=%d played=%s after=%s", player, coe5.turn(), tostring(turnStartAsAi),
    (stocksAndAp(player))))
  turnStartAsAi = false
end

-- What the player touched in X's turn.
local turnStartState   -- {[commander] = {used, x, y, followers}} of the played turn, or nil
local touched = {}     -- {[commander] = true}: the commanders the AI leaves alone at the hand-over
local function commanderState(c, unit)
  local x, y, followers = coe5.get("unitX", unit), coe5.get("unitY", unit), 0
  for _, u in ipairs(coe5.squareUnits(x, y)) do
    if u ~= unit and coe5.get("unitCommander", u) == c then followers = followers + 1 end
  end
  return {used = coe5.get("unitApUsed", unit), x = x, y = y, followers = followers}
end
-- How many times player s has cast each ritual (slotRitualCasts, what maxcast counts), those cast at all.
function Ins.castCounts(s)
  local out = {}
  for ritual = 0, coe5.get("ritualLastIndex") or -1 do
    local n = coe5.get("slotRitualCasts", s, ritual) or 0
    if n > 0 then out[ritual] = n end
  end
  return out
end

-- The played turn's start for the record: its stocks, each commander's square, and the player's buys.
local partnerTurn   -- {turn =, gold =, iron =, starts = {[c] = "x y"}, buys = {}}
local function notePlayedTurnStart(player)
  touched = {}
  if not (playAs and playAs.active and playAs.slot == player) then turnStartState = nil; return end
  turnStartState = {}
  partnerTurn = {turn = coe5.turn(), gold = coe5.get("slotResource", player, 0), iron = coe5.get("slotResource", player, 1),
    res = Records.packRes(Ins.resNow(player), "stock"),
                 starts = {}, buys = {}, casts = Ins.castCounts(player)}
  for c, unit in pairs(commandersOf(player)) do
    turnStartState[c] = commanderState(c, unit)
    partnerTurn.starts[c] = coe5.get("unitX", unit) .. " " .. coe5.get("unitY", unit)
  end
end
-- Whether commander c moved, used action points or changed its army.
function Ins.handTouched(c)
  if not (playAs and playAs.active and turnStartState and turnStartState[c]) then return false end
  local unit = coe5.get("commanderUnit", c)
  if not unit or unit < 0 then return false end
  local was, now = turnStartState[c], commanderState(c, unit)
  return now.used ~= was.used or now.x ~= was.x or now.y ~= was.y or now.followers ~= was.followers
end

-- At the turn's end: the touched commanders, each with no action points left.
local function restTouched(player)
  touched = {}
  local list, marked = {}, {}
  for c, unit in pairs(commandersOf(player)) do
    local was = turnStartState and turnStartState[c]
    local now = commanderState(c, unit)
    local done = coe5.get("commanderDone", c) == true
    local changed = was and (now.used ~= was.used or now.x ~= was.x or now.y ~= was.y or now.followers ~= was.followers)
    if done then marked[#marked + 1] = c end
    if changed or done then
      touched[c] = true
      list[#list + 1] = c
      local left = coe5.get("unitApLeft", unit)
      if left > 0 then coe5.set("unitApUsed", unit, now.used + left) end
    end
  end
  table.sort(list)
  table.sort(marked)
  for _, c in ipairs(marked) do
    coe5.log(string.format("play_as_marked slot=%d turn=%d commander=%d name=%s", player, coe5.turn(), c,
      clean(coe5.get("commanderName", c))))
  end
  coe5.log(string.format("play_as_touched slot=%d turn=%d commanders=%s marked=%s", player, coe5.turn(),
    #list > 0 and table.concat(list, ",") or "-", #marked > 0 and table.concat(marked, ",") or "-"))
  turnStartState = nil
end

-- The end of X's turn: the game's AI moves X from there.
local handingOver = false
local plansAtStart   -- {slot =, [commander] = ritual plan}: an AI's plans when its turn starts (AI_RECRUIT compares)
local ritualPlansOf  -- set with the recorders below
local function reviewTurnEnd(player)
  if not (playAs and playAs.active and playAs.slot == player) then return end
  local stocks, ap = stocksAndAp(player)
  coe5.log(string.format("play_as_hand_over slot=%d turn=%d stocks=%s ap=%s", player, coe5.turn(), stocks, ap))
  restTouched(player)
  seatHumanWhile()
  handingOver = true
  plansAtStart = ritualPlansOf(player)
  runAiTurn(player)   -- false when refused: CoE5SE logged why
  handingOver = false
  seatAiAgain()
  touched = {}
  if playAs and playAs.returning then endReview("returned") end
end

-- The seat as the AI's stays where it is.
local REST_PRIORITY = 1000
local function touchedNow(c) return handingOver and touched[c] == true end
local function restingCommander(c)
  if not (playAs and playAs.active) then return false end
  if touchedNow(c) then return true end
  local unit = coe5.get("commanderUnit", c)
  return unit >= 0 and coe5.get("unitOwner", unit) == playAs.seat
end
for _, step in ipairs(STEPS) do
  coe5.wrap(STEP_SITE[step], function(next, c, comstr)
    if restingCommander(c) then return 0, coe5.aiMove.noSquare, coe5.aiMove.noSquare end
    return next(c, comstr)
  end, REST_PRIORITY)
end
-- A touched commander's army stays as the player left it, and it plans no ritual this turn.
coe5.wrap("AI_GATHER_TROOPS", function(next, c)
  if touchedNow(c) then return end
  next(c)
end, REST_PRIORITY)
coe5.wrap("AI_PICK_UP_LONER", function(next, c, unit)
  if touchedNow(c) or touchedNow(coe5.get("unitCommander", unit)) then return 0 end
  return next(c, unit)
end, REST_PRIORITY)
coe5.wrap("AI_SPLIT_ARMY", function(next, c, minUnits)
  if touchedNow(c) then return end
  next(c, minUnits)
end, REST_PRIORITY)
coe5.wrap("AI_RITUAL_CANDIDATE", function(next, c, ritual)
  if touchedNow(c) then return false end
  return next(c, ritual)
end, REST_PRIORITY)

-- ai_move's start for X at the hand-over: its stocks and action points as the player left them.
local function reviewAiTurnStart(player)
  if not (handingOver and playAs and playAs.slot == player) then return end
  local stocks, ap = stocksAndAp(player)
  coe5.log(string.format("play_as_ai_move slot=%d turn=%d stocks=%s ap=%s", player, coe5.turn(), stocks, ap))
end

-- X's buys on the Recruit screen go through the game's AI buy.
local recruitScreen = coe5.recruitScreen
local OFFER_ROW_FIRST, SHOP_OFFERS = 100, 160   -- the Recruit screen's offer rows; a shop's 160 offers
local STOCK_LAST = 13   -- the stocks a player has (resource types 0 Gold to 13 Lifeforce, RVA_PLAYER_INTS)
local function shopOffer(s, offer)   -- the offer's coe5.shopOffers entry, nil for an item or one bought
  for _, o in ipairs(coe5.shopOffers(s)) do
    if o.offer == offer then return o end
  end
end
local function stocksOf(s)   -- every stock, Gold first: "gold,iron,...,lifeforce"
  local out = {}
  for r = 0, STOCK_LAST do out[#out + 1] = coe5.get("slotResource", s, r) end
  return table.concat(out, ",")
end
-- The reasons coe5.aiBuyOffer gives that are the Recruit screen's own checks.
local GAME_REFUSES = {square_used = true, not_enough_gold = true, not_enough_iron = true, not_enough_units = true, cannot_pay = true,
  wrong_plane = true, no_upgrade_units = true, sold_out = true, not_here = true, not_possible = true}
local function costsOf(o)   -- an offer's costs other than 0, "type:amount,..." by cost type ("-" for none)
  local out = {}
  for t = 0, 18 do
    if (o.costs[t] or 0) ~= 0 then out[#out + 1] = t .. ":" .. o.costs[t] end
  end
  return #out > 0 and table.concat(out, ",") or "-"
end
local function countOf(s, offer)   -- the offer's count as coe5.shopOffers has it, 0 when it isn't there
  local o = shopOffer(s, offer)
  return o and o.count or 0
end
local wentOn   -- the click that went on to the game, checked at the screen's next wait
local function checkWentOn()
  local w = wentOn
  wentOn = nil
  if not w then return end
  local stocks, units, count = stocksOf(w.slot), #coe5.squareUnits(w.x, w.y), countOf(w.slot, w.offer)
  local changed = stocks ~= w.stocks or units ~= w.units or count ~= w.count
  if w.item then
    coe5.log(string.format("play_as_ai_buy slot=%d turn=%d offer=%d path=item_human x=%d y=%d stocks_before=%s stocks_after=%s units_before=%d units_after=%d bought=%s",
      w.slot, coe5.turn(), w.offer, w.x, w.y, w.stocks, stocks, w.units, units, tostring(changed)))
  elseif changed then
    coe5.log(string.format("play_as_human_buy_leak slot=%d turn=%d offer=%d x=%d y=%d stocks_before=%s stocks_after=%s units_before=%d units_after=%d count_before=%d count_after=%d",
      w.slot, coe5.turn(), w.offer, w.x, w.y, w.stocks, stocks, w.units, units, w.count, count))
  end
end
coe5.wrap("RECRUIT_WAIT", function(next, block, mode)
  checkWentOn()
  local id = next(block, mode)
  if not (recruitScreen and playAs and playAs.active) or id < OFFER_ROW_FIRST or id >= OFFER_ROW_FIRST + SHOP_OFFERS then return id end
  local buttons = coe5.get("mouseButtons")
  if buttons ~= 0 and buttons ~= 1 then return id end
  local screen = recruitScreen()
  if not screen or screen.player ~= playAs.slot or screen.ctrl or screen.latch ~= 1 or screen.x < 0 then return id end
  local s, offer, x, y = playAs.slot, id - OFFER_ROW_FIRST, screen.x, screen.y
  local o = shopOffer(s, offer)
  local stocks, units = stocksOf(s), #coe5.squareUnits(x, y)
  -- Whether a listed row is a magic item for sale.
  if not o then
    coe5.log(string.format("play_as_buy_to_game slot=%d turn=%d offer=%d path=item_human x=%d y=%d", s, coe5.turn(), offer, x, y))
    wentOn = {slot = s, offer = offer, x = x, y = y, stocks = stocks, units = units, count = 0, item = true}
    return id
  end
  local had = commandersOf(s)
  local bought, why = coe5.aiBuyOffer(s, offer, x, y)
  local count = countOf(s, offer)
  if why and GAME_REFUSES[why] then
    -- The Recruit screen's own check refused it: the game's buy says why.
    coe5.log(string.format("play_as_buy_to_game slot=%d turn=%d offer=%d monster=%d x=%d y=%d reason=%s", s, coe5.turn(), offer, o.monster, x, y, why))
    wentOn = {slot = s, offer = offer, x = x, y = y, stocks = stocks, units = units, count = o.count}
    return id
  elseif why then
    -- CoE5SE refused it for its own reason: the click goes nowhere.
    coe5.log(string.format("play_as_buy_refused slot=%d turn=%d offer=%d monster=%d x=%d y=%d reason=%s", s, coe5.turn(), offer, o.monster, x, y, why))
    return 0
  end
  local new = {}
  for c in pairs(commandersOf(s)) do
    if not had[c] then new[#new + 1] = c end
  end
  table.sort(new)
  local after = stocksOf(s)
  if bought and partnerTurn and partnerTurn.turn == coe5.turn() then
    local n = o.commander and 1 or math.max(#coe5.squareUnits(x, y) - units, 1)
    partnerTurn.buys[#partnerTurn.buys + 1] = string.format("%d %s (%s%d gold), bought by you", n,
      clean(coe5.get("monsterName", o.monster) or "?"), o.commander and "commander, " or "", o.gold or 0)
  end
  coe5.log(string.format("play_as_ai_buy slot=%d turn=%d offer=%d monster=%d path=%s new=%s x=%d y=%d gold_before=%s gold_after=%s "
    .. "stocks_before=%s stocks_after=%s costs=%s units_before=%d units_after=%d count_before=%d count_after=%d bought=%s",
    s, coe5.turn(), offer, o.monster, o.commander and "commander" or "troops", #new > 0 and table.concat(new, ",") or "-", x, y,
    stocks:match("^[^,]*"), after:match("^[^,]*"), stocks, after, costsOf(o), units, #coe5.squareUnits(x, y), o.count, count, tostring(bought)))
  return 0   -- the game's own buy never runs: 0 is no click
end)

-- ---- The recorders -------------------------------------------------------------------------------
-- `rec` is the record of the AI turn running now (AI_TURN before to after), nil between AI turns.
local rec

local function newCommander(c, unit)
  return {c = c, name = commanderLabel(c), plans = {}, troops = {}, rituals = {},
          start = coe5.get("unitX", unit) .. " " .. coe5.get("unitY", unit),
          start_place = placeOf(coe5.get("unitX", unit), coe5.get("unitY", unit))}
end

local function comOf(c)
  if not rec then return nil end
  local com = rec.coms[c]
  if not com then
    local unit = coe5.get("commanderUnit", c)
    if not unit or unit < 0 then return nil end
    com = newCommander(c, unit)
    rec.coms[c] = com
  end
  return com
end

local function startRecord(player)
  rec = nil
  if not (game and settings.recording) then return end
  if not isController(player, coe5.controller.ai) then return end
  if playAs and playAs.active and player == playAs.seat then return end   -- the seat resting as the AI's
  noteRecordingStarts()
  local name, class, level = playerLabel(player)
  rec = {turn = math.max(coe5.turn(), 0), slot = player, name = name, class = class, level = level, coms = {},
         played_by = (handingOver and playAs and playAs.slot == player and not (Ins.skip and not Ins.skip.mode)) and "you, then the AI" or "AI",
         gold_start = coe5.get("slotResource", player, 0), iron_start = coe5.get("slotResource", player, 1),
         res_start = Records.packRes(Ins.resNow(player), "stock"),
         castsAtStart = Ins.castCounts(player), plansAtMove = {}}
  for c, unit in pairs(commandersOf(player)) do rec.coms[c] = newCommander(c, unit) end
  -- A played turn: its start is the player's turn start, before the player's moves and buys.
  local pt = partnerTurn
  if handingOver and pt and pt.turn == coe5.turn() then
    rec.gold_start, rec.iron_start, rec.partnerBuys = pt.gold, pt.iron, pt.buys
    rec.res_start = pt.res or rec.res_start
    rec.castsAtStart = pt.casts or rec.castsAtStart
    for c, com in pairs(rec.coms) do
      local xy = pt.starts[c]
      if xy then
        local sx, sy = xy:match("^(%-?%d+) (%-?%d+)$")
        com.start, com.start_place = xy, placeOf(tonumber(sx), tonumber(sy))
      end
    end
  end
end

-- The plan a step of commander c belongs to.
local function planFor(com, step, comstr)
  local plan = com.plans[#com.plans]
  local fresh = not plan or step == "rally"
  if plan and not fresh then
    for s, e in pairs(plan) do
      if STEP_ORDER[s] and e and STEP_ORDER[s] >= STEP_ORDER[step] then fresh = true break end
    end
  end
  if fresh then
    plan = {comstr = comstr}
    com.plans[#com.plans + 1] = plan
  end
  return plan
end

local function seenText(slot, x, y)
  if not x or x < 0 then return "" end
  return SEEN_WORD[coe5.get("squareSeen", slot, x, y) or 0] or ""
end

local pack, unpack = table.pack, table.unpack

-- The mods whose wraps changed this site's answer or arguments, or nil.
local MOD_NAME = "JazzyAIObserver"   -- the install name, as coe5.changedBy lists a mod
local changedBy = rawget(coe5, "changedBy")
local function changers(c)
  if not changedBy then return nil end
  local names, seen = {}, {}
  for _, ch in ipairs(changedBy()) do
    if ch.answer or ch.arguments then
      local name = ch.mod
      if name == MOD_NAME then name = (c and touchedNow(c)) and "you" or "Play as AI" end
      if not seen[name] then
        seen[name] = true
        names[#names + 1] = name
      end
    end
  end
  return #names > 0 and table.concat(names, ",") or nil
end
local current = {}   -- {[step] = the step entry the outermost recorder opened, for the innermost one}
for _, step in ipairs(STEPS) do
  coe5.wrap(STEP_SITE[step], function(next, c, comstr)
    local com = comOf(c)
    local entry
    if com then
      local plan = planFor(com, step, comstr)
      entry = {outer = true}
      plan[step] = entry
    end
    current[step] = entry
    local r = pack(next(c, comstr))
    local by = changers(c)
    current[step] = nil
    if entry then
      local g = entry.game or {}
      local f = {r[1] or g[1] or 0, r[2] or g[2] or NO_SQUARE, r[3] or g[3] or NO_SQUARE}
      entry.final = f
      local x, y = f[2], f[3]
      if x >= 0 then entry.place, entry.seen = placeOf(x, y), seenText(rec.slot, x, y) end
      if by or not entry.game or g[1] ~= f[1] or g[2] ~= f[2] or g[3] ~= f[3] then
        -- Who changed it, by mod.
        entry.by = by or (touchedNow(c) and "you" or (restingCommander(c) and "Play as AI" or "a script"))
        coe5.log(string.format("inspector_override turn=%d player=%d commander=%d step=%s game=%s kept=%d,%d,%d by=%s", rec.turn,
          rec.slot, c, step, entry.game and (g[1] .. "," .. g[2] .. "," .. g[3]) or "-", f[1], f[2], f[3], entry.by))
      end
    end
    return unpack(r, 1, r.n)
  end, OUTER)
  coe5.wrap(STEP_SITE[step], function(next, c, comstr)
    local r = pack(next(c, comstr))
    local entry = current[step]
    if entry then entry.game = {r[1] or 0, r[2] or NO_SQUARE, r[3] or NO_SQUARE} end
    return unpack(r, 1, r.n)
  end, INNER)
end

-- Rituals: each candidate the game's checks reached, with the game's answer and the one kept.
local ritualGame
coe5.wrap("AI_RITUAL_CANDIDATE", function(next, c, ritual)
  ritualGame = nil
  local r = pack(next(c, ritual))
  local by = changers(c)
  local com = comOf(c)
  if com then
    local kept = r[1]
    local gameSaid = ritualGame == nil and "skipped" or (ritualGame and "kept" or "dropped")
    com.rituals[#com.rituals + 1] = clean(coe5.get("ritualName", ritual) or ("ritual " .. ritual)) .. ":" .. gameSaid .. ":"
      .. (kept == false and "dropped" or "kept") .. (by and (":" .. by) or "")
  end
  return unpack(r, 1, r.n)
end, OUTER)
coe5.wrap("AI_RITUAL_CANDIDATE", function(next, c, ritual)
  local r = pack(next(c, ritual))
  ritualGame = r[1] ~= false
  return unpack(r, 1, r.n)
end, INNER)

-- Troops: what the pick-up took or left, a lone commander picked up, a split.
local gatherRan, lonerGame, splitRan
coe5.wrap("AI_GATHER_TROOPS", function(next, c)
  local com = comOf(c)
  local before = com and followerCount(c)
  gatherRan = false
  local r = pack(next(c))
  if com then
    local after = followerCount(c)
    if not gatherRan then
      com.troops[#com.troops + 1] = "troop pick-up skipped by " .. (changers(c) or "a script")
    elseif after ~= before then
      com.troops[#com.troops + 1] = string.format("troop pick-up: %s%d units (%d following it now)", after > before and "+" or "",
        after - before, after)
    end
  end
  return unpack(r, 1, r.n)
end, OUTER)
coe5.wrap("AI_GATHER_TROOPS", function(next, c)
  gatherRan = true
  return next(c)
end, INNER)
coe5.wrap("AI_PICK_UP_LONER", function(next, c, unit)
  lonerGame = nil
  local r = pack(next(c, unit))
  local by = changers(c)
  local com = comOf(c)
  if com then
    local joined = tonumber(r[1]) or 0
    local who = clean(coe5.get("monsterName", coe5.get("unitMonster", unit)) or "?")
    if lonerGame == nil then
      com.troops[#com.troops + 1] = "lone commander pick-up (" .. who .. ") skipped by " .. (by or "a script")
    elseif joined > 0 then
      com.troops[#com.troops + 1] = string.format("picked up a lone %s: %d units joined", who, joined)
    end
  end
  return unpack(r, 1, r.n)
end, OUTER)
coe5.wrap("AI_PICK_UP_LONER", function(next, c, unit)
  local r = pack(next(c, unit))
  lonerGame = tonumber(r[1]) or 0
  return unpack(r, 1, r.n)
end, INNER)
coe5.wrap("AI_SPLIT_ARMY", function(next, c, minUnits)
  local com = comOf(c)
  local before = com and followerCount(c)
  splitRan = false
  local r = pack(next(c, minUnits))
  if com then
    local after = followerCount(c)
    if splitRan and after < before then
      com.troops[#com.troops + 1] = string.format("split off %d of %d units", before - after, before)
    end
  end
  return unpack(r, 1, r.n)
end, OUTER)
coe5.wrap("AI_SPLIT_ARMY", function(next, c, minUnits)
  splitRan = true
  return next(c, minUnits)
end, INNER)

-- Buys: the offers and gold before and after the AI's recruiting step.
local function offerCounts(s)
  local out = {}
  for _, o in ipairs(coe5.shopOffers(s)) do
    out[o.offer] = {count = o.count, name = clean(coe5.get("monsterName", o.monster) or "?"), gold = o.gold or 0}
  end
  return out
end
-- The rituals cast between two castCounts, as "Raise Dead x2".
function Records.castsBetween(before, after, nameOf)
  local out = {}
  for ritual, n in pairs(after) do
    local d = n - (before[ritual] or 0)
    if d > 0 then out[#out + 1] = {ritual = ritual, text = nameOf(ritual) .. (d > 1 and (" x" .. d) or "")} end
  end
  table.sort(out, function(a, b) return a.text < b.text end)
  return out
end

-- The buys between two offerCounts, "2 Spearman (50 gold each)", and how many units in all.
function Records.buysBetween(before, after)
  local buys, units = {}, 0
  for offer, b in pairs(before) do
    local a = after[offer]
    local n = b.count > 0 and (b.count - (a and a.count or 0)) or (a and 0 or 1)
    if n > 0 then
      buys[#buys + 1] = {offer = offer, n = n, text = string.format("%d %s (%d gold each)", n, b.name, b.gold)}
      units = units + n
    end
  end
  table.sort(buys, function(x, y) return x.text < y.text end)
  return buys, units
end
-- The buys of the whole step against the game's own.
function Records.splitBuys(all, gameBuys, who)
  local fromGame, out = {}, {}
  for _, b in ipairs(gameBuys) do fromGame[b.offer] = b.n end
  for _, b in ipairs(all) do
    local g = math.min(fromGame[b.offer] or 0, b.n)
    local name = b.text:match("^%d+ (.*)$")
    if g > 0 then out[#out + 1] = g .. " " .. name end
    if b.n > g then out[#out + 1] = (b.n - g) .. " " .. name .. ", bought by " .. who end
  end
  return out
end
local recruitGameRan, recruitGame
-- Each commander's ritual plan at the player's turn start, against the same when its AI move begins:
function ritualPlansOf(s)
  local out = {slot = s}
  for c in pairs(commandersOf(s)) do out[c] = coe5.get("commanderRitualPlan", c) or 0 end
  return out
end
coe5.wrap("AI_RECRUIT", function(next, player)
  local recording = rec and rec.slot == player
  local before = recording and offerCounts(player)
  -- Who made this turn's script buys and plans (coe5.aiActions; an older CoE5SE without it: "a script").
  local actions = rawget(coe5, "aiActions") and coe5.aiActions(player) or nil
  if recording and plansAtStart and plansAtStart.slot == player then
    for c, was in pairs(plansAtStart) do
      local now = c ~= "slot" and coe5.get("commanderRitualPlan", c) or 0
      local com = c ~= "slot" and now ~= was and now > 0 and comOf(c)
      if com then
        com.plan_set = clean(coe5.get("ritualName", now) or ("ritual " .. now)) .. " by "
          .. (Records.actorsOf(actions, "planRitual", c) or "a script"):gsub(",", ", ") .. " before its AI move"
      end
    end
  end
  plansAtStart = nil
  if recording then   -- the plans as the AI move begins, every script's at AI_TURN included: who casts what
    for c in pairs(commandersOf(player)) do rec.plansAtMove[c] = coe5.get("commanderRitualPlan", c) or 0 end
  end
  local gold = coe5.get("slotResource", player, 0)
  recruitGameRan, recruitGame = false, nil
  local r = pack(next(player))
  local by = changers(nil)
  if before then
    local buyers = Records.actorsOf(rawget(coe5, "aiActions") and coe5.aiActions(player) or nil, "aiBuyOffer")
    buyers = buyers and (buyers:gsub(",", ", ")) or nil
    local after = offerCounts(player)
    local all, units = Records.buysBetween(before, after)
    local gameBuys, gameUnits = {}, 0   -- the game's own step never ran: every buy is a script's
    if recruitGame then gameBuys, gameUnits = recruitGame.buys, recruitGame.units end
    local buys = Records.splitBuys(all, gameBuys, buyers or by or "a script")
    local left = {}
    for _, a in pairs(after) do
      if a.count ~= 0 then left[#left + 1] = string.format("%s%s, %d gold", a.count > 0 and (a.count .. " ") or "", a.name, a.gold) end
    end
    table.sort(left)
    rec.buys, rec.offers_left, rec.gold_at_buy = buys, left, gold
    local scripted = recruitGameRan and units - gameUnits or 0
    rec.buy_step = recruitGameRan and ((by and ("ran; changed by " .. by) or "ran")
      .. (scripted > 0 and string.format("; %d bought by %s", scripted, buyers or by or "a script") or "")) or ("skipped by " .. (by or "a script"))
  end
  return unpack(r, 1, r.n)
end, OUTER)
coe5.wrap("AI_RECRUIT", function(next, player)
  recruitGameRan = true
  local before = rec and rec.slot == player and offerCounts(player)
  local r = pack(next(player))
  if before then
    local buys, units = Records.buysBetween(before, offerCounts(player))
    recruitGame = {buys = buys, units = units}
  end
  return unpack(r, 1, r.n)
end, INNER)

-- The world's squares owned by and citadels of player s.
local tradeItems
local function landOf(s)
  local land, citadels = 0, 0
  for x = 0, coe5.get("mapDepth") - 1 do
    for y = 0, coe5.get("mapH") - 1 do
      if coe5.get("squareOwner", x, y) == s then
        land = land + 1
        if coe5.get("squareCitadel", s, x, y) then citadels = citadels + 1 end
      end
    end
  end
  return land, citadels
end

-- What the next turn start's trade step moves for player s.
function tradeItems(s)
  local out = {}
  for res = 0, 13 do
    local ok, n = pcall(coe5.get, "slotAiTrade", s, res)
    local name = clean(coe5.get("resourceName", res) or ("resource " .. res))
    if ok and n and n ~= 0 and name ~= "Trade" then out[#out + 1] = name .. " " .. n end   -- the trade points it spends: left out
  end
  return out
end

-- AI_TURN's after: what the AI's move left of each commander.
local pending = {}   -- the records whose battles are still to come, oldest first
function Ins.closeAiMove(player)
  local r = rec
  rec = nil
  if not (r and game and r.slot == player) then return end
  local alive = commandersOf(player)
  for c, unit in pairs(alive) do
    if not r.coms[c] then r.coms[c] = newCommander(c, unit) end   -- new this turn: bought or joined
  end
  for c, com in pairs(r.coms) do
    local unit = alive[c]
    if unit then
      local x, y = coe5.get("unitX", unit), coe5.get("unitY", unit)
      com.moved_to = x .. " " .. y   -- where its move ended, before the battles
      com.kind = coe5.get("commanderAiMoveKind", c) or -1
      local tx, ty = coe5.get("commanderAiTargetX", c) or -1, coe5.get("commanderAiTargetY", c) or -1
      com.target = tx .. " " .. ty
      if tx >= 0 then com.target_place, com.target_seen = placeOf(tx, ty), seenText(player, tx, ty) end
      local plan = coe5.get("commanderRitualPlan", c) or 0
      com.ritual_plan = plan > 0 and clean(coe5.get("ritualName", plan) or "") or (plan < 0 and "raise a spawn" or "")
      local follows = coe5.get("unitCommander", unit) or -1
      if follows >= 0 and follows ~= c then com.follows, com.follows_name = follows, commanderLabel(follows) end
      local last = com.plans[#com.plans]
      com.strength = last and last.comstr or -1
      local mods, seenMod = {}, {}
      for _, p in ipairs(com.plans) do
        for _, step in ipairs(STEPS) do
          for name in tostring(p[step] and p[step].by or ""):gmatch("[^,]+") do
            if name ~= "you" and name ~= "Play as AI" and not seenMod[name] then
              seenMod[name] = true
              mods[#mods + 1] = name
              r.scripts = r.scripts or {}
              if not r.scripts[name] then
                r.scripts[name] = true
                r.scriptOrder = r.scriptOrder or {}
                r.scriptOrder[#r.scriptOrder + 1] = name
              end
            end
          end
        end
      end
      com.played_by = (r.played_by ~= "AI" and touched[c]) and "you" or (#mods > 0 and ("AI + " .. table.concat(mods, ", ")) or "AI")
      local flags = coe5.get("unitMoveFlags", unit) or 0
      com.skip = ((flags >> 25) & 1 == 1 and "stupid " or "") .. ((flags >> 29) & 1 == 1 and "loner " or "")
        .. ((flags >> 62) & 1 == 1 and "hero for hire " or "") .. (coe5.get("unitStationary", unit) == true and "stationary" or "")
    else
      com.finish, com.finish_place = "-1 -1", "during its AI move"
    end
  end
  if r.scriptOrder and r.played_by == "AI" then r.played_by = "AI + " .. table.concat(r.scriptOrder, ", ") end
  pending[#pending + 1] = r
end

-- A record whose turn's battles are over, written into the data area.
function Ins.writeSettled(r)
  local player = r.slot
  local alive = commandersOf(player)
  r.buys = r.buys or {}
  for i, b in ipairs(r.partnerBuys or {}) do table.insert(r.buys, i, b) end
  game.lastTurnOf[r.slot] = math.max(game.lastTurnOf[r.slot] or -1, r.turn)
  for c, unit in pairs(alive) do
    if not r.coms[c] then r.coms[c] = newCommander(c, unit) end
  end
  r.gold_end, r.iron_end = coe5.get("slotResource", player, 0), coe5.get("slotResource", player, 1)
  r.income_gold, r.income_iron = coe5.get("slotIncome", player, 0), coe5.get("slotIncome", player, 1)
  local resEnd = Ins.resNow(player)
  r.res_end, r.res_income = Records.packRes(resEnd, "stock"), Records.packRes(resEnd, "income", true)
  r.trade = (isController(player, coe5.controller.ai) or isController(player, coe5.controller.human)) and tradeItems(player) or {}
  r.land, r.citadels = landOf(player)
  r.leader = coe5.get("slotLeader", player)
  if r.leader >= 0 then r.leader_name = commanderLabel(r.leader) end
  local n = 0
  for c, com in pairs(r.coms) do
    local unit = alive[c]
    if unit then
      n = n + 1
      local x, y = coe5.get("unitX", unit), coe5.get("unitY", unit)
      com.finish, com.finish_place = x .. " " .. y, placeOf(x, y)
    elseif com.moved_to then
      local mx, my = com.moved_to:match("^(%-?%d+) (%-?%d+)$")
      com.finish, com.finish_place = "-1 -1", "in the battles after its AI move, at " .. placeOf(tonumber(mx), tonumber(my))
    end
    com.moved_to = nil
  end
  r.commanders = n
  -- The rituals the turn cast, each on the commander that planned it.
  local casts = Records.castsBetween(r.castsAtStart or {}, Ins.castCounts(player),
    function(ritual) return clean(coe5.get("ritualName", ritual) or ("ritual " .. ritual)) end)
  r.casts = {}
  for _, cast in ipairs(casts) do
    r.casts[#r.casts + 1] = cast.text
    for c, plan in pairs(r.plansAtMove or {}) do
      local com = r.coms[c]
      if plan == cast.ritual and com then com.casts = (com.casts and (com.casts .. ", ") or "") .. cast.text end
    end
  end
  coe5.dataFolder(game.id)
  local key = recordName(r.turn, r.slot)
  if not coe5.writeIni(key, Records.toIni(r)) then   -- CoE5SE logged why (lua_refused)
    coe5.log(string.format("inspector_record_not_written turn=%d player=%d", r.turn, r.slot))
  end
  game.cache[key] = nil
end

-- At a player's turn start and the month's start: the AI turns before it are over, battles included.
function Ins.settleRecords()
  local list = pending
  pending = {}
  if not game then return end
  for _, r in ipairs(list) do Ins.writeSettled(r) end
end

-- ---- What the screens show -----------------------------------------------------------------------
local viewSlot   -- the AI whose records the screens show first
local lastCommander   -- {slot, c}: the commander screen opened last, for its arrows

local function aiSlots()
  local out = {}
  if playAs and playAs.active then return {playAs.slot} end
  for s = 0, 31 do
    if isController(s, coe5.controller.ai) then out[#out + 1] = s end
  end
  return out
end

-- The AIs with records that are no longer in the game (eliminated), by slot.
local function goneSlots()
  local out = {}
  if not game or (playAs and playAs.active) then return out end
  for s = 0, 31 do
    if game.lastTurnOf[s] and not isController(s, coe5.controller.ai) and not isController(s, coe5.controller.human) then
      out[#out + 1] = s
    end
  end
  return out
end

-- The Observer's map shows `slot`'s sight (nil: the seat's own). Only on the Observer's own turn:
function Ins.observerSeatNow()
  local seat = coe5.get("activePlayer")
  if not seat or seat < 0 or (playAs and playAs.active) then return nil end
  local cls = coe5.get("slotClass", seat)
  if not (cls and cls > 0 and coe5.get("className", cls) == "Observer") then return nil end
  return seat
end

-- The Observer's map shows `slot`'s sight (nil: its own view).
local function showView(slot)
  local seat = Ins.observerSeatNow()
  if not seat then return end
  if slot then
    if coe5.showSight(seat, slot) then sightShown, seatKept = slot, true end
    return
  end
  sightShown = nil
  if Ins.revealOn and rawget(coe5, "revealMap") then
    if not seatKept then   -- keep the seat's own sight first: any AI's sight shown, then revealed over
      local other = nil
      for s2 = 0, 31 do
        if s2 ~= seat and isController(s2, coe5.controller.ai) then other = s2 break end
      end
      seatKept = other ~= nil and coe5.showSight(seat, other) or false
    end
    if seatKept then coe5.revealMap(seat) end
  elseif seatKept then
    coe5.restoreSight(seat)
    seatKept = false
  end
end

-- The Observer's own view is the whole map, every plane (coe5.revealMap), during its turns only.
Ins.revealOn = true

-- The view of the Observer's turn, at its start.
function Ins.applyView(turnStart)
  if not Ins.observerSeatNow() then return end
  if Ins.appliedTurn == coe5.turn() and not turnStart then return end
  Ins.appliedTurn = coe5.turn()
  if sightShown and not isController(sightShown, coe5.controller.ai) then
    viewGone = {turn = coe5.turn(), text = (playerLabel(sightShown)) .. " has been eliminated. The map shows the Observer's own view."}
    sightShown = nil
  end
  showView(sightShown)
end

-- The Observer's turn ends: its own sight back for the AI turns (sightShown is kept for the next turn).
function Ins.endObserverTurn()
  local seat = Ins.observerSeatNow()
  if seat and seatKept then
    local shown = sightShown
    coe5.restoreSight(seat)
    seatKept, sightShown = false, shown
  end
end

-- The scope line while an AI is played ("Play as AI: X only"); nil otherwise.
local function scopeName()
  if playAs and playAs.active then
    return "Play as AI: " .. (playerLabel(playAs.slot)) .. " only"
  end
end

-- The two fixed status lines every screen shows.
local function statusLines()
  local recording
  local gaps = game and game.offTurns ~= "" and (" (off: " .. Records.spansText(game.offTurns) .. ")") or ""
  if not settings.recording then
    recording = "Recording off (Settings)"
  elseif game and game.firstRecorded then
    recording = string.format("Recording every AI's turns since turn %d%s", game.firstRecorded, gaps)
  else
    recording = "Recording every AI's turns from its next turn"
  end
  return "Read only: the AIs play as they would without the AI Inspector.", recording .. "." .. (scopeName() and (" " .. scopeName() .. ".") or "")
end

-- Why there is no record of `turn`.
local function missingReason(turn)
  if game and game.firstRecorded and turn < game.firstRecorded then
    return string.format("before turn %d, the first with recording on", game.firstRecorded)
  end
  for from, to in tostring(game and game.offTurns or ""):gmatch("(%d+)%-(%d+)") do
    if turn >= tonumber(from) and turn <= tonumber(to) then return "recording off then" end
  end
  return "not recorded (recording off, or no AI turn)"
end

-- Every key, the same on every screen.

local ID_K_BACK, ID_K_NEXT_AI, ID_K_NEXT_COM, ID_K_PREV_COM, ID_K_OLDER, ID_K_LATER, ID_K_NEXT_PAGE, ID_K_PREV_PAGE,
  ID_K_OFFERS, ID_K_SETTINGS = 9, 11, 12, 13, 14, 15, 16, 17, 18, 19
local ID_AI_FIRST, ID_COM_FIRST = 100, 1000
local ID_TURN_FIRST = 50000   -- the commander screen's history lines: 50000 + the turn
local ID_VIEW_ARMY, ID_VIEW_FIRST = 64, 40000   -- View army: the commander screen's; commanders here's, 40000 + the row
-- A living commander's square now, or nil: where its army stands for View army.
function Ins.armySquare(c)
  local unit = coe5.get("commanderUnit", c) or -1
  if unit < 0 or (coe5.get("unitHp", unit) or 0) <= 0 then return nil end
  return coe5.get("unitX", unit), coe5.get("unitY", unit)
end
-- The game's army view of commander c's army.
function Ins.viewArmy(c)
  local x, y = Ins.armySquare(c)
  local view = rawget(coe5, "viewArmy")
  if x and view then view(x, y) end
end

-- A screen line cut at spaces into lines that fit its box.
local SAY_CHARS = 150
function Ins.say(id, text)
  for _, piece in ipairs(Records.wrapText(text, SAY_CHARS)) do coe5.ui.text(id, piece) end
end

local function button(id, label, key)
  coe5.ui.text(id, key and string.format("%s (%s)", label, key) or label)
  if key == "Esc" then coe5.ui.bindEscape(id) elseif key then coe5.ui.bindKey(id, string.byte(key)) end
end

-- A key with no button line: the key strip names it (the button row stacks one line per button).
local function bindOnly(id, key) coe5.ui.bindKey(id, string.byte(key)) end

local function keyStrip(keys) coe5.ui.text(0, "Keys: " .. keys .. ", Esc back") end

local SCREEN_WIDTH = 860   -- of the game's 914 UI units; text wraps a little past the sections, so less than 880
local ROW_WIDTH = 800      -- a coloured row, inside its section
local ID_TIP = 21   -- a line with hover text: clicking it does nothing
local function frame(title)
  coe5.ui.screenBegin(title)
  coe5.ui.screenWidth(SCREEN_WIDTH)
  local a, b = statusLines()
  if not settings.recording then   -- the one status a screen can't show otherwise
    coe5.ui.text(ID_TIP, "Recording off (Settings)")
    coe5.ui.tooltip(a .. " " .. b)
  end
end

-- A row of cells at fixed columns, on a gold background when highlighted.
local function tableRow(id, columns, cells, highlighted, tip, tipCol)   -- tipCol: the hover text on that cell only
  if highlighted then coe5.ui.rowBegin(id, ROW_WIDTH, 0.878, 0.690, 0.282, 0.55) else coe5.ui.rowBegin(id, ROW_WIDTH) end
  for i, cell in ipairs(cells) do
    coe5.ui.columnX(columns[i])
    coe5.ui.text(id, cell)
    if tip and (not tipCol or tipCol == i) then coe5.ui.tooltip(tip) end
  end
  coe5.ui.rowEnd()
end

-- Go to a square: the map centred there, and every inspector screen closes.
Ins.goRows = {}   -- this frame's rows that go to a square: id -> {x, y, columns =, col =} or {x, y, span = {from, to}}
function Ins.goToSquare(x, y)
  Ins.goTo = {x = x, y = y}
  local centerMap = rawget(coe5, "centerMap")
  if centerMap then centerMap(x, y) end
  coe5.log(string.format("inspector_go_to x=%d y=%d", x, y))
end
-- A row (id) that goes to square (x, y).
function Ins.goRow(id, x, y, columns, col)
  if x and y then Ins.goRows[id] = {x, y, columns = columns, col = col} end
end
-- A screen's rows start this far left of the UI's middle (the screen is centred; 50 at the 914 wide UI).
Ins.ROW_FROM_MIDDLE = 407
function Ins.rowLeft() return (coe5.get("screenW") or 914) / 2 - Ins.ROW_FROM_MIDDLE end
-- After a screen's wait: a click on a place goes to its square; true when it did.
function Ins.clickGo(id)
  local rows = Ins.goRows
  Ins.goRows = {}
  local row = rows[math.abs(id)]
  if not row or id == 0 then return false end
  if id > 0 then
    local px = coe5.get("uiPointerX")
    local hit
    if not row.columns then hit = true   -- the place's own element
    else hit = Records.columnAt(row.columns, Ins.rowLeft(), ROW_WIDTH, px) == row.col end
    coe5.log(string.format("inspector_go_click id=%d px=%s hit=%s", id, tostring(px), tostring(hit)))
    if not hit then return false end
  end
  Ins.goToSquare(row[1], row[2])
  return true
end


local function names(by) return (tostring(by):gsub(",", ", ")) end

-- What the commander did, in words: where it holds or heads, or why it made no move.
local function resultLine(com)
  local kind = tonumber(com.kind) or -1
  local where = com.finish_place or "?"
  if com.finish == "-1 -1" then return "Died " .. (com.finish_place or "in this AI turn") end
  if com.played_by == "you" then return "Moved by hand; the AI left it alone. At " .. where end
  if com.follows then return string.format("Follows %s; makes no plan of its own. At %s", com.follows_name or "?", where) end
  local seen = com.target_seen and com.target_seen ~= "" and (" (" .. seenPhrase(com.target_seen, "its", "it") .. ")") or ""
  if kind == 1 then return string.format("Heading to %s%s", com.target_place or "?", seen) end
  if kind == 2 then return string.format("Exploring toward %s%s", com.target_place or "?", seen) end
  if com.target and com.target ~= "-1 -1" and com.finish == com.target then return "On its target, " .. where end
  if #com.plans == 0 then
    if com.skip and com.skip ~= "" then
      return string.format("No plan (%s). At %s", Records.skipText(com.skip), where)
    end
    return "No plan this turn. At " .. where
  end
  local moved = com.start and com.finish and com.start ~= com.finish
  local why = Records.restReason(com)
  return (moved and "Moved to " or "Rested at ") .. where .. (why and ("; " .. why) or "")
end

-- The step grid of one AI over its last HISTORY_TURNS turns up to `turn`.
local function aiGrid(slot, turn)
  local turns, byTurn = {}, {}
  for t = math.max(turn - HISTORY_TURNS + 1, 0), turn do
    turns[#turns + 1] = t
    byTurn[t] = readRecord(t, slot)
  end
  local list, inList = {}, {}
  local r = byTurn[turn]
  for _, com in pairs(r and r.coms or {}) do
    if com.finish ~= "-1 -1" then
      list[#list + 1] = com
      inList[com.c] = true
    end
  end
  table.sort(list, function(a, b)
    local fa, fb = a.follows and 1 or 0, b.follows and 1 or 0
    if fa ~= fb then return fa < fb end
    return a.c < b.c
  end)
  -- Commanders of the earlier turns that died or left: after the living, newest first.
  for i = #turns, 1, -1 do
    local tr = byTurn[turns[i]]
    local more = {}
    for _, com in pairs(tr and tr.coms or {}) do
      if not inList[com.c] then
        more[#more + 1] = com
        inList[com.c] = true
      end
    end
    table.sort(more, function(a, b) return a.c < b.c end)
    for _, com in ipairs(more) do list[#list + 1] = com end
  end
  local ids = {}
  for i, com in ipairs(list) do ids[i] = com.c end
  local rows, totals = Records.grid(byTurn, turns, ids)
  return turns, rows, totals, list, byTurn
end

-- The flags the mod counts for a turn, as facts.
local function flagsOf(r)
  local flags = {}
  if (r.leader or -1) < 0 then flags[#flags + 1] = "leader dead, still playing" end
  if r.buy_step == "ran" and #r.buys == 0 and (r.gold_at_buy or 0) >= 200 then
    flags[#flags + 1] = string.format("%d gold at recruiting, nothing bought", r.gold_at_buy)
  end
  local changed = 0
  for _, com in pairs(r.coms) do
    for _, plan in ipairs(com.plans) do
      for _, step in ipairs(STEPS) do
        if plan[step] and plan[step].by then changed = changed + 1 end
      end
    end
  end
  if changed > 0 then flags[#flags + 1] = Records.count(changed, "step") .. " changed by a mod or by hand" end
  return flags
end

-- The AI table's cells for one AI, and the turn box's lines.
local landNow = {}   -- {["turn:slot"] = {land, citadels}} for AIs with no record yet
local function aiInfo(s)
  local name, class = playerLabel(s)
  local turn, r = latestRecord(s)
  local human = isController(s, coe5.controller.human)
  local gone = not human and not isController(s, coe5.controller.ai)
  local function income(res) return coe5.get("slotAiIncome", s, res) or coe5.get("slotIncome", s, res) or 0 end
  if not r then
    if gone then return {name .. ", " .. class, "not recorded"}, nil end
    -- No record yet (a fresh folder, a first played turn): the numbers now (land counted once a turn).
    local key = coe5.turn() .. ":" .. s
    landNow[key] = landNow[key] or {landOf(s)}
    local land, citadels = landNow[key][1], landNow[key][2]
    local n = 0
    for _ in pairs(commandersOf(s)) do n = n + 1 end
    local cells = {name .. ", " .. class, string.format("%d (%+d)", coe5.get("slotResource", s, 0), income(0)),
      string.format("%d (%+d)", coe5.get("slotResource", s, 1), income(1)), tostring(land), tostring(citadels), tostring(n), ""}
    return cells, {"No AI turn yet: the numbers above are the current ones.", Records.tradeText(tradeItems(s))}, nil
  end
  local flags = flagsOf(r)
  if gone then table.insert(flags, 1, string.format("eliminated after turn %d", turn)) end
  local gold = gone and (r.gold_end or 0) or coe5.get("slotResource", s, 0)
  local iron = gone and (r.iron_end or 0) or coe5.get("slotResource", s, 1)
  local cells = {name .. ", " .. class, string.format("%d (%+d%s)", gold, gone and (r.income_gold or 0) or income(0),
    human and string.format("; human %+d", coe5.get("slotIncome", s, 0) or 0) or ""),
    string.format("%d (%+d)", iron, gone and (r.income_iron or 0) or income(1)), tostring(r.land), tostring(r.citadels),
    tostring(r.commanders), gone and "eliminated" or (#flags > 0 and ("! " .. #flags) or "")}
  local prev = readRecord(turn - 1, s)
  local changes = {}
  if prev then
    local function diff(label, a, b) if a and b and a ~= b then changes[#changes + 1] = string.format("%+d %s", a - b, label) end end
    diff("land", r.land, prev.land)
    diff("citadels", r.citadels, prev.citadels)
    diff("commanders", r.commanders, prev.commanders)
    if (prev.leader or -1) >= 0 and (r.leader or -1) < 0 then changes[#changes + 1] = "leader died" end
  end
  if #r.buys > 0 then changes[#changes + 1] = "bought " .. Records.whoText(table.concat(r.buys, ", ")) end
  if #(r.casts or {}) > 0 then changes[#changes + 1] = "cast " .. table.concat(r.casts, ", ") end
  local age = coe5.turn() - 1 - turn
  local lines = {
    string.format("%s%s, played by %s. Changes: %s.", turnLabel(turn),
      (not gone and age > 0) and string.format(" (%s ago; turn %d: %s)", Records.count(age, "turn"), turn + 1, missingReason(turn + 1)) or "",
      Records.whoText(r.played_by or "AI"),
      #changes > 0 and table.concat(changes, ", ") or (prev and "none" or string.format("unknown (turn %d: %s)", turn - 1, missingReason(turn - 1)))),
    #flags > 0 and ("Flags: " .. table.concat(flags, "; ") .. ".") or "",
    string.format("End of turn %d, after its battles: %s.", turn, Records.resJoin(Records.resFromRecord(r))) .. (human and string.format(" As a human now: %+d, with no AI bonus.",
      coe5.get("slotIncome", s, 0) or 0) or ""),
  }
  -- The trade: now for a living AI (its next turn start is ahead); none for an eliminated one.
  if not gone then
    lines[#lines + 1] = Records.tradeText(tradeItems(s)) .. " Trade and the AI's own moves change the gold after income."
  end
  return cells, lines, turn
end

local openReasoning, openOffers, openSettings, openOverview

do   -- the overview: its locals end with it
-- A text cut to `n` characters with "..".
function Ins.chars(text, n) return #text > n and (text:sub(1, n - 2) .. "..") or text end
local overviewPages   -- the grid's pages as last drawn
-- The overview: every AI in one table, then the shown AI's summary and grid.
local OV_COLUMNS = {0, 95, 195, 265, 330, 380, 430, 495, 540, 705}
local GRID_COLUMNS = {0, 230, 320, 410, 500, 590, 680, 760}
function Ins.aiRow(s)
  local name, class = playerLabel(s)
  local turn, r = latestRecord(s)
  local alive = isController(s, coe5.controller.ai) or isController(s, coe5.controller.human)
  if not alive then return {name, class, r and string.format("eliminated after turn %d", turn) or "eliminated"} end
  local res = Ins.resNow(s)
  local cells = {name, class, Records.resText(res[1].stock, res[1].income), Records.resText(res[2].stock, res[2].income)}
  cells[9] = Ins.chars(Records.resJoin(res, 3), 28)
  if not r then
    cells[5] = "no AI turn yet"
    for i = 6, 8 do cells[i] = "" end
    return cells
  end
  local prev = readRecord(turn - 1, s)
  cells[5] = tostring(r.land or "") .. ((prev and r.land and prev.land and r.land ~= prev.land) and string.format(" %+d", r.land - prev.land) or "")
  cells[6], cells[7] = tostring(r.citadels or ""), tostring(r.commanders or "")
  local units = 0
  for _, b in ipairs(r.buys) do units = units + (tonumber(b:match("^(%d+)")) or 1) end
  cells[8] = units > 0 and tostring(units) or ""
  if (r.leader or -1) < 0 then cells[10] = "! leader dead"
  elseif r.buy_step == "ran" and #r.buys == 0 and (r.gold_at_buy or 0) >= 200 then cells[10] = string.format("! %d gold, no buy", r.gold_at_buy) end
  return cells
end

function openOverview()
  Ins.goTo = nil
  local page = 0
  while true do
    if Ins.closeScreens then return end   -- a sub-screen ended the turn
    local slots = aiSlots()
    for _, s in ipairs(goneSlots()) do slots[#slots + 1] = s end   -- eliminated AIs after the living
    local at = 1
    for i, s in ipairs(slots) do if s == viewSlot then at = i end end
    viewSlot = slots[at]
    local partner = playAs and playAs.active
    frame("AI Inspector")
    coe5.ui.sectionBegin(string.format("After turn %d", math.max(coe5.turn() - 1, 0)))
    Ins.headRow(OV_COLUMNS, {"Player", "Class", "Gold", "Iron", "Land", "Citadels", "Cmdrs", "Bought", "Other", "Flags"},
      "Gold, iron and every other resource now, each with its income as the game pays the AI; the rest from its last turn. Land: the change since the turn before.")
    for _, s in ipairs(slots) do tableRow(ID_AI_FIRST + s, OV_COLUMNS, Ins.aiRow(s), s == viewSlot) end
    if #slots == 0 then Ins.say(0, "No AI players") end
    coe5.ui.sectionEnd()
    -- The shown AI's section, at a fixed size.
    local grid = 0
    local turn, r, lines
    if viewSlot then
      local name, class = playerLabel(viewSlot)
      turn, r = latestRecord(viewSlot)
      lines = select(2, aiInfo(viewSlot))
      coe5.ui.sectionBegin(string.format("%s, %s: commanders, the step that won each turn", name, class))
    else
      coe5.ui.sectionBegin("Commanders")
    end
    if r then
      local alive = isController(viewSlot, coe5.controller.ai) or isController(viewSlot, coe5.controller.human)
      -- The other resources, in one line.
      local others = Records.resJoin(alive and Ins.resNow(viewSlot) or Records.resFromRecord(r), 3)
      Ins.say(0, Ins.chars(string.format("Turn %d: %s (%s gold at recruiting).%s %s", turn,
        #r.buys > 0 and ("bought " .. Records.whoText(table.concat(r.buys, ", "))) or "bought nothing", tostring(r.gold_at_buy or "?"),
        others ~= "" and (" " .. others .. ".") or "",
        isController(viewSlot, coe5.controller.ai) and Records.tradeText(tradeItems(viewSlot)) or ""), 140))
      Ins.say(0, Ins.chars((lines and lines[1] or ""):gsub("^Turn %d+", ""):gsub("^[,%s]*", ""):gsub("^%l", string.upper)
        .. " " .. (lines and lines[2] or ""), 140))   -- one line, as the first
    else
      Ins.say(0, lines and lines[1] or "No AI players")
      Ins.say(0, lines and lines[2] or "")
    end
    local turns, rows, totals, list = {}, {}, {}, {}
    if r then turns, rows, totals, list = aiGrid(viewSlot, turn) end
    local perPage = math.max(3, 19 - #slots)
    local pages = math.max(1, math.ceil(#rows / perPage))
    if page >= pages then page = 0 end   -- the page line's click wraps to the first
    overviewPages = pages
    local heads = {}
    for i, t in ipairs(turns) do heads[i] = "Turn " .. t end
    local head = {"Commander"}
    for _, h in ipairs(Records.alignRight(heads, HISTORY_TURNS, "")) do head[#head + 1] = h end
    head[#head + 1] = "At"
    head[#head + 1] = "Played by"
    Ins.headRow(GRID_COLUMNS, head, "*: a step changed by a mod or by hand. died: in its own turn. gone: died or left the army between its turns. "
      .. "-: not recorded, or not in the army yet.")
    for _, i in ipairs(Records.pageRows(#rows, page, perPage)) do
      if i then
        local row, com = rows[i], list[i]
        local cells = {Ins.chars(com.name or "?", 34)}
        local turnCells = {}
        for k, cell in ipairs(row.cells) do turnCells[k] = (cell:gsub("^%u", string.lower)) end
        while #turnCells < #turns do turnCells[#turnCells + 1] = "-" end
        for _, cell in ipairs(Records.alignRight(turnCells, HISTORY_TURNS, "")) do cells[#cells + 1] = cell end
        cells[#cells + 1] = com.finish == "-1 -1" and "died" or Ins.chars((Ins.short(com.finish_place):gsub("^%-?%d+,%-?%d+ ", "")), 14)
        cells[#cells + 1] = Ins.chars(Records.whoText(com.played_by or "AI"), 16)
        local gx, gy = Records.squareAt(com.finish)
        Ins.goRow(ID_COM_FIRST + com.c, gx, gy, GRID_COLUMNS, HISTORY_TURNS + 2)
        tableRow(ID_COM_FIRST + com.c, GRID_COLUMNS, cells, lastCommander and lastCommander.slot == viewSlot and lastCommander.c == com.c,
          gx and Records.goText(com.finish_place), HISTORY_TURNS + 2)
        grid = grid + 1
      else
        tableRow(ID_TIP, GRID_COLUMNS, {""}, false)
      end
    end
    local parts = {}
    for _, k in ipairs({"follows", "conquer", "rally", "friendly", "explore", "far", "stays", "rested", "no plan", "you", "died", "gone"}) do
      if totals[k] then parts[#parts + 1] = (k == "you" and "human" or (STEP_NAME[k] or k):lower()) .. " " .. totals[k] end
    end
    local totalsText = r and string.format("Totals over %d turns: %s.", #turns, #parts > 0 and table.concat(parts, ", ") or "none")
      or "Totals: none."
    -- The line clicks to the next page when there is one; a row either way, so its height never changes.
    coe5.ui.rowBegin(0, ROW_WIDTH)
    coe5.ui.text(ID_TIP, totalsText)
    if pages > 1 then
      coe5.ui.columnX(560)
      coe5.ui.text(ID_TIP, string.format("Page %d of %d", page + 1, pages))
      coe5.ui.columnX(650)
      coe5.ui.text(ID_K_PREV_PAGE, "< Previous (z)")
      coe5.ui.columnX(735)
      coe5.ui.text(ID_K_NEXT_PAGE, "Next (x) >")
    end
    coe5.ui.rowEnd()
    coe5.ui.sectionEnd()
    Ins.say(0, partner and "Click a commander: its reasoning.   v offers and buys   e settings"
      or "Click a player: its commanders below and its view on the map. Click a commander: its reasoning.   g next AI   v offers   e settings")
    coe5.ui.buttonRow()
    if #slots > 1 then bindOnly(ID_K_NEXT_AI, "g") end
    bindOnly(ID_K_PREV_PAGE, "z")
    bindOnly(ID_K_NEXT_PAGE, "x")
    bindOnly(ID_K_OFFERS, "v")
    bindOnly(ID_K_SETTINGS, "e")
    button(ID_K_BACK, "Back", "Esc")
    local id = coe5.ui.wait()
    if Ins.closeScreens then return end
    if id == ID_K_BACK or Ins.clickGo(id) then return end
    if id == ID_K_NEXT_AI and #slots > 0 then
      viewSlot, page = slots[at % #slots + 1], 0
      if sightShown and isController(viewSlot, coe5.controller.ai) then showView(viewSlot) end
    elseif id == ID_K_NEXT_PAGE then page = (page + 1) % math.max(overviewPages or 1, 1)
    elseif id == ID_K_PREV_PAGE then page = (page - 1) % math.max(overviewPages or 1, 1)
    elseif id == ID_K_OFFERS then openOffers()
    elseif id == ID_K_SETTINGS then openSettings()
    elseif id >= ID_AI_FIRST and id < ID_AI_FIRST + 32 then
      viewSlot, page = id - ID_AI_FIRST, 0
      if isController(viewSlot, coe5.controller.ai) then showView(viewSlot) end
    elseif id >= ID_COM_FIRST then openReasoning(viewSlot, id - ID_COM_FIRST)
    end
    if Ins.goTo then return end
  end
end

end

local OFFERS_PER_LINE = 5
function openOffers()
  while true do
    if Ins.closeScreens then return end   -- a sub-screen ended the turn
    local turn, r = latestRecord(viewSlot or -1)
    local name = viewSlot and (playerLabel(viewSlot)) or "?"
    frame("AI Inspector: offers")
    coe5.ui.sectionBegin(string.format("%s, %s", name, turn and ("after turn " .. turn) or "not recorded"))
    if r then
      Ins.say(0, string.format("Gold at recruiting %s. Recruiting: %s.", tostring(r.gold_at_buy or "?"), r.buy_step or "not reached"))
      Ins.say(0, "Bought: " .. (#r.buys > 0 and Records.whoText(table.concat(r.buys, ", ")) or "nothing"))
      coe5.ui.text(ID_TIP, "Recruitment offers left (count, unit, gold each):" .. (#r.offers_left == 0 and " none" or ""))
      coe5.ui.tooltip("The offers change at each player's turn start: these are turn " .. turn .. "'s. The list holds every offer, "
        .. "the location offers included, and each of those is bought only at its location.")
      for i = 1, #r.offers_left, OFFERS_PER_LINE do
        Ins.say(0, table.concat(r.offers_left, "; ", i, math.min(i + OFFERS_PER_LINE - 1, #r.offers_left)))
      end
    end
    coe5.ui.sectionEnd()
    keyStrip("g next AI")
    coe5.ui.buttonRow()
    bindOnly(ID_K_NEXT_AI, "g")
    button(ID_K_BACK, "Back", "Esc")
    local id = coe5.ui.wait()
    if Ins.closeScreens then return end
    if id == ID_K_BACK then return end
    if id == ID_K_NEXT_AI then
      local slots, at = aiSlots(), 1
      for i, s in ipairs(slots) do if s == viewSlot then at = i end end
      if #slots > 0 then viewSlot = slots[at % #slots + 1] end
    end
  end
end

-- The "changed by" of a commander's turn, for the history: the mods (or you) that changed any step.
local function changedByOf(com)
  local seen, out = {}, {}
  for _, p in ipairs(com.plans or {}) do
    for _, step in ipairs(STEPS) do
      for name in tostring(p[step] and p[step].by or ""):gmatch("[^,]+") do
        if not seen[name] then
          seen[name] = true
          out[#out + 1] = name
        end
      end
    end
  end
  return #out > 0 and table.concat(out, ", ") or "none"
end

-- Why commander c has no entry in slot's record of `turn`.
local function absentReason(slot, c, turn)
  for t = turn - 1, math.max(turn - 20, 0), -1 do
    local r = readRecord(t, slot)
    local com = r and r.coms[c]
    if com then
      local now = readRecord(turn, slot)
      if r.leader == c and now and now.leader ~= c then return string.format("the leader died after turn %d", t) end
      if com.finish == "-1 -1" then return string.format("died in turn %d", t) end
      return string.format("gone after turn %d: died or left the army", t)
    end
  end
  return "not in this AI's army yet"
end

do   -- the commander screen: its locals end with it
-- A table's head row: dark, the column names.
function Ins.headRow(columns, cells, tip)
  coe5.ui.rowBegin(0, ROW_WIDTH, 0.23, 0.17, 0.11, 0.16)
  for i, cell in ipairs(cells) do
    coe5.ui.columnX(columns[i])
    coe5.ui.text(ID_TIP, cell)
    if tip then coe5.ui.tooltip(tip) end
  end
  coe5.ui.rowEnd()
end

-- A place without its plane: "39,19 Silver Mine".
function Ins.short(place) return (tostring(place or ""):gsub(" %b()$", "")) end

-- One commander's turn: what it did, its move steps, its turns and its AI's commanders.
local STEP_COLUMNS = {0, 130, 200, 440, 580}
local TURN_COLUMNS = {0, 60, 190, 250, 450, 650, 730}
local COM_COLUMNS = {0, 300, 420, 620}
local HEAD_REST_X = 180   -- the header sentence's x, after its place
local ID_GO_HEAD, ID_GO_STEP = 22, 23   -- the header's place; the move steps' rows, 23 + the step's index
function openReasoning(slot, c)
  Ins.goTo = nil
  lastCommander = {slot = slot, c = c}
  local latest = latestRecord(slot)
  local turn = latest
  while turn do
    if Ins.closeScreens then return end
    local r = readRecord(turn, slot)
    local com = r and r.coms[c]
    local slotName = (playerLabel(slot))
    frame("AI Inspector: commander")
    local shownName = com and com.name
    for back = turn - 1, math.max(turn - 20, 0), -1 do   -- a dead commander's name, from its last record
      if shownName then break end
      local br = readRecord(back, slot)
      shownName = br and br.coms[c] and br.coms[c].name
    end
    coe5.ui.sectionBegin(string.format("%s, %s's commander, after turn %d", shownName or commanderLabel(c), slotName, turn))
    if not r or not com then   -- the same three lines as a recorded turn, so the screen keeps its size
      Ins.say(0, string.format("Turn %d: %s.", turn, r and absentReason(slot, c, turn) or missingReason(turn)))
      Ins.say(0, "Troops picked up or split off: -.")
      Ins.say(0, "Rituals considered for a plan: -.")
    else
      local moved = com.start and com.finish and com.start ~= com.finish and com.finish ~= "-1 -1"
      -- The header's place: the target it heads or explores toward, else where it ended.
      local toTarget = (tonumber(com.kind) or -1) == 1 or (tonumber(com.kind) or -1) == 2
      local hx, hy = Records.squareAt(toTarget and com.target or com.finish)
      local headText = string.format("%s.%s%s Played by %s.", resultLine(com), moved and (" Moved from " .. Ins.short(com.start_place) .. ".") or "",
        (tonumber(com.strength) or -1) >= 0 and (" Strength " .. com.strength .. ".") or "",
        Records.whoText(com.played_by or "AI"))
      -- The place, an element of its own at the line's start.
      local headPlace = tostring(toTarget and com.target_place or com.finish_place or "")
      local from, to = headText:find(headPlace, 1, true)
      if not from then from, to = headText:find(Ins.short(headPlace), 1, true) end
      coe5.ui.rowBegin(0, ROW_WIDTH)
      if hx and from then
        Ins.goRow(ID_GO_HEAD, hx, hy)
        coe5.ui.text(ID_GO_HEAD, Ins.chars(Ins.short(headPlace), 34))
        coe5.ui.tooltip(Records.goText(headPlace))
        coe5.ui.columnX(HEAD_REST_X)
        coe5.ui.text(0, Ins.chars(Records.headRest(headText, from, to), 115))
      else
        coe5.ui.text(0, Ins.chars(headText, 140))
      end
      coe5.ui.rowEnd()
      -- Each header line is cut to one line, so the screen keeps its three (a long buy list wrapped it).
      Ins.say(0, Ins.chars(string.format("Troops picked up or split off: %s. %s bought: %s. Gold %s at recruiting, %s at the turn's end.",
        #com.troops > 0 and Records.whoText(Records.counted(com.troops)) or "none", slotName,
        #r.buys > 0 and Records.whoText(table.concat(r.buys, ", ")) or "nothing", tostring(r.gold_at_buy or "?"), tostring(r.gold_end or "?")), 140))
      local bits = {}
      if #com.rituals > 0 then
        local counts, order, parts = {}, {}, {}
        for _, line in ipairs(com.rituals) do
          if not counts[line] then order[#order + 1] = line end
          counts[line] = (counts[line] or 0) + 1
        end
        for _, line in ipairs(order) do parts[#parts + 1] = Records.ritualText(line, counts[line]) end
        bits[#bits + 1] = "Rituals considered for a plan: " .. table.concat(parts, "; ") .. "."
      else
        bits[#bits + 1] = "Rituals considered for a plan: none."
      end
      if com.casts and com.casts ~= "" then bits[#bits + 1] = "Cast " .. com.casts .. "." end
      if com.plan_set and com.plan_set ~= "" then bits[#bits + 1] = "Ritual plan " .. com.plan_set .. "." end
      if com.ritual_plan and com.ritual_plan ~= "" then bits[#bits + 1] = "Ritual plan after the move: " .. com.ritual_plan .. "." end
      if #(r.casts or {}) > 0 and not (com.casts and com.casts ~= "") then bits[#bits + 1] = slotName .. " cast " .. table.concat(r.casts, ", ") .. "." end
      local by = changedByOf(com)
      if by ~= "none" then bits[#bits + 1] = Records.whoText("Steps changed by " .. by) .. "." end
      Ins.say(0, Ins.chars(table.concat(bits, " "), 140))
    end
    coe5.ui.sectionEnd()
    -- The move steps, at a fixed size.
    local plans = com and com.plans or {}
    local last = plans[#plans]
    local winner, best, nextScore, nextStep
    if last then
      winner, best = Records.pick(last, "final")
      nextScore, nextStep = Records.nextBest(last, winner)
    end
    local noPlan = not r and "not recorded" or not com and "-" or (Records.gridCell(com):gsub("^%u", string.lower))
    coe5.ui.sectionBegin(last and string.format("Move steps, plan %d of %d", #plans, #plans) or ("Move steps: " .. noPlan))
    do
      -- Planned again: the earlier plans go in the head row's hover text, so the rows stay put.
      local earlierText = ""
      if #plans > 1 then
        local earlier = {}
        for n = 1, #plans - 1 do
          local w, b = Records.pick(plans[n], "final")
          earlier[#earlier + 1] = string.format("plan %d: %s", n, w and (STEP_NAME[w] .. " " .. b .. " at " .. Ins.short(plans[n][w].place)) or "no step won")
        end
        earlierText = " Planned again: it reached its target with Action Points left (at most 4 plans a turn). Earlier: " .. table.concat(earlier, "; ") .. "."
      end
      Ins.headRow(STEP_COLUMNS, {"Step", "Score", "Square", slotName .. "'s sight", "Result"},
        "Higher wins: rally above 0, each later step above the best so far; a tie to the earlier step. Explore runs only while the best is under 50, far explore under 10."
        .. earlierText)
      for i, step in ipairs(STEPS) do
        local e = last and last[step]
        local why = last and Records.notRun(last, step)
        local cells = {STEP_NAME[step], "-", "", "", ""}
        local goId, tip = 0, nil
        if not last then
          cells[5] = "-"   -- the section's title says why
        elseif why then
          cells[5] = why
        elseif e then
          local f = e.final or {}
          cells[2] = tostring(f[1] or 0)   -- the Square column says "no square" for -999
          cells[3] = (f[2] or -1) >= 0 and Ins.short(e.place) or "no square"
          cells[4] = e.seen or ""
          if step == winner then cells[5] = "won"
          elseif step == nextStep then cells[5] = string.format("next best, %d lower", best - nextScore)
          elseif (f[2] or -1) < 0 then cells[5] = "no square"
          else cells[5] = "lower" end
          if e.by then cells[5] = cells[5] .. " (" .. Records.whoText("by " .. names(e.by)) .. "; the game: " .. (e.game and Records.answerWords(e.game) or "not run") .. ")" end
          if (f[2] or -1) >= 0 and (f[3] or -1) >= 0 then   -- the Square column goes there
            goId, tip = ID_GO_STEP + i, Records.goText(e.place)
            Ins.goRow(goId, f[2], f[3], STEP_COLUMNS, 3)
          end
        end
        tableRow(goId, STEP_COLUMNS, cells, step == winner, tip, 3)
      end
      coe5.ui.sectionEnd()
    end
    -- The turns around the one on screen, one row each.
    local first = math.max(game and game.firstRecorded or 0, 0)
    local newest = math.max(latest or turn, coe5.turn() - 1)   -- a turn not recorded since says why
    local window = Records.window(turn, math.min(first, turn), newest, 5)
    table.sort(window)
    coe5.ui.sectionBegin(string.format("Turns %d to %d: click one to show it (o / l)", window[1] or turn, window[#window] or turn))
    Ins.headRow(TURN_COLUMNS, {"Turn", "Won", "Score", "Target", "Ended at", "Played by", "Plans"})
    for _, t in ipairs(Records.fixedRows(window, 5)) do
      if not t then tableRow(ID_TIP, TURN_COLUMNS, {""}, false) goto nextTurn end
      local hr = readRecord(t, slot)
      local hc = hr and hr.coms[c]
      local cells
      if not hr then cells = {tostring(t), missingReason(t)}
      elseif not hc then cells = {tostring(t), absentReason(slot, c, t)}
      else
        local w, b
        if hc.plans[#hc.plans] then w, b = Records.pick(hc.plans[#hc.plans], "final") end
        cells = {tostring(t), w and STEP_NAME[w] or (Records.gridCell(hc):gsub("^%u", string.lower)),   -- the grid's word
          b and tostring(b) or "", hc.target and hc.target ~= "-1 -1" and Ins.short(hc.target_place) or "",
          hc.finish == "-1 -1" and "died" or Ins.short(hc.finish_place), hc.played_by == "you" and "human" or "AI",
          #hc.plans > 1 and (#hc.plans .. " (planned again)") or tostring(#hc.plans)}
      end
      local tx, ty = Records.squareAt(hc and hc.target)
      Ins.goRow(ID_TURN_FIRST + t, tx, ty, TURN_COLUMNS, 4)   -- the Target column
      tableRow(ID_TURN_FIRST + t, TURN_COLUMNS, cells, t == turn, tx and Records.goText(hc.target_place), 4)
      ::nextTurn::
    end
    coe5.ui.sectionEnd()
    -- Its AI's commanders that turn.
    local list = {}
    if r then list = select(4, aiGrid(slot, turn)) end
    do
      -- Always 6 rows, around the shown commander (empty ones fill a short list); n and j reach the rest.
      local at = 1
      for i, other in ipairs(list) do if other.c == c then at = i end end
      local from = math.max(1, math.min(at - 2, #list - 5))
      coe5.ui.sectionBegin(string.format("%s's commanders after turn %d: %s%s", slotName, turn, r and "click one" or missingReason(turn),
        #list > 6 and string.format(", %d to %d of %d", from, math.min(#list, from + 5), #list) or ""))
      local leader = r and r.leader
      local shown = {}
      for i = from, math.min(#list, from + 5) do shown[#shown + 1] = list[i] end
      for _, other in ipairs(Records.fixedRows(shown, 6)) do
        if not other then tableRow(ID_TIP, COM_COLUMNS, {""}, false) goto nextCommander end
        local cell = Records.gridCell(other)
        local note
        if other.follows then note = "follows " .. ((other.follows_name or ""):match("^(.-) the ") or other.follows_name or "?")
        elseif other.c == leader and #other.plans == 0 then note = "leader, no plan"
        elseif other.skip and other.skip:find("stationary") then note = "stationary"
        elseif (tonumber(other.strength) or -1) >= 0 then note = "strength " .. other.strength
        else note = "" end
        local ox, oy = Records.squareAt(other.finish)
        Ins.goRow(ID_COM_FIRST + other.c, ox, oy, COM_COLUMNS, 3)   -- the Ended at column
        tableRow(ID_COM_FIRST + other.c, COM_COLUMNS, {other.name or commanderLabel(other.c), (cell:gsub("^%u", string.lower)),
          other.finish == "-1 -1" and "died" or Ins.short(other.finish_place), note}, other.c == c, ox and Records.goText(other.finish_place), 3)
        ::nextCommander::
      end
      -- The list's two controls, each a click of its own (n and j too); a row either way.
      coe5.ui.rowBegin(0, ROW_WIDTH)
      if #list > 1 then
        coe5.ui.text(ID_K_PREV_COM, "< Previous commander (j)")
        coe5.ui.columnX(200)
        coe5.ui.text(ID_K_NEXT_COM, "Next commander (n) >")
      else
        coe5.ui.text(0, "")
      end
      if Ins.armySquare(c) and rawget(coe5, "viewArmy") then   -- the shown commander's army, where it stands now
        coe5.ui.columnX(420)
        coe5.ui.text(ID_VIEW_ARMY, "View army (v)")
        coe5.ui.tooltip("Its army on the battlefield, as a right click on it shows it")
        coe5.ui.bindKey(ID_VIEW_ARMY, string.byte("v"))
      end
      coe5.ui.rowEnd()
      coe5.ui.sectionEnd()
    end
    coe5.ui.buttonRow()
    bindOnly(ID_K_OLDER, "o")
    bindOnly(ID_K_LATER, "l")
    bindOnly(ID_K_NEXT_COM, "n")
    bindOnly(ID_K_PREV_COM, "j")
    button(ID_K_BACK, "Back", "Esc")
    local id = coe5.ui.wait()
    if Ins.closeScreens then return end
    if id == ID_K_BACK or Ins.clickGo(id) then return end
    if id == ID_VIEW_ARMY then Ins.viewArmy(c)   -- back on this screen after
    elseif id >= ID_TURN_FIRST then turn = id - ID_TURN_FIRST
    elseif id >= ID_COM_FIRST then c = id - ID_COM_FIRST; lastCommander = {slot = slot, c = c}
    elseif id == ID_K_OLDER and turn > 0 then turn = turn - 1
    elseif id == ID_K_LATER and latest and turn < latest then turn = turn + 1
    elseif (id == ID_K_NEXT_COM or id == ID_K_PREV_COM) and #list > 0 then
      for i, other in ipairs(list) do
        if other.c == c then
          c = list[(i + (id == ID_K_NEXT_COM and 0 or -2)) % #list + 1].c
          break
        end
      end
      lastCommander = {slot = slot, c = c}
    end
  end
end

end

-- The commanders of every AI on square (x, y): {{c =, slot =}}.
function Ins.aiCommandersAt(x, y)
  local found = {}
  for c = 0, coe5.get("commanderLastIndex") do
    local unit = coe5.get("commanderUnit", c)
    if unit >= 0 and coe5.get("unitHp", unit) > 0 and coe5.get("unitX", unit) == x and coe5.get("unitY", unit) == y then
      local owner = coe5.get("unitOwner", unit)
      if isController(owner, coe5.controller.ai) or (playAs and playAs.active and owner == playAs.slot) then found[#found + 1] = {c = c, slot = owner} end
    end
  end
  return found
end

-- The commanders of every AI on square (x, y), to pick one.
local CHOOSER_COLUMNS = {0, 280, 380, 480, 640, 745}
local CHOOSER_ROWS = 6   -- rows always drawn, empty ones included
local function openChooser(x, y)
  Ins.goTo = nil
  local found = Ins.aiCommandersAt(x, y)
  while true do
    if Ins.closeScreens then return end
    frame("AI Inspector: commanders here")
    coe5.ui.sectionBegin(string.format("AI commanders on %s: click one", Ins.short(placeOf(x, y))))
    local rows = {}
    for i, f in ipairs(found) do
      local _, r = latestRecord(f.slot)
      local com = r and r.coms[f.c]
      local note = ""
      if com and com.follows then note = "follows " .. ((com.follows_name or ""):match("^(.-) the ") or com.follows_name or "?")
      elseif com and (tonumber(com.strength) or -1) >= 0 then note = "strength " .. com.strength end
      local tx, ty = Records.squareAt(com and com.target)
      Ins.goRow(ID_COM_FIRST + i, tx, ty, CHOOSER_COLUMNS, 4)
      rows[i] = {id = ID_COM_FIRST + i, i = i, c = f.c, cells = {commanderLabel(f.c), (playerLabel(f.slot)),
        com and (Records.gridCell(com):gsub("^%u", string.lower)) or "not recorded", com and Ins.short(com.target_place) or "", note},
        tip = tx and Records.goText(com.target_place)}
    end
    for _, row in ipairs(Records.fixedRows(rows, CHOOSER_ROWS, {id = ID_TIP, cells = {"None"}})) do
      if row and row.c and Ins.armySquare(row.c) and rawget(coe5, "viewArmy") then   -- the row, then its View army cell
        coe5.ui.rowBegin(row.id, ROW_WIDTH)
        for k, cell in ipairs(row.cells) do
          coe5.ui.columnX(CHOOSER_COLUMNS[k])
          coe5.ui.text(row.id, cell)
          if k == 4 and row.tip then coe5.ui.tooltip(row.tip) end
        end
        coe5.ui.columnX(CHOOSER_COLUMNS[6])
        coe5.ui.text(ID_VIEW_FIRST + row.i, "View army")
        coe5.ui.tooltip("Its army on the battlefield, as a right click on it shows it")
        coe5.ui.rowEnd()
      elseif row then tableRow(row.id, CHOOSER_COLUMNS, row.cells, false, row.tip, 4) else tableRow(ID_TIP, CHOOSER_COLUMNS, {""}, false) end
    end
    coe5.ui.sectionEnd()
    coe5.ui.buttonRow()
    button(ID_K_BACK, "Back", "Esc")
    local id = coe5.ui.wait()
    if Ins.closeScreens then return end
    if id == ID_K_BACK or Ins.clickGo(id) then return end
    local v = found[id - ID_VIEW_FIRST]
    if id > ID_VIEW_FIRST and v then Ins.viewArmy(v.c)   -- back on this screen after
    else
      local f = id < ID_VIEW_FIRST and found[id - ID_COM_FIRST]
      if f then openReasoning(f.slot, f.c) end
    end
    if Ins.goTo then return end
  end
end

-- A confirmation: the title, its lines, Ok (Enter) or Cancel (Esc); true on Ok.
local ID_OK, ID_NO = 61, 62
function Ins.confirm(title, lines)
  while true do
    coe5.ui.screenBegin(title)
    coe5.ui.screenWidth(SCREEN_WIDTH)
    coe5.ui.sectionBegin(lines[1])
    for i = 2, #lines do Ins.say(0, lines[i]) end
    coe5.ui.sectionEnd()
    coe5.ui.buttonRow()
    coe5.ui.text(ID_OK, "Ok (Enter)")
    coe5.ui.bindKey(ID_OK, coe5.key.enter)
    coe5.ui.text(ID_NO, "Cancel (Esc)")
    coe5.ui.bindEscape(ID_NO)
    local id = coe5.ui.wait()
    if id ~= 0 then return id == ID_OK end   -- Ok; anything else (Cancel, Esc) leaves it as it was
  end
end

-- After a confirmed pick or Return: the turn ends at once.
function Ins.startHandover(mode)
  Ins.skip = {untilTurn = math.huge, total = 0, mode = mode, looks = Ins.lookAtAis(), frames = 0}
  Ins.skipNote, Ins.closeScreens = nil, true
  coe5.log(string.format("inspector_handover_start mode=%s turn=%d", mode, coe5.turn()))
end

-- Play as AI's picker (the same ids since AI Test Kit: ai/lib/playas.sh clicks 100 + the slot, then Ok, 61).
local function openPlayAs()
  while true do
    coe5.ui.screenBegin("Play as AI")
    coe5.ui.sectionBegin("AI players")
    local any = false
    for s = 0, 31 do
      if coe5.get("slotController", s) == coe5.controller.ai then
        any = true
        local name, class, level = playerLabel(s)
        local picked = playAs and playAs.slot == s
        coe5.ui.choice(ID_PLAYER_FIRST + s, picked, string.format("%s, %s (%s)%s", name, class, level, picked and ": from its next turn" or ""))
        coe5.ui.tooltip("Under human control from its next turn, with every AI bonus; the Observer's seat goes to the AI meanwhile")
      end
    end
    if not any then Ins.say(0, "No AI players") end
    coe5.ui.sectionEnd()
    coe5.ui.buttonRow()
    if playAs then coe5.ui.text(ID_CANCEL, "Clear") end
    coe5.ui.text(ID_BACK, "Back")
    coe5.ui.bindEscape(ID_BACK)
    local id = coe5.ui.wait()
    if Ins.closeScreens then return end
    if id == ID_BACK then return end
    if id == ID_CANCEL and playAs then endReview("cancelled") end
    local s = id - ID_PLAYER_FIRST
    if s >= 0 and s <= 31 and isController(s, coe5.controller.ai) then
      local name, class = playerLabel(s)
      if Ins.confirm("AI Inspector: Play as AI", {string.format("Play as %s, %s", name, class),
          "Ends the Observer's turn now. The other players move until " .. name .. "'s turn.",
          name .. "'s turn is then played by hand."}) then
        playAs = {slot = s, seat = coe5.get("activePlayer"), active = false}
        coe5.log(string.format("play_as_picked slot=%d seat=%d turn=%d", s, playAs.seat, coe5.turn()))
        Ins.startHandover("pick")
        return true
      end
    end
  end
end

local returnToAi
-- Play as Observer (returnToAi) asks first, then ends the played turn at once: true when it did.
function Ins.confirmReturn()
  local name = (playerLabel(playAs.slot))
  if not Ins.confirm("AI Inspector: Play as Observer", {"Play as Observer: " .. name .. " back to the AI",
      "This turn ends now.", "The Observer's seat is back at its next turn."}) then
    return false
  end
  returnToAi()
  Ins.startHandover("return")
  Ins.skip.name = name
  return true
end
function returnToAi()
  playAs.returning = true
  if isController(playAs.seat, coe5.controller.ai) then coe5.set("slotController", playAs.seat, coe5.controller.human) end
  coe5.log(string.format("play_as_returning slot=%d seat=%d turn=%d", playAs.slot, playAs.seat, coe5.turn()))
end

local function canPickPlayAs()
  return runAiTurn ~= nil and game ~= nil and not (playAs and playAs.active) and isController(coe5.get("activePlayer"), coe5.controller.human)
end
-- Whether the Observer's seat is still in the game. The game can remove it while it is the AI's (seen:
function Ins.seatInGame()
  return playAs ~= nil and (isController(playAs.seat, coe5.controller.ai) or isController(playAs.seat, coe5.controller.human))
end
local function canReturn()
  return playAs ~= nil and playAs.active and not playAs.returning and coe5.get("activePlayer") == playAs.slot and Ins.seatInGame()
end

local ID_S = {PLAY = 30, RETURN = 31, RECORDING = 32, LIST = 33, TOP_BAR = 34, ARROWS = 35, TAGS = 36, CARD = 37}
local ARROW_CHOICES = {[0] = "Arrows: none (a)", "Arrows: every commander's target (a)", "Arrows: the last commander opened, each step's square (a)"}
function openSettings()
  while true do
    if Ins.closeScreens then return end
    frame("AI Inspector: settings")
    coe5.ui.sectionBegin("Play as AI")
    if playAs and playAs.active then
      if canReturn() then coe5.ui.text(ID_S.RETURN, "Playing " .. (playerLabel(playAs.slot)) .. ": Play as Observer (r)")
      else Ins.say(0, "Playing " .. (playerLabel(playAs.slot)) .. (playAs.returning and ": back to the AI at this turn's end" or "")) end
      coe5.ui.bindKey(ID_S.RETURN, string.byte("r"))
    elseif canPickPlayAs() then
      coe5.ui.text(ID_S.PLAY, playAs and ("Play as AI: " .. (playerLabel(playAs.slot)) .. " from its next turn, change (p)") or "Play as AI (p)")
      coe5.ui.bindKey(ID_S.PLAY, string.byte("p"))
    elseif not runAiTurn then
      Ins.say(0, "Play as AI: needs a newer CoE5SE")
    else
      Ins.say(0, "Play as AI: only in the Observer's own turn")
    end
    coe5.ui.tooltip("Battles show the Fight box (Ok). A commander bought on the Recruit screen is bought as the AI buys, which adds its price to the "
      .. "stocks instead of taking it. The Recruit screen's treasury shows a human's income.")
    coe5.ui.sectionEnd()
    coe5.ui.sectionBegin("Recording")
    coe5.ui.checkbox(ID_S.RECORDING, settings.recording, "Record every AI's turns (c)")
    coe5.ui.tooltip("A few milliseconds a turn. The same save and seed with the inspector on and off play the same game.")
    coe5.ui.bindKey(ID_S.RECORDING, string.byte("c"))
    coe5.ui.text(ID_TIP, string.format("Records: %%APPDATA%%\\coe5\\coe5se_data\\JazzyAIObserver\\%s\\ (since turn %s)", game and game.id or "game_NNNN",
      tostring(game and game.firstRecorded or "?")))
    coe5.ui.tooltip("One file per AI turn: tTTTT_pSS.ini, TTTT the AI turn and SS the player slot; each file's [about] lists its fields. "
      .. "Loading an earlier save moves the later turns' records to a folder of their own (game_NNNN_left_tTTTT).")
    coe5.ui.sectionEnd()
    coe5.ui.sectionBegin("On the map")
    coe5.ui.checkbox(ID_S.LIST, settings.list, "The AI list (l)")
    coe5.ui.bindKey(ID_S.LIST, string.byte("l"))
    coe5.ui.checkbox(ID_S.TAGS, settings.tags, "Decision tags on the shown AI's commanders (d)")
    coe5.ui.bindKey(ID_S.TAGS, string.byte("d"))
    coe5.ui.checkbox(ID_S.CARD, settings.hoverCard, "Hover card: the commander under the pointer (h)")
    coe5.ui.bindKey(ID_S.CARD, string.byte("h"))
    coe5.ui.checkbox(ID_S.TOP_BAR, settings.topBar, "Play as AI: the AI's income on the top bar (t)")
    coe5.ui.bindKey(ID_S.TOP_BAR, string.byte("t"))
    coe5.ui.text(ID_S.ARROWS, ARROW_CHOICES[settings.arrows] or ARROW_CHOICES[1])
    coe5.ui.tooltip("A click or a goes to the next of the three")
    coe5.ui.bindKey(ID_S.ARROWS, string.byte("a"))
    coe5.ui.sectionEnd()
    coe5.ui.buttonRow()
    button(ID_K_BACK, "Back", "Esc")
    local id = coe5.ui.wait()
    if Ins.closeScreens then return end
    if id == ID_K_BACK then return end
    if id == ID_S.PLAY and canPickPlayAs() then openPlayAs()
    elseif id == ID_S.RETURN and canReturn() then Ins.confirmReturn()
    elseif id == ID_S.RECORDING then
      settings.recording = not settings.recording
      writeSettings()
      noteRecordingSwitch(settings.recording)
      coe5.log("inspector_recording on=" .. tostring(settings.recording))
    elseif id == ID_S.LIST then settings.list = not settings.list writeSettings()
    elseif id == ID_S.TAGS then settings.tags = not settings.tags writeSettings()
    elseif id == ID_S.CARD then settings.hoverCard = not settings.hoverCard writeSettings()
    elseif id == ID_S.TOP_BAR then settings.topBar = not settings.topBar writeSettings()
    elseif id == ID_S.ARROWS then settings.arrows = (settings.arrows + 1) % 3 writeSettings()
    end
  end
end

-- ---- The Game Menu -------------------------------------------------------------------------------
coe5.addGameMenuLine{
  label = "AI Inspector",
  tooltip = "What each AI player did on its last turns, and why",
  section = SECTION,
  visible = function() return game ~= nil end,
  onClick = function()
    openOverview()
    return (Ins.closeScreens or Ins.goTo) and coe5.gameMenu.close or coe5.gameMenu.stay   -- closed when a screen in it ended the turn or went to a square
  end,
}
coe5.addGameMenuLine{
  label = "Skip turns",
  tooltip = "End turns until N have passed (5, 10, or 1 to 50); a key or a click on the map stops it",
  section = SECTION,
  visible = function() return game ~= nil end,
  onClick = function()
    Ins.openSkip()
    return coe5.gameMenu.close
  end,
}
coe5.addGameMenuLine{
  label = "Play as AI",
  tooltip = "An AI player played by hand, with every AI bonus: the Observer's turn ends now and the game runs on to that player's turn",
  section = SECTION,
  visible = canPickPlayAs,
  onClick = function()
    return openPlayAs() and coe5.gameMenu.close or coe5.gameMenu.stay
  end,
}
coe5.addGameMenuLine{
  label = "Play as Observer",
  tooltip = "Ends this turn now; the Observer's seat returns to human control",
  section = SECTION,
  visible = canReturn,
  onClick = function()
    return Ins.confirmReturn() and coe5.gameMenu.close or coe5.gameMenu.stay
  end,
}

-- ---- The map -------------------------------------------------------------------------------------
-- The map's keys and the clicks kept from the map.
local MAP_ID = {OPEN = 31001, NEXT_AI = 31003, PREV_AI = 31005, OWN_VIEW = 31006, PICK = 31007, SKIP = 31008, CANCEL = 31009}
local SQUARE_INFO_IDS = {[96] = true, [0x2a] = true}
for id = 0x50, 0x58 do SQUARE_INFO_IDS[id] = true end
local swallowing = false
local glanceBox   -- where the list was last drawn, for a click on a line: Records.lineAt's box and lines
local function observerSeat()
  local s = coe5.get("activePlayer")
  if not s or s < 0 or (playAs and playAs.active) then return false end
  local cls = coe5.get("slotClass", s)
  return cls and cls > 0 and coe5.get("className", cls) == "Observer"
end
-- Centres the map on an AI's leader, or its home once the leader is dead.
local function centerOn(slot)
  local centerMap = rawget(coe5, "centerMap")
  if not centerMap then return end
  local s = slot or coe5.get("activePlayer")
  local x, y
  if slot then
    local leader = coe5.get("slotLeader", slot) or -1
    local unit = leader >= 0 and coe5.get("commanderUnit", leader) or -1
    if unit and unit >= 0 then x, y = coe5.get("unitX", unit), coe5.get("unitY", unit) end
  end
  if not x or x < 0 then x, y = coe5.get("slotHomeX", s), coe5.get("slotHomeY", s) end
  if x and x >= 0 then centerMap(x, y) end
end
-- The Observer's map shows an AI's view, centred on it.
local function viewAI(slot)
  viewSlot = slot
  showView(slot)
  centerOn(slot)
end

-- The map's own commands, for its keys and the quick commands in the commander panel.
local function stepView(dir)
  -- The next (dir 1) or previous (dir -1) AI; on the Observer's map, past either end its own view.
  local slots = aiSlots()
  local shown = sightShown or (not observerSeat() and viewSlot)
  local at = dir > 0 and 0 or #slots + 1
  for i, s in ipairs(slots) do if s == shown then at = i end end
  local target = slots[at + dir]
  if dir < 0 and not target and at == #slots + 1 then target = slots[#slots] end
  if observerSeat() then
    showView(target)
    centerOn(target)
    if target then viewSlot = target end
  elseif #slots > 0 then
    viewSlot = slots[(at + dir - 1) % #slots + 1]
  end
end
local function ownView()
  if observerSeat() then
    showView(nil)
    centerOn(nil)
  end
end

-- The pick list (Tab on the map): every AI, living and eliminated; a click shows its view.
local ID_PICK_FIRST = 100
local function openPick()
  while true do
    if Ins.closeScreens then return end
    frame("AI Inspector: pick an AI's view")
    coe5.ui.sectionBegin("AI players (click one: the map shows its view)")
    local slots = aiSlots()
    for _, s in ipairs(goneSlots()) do slots[#slots + 1] = s end
    local shown = sightShown
    if not observerSeat() then shown = viewSlot end   -- the map's view, or off the Observer's map the overview's AI
    for _, s in ipairs(slots) do
      local name, class = playerLabel(s)
      local turn, r = latestRecord(s)
      local alive = isController(s, coe5.controller.ai) or isController(s, coe5.controller.human)
      local res = alive and Ins.resNow(s) or (r and Records.resFromRecord(r)) or {{name = "Gold", stock = 0, income = 0}}
      Ins.say(ID_PICK_FIRST + s, string.format("%s%s, %s: gold %s%s", s == shown and "> " or "", name, class,
        Records.resText(res[1].stock, res[1].income), alive and "" or string.format(", eliminated after turn %d", turn or -1)))
    end
    if #slots == 0 then Ins.say(0, "No AI players") end
    if observerSeat() then coe5.ui.text(ID_PICK_FIRST - 1, "The Observer's own view") end
    coe5.ui.sectionEnd()
    keyStrip("click a line")
    coe5.ui.buttonRow()
    button(ID_K_BACK, "Back", "Esc")
    local id = coe5.ui.wait()
    if Ins.closeScreens then return end
    if id == ID_K_BACK then return end
    if id == ID_PICK_FIRST - 1 then
      ownView()
      return
    end
    local s = id - ID_PICK_FIRST
    if s >= 0 and s < 32 then
      if isController(s, coe5.controller.ai) and observerSeat() then viewAI(s) else viewSlot = s end
      return
    end
  end
end

-- ---- Skip turns -----------------------------------------------------------------------------------
-- A skip ends the Observer's turns for it until N turns have passed.
Ins.skip = nil   -- {untilTurn =, total =, looks =, frames =} while skipping
function Ins.lookAtAis()
  local out = {}
  for s2 = 0, 31 do
    if isController(s2, coe5.controller.ai) or (playAs and playAs.active and s2 == playAs.slot) then
      out[s2] = {ai = true, leader = coe5.get("slotLeader", s2) or -1, name = (playerLabel(s2))}
    end
  end
  return out
end
function Ins.startSkip(n)
  Ins.skip = {untilTurn = coe5.turn() + n, total = n, looks = Ins.lookAtAis(), frames = 0}
  Ins.skipNote = nil
  coe5.log(string.format("inspector_skip_start turn=%d turns=%d stop_early=%s", coe5.turn(), n, tostring(settings.skipStop)))
end
function Ins.stopSkip(why)
  if not Ins.skip then return end
  Ins.showBattlesAgain()
  if Ins.skip.mode then   -- a hand-over stopped by hand: the turn stays with the player
    coe5.log(string.format("inspector_handover_end mode=%s turn=%d reason=%s", Ins.skip.mode, coe5.turn(), why))
    Ins.skip = nil
    return
  end
  coe5.log(string.format("inspector_skip_end turn=%d reason=%s", coe5.turn(), why))
  Ins.skipNote = {turn = coe5.turn(), text = "Skip ended: " .. why .. "."}
  Ins.skip = nil
end
-- At a turn of the seat (or the played AI): stop at the end, or early on a loss when set.
function Ins.skipTurnStart()
  local sk = Ins.skip
  if not sk then return end
  if sk.mode then
    local played = playAs and playAs.active and coe5.get("activePlayer") == playAs.slot or false
    if not Records.handoverArrived(sk.mode, played) then return end
    Ins.skip = nil
    local seat = coe5.get("activePlayer")
    if sk.mode == "pick" then
      local name = (playerLabel(playAs.slot))
      Ins.arrival = {turn = coe5.turn(), player = seat, text = string.format("Playing %s. Play as Observer: F8.", name)}
    else
      Ins.arrival = {turn = coe5.turn(), player = seat, text = string.format("Back on the Observer: %s is the AI's again.", sk.name or "the played AI")}
    end
    coe5.log(string.format("inspector_handover_end mode=%s turn=%d player=%d", sk.mode, coe5.turn(), seat or -1))
    return
  end
  if coe5.turn() >= sk.untilTurn then return Ins.stopSkip(sk.total .. " turns skipped") end
  local now = Ins.lookAtAis()
  local loss = settings.skipStop and Records.skipLoss(sk.looks, now)
  sk.looks = now
  if loss then Ins.stopSkip(loss) end
end
-- While a skip runs, battles are reported, not shown.
Ins.reportsKept = nil   -- the player's own value while the skip has it on
function Ins.reportBattles()
  if Ins.reportsKept ~= nil then return end
  Ins.reportsKept = coe5.get("battleReports") or 0
  if Ins.reportsKept == 0 then
    coe5.set("battleReports", 1)
    coe5.log(string.format("inspector_battle_reports on=1 turn=%d", coe5.turn()))
  end
end
function Ins.showBattlesAgain()
  if Ins.reportsKept == nil then return end
  if Ins.reportsKept == 0 then
    coe5.set("battleReports", 0)
    coe5.log(string.format("inspector_battle_reports on=0 turn=%d", coe5.turn()))
  end
  Ins.reportsKept = nil
end
-- One frame of the map while skipping: a key or click stops it; else, after a moment to stop it, End Turn.
function Ins.skipFrame(id)
  local buttons = coe5.get("mouseButtons") or 0
  local step = Records.skipStep(Ins.skip, id ~= 0 or buttons ~= 0, 45)   -- 45 frames: about a second and a half to stop it
  if buttons ~= 0 then coe5.set("mouseButtons", 0) swallowing = true end   -- the map never takes a click meanwhile
  if step == "stop" then
    Ins.stopSkip("a key or click")
    return 0
  end
  if step == "end" then Ins.reportBattles() end
  return step == "end" and coe5.mapAction.endTurn or 0
end
coe5.wrap("MESSAGES_WAIT", function(next, block, mode)
  if not Ins.skip then return next(block, mode) end
  return coe5.box.messagesOk
end)
coe5.wrap("WARN_WAIT", function(next, block, mode)
  if not Ins.skip then return next(block, mode) end
  return coe5.box.warnOk
end)
coe5.wrap("MESSAGE_BOX_WAIT", function(next, block, mode)
  local text = Ins.skip and coe5.get("messageBoxText")
  if not (text and text:find("AP left")) then return next(block, mode) end   -- only the End Turn Warning
  return coe5.box.firstButton
end)

local ID_SK = {FIVE = 41, TEN = 42, OTHER = 43, STOP = 44}
function Ins.openSkip()
  while true do
    if Ins.closeScreens then return end
    frame("AI Inspector: skip turns")
    coe5.ui.sectionBegin("Skip turns")
    coe5.ui.text(ID_SK.FIVE, "Skip 5 turns (Enter)")
    coe5.ui.bindKey(ID_SK.FIVE, coe5.key.enter)
    coe5.ui.text(ID_SK.TEN, "Skip 10 turns (t)")
    coe5.ui.bindKey(ID_SK.TEN, string.byte("t"))
    coe5.ui.text(ID_SK.OTHER, "Skip another number, 1 to 50 (n)")
    coe5.ui.bindKey(ID_SK.OTHER, string.byte("n"))
    coe5.ui.checkbox(ID_SK.STOP, settings.skipStop, "Stop when a leader dies or an AI is eliminated (s)")
    coe5.ui.bindKey(ID_SK.STOP, string.byte("s"))
    coe5.ui.text(ID_TIP, "A key or a click on the map stops it after the turn in progress.")
    coe5.ui.sectionEnd()
    coe5.ui.buttonRow()
    button(ID_K_BACK, "Back", "Esc")
    local id = coe5.ui.wait()
    if Ins.closeScreens then return end
    if id == ID_K_BACK then return end
    if id == ID_SK.FIVE then return Ins.startSkip(5) end
    if id == ID_SK.TEN then return Ins.startSkip(10) end
    if id == ID_SK.STOP then settings.skipStop = not settings.skipStop writeSettings() end
    if id == ID_SK.OTHER then
      local seat = coe5.get("activePlayer") or 0
      local leader = coe5.get("slotLeader", seat) or -1
      local unit = leader >= 0 and coe5.get("commanderUnit", leader) or -1
      local monster = unit >= 0 and coe5.get("unitMonster", unit) or 0
      local n = Records.skipCount(coe5.ui.nameBox("Skip turns", "Turns to skip, 1 to 50", coe5.get("monsterSprite", monster) or 0, 4))
      if n then return Ins.startSkip(n) end
    end
  end
end

-- ---- Commanders here ----------------------------------------------------------------------------
-- Commanders here: a click on the line, then on a map square.
Ins.picking = nil      -- {player =} while the line waits for a square
Ins.pickButtons = 0    -- the mouse buttons at the last map frame, so a press is told from a hold
Ins.pickHold = false   -- a used click held: kept from the map until let go
function Ins.stopPicking(reason)
  coe5.log(string.format("inspector_pick_stopped slot=%d reason=%s", Ins.picking.player, reason))
  Ins.picking = nil
end
-- One map frame while waiting: the id to return, or nil for the map's usual handling.
function Ins.pickFrame(id, x, y)
  local buttons = coe5.get("mouseButtons") or 0
  local pressed = buttons ~= 0 and Ins.pickButtons == 0
  Ins.pickButtons = buttons
  if coe5.get("activePlayer") ~= Ins.picking.player then
    Ins.stopPicking("turn_changed")
    return id
  end
  if id == MAP_ID.CANCEL then
    Ins.stopPicking("esc")
    return 0
  end
  if id == coe5.mapAction.gameMenu then
    Ins.stopPicking("game_menu")
    return id
  end
  if buttons == 0 then return nil end
  -- A right button stops it and never reaches the map.
  if buttons % 4 >= 2 then
    Ins.stopPicking("right_click")
    coe5.set("mouseButtons", 0)
    Ins.pickHold = true
    return 0
  end
  if id ~= 0 then return id end   -- the map's own lines keep their clicks
  coe5.set("mouseButtons", 0)     -- no other button on the map reaches it while picking
  if not pressed or buttons ~= 1 or not x or x < 0 or y < 0 then return 0 end
  Ins.pickHold = true
  coe5.log(string.format("inspector_pick_square slot=%d x=%d y=%d", Ins.picking.player, x, y))
  Ins.picking = nil
  openChooser(x, y)
  return 0
end

coe5.wrap("MAP_SCREEN_WAIT", function(next, block, mode)
  Ins.addQuickLines()
  Ins.closeScreens = false
  if not game then return next(block, mode) end
  Ins.applyView()   -- a loaded game's first frame: the Observer's turn start ran before the load
  if not viewSlot then viewSlot = aiSlots()[1] end
  coe5.ui.bindKey(MAP_ID.OPEN, coe5.key.enter)
  if Ins.picking then coe5.ui.bindKey(MAP_ID.CANCEL, coe5.key.esc) end
  coe5.ui.bindKey(MAP_ID.NEXT_AI, string.byte("g"))
  coe5.ui.bindKey(MAP_ID.PREV_AI, coe5.key.f7)    -- F7, F8 and Tab: no map shortcut of the game's
  coe5.ui.bindKey(MAP_ID.OWN_VIEW, coe5.key.f8)
  coe5.ui.bindKey(MAP_ID.PICK, coe5.key.tab)
  coe5.ui.bindKey(MAP_ID.SKIP, coe5.key.endKey)
  local id = next(block, mode)
  if Ins.skip and Records.skipEnds(coe5.get("activePlayer"), Ins.observerSeatNow(), playAs) then
    return Ins.skipFrame(id)
  end
  if Ins.skipNote then   -- "Skip ended" goes at the next key or click, once the one that stopped it is let go
    local down = id ~= 0 or (coe5.get("mouseButtons") or 0) ~= 0
    if not down then Ins.skipNote.armed = true elseif Ins.skipNote.armed then Ins.skipNote = nil end
  end
  if Ins.pickHold then   -- the click Commanders here used is kept from the map until it is let go
    if (coe5.get("mouseButtons") or 0) ~= 0 then coe5.set("mouseButtons", 0) return 0 end
    Ins.pickHold = false
  end
  if Ins.goTo then   -- back from a go: the click that went is kept from the map until it is let go
    if (coe5.get("mouseButtons") or 0) ~= 0 then coe5.set("mouseButtons", 0) return 0 end
    Ins.goTo = nil
  end
  local x, y = coe5.get("mapPointerX"), coe5.get("mapPointerY")
  Ins.hover = (x and x >= 0) and {x = x, y = y} or nil   -- the hover card's square (drawn next frame)
  if Ins.picking then
    local back = Ins.pickFrame(id, x, y)
    if back then return back end
  end
  local pb = Ins.partnerBox
  if pb and playAs and playAs.active and id == 0 and coe5.get("mouseButtons") == 1 then
    local px, py = coe5.get("uiPointerX"), coe5.get("uiPointerY")
    if px and px >= pb.left and px < pb.left + pb.width and py >= pb.arrowsY - 3 and py < pb.arrowsY + pb.pitch - 3 then
      coe5.set("mouseButtons", 0)
      settings.arrows = (settings.arrows + 1) % 3
      writeSettings()
      return 0
    end
  end
  if observerSeat() then
    local buttons = coe5.get("mouseButtons")
    if swallowing then
      if buttons ~= 0 then coe5.set("mouseButtons", 0) else swallowing = false end
    elseif buttons ~= 0 and id == 0 then
      -- Clicks on the map's own lines: left shows, right opens.
      local right = buttons % 4 >= 2
      local px, py = coe5.get("uiPointerX"), coe5.get("uiPointerY")
      local sb = Ins.stripBox
      local at = glanceBox and Records.lineAt(glanceBox, px, py)
      if sb and px and px >= sb.left and px < sb.left + sb.width and py >= sb.bottom and py < sb.bottom + sb.height then
        coe5.set("mouseButtons", 0)
        swallowing = true
        if not right and sb.emptyLine and py < sb.bottom + 19 then centerOn(sightShown) else openOverview() end
        return 0
      elseif at then
        coe5.set("mouseButtons", 0)
        swallowing = true
        local line = glanceBox.lines[at]
        if line.kind == "slot" and right then
          viewSlot = line.slot
          openOverview()
        elseif line.kind == "slot" and isController(line.slot, coe5.controller.ai) then
          viewAI(line.slot)
        elseif line.kind == "footer" and px < line.mid then
          settings.arrows = (settings.arrows + 1) % 3
          writeSettings()
        elseif line.kind == "footer" and not right then
          ownView()
        end
        return 0
      elseif right and x and x >= 0 then
        coe5.set("mouseButtons", 0)
        swallowing = true
        local found = Ins.aiCommandersAt(x, y)
        if #found == 1 then openReasoning(found[1].slot, found[1].c) elseif #found > 1 then openChooser(x, y) end
        return 0
      end
    end
    if not swallowing and buttons ~= 0 and id == 0 and x and x >= 0 then
      coe5.set("mouseButtons", 0)
      swallowing = true
    end
    if SQUARE_INFO_IDS[id] then return 0 end
  end
  if id == MAP_ID.OPEN then openOverview() return 0 end
  if id == MAP_ID.NEXT_AI then stepView(1) return 0 end
  if id == MAP_ID.PREV_AI then stepView(-1) return 0 end
  if id == MAP_ID.OWN_VIEW and playAs and playAs.active then   -- in Play as AI, F8 is Play as Observer
    if canReturn() then Ins.confirmReturn() end
    return 0
  end
  if id == MAP_ID.OWN_VIEW then ownView() return 0 end
  if id == MAP_ID.PICK then openPick() return 0 end
  if id == MAP_ID.SKIP then Ins.openSkip() return 0 end
  return id
end)

-- The quick commands: lines at the end of the game's commander panel.
local QUICK = {0.95, 0.8, 0.35}
local quickLines, quickAdded = {}, false
function Ins.quickLine(label, tip, onClick, visible, color)
  quickLines[#quickLines + 1] = {label = label, tooltip = tip, color = color or QUICK, onClick = onClick,
    visible = function() return game ~= nil and (not visible or visible()) end}
end
function Ins.addQuickLines()
  if quickAdded then return end
  quickAdded = true
  for _, line in ipairs(quickLines) do coe5.addCommanderLine(line) end
end
local function partnerNow() return playAs ~= nil and playAs.active == true end
Ins.quickLine("AI Inspector (Enter)", "Every AI's last turns: the overview", function() openOverview() end)
Ins.quickLine("Pick a view (Tab)", "Every AI in a list; a click shows its view, centred on its leader", function() openPick() end, observerSeat)
Ins.quickLine("Next AI (g)", "The next AI's view, centred on its leader (its home once the leader is dead)", function() stepView(1) end, observerSeat)
Ins.quickLine("Previous AI (F7)", "The previous AI's view, centred on its leader (its home once the leader is dead)", function() stepView(-1) end, observerSeat)
Ins.quickLine("Own view (F8)", "Back to the Observer's own view, centred on its post", function() ownView() end, function() return observerSeat() and sightShown ~= nil end)
Ins.quickLine("Commanders here", "The AI commanders on a square: click this, then the square",
  function()
    Ins.picking = {player = coe5.get("activePlayer")}
    Ins.pickButtons = 1   -- the click on this line may still be held
    coe5.log(string.format("inspector_pick_started slot=%d", Ins.picking.player))
  end, function() return Ins.picking == nil end)
Ins.quickLine("Commanders: pick a square", "Left click a square to see its AI commanders. Right click cancels.",
  function() if Ins.picking then Ins.stopPicking("line") end end, function() return Ins.picking ~= nil end, {1, 0.85, 0.3})
Ins.quickLine("Skip turns (End)", "End turns until N have passed: 5, 10 or 1 to 50; a key or a click on the map stops it", function() Ins.openSkip() end)
-- One slot before Settings: Play as AI, or while an AI is played, Play as Observer.
Ins.quickLine("Play as AI", "An AI player played by hand, with every AI bonus: the Observer's turn ends now and the game runs on to that player's turn",
  function() openPlayAs() end, canPickPlayAs)
Ins.quickLine("Play as Observer (F8)", "Ends this turn now; the Observer's seat returns to human control",
  function() Ins.confirmReturn() end, function() return partnerNow() and canReturn() end)
Ins.quickLine("Settings (Enter, e)", "The inspector's settings", function() openSettings() end, observerSeat)
-- ---- Drawn over the map (MAP_DRAW) ---------------------------------------------------------------
-- The map as the inspector: tags, the hover card, the strip and the AI list.
do   -- the map section: its locals end with it (the main chunk holds at most 200)
local C = {LAMP = {0.106, 0.086, 0.071}, VELLUM = {0.847, 0.780, 0.627}, GILT = {0.878, 0.690, 0.282},
           VERD = {0.373, 0.639, 0.604}, CINN = {0.784, 0.333, 0.239}, ASH = {0.549, 0.518, 0.459}}
Ins.CHAR = 0.36   -- a character's average width, times the text size (guifont; measured on screen)
-- Sizes: tags, the list and its heads, the strip's name and line, the card's name and lines.
C.TAG, C.LIST, C.HEAD, C.NAME, C.SMALL, C.CARD, C.CARDNAME = 12.5, 13.5, 11.5, 16, 13, 13.5, 15
function Ins.rect(x, y, w, h, c, a) coe5.ui.drawRect(x, y, w, h, c[1], c[2], c[3], a or 1) end
function Ins.text(x, y, size, text, c, a) coe5.ui.drawText(x, y, size, text, c[1], c[2], c[3], a or 1) end
function Ins.line(x1, y1, x2, y2, c, a, w) coe5.ui.drawLine(x1, y1, x2, y2, c[1], c[2], c[3], a or 1, w or 1) end
function Ins.width(text, size) return #text * size * Ins.CHAR end
function Ins.cut(text, size, room)   -- text cut with ".." to fit `room` units
  local n = math.floor(room / (size * Ins.CHAR))
  if #text <= n then return text end
  return text:sub(1, math.max(n - 2, 1)) .. ".."
end

-- The UI point at the centre of square (x, y), nil when it isn't on the shown plane.
function Ins.squarePoint(v, x, y)
  if not x or x < 0 or coe5.get("squarePlane", x, y) ~= v.plane then return nil end
  return v.originX + (x + 0.5) * v.zoom, v.originY - (y + 0.5) * v.zoom
end

function Ins.arrow(v, x1, y1, x2, y2, c, alpha, width, dashed)
  local ax, ay = Ins.squarePoint(v, x1, y1)
  local bx, by = Ins.squarePoint(v, x2, y2)
  if not (ax and ay and bx and by) or (ax == bx and ay == by) then return end
  if dashed then
    for i = 0, 15, 2 do
      Ins.line(ax + (bx - ax) * i / 16, ay + (by - ay) * i / 16, ax + (bx - ax) * (i + 1) / 16, ay + (by - ay) * (i + 1) / 16, c, alpha, width)
    end
  else
    Ins.line(ax, ay, bx, by, c, alpha, width)
  end
  local dx, dy = ax - bx, ay - by
  local len = math.sqrt(dx * dx + dy * dy)
  local hx, hy, size = dx / len, dy / len, math.max(v.zoom * 0.3, 6)
  Ins.line(bx, by, bx + (hx * 0.87 - hy * 0.5) * size, by + (hy * 0.87 + hx * 0.5) * size, c, alpha, width)
  Ins.line(bx, by, bx + (hx * 0.87 + hy * 0.5) * size, by + (hy * 0.87 - hx * 0.5) * size, c, alpha, width)
end

function Ins.xy(text)
  local a = Records.answerOf("0 " .. (text or ""))
  return a and a[2], a and a[3]
end

C.STEP_LETTER = {rally = "R", friendly = "F", conquer = "C", explore = "X", far = "XX"}
-- A commander's tag and its colour.
function Ins.cellOf(com)
  if com.played_by == "you" then return "human", C.VERD end
  if #com.plans == 0 then return (com.skip and com.skip ~= "") and "stays" or "no plan", C.ASH end
  local winner, best = Records.pick(com.plans[#com.plans], "final")
  if not winner then return "rested", C.ASH end
  return C.STEP_LETTER[winner] .. " " .. best, (winner == "explore" or winner == "far") and C.VERD or C.GILT
end

-- The shown AI's tags, made once per AI, turn and record.
function Ins.tagsFor(slot)
  if not slot then return nil end
  local turn, r = latestRecord(slot)
  local key = slot .. ":" .. tostring(turn) .. ":" .. coe5.turn() .. ":" .. Ins.handKey()
  if Ins.tagCache and Ins.tagCache.key == key then return Ins.tagCache end
  local followers, bySquare, squares = {}, {}, {}
  for _, com in pairs(r and r.coms or {}) do
    if com.follows then followers[com.follows] = (followers[com.follows] or 0) + 1 end
  end
  for c, com in pairs(r and r.coms or {}) do
    local unit = coe5.get("commanderUnit", c) or -1
    if not com.follows and com.finish ~= "-1 -1" and unit >= 0 and (coe5.get("unitHp", unit) or 0) > 0
      and coe5.get("unitOwner", unit) == slot then
      local x, y = coe5.get("unitX", unit), coe5.get("unitY", unit)
      local text, colour = Ins.cellOf(com)
      local entry = {com = com, text = text, colour = colour, followers = followers[c] or 0}
      if Ins.handTouched(c) then entry.text, entry.colour, entry.hand = "human", C.VERD, true end
      for _, p in ipairs(com.plans) do
        for _, step in ipairs(STEPS) do
          if p[step] and p[step].by then entry.changed = true end
        end
      end
      local k = x .. " " .. y
      if not bySquare[k] then
        bySquare[k] = {x = x, y = y, coms = {}}
        squares[#squares + 1] = bySquare[k]
      end
      table.insert(bySquare[k].coms, entry)
    end
  end
  for _, sq in ipairs(squares) do
    table.sort(sq.coms, function(a, b)
      if a.followers ~= b.followers then return a.followers > b.followers end
      return a.com.c < b.com.c
    end)
    sq.lead = sq.coms[1]
    sq.plus = sq.lead.followers + #sq.coms - 1
  end
  Ins.tagCache = {key = key, slot = slot, turn = turn, squares = squares, bySquare = bySquare}
  return Ins.tagCache
end

-- The AI whose tags and card the map shows: the played AI, the shown view, else none (own view).
function Ins.mapSlot() return (playAs and playAs.active) and playAs.slot or sightShown end

function Ins.drawTags(v, tags)
  local onScreen = 0
  for _, sq in ipairs(tags.squares) do
    if sq.x >= v.firstX and sq.x <= v.lastX and sq.y >= v.firstY and sq.y <= v.lastY and coe5.get("squarePlane", sq.x, sq.y) == v.plane then
      onScreen = onScreen + #sq.coms
      local lead, z = sq.lead, v.zoom
      local sx, sy = v.originX + sq.x * z, v.originY - sq.y * z
      local com = lead.com
      -- Its move this turn, start to finish, dashed in vellum.
      local ax, ay = Ins.xy(com.start)
      local fx, fy = Ins.xy(com.finish)
      if ax and fx and com.start ~= com.finish then
        local p1x, p1y = Ins.squarePoint(v, ax, ay)
        local p2x, p2y = Ins.squarePoint(v, fx, fy)
        if p1x and p2x then
          for i = 0, 7, 2 do
            Ins.line(p1x + (p2x - p1x) * i / 8, p1y + (p2y - p1y) * i / 8, p1x + (p2x - p1x) * (i + 1) / 8, p1y + (p2y - p1y) * (i + 1) / 8, C.VELLUM, 0.6, 1)
          end
        end
      end
      if z < 24 then   -- zoomed out: a dot, so the plates don't hide the map
        Ins.rect(sx + z / 2 - 1.5, sy - z / 2 - 1.5, 3, 3, lead.colour, 1)
      else
        local text = lead.text
        local w = Ins.width(text, C.TAG) + 6 + (lead.changed and 6 or 0)
        Ins.rect(sx, sy + 1, w, 15, C.LAMP, 0.82)
        Ins.rect(sx, sy + 1, 1.5, 15, lead.colour, 1)
        Ins.text(sx + 4, sy + 3.5, C.TAG, text, lead.colour, 1)
        if lead.changed then Ins.text(sx + 4 + Ins.width(text, C.TAG) + 1, sy + 3.5, C.TAG, "*", C.CINN, 1) end
        if sq.plus > 0 then
          local plus = "+" .. sq.plus
          local pw = Ins.width(plus, C.TAG) + 6
          Ins.rect(sx + z - pw, sy - z + 1, pw, 15, C.LAMP, 0.82)
          Ins.text(sx + z - pw + 3, sy - z + 3.5, C.TAG, plus, C.VELLUM, 1)
        end
        if lead.hand or (com.target and com.target ~= "-1 -1" and com.finish == com.target) then   -- on its target, or moved by hand
          Ins.line(sx, sy, sx + z, sy, lead.colour, 1, 1.5)
          Ins.line(sx, sy - z, sx + z, sy - z, lead.colour, 1, 1.5)
          Ins.line(sx, sy, sx, sy - z, lead.colour, 1, 1.5)
          Ins.line(sx + z, sy, sx + z, sy - z, lead.colour, 1, 1.5)
        end
      end
    end
  end
  return onScreen
end

-- Arrows to the shown AI's targets.
function Ins.drawArrows(v)
  if settings.arrows == 1 then
    local slot = Ins.mapSlot() or viewSlot
    local _, r = latestRecord(slot or -1)
    for _, com in pairs(r and r.coms or {}) do
      local kind = tonumber(com.kind) or 0
      if kind > 0 and not com.follows then
        local fx, fy = Ins.xy(com.finish)
        local tx, ty = Ins.xy(com.target)
        if kind == 1 then Ins.arrow(v, fx, fy, tx, ty, C.GILT, 0.9, 2) else Ins.arrow(v, fx, fy, tx, ty, C.VERD, 0.9, 2, true) end
      end
    end
  elseif settings.arrows == 2 and lastCommander then
    local _, r = latestRecord(lastCommander.slot)
    local com = r and r.coms[lastCommander.c]
    local last = com and com.plans[#com.plans]
    if last then
      local sx, sy = Ins.xy(com.start)
      for _, plan in ipairs(com.plans) do
        local winner = Records.pick(plan, "final")
        for _, step in ipairs(STEPS) do
          local e = plan[step]
          if e and e.final and e.final[2] >= 0 then
            if step == winner then Ins.arrow(v, sx, sy, e.final[2], e.final[3], C.GILT, 1, 3)
            else Ins.arrow(v, sx, sy, e.final[2], e.final[3], C.ASH, 0.7, 1) end
          end
        end
        local w = winner and plan[winner]
        if w then sx, sy = w.final[2], w.final[3] end   -- the next plan starts where this one's target was
      end
    end
  end
end

-- When the shown AI's sight has no square on the screen, say so.
function Ins.emptyViewText(v)
  local key = table.concat({tostring(sightShown), v.firstX, v.lastX, v.firstY, v.lastY, v.plane, coe5.turn()}, ":")
  if Ins.emptyView and Ins.emptyView.key == key then return Ins.emptyView.text end
  local text
  if sightShown then
    local seen = false
    for x = v.firstX, v.lastX do
      for y = v.firstY, v.lastY do
        if (coe5.get("squareSeen", sightShown, x, y) or 0) > 0 then seen = true break end
      end
      if seen then break end
    end
    if not seen then
      local leader = coe5.get("slotLeader", sightShown) or -1
      local unit = leader >= 0 and coe5.get("commanderUnit", leader) or -1
      if unit >= 0 then
        text = "Sees nothing here. Its leader: " .. placeOf(coe5.get("unitX", unit), coe5.get("unitY", unit)) .. " (click: go there)."
      else   -- the leader is dead: a living commander's square, else its home
        local c, u = next(commandersOf(sightShown))
        if c then
          text = string.format("Sees nothing here; leader dead. %s: %s (click: go there).", commanderLabel(c), placeOf(coe5.get("unitX", u), coe5.get("unitY", u)))
        else
          text = "Sees nothing here; leader dead. Home: " .. placeOf(coe5.get("slotHomeX", sightShown), coe5.get("slotHomeY", sightShown)) .. "."
        end
      end
    end
  end
  Ins.emptyView = {key = key, text = text}
  return text
end

-- The view strip, top centre.
function Ins.drawStrip(v, slot, tags, onScreen)
  local w, h = 520, 40
  local left, bottom = (v.screenW - w) / 2, v.screenH - 76
  local arrival = Ins.arrival and Ins.arrival.turn == coe5.turn() and Ins.arrival.player == coe5.get("activePlayer") and Ins.arrival.text
  if arrival then h, bottom = 54, bottom - 14 end
  local partner = playAs and playAs.active
  local accent = partner and C.VERD or C.GILT
  Ins.rect(left, bottom, w, h, C.LAMP, 0.86)
  Ins.rect(left, bottom, w, 1.5, accent, 1)
  local name
  if partner then name = "Playing " .. (playerLabel(playAs.slot))
  elseif slot then name = (playerLabel(slot)) .. "'s view"
  else name = "The Observer's own view" end
  local lift = arrival and 14 or 0   -- the arrival line goes under the two
  Ins.text(left + 8, bottom + 22 + lift, C.NAME, name, accent, 1)
  local x = left + 8 + Ins.width(name, C.NAME) + 12
  if slot then
    local _, class = playerLabel(slot)
    local n = 0
    for _, sq in ipairs(tags and tags.squares or {}) do n = n + #sq.coms end
    local _, last = latestRecord(slot)   -- the commanders as the overview counts them (tags leave followers out)
    if last and last.commanders then n = last.commanders end
    local facts = string.format("%s   %s   %s, %d on this screen", class,
      tags and tags.turn and ("after turn " .. tags.turn) or "no AI turn yet", Records.count(n, "commander"), onScreen or 0)
    if partner then facts = class .. "   turn " .. coe5.turn() end
    Ins.text(x, bottom + 23 + lift, C.SMALL, Ins.cut(facts, C.SMALL, left + w - x - 6), C.VELLUM, 1)
  end
  local line2, colour, emptyLine = "Left click a name: its view.   Right click a square or a name: open.", C.ASH, false
  if partner and not Ins.seatInGame() then
    line2, colour = "The Observer is out of the game: Play as Observer would end it.", C.CINN
  elseif Ins.skip and Ins.skip.mode then
    line2, colour = (Ins.skip.mode == "pick" and playAs and ("Ending this turn, on to " .. (playerLabel(playAs.slot)) .. "'s turn")
      or "Ending this turn, back to the Observer") .. "; a key or a click on the map stops it", C.CINN
  elseif partner then
    local items = tradeItems(playAs.slot)
    line2 = Records.tradeText(items)   -- the stocks and incomes are in the panel
  else
    local empty = Ins.emptyViewText(v)
    if Ins.skip then
      line2, colour = string.format("Skipping: %d of %d turns left; a key or a click on the map stops it",
        math.max(Ins.skip.untilTurn - coe5.turn(), 0), Ins.skip.total), C.CINN
    elseif Ins.skipNote and Ins.skipNote.turn == coe5.turn() then line2, colour = Ins.skipNote.text, C.CINN
    elseif empty then line2, colour, emptyLine = empty, C.VELLUM, true
    elseif not sightShown and viewGone and viewGone.turn == coe5.turn() then line2, colour = viewGone.text, C.CINN end
  end
  Ins.text(left + 8, bottom + 6 + lift, C.SMALL, Ins.cut(line2, C.SMALL, w - 14), colour, 1)
  if arrival then Ins.text(left + 8, bottom + 6, C.SMALL, Ins.cut(arrival, C.SMALL, w - 14), C.GILT, 1) end
  Ins.stripBox = {left = left, bottom = bottom + lift, width = w, height = h - lift, emptyLine = emptyLine}
end

-- The hover card beside the square under the pointer.
function Ins.drawCard(v, tags)
  local hv = Ins.hover
  if not (hv and tags and settings.hoverCard ~= false) then return end
  local sq = tags.bySquare[hv.x .. " " .. hv.y]
  if not sq or coe5.get("squarePlane", sq.x, sq.y) ~= v.plane then return end
  local lines = {}   -- {text, size, colour, right}
  local first = sq.coms[1].com
  if #sq.coms == 1 then
    local com = first
    lines[1] = {first.name or commanderLabel(first.c), C.CARDNAME, C.GILT, (tonumber(com.strength) or -1) >= 0 and ("strength " .. com.strength) or nil}
    local plan = com.plans[#com.plans]
    local winner, best
    if plan then winner, best = Records.pick(plan, "final") end
    if winner then
      local e = plan[winner]
      lines[#lines + 1] = {string.format("%s %d at %s", STEP_NAME[winner], best, e.place or (e.final[2] .. "," .. e.final[3])), C.CARD, C.VELLUM}
      local score, step = Records.nextBest(plan, winner)
      lines[#lines + 1] = {score and string.format("Next best: %s %s at %s, %d lower", STEP_NAME[step]:lower(), Records.scoreText(score),
        plan[step].place or "?", best - score) or "Next best: none (no other step found a square)", C.CARD, C.VELLUM}
    else   -- the same two lines without a pick, so the card keeps its size
      lines[#lines + 1] = {plan and "No step found a square" or "No plan", C.CARD, C.VELLUM}
      lines[#lines + 1] = {"Next best: -", C.CARD, C.ASH}
    end
    lines[#lines + 1] = {resultLine(com), C.CARD, C.VELLUM}
    lines[#lines + 1] = {"Right click: its reasoning", C.CARD, C.ASH}
  else
    lines[1] = {string.format("%d commanders on %s", #sq.coms, placeOf(sq.x, sq.y)), C.CARDNAME, C.GILT}
    -- Always 5 commander lines and the "more" line, empty ones included, so the card keeps its size.
    for i = 1, 5 do
      local entry = sq.coms[i]
      lines[#lines + 1] = {entry and string.format("%s: %s", entry.com.name or commanderLabel(entry.com.c), entry.text) or "", C.CARD, C.VELLUM}
    end
    lines[#lines + 1] = {#sq.coms > 5 and ("+" .. (#sq.coms - 5) .. " more") or "", C.CARD, C.ASH}
    lines[#lines + 1] = {"Right click: choose one", C.CARD, C.ASH}
  end
  local w = 0
  for _, l in ipairs(lines) do w = math.max(w, Ins.width(l[1], l[2]) + (l[4] and Ins.width(l[4], C.SMALL) + 14 or 0)) end
  w = math.min(w + 14, 420)
  local h = #lines * 16 + 8
  local z = v.zoom
  local sx, sy = v.originX + sq.x * z, v.originY - sq.y * z
  local left = sx + z + 4
  if left + w > v.screenW - 160 then left = sx - 4 - w end
  local top = math.min(sy, v.screenH - 84)   -- never over the strip
  local bottom = math.max(top - h, 4)
  Ins.rect(left, bottom, w, h, C.LAMP, 0.92)
  Ins.rect(left, bottom + h - 1.5, w, 1.5, C.GILT, 1)
  for i, l in ipairs(lines) do
    local y = bottom + h - 4 - i * 16
    Ins.text(left + 7, y, l[2], Ins.cut(l[1], l[2], w - 14 - (l[4] and Ins.width(l[4], C.SMALL) + 8 or 0)), l[3], 1)
    if l[4] then Ins.text(left + w - 7 - Ins.width(l[4], C.SMALL), y, C.SMALL, l[4], C.ASH, 1) end
  end
end

-- The AI list on the map.
C.LIST_W, C.LIST_SIZE, C.LIST_PITCH = 430, C.LIST, 16
C.LIST_COLS = {16, 120, 204, 250, 292, 334}
function Ins.listRows()
  local slots = aiSlots()
  local key = coe5.turn() .. ":" .. #slots .. ":" .. tostring(playAs and playAs.active)
  if Ins.listCache and Ins.listCache.key == key then return Ins.listCache.rows end
  local rows = {}
  for _, s in ipairs(slots) do
    local turn, r = latestRecord(s)
    local row = {kind = "slot", slot = s, name = (playerLabel(s))}
    if r then
      local prev = readRecord(turn - 1, s)
      row.land = r.land and tostring(r.land) or ""
      if prev and r.land and prev.land and r.land ~= prev.land then row.dland = r.land - prev.land end
      row.cmdrs = tostring(r.commanders or "")
      local units = 0
      for _, b in ipairs(r.buys) do units = units + (tonumber(b:match("^(%d+)")) or 1) end
      row.bought = units > 0 and tostring(units) or ""
      if (r.leader or -1) < 0 then row.flag = "! leader dead"
      elseif r.buy_step == "ran" and #r.buys == 0 and (r.gold_at_buy or 0) >= 200 then row.flag = string.format("! %d gold, no buy", r.gold_at_buy) end
    else
      row.none = true
    end
    rows[#rows + 1] = row
  end
  for _, s in ipairs(goneSlots()) do rows[#rows + 1] = {kind = "slot", slot = s, name = (playerLabel(s)), out = true} end
  Ins.listCache = {key = key, rows = rows}
  return rows
end
function Ins.drawList(v)
  local rows = Ins.listRows()
  local seat = coe5.get("activePlayer") or 0
  local cards = 0
  for _ in pairs(commandersOf(seat)) do cards = cards + 1 end
  local height = (#rows + 2) * C.LIST_PITCH + 12
  -- With more than 4 commander cards, the list goes left of the commander panel.
  local left = cards <= 4 and 10 or (v.screenW - 160 - C.LIST_W)
  local bottom = cards <= 4 and 6 or 66
  Ins.rect(left, bottom, C.LIST_W, height, C.LAMP, 0.84)
  local top = bottom + height - 4
  local lines = {{kind = "head"}}
  local heads = {nil, "gold", "land", "cmdrs", "bought"}
  for i = 2, 5 do Ins.text(left + C.LIST_COLS[i], top - C.LIST_PITCH, C.HEAD, heads[i], C.ASH, 1) end
  local shown = Ins.mapSlot()
  for i, row in ipairs(rows) do
    local y = top - (i + 1) * C.LIST_PITCH
    lines[#lines + 1] = row
    local colour = C.VELLUM
    if row.slot == shown then
      Ins.rect(left + 2, y - 3, C.LIST_W - 4, C.LIST_PITCH, C.GILT, 0.22)
      Ins.rect(left + 2, y - 3, 2, C.LIST_PITCH, C.GILT, 1)
      colour = C.GILT
    end
    if row.out or row.none then colour = C.ASH end
    Ins.text(left + C.LIST_COLS[1], y, C.LIST_SIZE, Ins.cut(row.name, C.LIST_SIZE, C.LIST_COLS[2] - C.LIST_COLS[1] - 4), colour, 1)
    if row.out then
      Ins.text(left + C.LIST_COLS[2], y, C.LIST_SIZE, "eliminated", C.ASH, 1)
    elseif row.none then
      Ins.text(left + C.LIST_COLS[2], y, C.LIST_SIZE, Records.resText(coe5.get("slotResource", row.slot, 0),
        coe5.get("slotAiIncome", row.slot, 0) or coe5.get("slotIncome", row.slot, 0)), colour, 1)
      Ins.text(left + C.LIST_COLS[3], y, C.LIST_SIZE, "no AI turn yet", C.ASH, 1)
    else
      Ins.text(left + C.LIST_COLS[2], y, C.LIST_SIZE, Records.resText(coe5.get("slotResource", row.slot, 0),
        coe5.get("slotAiIncome", row.slot, 0) or coe5.get("slotIncome", row.slot, 0)), colour, 1)
      Ins.text(left + C.LIST_COLS[3], y, C.LIST_SIZE, row.land, colour, 1)
      if row.dland then
        Ins.text(left + C.LIST_COLS[3] + Ins.width(row.land, C.LIST_SIZE) + 3, y, C.HEAD, string.format("%+d", row.dland), row.dland > 0 and C.VERD or C.CINN, 1)
      end
      Ins.text(left + C.LIST_COLS[4], y, C.LIST_SIZE, row.cmdrs, colour, 1)
      Ins.text(left + C.LIST_COLS[5], y, C.LIST_SIZE, row.bought, colour, 1)
      if row.flag then Ins.text(left + C.LIST_COLS[6], y, C.HEAD, Ins.cut(row.flag, C.HEAD, C.LIST_W - C.LIST_COLS[6] - 4), C.CINN, 1) end
    end
  end
  local fy = top - (#rows + 2) * C.LIST_PITCH
  Ins.line(left + 8, fy + C.LIST_PITCH - 2, left + C.LIST_W - 8, fy + C.LIST_PITCH - 2, C.ASH, 0.8, 0.5)
  local arrowsText = ({[0] = "Arrows: none", "Arrows: every commander's target", "Arrows: the last commander opened"})[settings.arrows] or ""
  Ins.text(left + C.LIST_COLS[1], fy, C.HEAD, arrowsText, C.VELLUM, 1)
  if observerSeat() then
    local own = "Observer's own view"
    Ins.text(left + C.LIST_W - 10 - Ins.width(own, C.HEAD), fy, C.HEAD, own, sightShown and C.VELLUM or C.ASH, 1)
  end
  lines[#lines + 1] = {kind = "footer", mid = left + C.LIST_W / 2}
  glanceBox = {left = left, width = C.LIST_W, top = top, line = C.LIST_PITCH, count = #lines, lines = lines}
end

-- What the player's hand changed this played turn, as a key: the tags are made again when it changes.
function Ins.handKey()
  if not (playAs and playAs.active and turnStartState) then return "" end
  local out = {}
  for c in pairs(turnStartState) do if Ins.handTouched(c) then out[#out + 1] = c end end
  table.sort(out)
  return table.concat(out, ",")
end

-- Play as AI: the played AI's panel, in place of the list.
function Ins.drawPartnerPanel()
  local slot = playAs.slot
  local turn, r = latestRecord(slot)
  local name = (playerLabel(slot))
  local lines = {}
  lines[1] = {r and string.format("%s after its turn %d", name, turn) or (name .. ": no AI turn yet"), C.NAME - 1.5, C.VERD}
  lines[#lines + 1] = {"As the AI: " .. Records.resJoin(Ins.resNow(slot)) .. ".", C.LIST, C.VELLUM}
  lines[#lines + 1] = {string.format("Land %s.  Citadels %s.", r and tostring(r.land) or "-", r and tostring(r.citadels) or "-"), C.LIST, C.VELLUM}
  for _, text in ipairs(Records.partnerLines(r)) do lines[#lines + 1] = {text, C.LIST, C.VELLUM} end
  local moved, total = {}, 0
  for c in pairs(commandersOf(slot)) do
    total = total + 1
    if Ins.handTouched(c) then moved[#moved + 1] = (commanderLabel(c):match("^(.-) the ") or commanderLabel(c)) end
  end
  table.sort(moved)
  lines[#lines + 1] = {#moved > 0 and string.format("This turn: %s moved by hand", table.concat(moved, ", "))
    or "This turn: none moved by hand", C.LIST, C.VELLUM}
  local arrowsText = ({[0] = "Arrows: none", "Arrows: every commander's target", "Arrows: the last commander opened"})[settings.arrows] or ""
  lines[#lines + 1] = {arrowsText, C.HEAD, C.ASH}
  local w, pitch = 440, 17
  local h = #lines * pitch + 10
  local left, bottom = 10, 6
  Ins.rect(left, bottom, w, h, C.LAMP, 0.86)
  Ins.rect(left, bottom + h - 1.5, w, 1.5, C.VERD, 1)
  for i, l in ipairs(lines) do
    Ins.text(left + 10, bottom + h - 4 - i * pitch, l[2], Ins.cut(l[1], l[2], w - 20), l[3], 1)
  end
  Ins.partnerBox = {left = left, width = w, arrowsY = bottom + h - 4 - #lines * pitch, pitch = pitch}
end

-- A new or loaded game: the map's caches are another game's.
function Ins.resetMapCaches() Ins.tagCache, Ins.listCache, Ins.emptyView = nil, nil, nil end

coe5.wrap("MAP_DRAW", function(next)
  if game then
    local v = coe5.mapView()
    if v then
      local slot = Ins.mapSlot()
      local tags = Ins.tagsFor(slot)   -- the strip counts from them whether they are drawn or not
      if settings.arrows > 0 then Ins.drawArrows(v) end
      local onScreen = (tags and settings.tags ~= false) and Ins.drawTags(v, tags) or 0
      glanceBox, Ins.stripBox = nil, nil
      Ins.drawStrip(v, slot, tags, onScreen)
      Ins.partnerBox = nil
      if playAs and playAs.active then Ins.drawPartnerPanel()
      elseif settings.list then Ins.drawList(v) end
      if settings.hoverCard ~= false then Ins.drawCard(v, tags) end
    end
  end
  return next()
end)

end   -- the map section

-- Play as AI: the top bar's income is what the game pays the played AI.
coe5.wrap("TOP_BAR_INCOME", function(next, player, resource, income)
  local drawn = next(player, resource, income)
  if settings.topBar and playAs and playAs.active and player == playAs.slot then
    local asAi = coe5.get("slotAiIncome", player, resource)
    if asAi then return asAi end
  end
  return drawn
end)

-- The map's menu button text names the AI the inspector shows and its key.
coe5.wrap("MAP_MENU_BUTTON_TEXT", function(next, text)
  if not game then return next(text) end
  if Ins.picking then return next("Commanders here: pick a square (right click cancels)") end
  local s = playAs and playAs.active and playAs.slot or sightShown
  return next(string.format("%s. AI Inspector (Enter). Map: %s", tostring(text), s and ((playerLabel(s)) .. "'s view")
    or "the Observer's own view"))
end)

-- ---- The game's calls ---------------------------------------------------------------------------
coe5.observe("GAME_LOAD", nil, function(_, path) pendingLoad = baseName(path) end)
coe5.observe("GAME_SAVE", nil, function(result, path)
  if not game or not result or result <= 0 then return end
  readIndex()
  local name = baseName(path)
  local review = playAs and playAs.active and (playAs.slot .. "," .. playAs.seat) or nil
  if index.saves[name] ~= game.id or index.playas[name] ~= review then
    index.saves[name] = game.id
    index.playas[name] = review
    writeIndex()
  end
end)
coe5.observe("GAME_START", nil, function()
  Ins.reportsKept = nil   -- the game just set up or loaded has its own Battle reports value
  local loaded = pendingLoad
  playAs, rec, viewSlot, sightShown, lastCommander, glanceBox, viewGone, seatKept = nil, nil, nil, nil, nil, nil, nil, false
  swallowing = false   -- a click latch from before the load must not take the first click
  Ins.resetMapCaches()
  Ins.appliedTurn = nil
  pending = {}   -- another game's turns: never written
  startGame()
  -- A save made while an AI was played: X played on.
  local slot, seat = (loaded and index.playas[loaded] or ""):match("^(%d+),(%d+)$")
  slot, seat = tonumber(slot), tonumber(seat)
  if slot and seat then
    playAs = {slot = slot, seat = seat, active = true}
    if not runAiTurn then
      playAs.returning = true
      endReview("loaded")
    else
      -- A save made after Return to AI: the seat goes back to the AI.
      local seatToAi = isController(slot, coe5.controller.human) and isController(seat, coe5.controller.human)
        and coe5.get("activePlayer") ~= seat
      if seatToAi then coe5.set("slotController", seat, coe5.controller.ai) end
      coe5.log(string.format("play_as_resumed slot=%d seat=%d turn=%d seat_to_ai=%s", slot, seat, coe5.turn(), tostring(seatToAi)))
    end
  end
end)
coe5.observe("MONTH_START", function() Ins.settleRecords() end, function() maybeStartReview(-1) end)
coe5.observe("PLAYER_TURN_END", nil, function(_, player) maybeStartReview(player) end)
coe5.observe("PLAYER_TURN_START", function(player)
  if isController(player, coe5.controller.human) then Ins.showBattlesAgain() end
  Ins.settleRecords()
  reviewTurnStartBefore(player)
end, function(_, player)
  reviewTurnStartAfter(player)
  if game and isController(player, coe5.controller.ai) then plansAtStart = ritualPlansOf(player) end
end)
coe5.observe("AI_TURN", function(player)
  reviewAiTurnStart(player)
  startRecord(player)
end, function(_, player) Ins.closeAiMove(player) end)
coe5.observe("HUMAN_TURN", function(player)
  notePlayedTurnStart(player)
  Ins.applyView(true)
  Ins.skipTurnStart()
end, function(_, player)
  Ins.endObserverTurn()
  reviewTurnEnd(player)
end)
coe5.wrap("MAIN_MENU_WAIT", function(next, block, mode)
  playAs, game, rec, pending = nil, nil, nil, {}   -- the game it was for is over
  return next(block, mode)
end)

-- Battle reports for the Observer: one per battle an AI faction fights.
local function findObserverSeat()
  for s = 0, 31 do
    local cls = coe5.get("slotClass", s)
    if cls and cls > 0 and coe5.get("className", cls) == "Observer" then return s end
  end
end
coe5.observe("GAME_START", nil, function()
  local reportBattlesTo = rawget(coe5, "reportBattlesTo")
  local seat = findObserverSeat()
  if not (seat and reportBattlesTo) then return end
  if (coe5.get("battleReports") or 0) == 0 then coe5.set("battleReports", 1) end
  local ok = reportBattlesTo(seat)
  coe5.log(string.format("inspector_battle_report_seat seat=%d ok=%s", seat, tostring(ok)))
end)

-- The unit test's Lua is added after this line, in the same chunk: it reads Records.
