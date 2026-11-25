-- Traits a hero can roll

local T = {}

local function row(tier, traits)
  traits.tier = tier
  for _, trait in ipairs(traits) do
    trait.listedTier, trait.listedLabel = tier, trait.label
    local amount = trait.add or trait.atLeast or trait.value or (trait.abilities and trait.abilities[1].value)
    if amount then
      trait.listedAmount = amount
      local escaped = trait.label:gsub("%%", "%%%%")
      trait.labelFormat = escaped:gsub("%f[%d]" .. amount .. "%f[%D]", "%%d")
    end
  end
  return traits
end

T.GROUPS = {common = true, rare = true, legendary = true}

T.listedRows = {
  -- Common traits
  row("common", {{key = "strength", route = "site", site = "UNIT_DAMAGE_BONUS", add = 2, label = "Strength +2", names = "strength"}}),
  row("common", {{key = "armor", route = "site", site = "UNIT_ARMOR", add = 1, label = "Armor +1", names = "armor"}}),
  row("common", {{key = "morale", route = "site", site = "UNIT_MORALE", add = 3, label = "Morale +3", names = "morale"}}),
  row("common", {{key = "mr", route = "site", site = "UNIT_MR", add = 2, label = "Magic Resistance +2", names = "magicResistance"}}),
  row("common", {
    {key = "pierceres", route = "flag", bit = 25, label = "Pierce Resistance", names = "pierceResistance"},
    {key = "slashres", route = "flag", bit = 24, label = "Slash Resistance", names = "slashResistance"},
    {key = "bluntres", route = "flag", bit = 23, label = "Blunt Resistance", names = "bluntResistance"},
  }),
  row("common", {{key = "fireres", route = "site", site = "UNIT_FIRE_RESISTANCE", atLeast = 50, value = 50, element = "fire",
    label = "Fire Resistance (50)", names = "fireResistance"}}),
  row("common", {
    {key = "coldres", route = "ability", id = 0x7f, value = 50, element = "cold", label = "Cold Resistance (50)", names = "coldResistance"},     -- coldres
    {key = "shockres", route = "ability", id = 0x80, value = 50, element = "shock", label = "Shock Resistance (50)", names = "shockResistance"}, -- shockres
    {key = "poisonres", route = "ability", id = 0x81, value = 50, element = "poison", label = "Poison Resistance (50)", names = "poisonResistance"}, -- poisonres
  }),
  row("common", {
    {key = "sleepres", route = "flag", bit = 6, label = "Sleep Immunity", names = "sleepResistance"},
    {key = "charmres", route = "flag", bit = 7, label = "Charm Immunity", names = "charmResistance"},
    {key = "diseaseres", route = "flag", bit = 18, label = "Disease Resistance", names = "diseaseResistance"},
  }),
  row("common", {{key = "affres", route = "ability", id = 0x76, value = 75, orMore = true,                                           -- affres
    label = "Affliction Resistance (75)", names = "afflictionResistance"}}),
  row("common", {{key = "fastheal", route = "flag", bit = 9, label = "Fast Heal", names = "fastHealing"}}),
  row("common", {
    {key = "spiritsight", route = "flag", bit = 5, label = "Spirit Sight", names = "spiritSight"},
    {key = "acutesenses", route = "flag", bit = 16, label = "Acute Senses", names = "acuteSenses"},
  }),
  row("common", {{key = "farsight", route = "ability", id = 0x92, value = 1, label = "Farsight", names = "farsight"}}),    -- farsight
  row("common", {
    {key = "stealth", route = "flag", bit = 17, label = "Stealth", names = "stealth"},
    {key = "foreststealth", route = "flag", bit = 44, label = "Forest Stealth", names = "forestStealth"},
    {key = "desertstealth", route = "flag", bit = 41, label = "Desert Stealth", names = "desertStealth"},
    {key = "snowstealth", route = "flag", bit = 56, label = "Snow Stealth", names = "snowStealth"},
  }),
  row("common", {{key = "fast", route = "move", bit = 4, label = "Fast", names = "fast"}}),
  row("common", {
    {key = "swamp", route = "move", bit = 5, label = "Swamp Move", names = "swampMove"},
    {key = "mountain", route = "move", bit = 8, label = "Mountain Move", names = "mountainMove"},
    {key = "desert", route = "move", bit = 10, label = "Desert Move", names = "desertMove"},
    {key = "snow", route = "move", bit = 37, label = "Snow Move", names = "snowMove"},
    {key = "water", route = "move", bit = 6, label = "Amphibian", names = "waterMove"},
    {key = "float", route = "move", bit = 7, label = "Floating", names = "floating"},
    {key = "wall", route = "move", bit = 34, label = "Wall Climbing", names = "wallClimbing"},
  }),
  row("common", {
    {key = "shield", route = "flag", bit = 27, alsoBit = 28, label = "Shield", names = "shield"},
    {key = "largeshield", route = "flag", bit = 28, label = "Large Shield", names = "shield"},
  }),
  row("common", {{key = "iceprot", route = "ability", id = 0x60, value = 1, label = "Ice Armor (1)", names = "armor"}}),  -- iceprot
  row("common", {{key = "fireaura", route = "ability", id = 0x31, value = 5, label = "Fire Shield (5)", names = "fireShield"}}),  -- fireaura
  row("common", {{key = "coldaura", route = "ability", id = 0x32, value = 2, label = "Frost Shield (2)", names = "frostShield"}}),  -- coldaura
  row("common", {{key = "poisonaura", route = "ability", id = 0x33, value = 4, label = "Poison (4)", names = "poisonStrikeback"}}),  -- poisonaura
  row("common", {{key = "airshield", route = "ability", id = 0x41, value = 50, label = "Air Shield (50)", names = "airShield"}}),  -- airshield
  row("common", {{key = "awe", route = "ability", id = 0x15, value = 1, label = "Awe (+1)", names = "awe"}}),  -- awe
  row("common", {
    {key = "leadership", route = "ability", id = 0x8, value = 2, label = "Leadership (2)", names = "leadership"},              -- leadership
  }),
  row("common", {
    {key = "wall_cover", route = "ability", id = 0x198, value = 1, label = "Wall Cover (1)", names = "siege"},               -- wall_cover
    {key = "wall_range_bonus", route = "ability", id = 0x1a6, value = 1, label = "Wall Range Bonus (1)", names = "siege"},   -- wall_range_bonus
    {key = "siegetunnel", route = "ability", id = 0x1b1, value = 5, label = "Siege Tunneling (5)", names = "siege"},         -- siegetunnel
  }),
  row("common", {
    {key = "saner", route = "ability", id = 0x4f, value = 1, label = "Regain Sanity (1)", names = "sanity"},        -- saner
    {key = "voidsanity", route = "ability", id = 0x63, value = 20, label = "Void Sanity (20)", names = "sanity"},   -- voidsanity
  }),
  row("common", {{key = "unaging", route = "flag", bit = 54, label = "Unaging", names = "unaging"}}),
  row("common", {
    {key = "tangleres", route = "ability", id = 0x4ea, value = 1, label = "Tangle Vine Immunity", names = "tangleResistance"},  -- tangleres
    {key = "hadesres", route = "ability", id = 0xc2, value = 1, label = "Hades Immunity", names = "hadesImmunity"},            -- hadesres
  }),
  row("common", {{key = "coldheal", route = "ability", id = 0x87, value = 4, label = "Cold Heal (4 HP / month)", names = "coldHeal"}}),  -- coldheal
  row("common", {
    {key = "slayundead", route = "hit", slay = 4, label = "Double damage vs undead", names = "slayUndead"},
    {key = "slaydemons", route = "hit", slay = 33, label = "Double damage vs demons", names = "slayDemons"},
    {key = "slayanimals", route = "hit", slay = 11, label = "Double damage vs animals", names = "slayAnimals"},
    {key = "slayhumans", route = "hit", slay = 3, label = "Double damage vs humans", names = "slayHumans"},
  }),
  row("common", {{key = "range_bonus", route = "ability", id = 0xc7, value = 4, needs = "ranged",                    -- range_bonus
    label = "Range Bonus (4)", names = "rangeBonus"}}),
  row("common", {{key = "spellrange", route = "ability", id = 0xc6, value = 1, needs = "caster",                     -- spellrange
    label = "Spell Range Bonus (1)", names = "spellRange"}}),
  row("common", {{key = "diseaseshield", route = "ability", id = 0x19a, value = 1, label = "Disease", names = "diseaseShield"}}),  -- diseaseshield
  row("common", {{key = "slimeshield", route = "ability", id = 0x1ce, value = 1, label = "Slimy", names = "slimy"}}),             -- slimeshield
  row("common", {
    {key = "awakentrees", route = "ability", id = 0x1bb, value = 3, label = "Awaken Trees (range 3)", names = "awakener"},         -- awakentrees
    {key = "awakenfungus", route = "ability", id = 0x1bc, value = 4, label = "Awaken Mushrooms (range 4)", names = "awakener"},    -- awakenfungus
  }),
  row("common", {{key = "farsightsea", route = "ability", id = 0x50b, value = 3, label = "Sea Farsight", names = "seaFarsight"}}),  -- farsightsea
  row("common", {{key = "berserker", route = "flag", bit = 40, label = "Berserker", names = "berserker"}}),
  row("common", {{key = "holy", route = "flag", bit = 14, label = "Blessed", names = "blessed"}}),
  row("common", {
    {key = "putridexpl", route = "ability", id = 0x8f, value = 3, label = "Putrid Explosion on Death (3)", names = "deathExplosion"},    -- putridexpl
    {key = "mindexpl", route = "ability", id = 0xbc, value = 1, label = "Mental Agony on Death", names = "deathExplosion"},             -- mindexpl
  }),
  row("common", {
    {key = "shardexpl", route = "ability", id = 0x2b, value = 6, label = "Shard Explosion on Death (6)", names = "deathExplosion"},     -- shardexpl
    {key = "fireexpl", route = "ability", id = 0x1f1, value = 12, label = "Fire Explosion on Death (12)", names = "deathExplosion"},    -- fireexpl
    {key = "poisonexpl", route = "ability", id = 0x1f0, value = 10, label = "Poison Explosion on Death (10)", names = "deathExplosion"}, -- poisonexpl
  }),
  row("common", {{key = "acidres", route = "ability", id = 0x82, value = 100, element = "acid",                        -- acidres
    label = "Acid Resistance (100)", names = "acidImmunity"}}),
  row("common", {{key = "acidblood", route = "ability", id = 0x2f, value = 50, label = "Acid Blood (50)", names = "acidBlood"}}),     -- acidblood
  row("common", {{key = "passwall", route = "move", bit = 32, label = "Passwall", names = "passWall"}}),
  row("common", {{key = "goldbonus", route = "ability", id = 0xc8, value = 25, label = "Gold Bonus (25%)", names = "gold"}}),   -- goldbonus
  row("common", {{key = "limitgold", route = "ability", abilities = {{id = 0xbd, value = 3}, {id = 0xbe, value = 3}},           -- limitgold, limitiron
    label = "Limited Gold Bonus (3 / month), Limited Iron Bonus (3 / month)", names = "gold"}}),
  row("common", {{key = "shockaura", route = "ability", id = 0x30, value = 5, label = "Shock Shield (5)", names = "shockShield"}}),   -- shockaura
  row("common", {{key = "poisoncloud", route = "ability", id = 0x73, value = 3, label = "Poison Aura (3)", names = "poisonAura"}}),   -- poisoncloud
  row("common", {{key = "fear", route = "flag", bit = 29, alsoAbility = 0x6a, label = "Causes Fear", names = "fear"}}),
  row("common", {{key = "invisible", route = "flag", bit = 22, label = "Invisible", names = "invisible"}}),
  row("common", {
    {key = "flying", route = "move", bit = 0, label = "Flying", names = "flying"},
    {key = "teleport", route = "move", bit = 39, label = "Teleporting", names = "teleport"},
  }),
  row("common", {{key = "trample", route = "ability", abilities = {{id = 0x2c, value = 2}, {id = 0x2d, value = 1, fixed = true}},  -- trample, tramplexsize
    label = "Trample (2)", names = "trample"}}),
  row("common", {{key = "extraeyes", route = "ability", id = 0x42, value = 6, label = "Extra Eyes (6)", names = "extraEyes"}}),  -- extraeyes

  -- Rare traits
  row("rare", {
    {key = "fireimmune", route = "flag", bit = 0, element = "fire", full = true, label = "Fire Resistance (100)", names = "fireImmunity"},
    {key = "coldimmune", route = "flag", bit = 1, element = "cold", full = true, label = "Cold Resistance (100)", names = "coldImmunity"},
    {key = "poisonimmune", route = "flag", bit = 2, element = "poison", full = true, label = "Poison Resistance (100)", names = "poisonImmunity"},
    {key = "shockimmune", route = "flag", bit = 15, element = "shock", full = true, label = "Shock Resistance (100)", names = "shockImmunity"},
  }),
  row("rare", {{key = "affres100", route = "ability", id = 0x76, value = 100, orMore = true,                        -- affres
    label = "Affliction Resistance (100)", names = "afflictionResistance"}}),
  row("rare", {{key = "ethereal", route = "flag", bit = 26, label = "Ethereal", names = "ethereal"}}),
  row("rare", {{key = "regeneration", route = "flag", bit = 10, label = "Regeneration", names = "regeneration"}}),
  row("rare", {{key = "lucky", route = "flag", bit = 48, label = "Lucky", names = "lucky"}}),
  row("rare", {
    {key = "evasion", route = "ability", id = 0x1b5, value = 1, label = "Evasion", names = "evasion"},        -- evasion
    {key = "displaced", route = "ability", id = 0x1ec, value = 1, label = "Displaced", names = "displaced"},  -- displaced
  }),
  row("rare", {{key = "mirrorimages", route = "ability", id = 0x1a8, value = 3, label = "Constant Mirror Images (3)", names = "mirrorImages"}}),  -- mirrorimages
  row("rare", {{key = "magicshield", route = "flag", bit = 31, label = "Magic Shield", names = "magicShield"}}),
  row("rare", {{key = "doubledamage", route = "hit", label = "Double damage", names = "doubleDamage"}}),
  row("rare", {{key = "tunnel", route = "move", bit = 42, label = "Tunneling", names = "tunnel"}}),
  row("rare", {{key = "tunnelmove", route = "ability", id = 0x1ab, value = 6, label = "Combat Burrowing (6)", names = "tunnel"}}),  -- tunnelmove
  row("rare", {{key = "armytrainer", route = "ability", id = 0x1d, value = 1, label = "Army Trainer (+1 xp per month)", names = "armyTrainer"}}),  -- armytrainer
  row("rare", {{key = "scry", route = "ability", id = 0x37, value = 15, label = "Scry (15)", names = "scry"}}),             -- scry
  row("rare", {{key = "twistfate", route = "ability", id = 0x1e0, value = 1, label = "Twist Fate", names = "twistFate"}}),   -- twistfate
  row("rare", {{key = "combatcaster", route = "flag", bit = 19, needs = "caster", label = "Combat Caster", names = "combatCaster"}}),
  row("rare", {{key = "pathbonus", route = "site", site = "UNIT_PATH_BONUS", add = 1, needs = "caster", label = "Magic Boost: + 1", names = "pathBonus"}}),
  row("rare", {{key = "seduceaura", route = "ability", id = 0x1e4, value = 5, label = "Seduction (5)", names = "seduction"}}),   -- seduceaura
  row("rare", {{key = "petriaura", route = "ability", id = 0x1a0, value = 3, label = "Petrification (3)", names = "petrification"}}),  -- petriaura

  -- Legendary traits
  row("legendary", {
    {key = "invul", route = "flag", bit = 32, label = "Invulnerability", names = "invulnerable"},
    {key = "nonmaginvul", route = "flag", bit = 49, label = "Non-Magical Invulnerability", names = "invulnerable"},
  }),
  row("legendary", {{key = "immortal", route = "flag", bit = 20, label = "Immortal", names = "immortal"}}),
  row("legendary", {{key = "absorbdead", route = "ability", abilities = {{id = 0x3f, value = 3}, {id = 0x3d, value = 1, fixed = true}},  -- absorbdead, hpoverflow
    label = "Absorb Dead (3)", names = "absorbDead"}}),
}

T.byKey = {}
for _, traits in ipairs(T.listedRows) do
  for _, trait in ipairs(traits) do
    assert(not T.byKey[trait.key], "two traits share a key")
    if trait.route == "ability" then trait.abilities = trait.abilities or {{id = trait.id, value = trait.value}} end
    T.byKey[trait.key] = trait
  end
end

-- Sets a trait's amount in every field that holds it, and its label.
local function setAmount(trait, amount)
  if trait.add then trait.add = amount end
  if trait.atLeast then trait.atLeast = amount end
  if trait.value then trait.value = amount end
  for _, given in ipairs(trait.abilities or {}) do
    if not given.fixed then given.value = amount end
  end
  local fills = select(2, trait.labelFormat:gsub("%%d", ""))
  local amounts = {}
  for i = 1, fills do amounts[i] = amount end
  trait.amount, trait.label = amount, string.format(trait.labelFormat, table.unpack(amounts))
end

function T.configure(settings)
  for _, trait in pairs(T.byKey) do
    trait.off, trait.tier = false, trait.listedTier
    if trait.listedAmount then setAmount(trait, trait.listedAmount) end
  end
  local problems = {}
  for key, setting in pairs(settings or {}) do
    local trait = T.byKey[key]
    local isTable = type(setting) == "table"
    local switch, group, amount = isTable and setting[1], isTable and setting[2], isTable and setting[3] or nil
    if not trait then
      problems[#problems + 1] = {key = tostring(key), reason = "no_trait"}
    elseif switch ~= 0 and switch ~= 1 then
      problems[#problems + 1] = {key = key, reason = "bad_switch"}
    elseif group ~= nil and not T.GROUPS[group] then
      problems[#problems + 1] = {key = key, reason = "bad_group"}
    elseif amount ~= nil and not trait.listedAmount then
      problems[#problems + 1] = {key = key, reason = "no_amount"}
    elseif amount ~= nil and (math.type(amount) ~= "integer" or amount < 1) then
      problems[#problems + 1] = {key = key, reason = "bad_amount"}
    else
      trait.off, trait.tier = switch == 0, group or trait.listedTier
      if amount then setAmount(trait, amount) end
    end
  end
  table.sort(problems, function(a, b) return a.key < b.key end)
  local rows = {}
  for _, listed in ipairs(T.listedRows) do
    local rowOfTier, tierOrder = {}, {}
    for _, trait in ipairs(listed) do
      if not trait.off then
        if not rowOfTier[trait.tier] then
          rowOfTier[trait.tier] = {tier = trait.tier}
          tierOrder[#tierOrder + 1] = trait.tier
        end
        table.insert(rowOfTier[trait.tier], trait)
      end
    end
    for _, tier in ipairs(tierOrder) do rows[#rows + 1] = rowOfTier[tier] end
  end
  T.rows = rows
  return problems
end

T.configure()

return T
