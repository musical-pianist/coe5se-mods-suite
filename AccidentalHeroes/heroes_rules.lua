-- Rules for who rises and when

local R = {}

R.TURNS_IN_A_YEAR = 12
R.EXPERIENCE_TO_RISE = 20
R.EXPERIENCE_TO_BECOME_LEGEND = 200
R.LIVING_LEGENDS_MOST = 10
R.EXPERIENCE_LEVELS = {{xp = 20, hp = 1}, {xp = 60, hp = 1}, {xp = 120, hp = 1}, {xp = 200, hp = 2}}
R.LEGEND = {hitPointPercent = 25, hitPointsUpTo = 40, armorUpTo = 3, armor = {0, 1}, moraleBelow = 9, morale = 1, mrUpTo = 13, mr = {0, 2}}
R.LEGEND_TIERS = {rare = 40, legendary = 10}
R.KILLS_NEEDED_ABOVE_LIVING_HEROES = 2
R.MOST_KILLS_NEEDED = 8
R.KILLS_FOR_A_MANY_KILLS_NAME = 10
R.WOUND_OR_KILLS_NAME_PERCENT = 50
R.KIND_NAME_PERCENT = 10
R.WOUND_EPITHET_PERCENT = 50

R.SETTINGS = {
  hitPoints = {smallBelow = 10, smallTimes = 2, percent = 25, most = 30, spread = 1},
  damage = 4,
  initiative = 2,
  leadership = 1,
  morale = {1, 3},
  magicResistance = {1, 2},
  armor = {0, 1},
  armorUpTo = 2,
  rarePercent = 10,
  legendaryPercent = 1,
}

function R.configure(settings)
  local rules = R.SETTINGS
  local apply = {
    hit_point_multiplier = function(value) rules.hitPoints.smallTimes = value end,
    multiplier_stops_at_hit_points = function(value) rules.hitPoints.smallBelow = value end,
    bigger_unit_hit_point_percent = function(value) rules.hitPoints.percent = value end,
    highest_hit_point_bonus = function(value) rules.hitPoints.most = value end,
    hit_point_spread = function(value) rules.hitPoints.spread = value end,
    damage_bonus = function(value) rules.damage = value end,
    initiative_bonus = function(value) rules.initiative = value end,
    leadership = function(value) rules.leadership = value end,
    lowest_morale_bonus = function(value) rules.morale = {value, rules.morale[2]} end,
    highest_morale_bonus = function(value) rules.morale = {rules.morale[1], value} end,
    lowest_magic_resistance_bonus = function(value) rules.magicResistance = {value, rules.magicResistance[2]} end,
    highest_magic_resistance_bonus = function(value) rules.magicResistance = {rules.magicResistance[1], value} end,
    lowest_armor_bonus = function(value) rules.armor = {value, rules.armor[2]} end,
    highest_armor_bonus = function(value) rules.armor = {rules.armor[1], value} end,
    rare_trait_percent = function(value) rules.rarePercent = value end,
    legendary_trait_percent = function(value) rules.legendaryPercent = value end,
  }
  for name, value in pairs(settings or {}) do
    if apply[name] then apply[name](value) end
  end
end

local function rollBetween(range, random)
  local lowest, highest = range[1], range[2] or range[1]
  if highest <= lowest then return lowest end
  return lowest + random(highest - lowest + 1) - 1
end

-- Promotion months
function R.yearOf(turn) return turn // R.TURNS_IN_A_YEAR + 1 end
function R.monthOf(turn) return turn % R.TURNS_IN_A_YEAR end

R.HEROES_A_YEAR = 4

function R.promotionsInYear(year, heroesAYear)
  local early = heroesAYear or R.HEROES_A_YEAR
  if early < 1 then return 1 end
  if year <= 5 then return early end
  if year == 6 then return math.max(1, early - 1) end
  return math.max(1, (early + 1) // 2)
end

function R.monthWeight(monthsApart)
  if monthsApart <= 0 then return 0.2 end
  if monthsApart == 1 then return 0.6 end
  if monthsApart == 2 then return 0.85 end
  return 1
end

function R.periodTurns(heroesAYear)
  return (heroesAYear or R.HEROES_A_YEAR) < 1 and 2 * R.TURNS_IN_A_YEAR or R.TURNS_IN_A_YEAR
end

function R.turnInPeriod(schedule, turn) return turn % (schedule.turns or R.TURNS_IN_A_YEAR) end

function R.weightOf(month, placedMonths, lastYearsFinalMonth, periodTurns)
  local weight = 1
  for _, placed in ipairs(placedMonths) do
    weight = weight * R.monthWeight(math.abs(month - placed))
  end
  if lastYearsFinalMonth then
    weight = weight * R.monthWeight(month + (periodTurns or R.TURNS_IN_A_YEAR) - lastYearsFinalMonth)
  end
  return weight
end

local function drawMonthByWeight(placedMonths, lastYearsFinalMonth, random, periodTurns)
  local lastMonth = periodTurns - 1
  local totalWeight = 0
  for month = 0, lastMonth do
    totalWeight = totalWeight + R.weightOf(month, placedMonths, lastYearsFinalMonth, periodTurns)
  end
  local remaining = random() * totalWeight
  for month = 0, lastMonth do
    remaining = remaining - R.weightOf(month, placedMonths, lastYearsFinalMonth, periodTurns)
    if remaining < 0 then return month end
  end
  return lastMonth
end

function R.drawMonths(count, lastYearsFinalMonth, random, periodTurns)
  local months = {}
  for _ = 1, count do
    months[#months + 1] = drawMonthByWeight(months, lastYearsFinalMonth, random, periodTurns or R.TURNS_IN_A_YEAR)
  end
  table.sort(months)
  return months
end

function R.ensurePeriod(schedule, turn, random, heroesAYear)
  local periodTurns = R.periodTurns(heroesAYear)
  local sameLength = (schedule.turns or R.TURNS_IN_A_YEAR) == periodTurns
  local period = turn // periodTurns + 1
  if schedule.period == period and sameLength then return false end
  local lastFinalMonth = sameLength and schedule.period == period - 1 and schedule.final or nil
  local drawn = R.drawMonths(R.promotionsInYear(period, heroesAYear), lastFinalMonth, random, periodTurns)
  local thisMonth, monthsToCome = turn % periodTurns, {}
  for _, month in ipairs(drawn) do
    if month >= thisMonth then monthsToCome[#monthsToCome + 1] = month end
  end
  schedule.period, schedule.turns, schedule.months, schedule.final = period, periodTurns, monthsToCome, drawn[#drawn]
  schedule.owed = schedule.owed or {}
  return true
end

function R.takeDue(schedule, turn)
  local thisMonth, dueNow, monthsToCome = R.turnInPeriod(schedule, turn), 0, {}
  for _, month in ipairs(schedule.months or {}) do
    if month == thisMonth then
      dueNow = dueNow + 1
    else
      monthsToCome[#monthsToCome + 1] = month
    end
  end
  schedule.months = monthsToCome
  return dueNow
end

function R.dropForFirstHero(schedule)
  local months = schedule.months or {}
  if #months > 0 then
    table.remove(months)
    return "month"
  end
  local owed = schedule.owed or {}
  if #owed > 0 then
    table.remove(owed)
    return "owed"
  end
  return nil
end

-- Who rises
R.HELD_BACK_UNIT_BITS = {25, 29, 47, 55, 62}
R.HOLDING_LEADER_BITS = {25, 29, 47, 62}

local function hasAnyBit(moveFlags, bits)
  if not moveFlags then return false end
  for _, bit in ipairs(bits) do
    if moveFlags & (1 << bit) ~= 0 then return true end
  end
  return false
end

-- leader: nil for none, else {moveFlags =, wanders =}.
function R.isHeldBack(moveFlags, leader)
  if hasAnyBit(moveFlags, R.HELD_BACK_UNIT_BITS) then return true end
  return leader ~= nil and (leader.wanders or hasAnyBit(leader.moveFlags, R.HOLDING_LEADER_BITS))
end

function R.bar(livingHeroes, monthsOwed)
  local killsNeeded = math.min(R.MOST_KILLS_NEEDED, livingHeroes + R.KILLS_NEEDED_ABOVE_LIVING_HEROES)
  return math.max(0, killsNeeded - monthsOwed)
end

function R.before(a, b)
  if a.kills ~= b.kills then return a.kills > b.kills end
  if a.xp ~= b.xp then return a.xp > b.xp end
  return a.unit < b.unit
end

function R.pickBest(candidates, killsNeeded)
  local best, place
  for i, candidate in ipairs(candidates) do
    if candidate.kills >= killsNeeded and (not best or R.before(candidate, best)) then
      best, place = candidate, i
    end
  end
  return best, place
end

function R.settle(schedule, turn, candidates, livingHeroes, neverHadHero, raise)
  local raised = {}

  local function promoteBest(killsNeeded, kind)
    local best, place = R.pickBest(candidates, killsNeeded)
    if not best then return nil end
    if not raise(best, kind) then return false end
    table.remove(candidates, place)
    livingHeroes = livingHeroes + 1
    raised[#raised + 1] = {candidate = best, kind = kind}
    return true
  end

  if neverHadHero and #candidates > 0 then
    local promoted = promoteBest(0, "first")
    if promoted == false then return raised end
    if promoted then R.dropForFirstHero(schedule) end
  end

  local dueNow = R.takeDue(schedule, turn)
  local owed = schedule.owed
  if #owed > 0 then
    local monthsOwed = turn - owed[1]
    local promoted = promoteBest(R.bar(livingHeroes, monthsOwed), "owed")
    if promoted == false then
      for _ = 1, dueNow do owed[#owed + 1] = turn end
      return raised
    end
    if promoted then table.remove(owed, 1) end
  end

  local gameRefused = false
  for _ = 1, dueNow do
    local promoted = not gameRefused and promoteBest(R.bar(livingHeroes, 0), "due")
    if promoted == false then gameRefused = true end
    if not promoted then owed[#owed + 1] = turn end
  end
  return raised
end

-- Base boost
function R.hpBoost(hitPoints)
  local rule = R.SETTINGS.hitPoints
  if hitPoints < rule.smallBelow then
    return math.max(0, hitPoints * (rule.smallTimes - 1))
  end
  local gainAtTheLine = rule.smallBelow * (rule.smallTimes - 1)
  local percentGain = (hitPoints * rule.percent + 50) // 100
  return math.min(rule.most, math.max(gainAtTheLine, percentGain))
end

function R.baseBoost(hitPoints, armor, random)
  local rules = R.SETTINGS
  local spread = rules.hitPoints.spread or 0
  local hasLittleArmor = armor <= rules.armorUpTo
  return {
    hp = math.max(1, R.hpBoost(hitPoints) + rollBetween({-spread, spread}, random)),
    damage = rules.damage,
    init = rules.initiative,
    leadership = rules.leadership,
    morale = rollBetween(rules.morale, random),
    mr = rollBetween(rules.magicResistance, random),
    armor = hasLittleArmor and rollBetween(rules.armor, random) or 0,
  }
end

-- Trait roll
local function rowsWithTraitsLeft(rows, tier, available)
  local rowsLeft = {}
  for _, row in ipairs(rows) do
    if row.tier == tier then
      local traitsLeft = {}
      for _, trait in ipairs(row) do
        if available(trait) then traitsLeft[#traitsLeft + 1] = trait end
      end
      if #traitsLeft > 0 then rowsLeft[#rowsLeft + 1] = traitsLeft end
    end
  end
  return rowsLeft
end

local TIER_ORDER = {
  legendary = {"legendary", "rare", "common"},
  rare = {"rare", "common", "legendary"},
  common = {"common", "rare", "legendary"},
}

function R.tierOf(roll, tiers)
  local legendary = tiers and tiers.legendary or R.SETTINGS.legendaryPercent
  local rare = tiers and tiers.rare or R.SETTINGS.rarePercent
  if roll <= legendary then return "legendary" end
  if roll <= legendary + rare then return "rare" end
  return "common"
end

function R.rollTrait(rows, available, random, tiers)
  for _, tier in ipairs(TIER_ORDER[R.tierOf(random(100), tiers)]) do
    local rowsLeft = rowsWithTraitsLeft(rows, tier, available)
    if #rowsLeft > 0 then
      local traitsLeft = rowsLeft[random(#rowsLeft)]
      return traitsLeft[random(#traitsLeft)]
    end
  end
  return nil
end

-- Becoming a legend
function R.becomesLegend(hero, xp, livingLegends)
  return not hero.deathTurn and not hero.legendTurn and (xp or 0) >= R.EXPERIENCE_TO_BECOME_LEGEND
    and (livingLegends or 0) < R.LIVING_LEGENDS_MOST
end

function R.experienceHitPoints(xp)
  local hitPoints = 0
  for _, level in ipairs(R.EXPERIENCE_LEVELS) do
    if (xp or 0) >= level.xp then hitPoints = hitPoints + level.hp end
  end
  return hitPoints
end

function R.heroMostHitPoints(hero, xp)
  local atRise = hero.bornHp or 0
  return atRise - R.experienceHitPoints(hero.bornXp) + R.experienceHitPoints(xp) + (hero.hpBoost or 0)
end

function R.legendBoost(stats, random)
  local rule = R.LEGEND
  local hitPoints = 0
  if stats.hp < rule.hitPointsUpTo then
    hitPoints = math.max(0, math.min(rule.hitPointsUpTo, (stats.hp * (100 + rule.hitPointPercent) + 50) // 100) - stats.hp)
  end
  local mr = 0
  if stats.mr < rule.mrUpTo then mr = math.min(rollBetween(rule.mr, random), rule.mrUpTo - stats.mr) end
  return {
    hp = hitPoints,
    armor = stats.armor <= rule.armorUpTo and rollBetween(rule.armor, random) or 0,
    morale = stats.morale < rule.moraleBelow and rule.morale or 0,
    mr = mr,
  }
end

-- Names and titles
function R.joinTitle(short, epithet)
  local startsWithComma = epithet:sub(1, 1) == ","
  if startsWithComma then return short .. epithet end
  return short .. " " .. epithet
end

local function namesNotUsed(list, used)
  local left = {}
  for _, name in ipairs(list or {}) do
    if not (used and used[name]) then left[#left + 1] = name end
  end
  return left
end

function R.drawName(lists, listOrder, usedByList, random)
  for _, listName in ipairs(listOrder) do
    local left = namesNotUsed(lists[listName], usedByList[listName])
    if #left > 0 then
      local name = left[random(#left)]
      usedByList[listName] = usedByList[listName] or {}
      usedByList[listName][name] = true
      return name, listName
    end
  end
  local lastListName = listOrder[#listOrder]
  local lastList = lists[lastListName]
  local name = lastList[random(#lastList)]
  usedByList[lastListName] = {[name] = true}
  return name, lastListName
end

function R.listToNameAfter(nameLists, percent, random)
  if #nameLists == 0 or random(100) > percent then return nil end
  return nameLists[random(#nameLists)]
end

function R.woundAndKillNameLists(woundLists, kills)
  local nameLists = {}
  for _, listName in ipairs(woundLists) do nameLists[#nameLists + 1] = listName end
  if kills >= R.KILLS_FOR_A_MANY_KILLS_NAME then nameLists[#nameLists + 1] = "manyKills" end
  return nameLists
end

function R.shortNameListToNameAfter(woundLists, kills, kindLists, random)
  local woundOrKillList = R.listToNameAfter(R.woundAndKillNameLists(woundLists, kills), R.WOUND_OR_KILLS_NAME_PERCENT, random)
  return woundOrKillList or R.listToNameAfter(kindLists, R.KIND_NAME_PERCENT, random)
end

function R.epithetOrder(traitList)
  if traitList == "fallback" then return {"fallback"} end
  return {traitList, "fallback"}
end

return R
