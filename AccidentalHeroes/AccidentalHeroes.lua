-- Hero settings
local SETTINGS = {
  hit_point_multiplier = 2,
  multiplier_stops_at_hit_points = 10,
  bigger_unit_hit_point_percent = 25,
  highest_hit_point_bonus = 30,
  hit_point_spread = 1,
  damage_bonus = 4,
  initiative_bonus = 2,
  leadership = 1,
  lowest_morale_bonus = 1,
  highest_morale_bonus = 3,
  lowest_magic_resistance_bonus = 1,
  highest_magic_resistance_bonus = 2,
  lowest_armor_bonus = 0,
  highest_armor_bonus = 1,
  rare_trait_percent = 10,
  legendary_trait_percent = 1,
}

-- Hero traits: 1 is on, 0 is off.
local TRAITS = {
  -- Common
  strength = {1, "common", 2},          -- Strength
  armor = {1, "common", 1},             -- Armor
  morale = {1, "common", 3},            -- Morale
  mr = {1, "common", 2},                -- Magic Resistance
  pierceres = {1, "common"},            -- Pierce Resistance
  slashres = {1, "common"},             -- Slash Resistance
  bluntres = {1, "common"},             -- Blunt Resistance
  fireres = {1, "common", 50},          -- Fire Resistance
  coldres = {1, "common", 50},          -- Cold Resistance
  shockres = {1, "common", 50},         -- Shock Resistance
  poisonres = {1, "common", 50},        -- Poison Resistance
  sleepres = {1, "common"},             -- Sleep Immunity
  charmres = {1, "common"},             -- Charm Immunity
  diseaseres = {0, "common"},           -- Disease Resistance
  affres = {0, "common", 75},           -- Affliction Resistance
  fastheal = {1, "common"},             -- Fast Heal
  spiritsight = {1, "common"},          -- Spirit Sight
  acutesenses = {1, "common"},          -- Acute Senses
  farsight = {1, "common", 1},          -- Farsight
  stealth = {1, "common"},              -- Stealth
  foreststealth = {1, "common"},        -- Forest Stealth
  desertstealth = {1, "common"},        -- Desert Stealth
  snowstealth = {1, "common"},          -- Snow Stealth
  fast = {1, "common"},                 -- Fast
  swamp = {1, "common"},                -- Swamp Move
  mountain = {1, "common"},             -- Mountain Move
  desert = {1, "common"},               -- Desert Move
  snow = {1, "common"},                 -- Snow Move
  water = {1, "common"},                -- Amphibian
  float = {1, "common"},                -- Floating
  wall = {1, "common"},                 -- Wall Climbing
  shield = {1, "common"},               -- Shield
  largeshield = {1, "common"},          -- Large Shield
  iceprot = {1, "common", 1},           -- Ice Armor
  fireaura = {1, "common", 5},          -- Fire Shield
  coldaura = {1, "common", 2},          -- Frost Shield
  poisonaura = {1, "common", 4},        -- Poison
  airshield = {1, "common", 50},        -- Air Shield
  awe = {1, "common", 1},               -- Awe
  leadership = {1, "common", 2},        -- Leadership
  wall_cover = {1, "common", 1},        -- Wall Cover
  wall_range_bonus = {1, "common", 1},  -- Wall Range Bonus
  siegetunnel = {1, "common", 5},       -- Siege Tunneling
  saner = {1, "common", 1},             -- Regain Sanity
  voidsanity = {0, "common", 20},       -- Void Sanity
  unaging = {1, "common"},              -- Unaging
  tangleres = {0, "common", 1},         -- Tangle Vine Immunity
  hadesres = {1, "common", 1},          -- Hades Immunity
  coldheal = {1, "common", 4},          -- Cold Heal
  slayundead = {0, "common"},           -- Double damage vs undead
  slaydemons = {0, "common"},           -- Double damage vs demons
  slayanimals = {0, "common"},          -- Double damage vs animals
  slayhumans = {0, "common"},           -- Double damage vs humans
  range_bonus = {1, "common", 4},       -- Range Bonus
  spellrange = {1, "common", 1},        -- Spell Range Bonus
  diseaseshield = {1, "common", 1},     -- Disease
  slimeshield = {1, "common", 1},       -- Slimy
  awakentrees = {1, "common", 3},       -- Awaken Trees
  awakenfungus = {1, "common", 4},      -- Awaken Mushrooms
  farsightsea = {1, "common", 3},       -- Sea Farsight
  berserker = {1, "common"},            -- Berserker
  holy = {1, "common"},                 -- Blessed
  putridexpl = {1, "common", 3},        -- Putrid Explosion on Death
  mindexpl = {1, "common", 1},          -- Mental Agony on Death
  shardexpl = {1, "common", 6},         -- Shard Explosion on Death
  fireexpl = {1, "common", 12},         -- Fire Explosion on Death
  poisonexpl = {1, "common", 10},       -- Poison Explosion on Death
  acidres = {1, "common", 100},         -- Acid Resistance
  acidblood = {1, "common", 50},        -- Acid Blood
  passwall = {1, "common"},             -- Passwall
  goldbonus = {0, "common", 25},        -- Gold Bonus
  limitgold = {1, "common", 3},         -- Limited Gold Bonus, Limited Iron Bonus
  shockaura = {1, "common", 5},         -- Shock Shield
  poisoncloud = {1, "common", 3},       -- Poison Aura
  fear = {1, "common"},                 -- Causes Fear
  invisible = {1, "common"},            -- Invisible
  flying = {1, "common"},               -- Flying
  teleport = {1, "common"},             -- Teleporting
  trample = {1, "common", 2},           -- Trample
  extraeyes = {0, "common", 6},         -- Extra Eyes
  -- Rare
  fireimmune = {1, "rare"},             -- Fire Resistance (100)
  coldimmune = {1, "rare"},             -- Cold Resistance (100)
  poisonimmune = {1, "rare"},           -- Poison Resistance (100)
  shockimmune = {1, "rare"},            -- Shock Resistance (100)
  affres100 = {0, "rare", 100},         -- Affliction Resistance
  ethereal = {1, "rare"},               -- Ethereal
  regeneration = {1, "rare"},           -- Regeneration
  lucky = {1, "rare"},                  -- Lucky
  evasion = {1, "rare", 1},             -- Evasion
  displaced = {1, "rare", 1},           -- Displaced
  mirrorimages = {1, "rare", 3},        -- Constant Mirror Images
  magicshield = {1, "rare"},            -- Magic Shield
  doubledamage = {0, "rare"},           -- Double damage
  tunnel = {1, "rare"},                 -- Tunneling
  tunnelmove = {1, "rare", 6},          -- Combat Burrowing
  armytrainer = {1, "rare", 1},         -- Army Trainer
  scry = {0, "rare", 15},               -- Scry
  twistfate = {0, "rare", 1},           -- Twist Fate
  combatcaster = {1, "rare"},           -- Combat Caster
  pathbonus = {0, "rare", 1},           -- Magic Boost
  seduceaura = {1, "rare", 5},          -- Seduction
  petriaura = {1, "rare", 3},           -- Petrification
  -- Legendary
  invul = {1, "legendary"},             -- Invulnerability
  nonmaginvul = {1, "legendary"},       -- Non-Magical Invulnerability
  immortal = {1, "legendary"},          -- Immortal
  absorbdead = {1, "legendary", 3},     -- Absorb Dead
}

local Rules = require("heroes_rules")
Rules.configure(SETTINGS)
local Traits = require("heroes_traits")
for _, problem in ipairs(Traits.configure(TRAITS)) do
  coe5.log(string.format("heroes_trait_setting_skipped key=%s reason=%s", problem.key, problem.reason))
end
local Text = require("heroes_text")
local Calls = require("heroes_calls")
local Names = require("names")
local Messages = require("heroes_messages")

local HERO_AFFLICTION = "Accidental Hero"
local HEROES_SHOWN_ALL_PLAYERS = 1
local HEROES_A_YEAR_OPTIONS = {[0] = 0.5, 1, 2, 3, 4}

local LAST_PLAYER_SLOT = 31
local LAST_ATTACK_ROW = 15
local LAST_ITEM_PLACE = 19

local SHIP_ABILITY = 0x1d2
local LEADERSHIP_ABILITY = 0x8

local SPELL_ATTACK_TYPES = {[3] = true, [6] = true, [14] = true, [15] = true, [19] = true}
local RANGED_ATTACK_TYPES = {[2] = true, [5] = true, [7] = true, [11] = true, [18] = true, [21] = true}
local DAMAGE_TYPES_NEVER_DOUBLED = {
  [13] = "special benefit", [18] = "summon", [21] = "mirror image", [25] = "animate", [26] = "heal",
}

local MOVE_BIT = {stupid = 25, loner = 29}
local STATE_BIT = {battleSummon = 17, removedAfterBattle = 42}
local AFFLICTION_STATE_BITS = {
  {nameList = "blind", bits = {26}},
  {nameList = "lostEye", bits = {20, 21}},
  {nameList = "chestWound", bits = {22}},
  {nameList = "neverHealingWound", bits = {23}},
  {nameList = "battleFright", bits = {24}},
  {nameList = "feeblemind", bits = {25}},
  {nameList = "disease", bits = {0}},
  {nameList = "curse", bits = {1}},
  {nameList = "insanity", bits = {13, 27}},
  {nameList = "horrormark", bits = {14, 15}},
  {nameList = "debilitatingWound", bits = {29}},
  {nameList = "festeringWound", bits = {30}},
}
local KIND_FLAG_BITS = {
  {nameList = "animal", bit = 11},
  {nameList = "undead", bit = 4},
  {nameList = "demonic", bit = 33},
}

local BOOST_FIELDS_AT_SITE = {
  UNIT_DAMAGE_BONUS = {"damageBoost"},
  UNIT_ARMOR = {"armorBoost", "legendArmorBoost"},
  UNIT_MORALE = {"moraleBoost", "legendMoraleBoost"},
  UNIT_MR = {"mrBoost", "legendMrBoost"},
  ATTACK_INIT = {"initBoost"},
}
local STAT_SITES = {
  "UNIT_DAMAGE_BONUS", "UNIT_ARMOR", "UNIT_MORALE", "UNIT_MR", "UNIT_FIRE_RESISTANCE", "UNIT_PATH_BONUS", "ATTACK_INIT",
}

local RESISTANCE_TO = {
  fire = coe5.unitFireResistance,
  cold = coe5.unitColdResistance,
  shock = coe5.unitShockResistance,
  poison = coe5.unitPoisonResistance,
  acid = coe5.unitAcidResistance,
}

local function numbersOf(list)
  local numbers = {}
  for i = 1, #list do numbers[i] = i end
  return numbers
end

local BORN_ENTRY_NUMBERS = {born = numbersOf(Messages.born)}
local DEATH_ENTRY_NUMBERS = {death = numbersOf(Messages.death)}

local function hasBit(word, bit)
  return word ~= nil and word & (1 << bit) ~= 0
end

local function savedGame()
  local saved = coe5.gameData
  saved.heroes = saved.heroes or {}
  saved.players = saved.players or {}
  return saved
end

local function heroOfUnit(unit)
  local heroNumber = coe5.afflictionValue(unit, HERO_AFFLICTION)
  local roster = heroNumber and coe5.gameData.heroes
  local hero = roster and roster[heroNumber]
  return hero, heroNumber
end

local function traitsOfHero(hero)
  local traits = {}
  for _, key in ipairs({hero and hero.trait or false, hero and hero.legendTrait or false}) do
    local trait = key and Traits.byKey[key]
    if trait then traits[#traits + 1] = trait end
  end
  return traits
end

local function traitsOfUnit(unit)
  return traitsOfHero(heroOfUnit(unit))
end

local function monsterName(monster)
  return coe5.get("monsterName", monster) or "?"
end

local function everyPlayerSeesEveryHero()
  return coe5.setting("heroes_shown") == HEROES_SHOWN_ALL_PLAYERS
end

local function heroesAYear()
  return HEROES_A_YEAR_OPTIONS[coe5.setting("heroes_a_year")] or Rules.HEROES_A_YEAR
end

local function aiPlayersRaiseHeroes()
  return coe5.setting("ai_heroes") ~= false
end

local function playerRaisesHeroes(player)
  if player < 0 or player > LAST_PLAYER_SLOT then return false end
  local controller = coe5.get("playerController", player)
  if controller == coe5.controller.human then return true end
  return controller == coe5.controller.ai and aiPlayersRaiseHeroes()
end

local function noPlayerOwnsSquare(x, y)
  local owner = coe5.get("squareOwner", x, y) or -1
  return owner < 0 or coe5.get("playerController", owner) == coe5.controller.independent
end

local function humanPlayers()
  local players = {}
  for player = 0, LAST_PLAYER_SLOT do
    if coe5.get("playerController", player) == coe5.controller.human then players[#players + 1] = player end
  end
  return players
end

-- Stat bonuses
coe5.afflictionEffect(HERO_AFFLICTION, "UNIT_MAX_HP", function(unit, hitPoints)
  local hero = heroOfUnit(unit)
  if hero and hero.hpBoost then return hitPoints + hero.hpBoost + (hero.legendHpBoost or 0) end
end)

local function statWithHeroBonuses(site, unit, value)
  local hero = heroOfUnit(unit)
  if not hero then return nil end
  local baseBoost = 0
  for _, field in ipairs(BOOST_FIELDS_AT_SITE[site] or {}) do baseBoost = baseBoost + (hero[field] or 0) end
  local traitChangedIt = false
  for _, trait in ipairs(traitsOfHero(hero)) do
    if trait.route == "site" and trait.site == site then
      value = trait.atLeast and math.max(value --[[@as integer]], trait.atLeast) or value + trait.add
      traitChangedIt = true
    end
  end
  if not traitChangedIt and baseBoost == 0 then return nil end
  return value + baseBoost
end

for _, site in ipairs(STAT_SITES) do
  coe5.afflictionEffect(HERO_AFFLICTION, site, function(unit, value)
    return statWithHeroBonuses(site, unit, value)
  end)
end

local function flagsWithTraitsOfRoute(route)
  return function(unit, flags)
    local withTraits = flags
    for _, trait in ipairs(traitsOfUnit(unit)) do
      if trait.route == route then withTraits = withTraits | (1 << trait.bit) end
    end
    if withTraits ~= flags then return withTraits end
  end
end

coe5.afflictionEffect(HERO_AFFLICTION, "UNIT_RESISTANCES", flagsWithTraitsOfRoute("flag"))
coe5.afflictionEffect(HERO_AFFLICTION, "UNIT_FLAGS", flagsWithTraitsOfRoute("move"))

-- Abilities: Leadership and the ability traits
local abilityAmountsKept = {}

local function abilityAmountsOfHero(heroNumber, hero)
  local kept = abilityAmountsKept[heroNumber]
  local stillRight = kept and kept.trait == hero.trait and kept.legendTrait == hero.legendTrait and kept.leadership == hero.leadershipBoost
  if stillRight then return kept.amounts end
  local amounts = {}
  if (hero.leadershipBoost or 0) ~= 0 then amounts[LEADERSHIP_ABILITY] = hero.leadershipBoost end
  for _, trait in ipairs(traitsOfHero(hero)) do
    for _, given in ipairs(trait.abilities or {}) do amounts[given.id] = (amounts[given.id] or 0) + given.value end
  end
  abilityAmountsKept[heroNumber] = {trait = hero.trait, legendTrait = hero.legendTrait, leadership = hero.leadershipBoost, amounts = amounts}
  return amounts
end

coe5.afflictionEffect(HERO_AFFLICTION, "UNIT_ABILITY", function(unit, value, ability)
  local heroNumber = coe5.afflictionValue(unit, HERO_AFFLICTION)
  local roster = heroNumber and coe5.gameData.heroes
  local hero = roster and roster[heroNumber]
  if not hero then return nil end
  local amount = abilityAmountsOfHero(heroNumber, hero)[ability]
  if amount then return value + amount end
  return nil
end)

-- Double damage
local function isWeaponOfMonster(monster, weapon)
  for row = 0, LAST_ATTACK_ROW do
    local attackType = coe5.get("monsterAttackType", monster, row)
    local isSpell = SPELL_ATTACK_TYPES[attackType]
    if attackType and not isSpell and coe5.get("monsterAttackWeapon", monster, row) == weapon then return true end
  end
  return false
end

local function isWeaponOfItemHeld(unit, weapon)
  for place = 0, LAST_ITEM_PLACE do
    local item = coe5.get("unitItem", unit, place)
    if item and item >= 0 and coe5.get("item", item, "itemwep") == weapon then return true end
  end
  return false
end

local function traitDoublesHitOn(trait, target)
  if trait.route ~= "hit" then return false end
  local slaysOnlyOneKind = trait.slay ~= nil
  return not slaysOnlyOneKind or hasBit(coe5.get("unitFlags", target), trait.slay)
end

local function hitIsDoubled(attacker, target, weapon, damageType)
  if attacker < 0 or target < 0 or DAMAGE_TYPES_NEVER_DOUBLED[damageType] then return false end
  local doubled = false
  for _, trait in ipairs(traitsOfUnit(attacker)) do
    doubled = doubled or traitDoublesHitOn(trait, target)
  end
  if not doubled then return false end
  return isWeaponOfMonster(coe5.get("unitMonster", attacker), weapon) or isWeaponOfItemHeld(attacker, weapon)
end

coe5.wrap("WEAPON_HIT", function(next, attacker, target, weapon, damage, value, damageType)
  local damageDealt = next(attacker, target, weapon, damage, value, damageType)
  if damageDealt and damageDealt > 0 and hitIsDoubled(attacker, target, weapon, damageType) then
    return damageDealt * 2
  end
  return damageDealt
end)

-- Hero's card line
Calls.cardLine(HERO_AFFLICTION, function(_, heroNumber)
  local roster = coe5.gameData.heroes
  local hero = roster and roster[heroNumber]
  if not hero then return nil end
  local label, clickText = hero.title, Text.cardText(hero, monsterName(hero.monster))
  return label, clickText
end)

-- Which traits a unit can take
local function hasRangedWeapon(monster)
  for row = 0, LAST_ATTACK_ROW do
    if RANGED_ATTACK_TYPES[coe5.get("monsterAttackType", monster, row)] then return true end
  end
  return false
end

local function alreadyHasTrait(unit, trait)
  if trait.element then
    local resistance = RESISTANCE_TO[trait.element](unit) or 0
    return resistance >= (trait.full and 100 or trait.value)
  end
  if trait.route == "flag" then
    local flags = coe5.get("unitFlags", unit)
    if hasBit(flags, trait.bit) or (trait.alsoBit and hasBit(flags, trait.alsoBit)) then return true end
    return trait.alsoAbility ~= nil and (coe5.get("unitAbility", unit, trait.alsoAbility) or 0) > 0
  end
  if trait.route == "move" then
    return hasBit(coe5.get("unitMoveFlags", unit), trait.bit)
  end
  if trait.route == "ability" then
    local first = trait.abilities[1]
    local total = coe5.get("unitAbility", unit, first.id) or 0
    return total >= first.value or (total > 0 and not trait.orMore)
  end
  return false
end

local function unitMayTakeTrait(unit, monster, trait)
  if trait.needs == "ranged" and not hasRangedWeapon(monster) then return false end
  if trait.needs == "caster" and not coe5.unitHasSpellAttack(unit) then return false end
  return not alreadyHasTrait(unit, trait)
end

local function heroMayTakeLegendTrait(hero, unit, monster, trait)
  local first = Traits.byKey[hero.trait or ""]
  if first then
    if trait.key == first.key then return false end
    local sameBit = trait.route == first.route and trait.bit ~= nil and (trait.bit == first.bit or trait.alsoBit == first.bit)
    local firstDoublesEveryHit = first.route == "hit" and first.slay == nil
    if sameBit or (firstDoublesEveryHit and trait.route == "hit") then return false end
  end
  return unitMayTakeTrait(unit, monster, trait)
end

-- Name and epithet
local function afflictionNameListsOf(unit)
  local stateFlags = coe5.get("unitStateFlags", unit)
  local nameLists = {}
  for _, affliction in ipairs(AFFLICTION_STATE_BITS) do
    for _, bit in ipairs(affliction.bits) do
      if hasBit(stateFlags, bit) then
        nameLists[#nameLists + 1] = affliction.nameList
        break
      end
    end
  end
  return nameLists
end

local function kindNameListsOf(unit)
  local flags = coe5.get("unitFlags", unit)
  local nameLists = {}
  for _, kind in ipairs(KIND_FLAG_BITS) do
    if hasBit(flags, kind.bit) then nameLists[#nameLists + 1] = kind.nameList end
  end
  return nameLists
end

local function shortNameFor(commander, listToNameAfter, usedShortNames)
  local gameName = coe5.get("commanderName", commander)
  local listHasNames = listToNameAfter ~= nil and Names.short[listToNameAfter] ~= nil
  local gameGaveAName = gameName ~= nil and gameName ~= ""
  if gameGaveAName and not listHasNames then return gameName end
  local nameList = listHasNames and listToNameAfter or "any"
  local name = Rules.drawName(Names.short, {nameList}, usedShortNames, math.random)
  coe5.set("commanderName", commander, name)
  return name
end

local function epithetFor(trait, afflictionToNameAfter, playerRecord)
  if afflictionToNameAfter and Names.woundEpithets[afflictionToNameAfter] then
    playerRecord.usedWoundEpithet = playerRecord.usedWoundEpithet or {}
    local epithet = Rules.drawName(Names.woundEpithets, {afflictionToNameAfter}, playerRecord.usedWoundEpithet, math.random)
    return epithet
  end
  local traitList = trait and trait.names or "fallback"
  local epithet = Rules.drawName(Names.epithets, Rules.epithetOrder(traitList), playerRecord.usedEpithet, math.random)
  return epithet
end

-- Born and death messages
local function sendHeroNews(hero, title, textForOwner, textForOthers, goTo)
  local messagesSent = 0
  for _, player in ipairs(humanPlayers()) do
    if player == hero.owner then
      Calls.message(player, title, textForOwner, goTo)
      messagesSent = messagesSent + 1
    elseif everyPlayerSeesEveryHero() then
      Calls.message(player, title, textForOthers)
      messagesSent = messagesSent + 1
    end
  end
  return messagesSent
end

local function announceBorn(hero, playerRecord, goTo)
  playerRecord.usedBorn = playerRecord.usedBorn or {}
  local entryNumber = Rules.drawName(BORN_ENTRY_NUMBERS, {"born"}, playerRecord.usedBorn, math.random)
  local entry = Messages.born[entryNumber]
  local ownerName = coe5.get("playerName", hero.owner)
  return sendHeroNews(hero, Text.BORN_TITLE, Text.bornText(hero, entry), Text.bornText(hero, entry, ownerName), goTo)
end

local function announceDeath(hero, saved)
  local playerRecord = saved.players[hero.owner] or {}
  playerRecord.usedDeath = playerRecord.usedDeath or {}
  local epitaphNumber = Rules.drawName(DEATH_ENTRY_NUMBERS, {"death"}, playerRecord.usedDeath, math.random)
  local epitaph = Messages.death[epitaphNumber]
  local ownerName = coe5.get("playerName", hero.owner)
  local lastSeenAt = hero.x and hero.y and {x = hero.x, y = hero.y} or nil
  return sendHeroNews(hero, Text.DEATH_TITLE, Text.deathText(hero, nil, epitaph), Text.deathText(hero, ownerName, epitaph), lastSeenAt)
end

-- Raising a hero
local function markUnitAsHero(unit, heroNumber, hitPointBoost)
  if not coe5.giveAffliction(unit, HERO_AFFLICTION) then
    coe5.log(string.format("heroes_affliction_refused unit=%d hero=%d", unit, heroNumber))
    return
  end
  coe5.setAfflictionValue(unit, HERO_AFFLICTION, heroNumber)
  coe5.set("unitHp", unit, coe5.get("unitHp", unit) + hitPointBoost)
end

local function raiseHero(saved, player, candidate, turn, kind)
  local unit = candidate.unit
  local monster = coe5.get("unitMonster", unit)
  local x, y = coe5.get("unitX", unit), coe5.get("unitY", unit)
  local commander = Calls.promote(unit, monster)
  if not commander then
    coe5.log(string.format("heroes_promote_refused player=%d unit=%d monster=%d turn=%d", player, unit, monster, turn))
    return false
  end

  local playerRecord = saved.players[player]
  playerRecord.usedShort = playerRecord.usedShort or {}
  playerRecord.usedEpithet = playerRecord.usedEpithet or {}

  local trait = Rules.rollTrait(Traits.rows, function(candidateTrait)
    return unitMayTakeTrait(unit, monster, candidateTrait)
  end, math.random)
  local afflictionNameLists = afflictionNameListsOf(unit)
  local kindNameLists = kindNameListsOf(unit)
  local listForName = Rules.shortNameListToNameAfter(afflictionNameLists, candidate.kills, kindNameLists, math.random)
  local afflictionForEpithet = Rules.listToNameAfter(afflictionNameLists, Rules.WOUND_EPITHET_PERCENT, math.random)
  local short = shortNameFor(commander, listForName, playerRecord.usedShort)
  local epithet = epithetFor(trait, afflictionForEpithet, playerRecord)
  local hitPointsAtRise = coe5.get("unitHp", unit) or 0
  local boost = Rules.baseBoost(hitPointsAtRise, coe5.get("unitArmor", unit) or 0, math.random)

  local heroNumber = #saved.heroes + 1
  local hero = {
    unit = unit, owner = player, monster = monster, kind = kind,
    short = short, epithet = epithet, title = Rules.joinTitle(short, epithet),
    trait = trait and trait.key, traitLabel = trait and trait.label,
    hpBoost = boost.hp, damageBoost = boost.damage, initBoost = boost.init, leadershipBoost = boost.leadership,
    moraleBoost = boost.morale, mrBoost = boost.mr, armorBoost = boost.armor,
    commander = commander, turn = turn, date = coe5.get("date"), x = x, y = y,
    place = Text.placeName(coe5.squareShownName(player, x, y), noPlayerOwnsSquare(x, y)),
    bornXp = candidate.xp, bornKills = candidate.kills, bornHp = hitPointsAtRise, xp = candidate.xp, kills = candidate.kills,
  }
  saved.heroes[heroNumber] = hero
  markUnitAsHero(unit, heroNumber, hero.hpBoost)
  hero.slotsFreed = true
  playerRecord.hadHero = true

  local messagesSent = announceBorn(hero, playerRecord, {x = x, y = y, unit = unit})
  coe5.log(string.format("heroes_raised player=%d unit=%d commander=%d hero=%d kind=%s trait=%s hp=%d kills=%d xp=%d turn=%d messages=%d afflictions=%s kinds=%s namedAfter=%s",
    player, unit, commander, heroNumber, kind, tostring(hero.trait), hero.hpBoost, candidate.kills, candidate.xp, turn, messagesSent,
    table.concat(afflictionNameLists, ","), table.concat(kindNameLists, ","), tostring(listForName)))
  return true
end

-- Who can rise
local function commandersByUnit()
  local commanderOf = {}
  for commander = 0, coe5.get("commanderLastIndex") do
    local unit = coe5.get("commanderUnit", commander)
    if unit and unit >= 0 then commanderOf[unit] = commander end
  end
  return commanderOf
end

local function unitMayRise(unit)
  local isAlive = (coe5.get("unitHp", unit) or 0) > 0
  if not isAlive then return false end
  local hasEnoughExperience = (coe5.get("unitXp", unit) or 0) >= Rules.EXPERIENCE_TO_RISE
  local isOnTheMap = (coe5.get("unitX", unit) or -1) >= 0
  if not hasEnoughExperience or not isOnTheMap or coe5.get("unitStationary", unit) then return false end

  local moveFlags, stateFlags = coe5.get("unitMoveFlags", unit), coe5.get("unitStateFlags", unit)
  local cannotLead = hasBit(moveFlags, MOVE_BIT.stupid) or hasBit(moveFlags, MOVE_BIT.loner)
  local leavesAfterBattle = hasBit(stateFlags, STATE_BIT.battleSummon) or hasBit(stateFlags, STATE_BIT.removedAfterBattle)
  if cannotLead or leavesAfterBattle or coe5.hasAffliction(unit, HERO_AFFLICTION) then return false end

  local isShip = (coe5.get("unitAbility", unit, SHIP_ABILITY) or 0) > 0
  local hasAnAttack = coe5.get("monsterAttackType", coe5.get("unitMonster", unit), 0) ~= nil
  return hasAnAttack and not isShip
end

local function candidatesOf(player)
  local commanderOf = commandersByUnit()
  local candidates = {}
  for unit = 0, coe5.get("unitLastIndex") do
    if coe5.get("unitOwner", unit) == player and commanderOf[unit] == nil and unitMayRise(unit) then
      candidates[#candidates + 1] = {
        unit = unit,
        kills = coe5.get("unitKills", unit) or 0,
        xp = coe5.get("unitXp", unit) or 0,
      }
    end
  end
  return candidates
end

-- Living and fallen heroes
local function unitsOfLivingHeroes()
  local unitOfHero = {}
  for _, unit in ipairs(coe5.afflictedUnits(HERO_AFFLICTION) or {}) do
    local heroNumber = coe5.afflictionValue(unit, HERO_AFFLICTION)
    if heroNumber then unitOfHero[heroNumber] = unit end
  end
  return unitOfHero
end

local function readHeroAgain(hero, unit)
  hero.unit, hero.owner = unit, coe5.get("unitOwner", unit)
  hero.currentMonster = coe5.get("unitMonster", unit) or hero.currentMonster
  hero.deathTurn, hero.deathDate = nil, nil
  hero.xp = coe5.get("unitXp", unit) or hero.xp
  hero.kills = coe5.get("unitKills", unit) or hero.kills
  hero.x, hero.y = coe5.get("unitX", unit), coe5.get("unitY", unit)
end

local function freeOldAbilitySlots(heroNumber, hero, unit)
  if hero.slotsFreed then return end
  hero.slotsFreed = true
  hero.leadershipBoost = hero.leadershipBoost or SETTINGS.leadership
  for ability in pairs(abilityAmountsOfHero(heroNumber, hero)) do coe5.unitRemoveTempAbility(unit, ability) end
end

local function renameHero(heroNumber, hero, name)
  hero.short, hero.title = name, Rules.joinTitle(name, hero.epithet)
  coe5.log(string.format("heroes_renamed hero=%d owner=%d", heroNumber, hero.owner))
end

local function keepHeroName(heroNumber, hero, commander)
  if commander == nil then return end
  local sameRecord = hero.commander == nil or hero.commander == commander
  hero.commander = commander
  local name = coe5.get("commanderName", commander)
  if not sameRecord then
    coe5.set("commanderName", commander, hero.short)
  elseif name and name ~= "" and name ~= hero.short then
    renameHero(heroNumber, hero, name)
  end
end

local function recordDeath(saved, heroNumber, hero, turn)
  hero.deathTurn, hero.deathDate = turn, coe5.get("date")
  local messagesSent = announceDeath(hero, saved)
  coe5.log(string.format("heroes_fell hero=%d owner=%d unit=%d turn=%d messages=%d", heroNumber, hero.owner, hero.unit, turn, messagesSent))
end

local function updateRoster(saved, player, turn)
  local unitOfHero, commanderOf = unitsOfLivingHeroes(), commandersByUnit()
  local livingHeroesOfPlayer = {}
  for heroNumber, hero in ipairs(saved.heroes) do
    local unit = unitOfHero[heroNumber]
    if unit then
      readHeroAgain(hero, unit)
      keepHeroName(heroNumber, hero, commanderOf[unit])
      if hero.owner == player then
        livingHeroesOfPlayer[#livingHeroesOfPlayer + 1] = {heroNumber = heroNumber, hero = hero, unit = unit}
        freeOldAbilitySlots(heroNumber, hero, unit)
      end
    elseif not hero.deathTurn then
      recordDeath(saved, heroNumber, hero, turn)
    end
  end
  return livingHeroesOfPlayer
end

-- Becoming a legend
local function announceLegend(hero)
  local ownerName = coe5.get("playerName", hero.owner)
  local goTo = {x = hero.x, y = hero.y, unit = hero.unit}
  return sendHeroNews(hero, Text.LEGEND_TITLE, Text.legendText(hero), Text.legendText(hero, ownerName), goTo)
end

local function heroStatsNow(hero, unit)
  local function withBoosts(site, value)
    return statWithHeroBonuses(site, unit, value) or value
  end
  return {
    hp = Rules.heroMostHitPoints(hero, coe5.get("unitXp", unit)),
    armor = withBoosts("UNIT_ARMOR", coe5.get("unitArmor", unit) or 0),
    morale = withBoosts("UNIT_MORALE", coe5.unitMorale(unit) or 0),
    mr = withBoosts("UNIT_MR", coe5.unitMagicResistance(unit) or 0),
  }
end

local function makeLegend(saved, heroNumber, hero, unit, turn)
  local monster = coe5.get("unitMonster", unit)
  local trait = Rules.rollTrait(Traits.rows, function(candidateTrait)
    return heroMayTakeLegendTrait(hero, unit, monster, candidateTrait)
  end, math.random, Rules.LEGEND_TIERS)
  local boost = Rules.legendBoost(heroStatsNow(hero, unit), math.random)
  hero.legendTurn, hero.legendDate, hero.legendAnnounced = turn, coe5.get("date"), false
  hero.legendTrait, hero.legendTraitLabel = trait and trait.key, trait and trait.label
  hero.legendHpBoost, hero.legendArmorBoost, hero.legendMoraleBoost, hero.legendMrBoost = boost.hp, boost.armor, boost.morale, boost.mr
  if boost.hp > 0 then coe5.set("unitHp", unit, coe5.get("unitHp", unit) + boost.hp) end
  saved.legendsMade = (saved.legendsMade or 0) + 1
  coe5.log(string.format("heroes_legend hero=%d owner=%d unit=%d trait=%s xp=%d turn=%d hp=%d armor=%d morale=%d mr=%d", heroNumber, hero.owner,
    unit, tostring(hero.legendTrait), hero.xp or 0, turn, boost.hp, boost.armor, boost.morale, boost.mr))
end

local function proclaimLegend(heroNumber, hero)
  hero.legendAnnounced = true
  local messagesSent = announceLegend(hero)
  coe5.log(string.format("heroes_legend_proclaimed hero=%d owner=%d messages=%d", heroNumber, hero.owner, messagesSent))
end

-- Turn start
local function promotionMayHappenNow(playerRecord, turn)
  local schedule = playerRecord.schedule
  if #schedule.owed > 0 or not playerRecord.hadHero then return true end
  local thisMonth = Rules.turnInPeriod(schedule, turn)
  for _, scheduledMonth in ipairs(schedule.months) do
    if scheduledMonth == thisMonth then return true end
  end
  return false
end

local function onPlayerTurnStart(player)
  local saved, turn = savedGame(), coe5.turn()
  if turn < 0 then return end
  local livingHeroes = updateRoster(saved, player, turn)
  if not playerRaisesHeroes(player) then return end
  local livingLegends = 0
  for _, living in ipairs(livingHeroes) do
    if living.hero.legendTurn then livingLegends = livingLegends + 1 end
  end
  for _, living in ipairs(livingHeroes) do
    local hero = living.hero
    if hero.legendAnnounced == false and hero.legendTurn < turn then proclaimLegend(living.heroNumber, hero) end
    if Rules.becomesLegend(hero, hero.xp, livingLegends) then
      makeLegend(saved, living.heroNumber, hero, living.unit, turn)
      livingLegends = livingLegends + 1
    end
  end

  local playerRecord = saved.players[player] or {}
  saved.players[player] = playerRecord
  playerRecord.schedule = playerRecord.schedule or {}
  local schedule = playerRecord.schedule
  local yearsHeroes = heroesAYear()
  if Rules.ensurePeriod(schedule, turn, math.random, yearsHeroes) then
    coe5.log(string.format("heroes_schedule player=%d period=%d turns=%d months=%s final=%s heroes_a_year=%g", player, schedule.period,
      schedule.turns, table.concat(schedule.months, ","), tostring(schedule.final), yearsHeroes))
  end
  if not promotionMayHappenNow(playerRecord, turn) then return end

  local candidates = candidatesOf(player)
  local neverHadHero = not playerRecord.hadHero
  local raised = Rules.settle(schedule, turn, candidates, #livingHeroes, neverHadHero, function(candidate, kind)
    return raiseHero(saved, player, candidate, turn, kind)
  end)
  if #raised > 0 or #schedule.owed > 0 then
    coe5.log(string.format("heroes_turn player=%d turn=%d raised=%d owed=%d candidates=%d", player, turn, #raised,
      #schedule.owed, #candidates + #raised))
  end
end

-- A legend's one chance at a new name
local renameOffered, holdClickFromMap = {}, false
local noLegendAwaits = {turn = nil, player = nil, legends = nil}

local function legendAwaitingName(player, turn)
  local legends = coe5.gameData.legendsMade
  local sameLook = noLegendAwaits.turn == turn and noLegendAwaits.player == player and noLegendAwaits.legends == legends
  if sameLook then return nil end
  for heroNumber, hero in ipairs(coe5.gameData.heroes or {}) do
    local isNewLegend = hero.legendTurn == turn and not hero.deathTurn
    if isNewLegend and hero.owner == player and not renameOffered[heroNumber] then return heroNumber, hero end
  end
  noLegendAwaits.turn, noLegendAwaits.player, noLegendAwaits.legends = turn, player, legends
  return nil
end

local function offerLegendRename(heroNumber, hero)
  renameOffered[heroNumber] = true
  local commander = commandersByUnit()[hero.unit]
  if commander == nil then return end
  local name = Calls.renameCommander(commander)
  coe5.log(string.format("heroes_rename_offered hero=%d commander=%d renamed=%s", heroNumber, commander, tostring(name ~= nil)))
  local isLanClient = coe5.netRole() == coe5.net.client
  if name and name ~= "" and not isLanClient then
    hero.commander = commander
    renameHero(heroNumber, hero, name)
  end
end

coe5.wrap("MAP_SCREEN_WAIT", function(next, block, mode)
  local clicked = next(block, mode)
  if holdClickFromMap then
    if (coe5.get("mouseButtons") or 0) ~= 0 then
      coe5.set("mouseButtons", 0)
      return 0
    end
    holdClickFromMap = false
  end
  local player = coe5.localPlayer()
  if player == nil or player < 0 then return clicked end
  local isLanClient = coe5.netRole() == coe5.net.client
  if not isLanClient and coe5.get("playerController", player) ~= coe5.controller.human then return clicked end
  local heroNumber, hero = legendAwaitingName(player, coe5.turn())
  if not heroNumber then return clicked end
  offerLegendRename(heroNumber, hero)
  holdClickFromMap = true
  return 0
end)

coe5.observe("PLAYER_TURN_START", nil, function(_, player)
  local isLanClient = coe5.netRole() == coe5.net.client
  if not isLanClient then onPlayerTurnStart(player) end
end)

-- Heroes list
require("heroes_list").install({affliction = HERO_AFFLICTION, showAll = everyPlayerSeesEveryHero, monsterName = monsterName})
