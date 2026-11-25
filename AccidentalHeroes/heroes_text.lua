-- Message, card and list text

local X = {}

X.BORN_TITLE = "Risen from the Ranks"
X.DEATH_TITLE = "Hero Fallen"
X.LEGEND_TITLE = "Risen to Legend"

X.NOT_OWN_TAIL = " (enemy controlled)"

local UNKNOWN_MONTH = "an unknown month"

local BASE_BOOSTS_AFTER_HIT_POINTS = {
  {field = "damageBoost", label = "Damage"},
  {field = "initBoost", label = "Initiative"},
  {field = "leadershipBoost", label = "Leadership"},
  {field = "moraleBoost", legendField = "legendMoraleBoost", label = "Morale"},
  {field = "mrBoost", legendField = "legendMrBoost", label = "Magic Resistance"},
  {field = "armorBoost", legendField = "legendArmorBoost", label = "Armor"},
}

function X.bornText(hero, entry, ownerNameForOtherPlayers)
  local telling, hail = entry[1], entry[2]
  local lines = {telling, string.format(hail, hero.title)}
  if ownerNameForOtherPlayers then
    lines[#lines + 1] = string.format("Sworn to %s.", ownerNameForOtherPlayers)
  end
  return table.concat(lines, "\n")
end

function X.legendText(hero, ownerNameForOtherPlayers)
  local lines = {hero.title .. " has become a legend."}
  if hero.legendTraitLabel then lines[#lines + 1] = hero.legendTraitLabel end
  if ownerNameForOtherPlayers then
    lines[#lines + 1] = string.format("Sworn to %s.", ownerNameForOtherPlayers)
  end
  return table.concat(lines, "\n")
end

function X.deathText(hero, ownerNameForOtherPlayers, epitaph)
  local nameOnTheMap = hero.short or hero.title
  local lines = {epitaph and epitaph:gsub("%%s", function() return nameOnTheMap end) or (hero.title .. " is dead.")}
  if ownerNameForOtherPlayers then
    lines[#lines + 1] = string.format("Sworn to %s.", ownerNameForOtherPlayers)
  end
  return table.concat(lines, "\n")
end

function X.placeName(shownName, noPlayerOwnsTheSquare)
  if not shownName then return nil end
  local endsWithTail = shownName:sub(-#X.NOT_OWN_TAIL) == X.NOT_OWN_TAIL
  if noPlayerOwnsTheSquare and endsWithTail then
    return shownName:sub(1, -#X.NOT_OWN_TAIL - 1)
  end
  return shownName
end

function X.baseLine(hero)
  local parts = {string.format("Hit Points +%d", hero.hpBoost + (hero.legendHpBoost or 0))}
  for _, boost in ipairs(BASE_BOOSTS_AFTER_HIT_POINTS) do
    local amount = (hero[boost.field] or 0) + (boost.legendField and hero[boost.legendField] or 0)
    if amount > 0 then parts[#parts + 1] = string.format("%s +%d", boost.label, amount) end
  end
  return table.concat(parts, ", ")
end

function X.cardText(hero, monsterName)
  local rose = string.format("A %s risen from the ranks in %s", monsterName, hero.date or UNKNOWN_MONTH)
  if hero.place and hero.place ~= "" then
    rose = rose .. ", at " .. hero.place
  end
  local lines = {rose .. ".", X.baseLine(hero)}
  if hero.traitLabel then lines[#lines + 1] = hero.traitLabel end
  if hero.legendTraitLabel then lines[#lines + 1] = hero.legendTraitLabel end
  return table.concat(lines, "\n")
end

function X.listTitle(hero, ownerNameForOtherPlayers)
  if ownerNameForOtherPlayers then
    return string.format("%s (%s)", hero.title, ownerNameForOtherPlayers)
  end
  return hero.title
end

function X.listLine(hero, monsterName)
  local parts = {monsterName}
  if hero.traitLabel then parts[#parts + 1] = hero.traitLabel end
  if hero.legendTraitLabel then parts[#parts + 1] = hero.legendTraitLabel end
  if hero.deathTurn then parts[#parts + 1] = "Fell in " .. (hero.deathDate or UNKNOWN_MONTH) end
  return table.concat(parts, ". ")
end

return X
