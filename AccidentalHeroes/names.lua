-- Names and epithets

return {
  short = {
    -- Names after an affliction
    lostEye = { "Squint", "Patch", "Winks", "Odd-Eye", "Half-Look", "Keyhole", "Blinker", "Monocle", "Cyclops" },
    blind = { "Mole", "Bat", "Bumps", "Fumble", "Gropes", "Tapper", "Darkly" },
    chestWound = { "Wheezy", "Bellows", "Puff", "Rattle", "Whistle", "Short-Wind", "Gasper" },
    neverHealingWound = { "Leaky", "Drips", "Seep", "Old Wound", "Red Sock", "Bandage", "Ooze" },
    battleFright = { "Jumpy", "Flinch", "Twitch", "Shivers", "Yelp", "Quiver", "Startle" },
    feeblemind = { "Dim", "Blank", "Mumbles", "Vacant", "Doorknob", "Simple", "Dodo" },
    disease = { "Cough", "Sniffle", "Pox", "Phlegm", "Spots", "Clammy", "Peaky", "Sweats" },
    curse = { "Jinx", "Hex", "Unlucky", "Black Cat", "Thirteen", "Hoodoo", "Magpie" },
    insanity = { "Giggles", "Loon", "Cuckoo", "Moonface", "Whispers", "Chatter", "Gibber" },
    horrormark = { "Markd", "Tentacarl", "Cultshade", "Barnabas", "Tick-Tock", "Florp" },
    debilitatingWound = { "Limpy", "Hobble", "Hop", "Crutch", "Lurch", "Shamble", "Peg-Leg" },
    festeringWound = { "Stinky", "Whiff", "Pong", "Ripe", "Reek", "Gamey", "Fester" },

    -- Names for 10 kills or more
    manyKills = { "Ripper", "Butcher", "Reaper" },

    -- Names by kind of unit
    animal = { "Biter", "Good Boy", "Fang", "Scruff", "Nibbles", "Mange", "Rex", "Fleabag", "Chomp", "Whiskers", "Big Lad", "Snuffles" },
    undead = { "Rattles", "Mostly", "Dusty", "Bones", "Stiff", "Clatter", "Leftovers", "Late", "Remains", "Rot" },
    demonic = { "Cinders", "Scorch", "Itch", "Pitch", "Sulphur", "Horns", "Smirk", "Brimstone" },

    any = {
      "Biscuit", "Mudge", "Tuppence", "Old Tam", "Lumpy", "Nob", "Pickle", "Gristle", "Dobbin", "Stubbs",
      "Marrow", "Fidget", "Barley", "Tallow", "Scuff", "Mopsy", "Gudgeon", "Whelk", "Crumb", "Hobnail",
      "Tinker", "Bodkin", "Pudding", "Nettle", "Scrag", "Dumpling", "Thistle", "Wattle", "Ferret", "Clod",
      "Turnip", "Bramble", "Snod", "Porridge", "Grub", "Twig", "Pip", "Chowder", "Mutton", "Sprat",
      "Bungo", "Knuckle", "Soggy", "Cobble", "Griddle", "Dribble", "Hodge", "Wobble", "Trug", "Moss",
      "Tatters", "Old Nan", "Stew", "Gravel", "Suet", "Rummage", "Flint", "Burdock", "Clump", "Hasp",
      "Jib", "Lard", "Midge", "Nudge", "Oaf", "Plum", "Quill", "Rind", "Slop", "Thump",
      "Uncle", "Vole", "Wick", "Yam", "Bog", "Crust", "Dregs", "Egg", "Fug", "Gob",
      "Hump", "Inch", "Jowl", "Kip", "Lug", "Mump", "Nit", "Ounce", "Peg", "Runt",
      "Tuft", "Wart", "Yolk", "Zest", "Odo", "Wilf", "Gert", "Hob", "Maud", "Ned",
      "Tib", "Bess", "Alf", "Dunstan", "Egbert", "Mildred", "Osric", "Hilda", "Cuthbert", "Agnes",
      "Bertram", "Godwin", "Edith", "Wat", "Jem", "Nell", "Perkin", "Sybil", "Hugo", "Auntie",
      "Ha'penny", "Bucket", "Ladle", "Trowel", "Kettle", "Spigot", "Bung", "Hinge", "Latch", "Mallet",
      "Noggin", "Offcut", "Pothook", "Quibble", "Rivet", "Skillet", "Trundle", "Umber", "Vinegar", "Whittle",
    },
  },

  epithets = {
    -- Stats
    strength = { "the Giant-Blooded", "the Ogre-Strong", "Whom Ogres Dare Not Shove" },
    armor = { "the Stone-Skinned", "the Dragon-Hided", "Whom Gargoyles Envy" },
    morale = { "the Dauntless", "Whom Fear Forgot", "Who Mistook a Rout for a Charge" },
    magicResistance = { "the Spell-Deaf", "the Magically Resistant", ", Too Dense for Sorcery" },

    -- Damage resistances
    pierceResistance = { "the Arrow-Proof", "the Spear-Breaker", "Whom Elven Archers Curse" },
    slashResistance = { "the Blade-Turner", "the Sword-Breaker", "Who Ruined the Headsman's Axe" },
    bluntResistance = { "the Hammer-Proof", "the Anvil-Headed", "Who Headbutted a Battering Ram" },

    -- Elements
    fireResistance = { "the Ember-Kissed", "the Flame-Tested", "Who Needed Two Pyres to Notice" },
    fireImmunity = { "the Phoenix-Touched", "the Salamander-Blooded", "Who Called Hellfire Chilly" },
    coldResistance = { "the Frost-Kissed", "the North-Born", "Who Found the Ice-Wastes Brisk" },
    coldImmunity = { "the Rime-Blooded", "the Winter-Born", "Who Thinks Ice Giants Overdress" },
    shockResistance = { "the Storm-Kissed", "the Spark-Touched", "Struck by Lightning Twice, Mildly Annoyed" },
    shockImmunity = { "the Stormborn", "the Thunder-Proof", "Who Asked the Queen of Thunder to Try Again" },
    poisonResistance = { "the Iron-Gutted", "the Venom-Hardened", "Who Asked the Assassin for Seconds" },
    poisonImmunity = { "the Serpent-Blooded", "the Basilisk-Kissed", "Who Drank the Poisoned Well Dry" },
    acidImmunity = { "the Unmelted", "the Vitriol-Proof", "Whom Acid-Wyrms Cannot Digest", "Who Rinses in Hydra Spit" },

    -- Mind and body
    sleepResistance = { "the Ever-Wakeful", "Whom Dreams Cannot Find", "Who Refused a Hundred-Year Slumber" },
    charmResistance = { "the Unbeguiled", "Whom No Glamour Holds", "Whom Love-Potions Only Annoy" },
    diseaseResistance = { "the Plague-Proof", "Whom Pestilence Passes By", "Whom the Plague-God Overlooked" },
    afflictionResistance = { "the Unblighted", "the Curse-Proof", "Who Refused to Stay Maimed" },
    sanity = { "the Unmaddened", "of the Unbroken Mind", "Who Bored an Eldritch God" },

    -- Healing
    fastHealing = { "the Troll-Blooded", "Whom Graves Give Back", ", the Gravedigger's Wasted Walk" },
    regeneration = { "the Hydra-Blooded", "the Regrown", "Whose Wounds Close Out of Spite" },

    -- Senses and stealth
    spiritSight = { "the Ghost-Seer", "of the Second Sight", "Who Argues With Ancestors" },
    acuteSenses = { "the Hawk-Eyed", "the Wolf-Eared", "Who Heard the Goblin Blink" },
    extraEyes = { "the Many-Eyed", "the Spider-Eyed" },
    farsight = { "the Far-Seer", "the Eagle-Eyed", "Who Counted Goblins Across the Sea" },
    stealth = { "the Shadow-Footed", "the Whisper-Walker", "Who Startled an Assassin" },
    forestStealth = { "the Leaf-Cloaked", "the Green Ghost", "Whom Dryads Mistake for Bark" },
    desertStealth = { "the Dune-Ghost", "the Sand-Cloaked" },
    snowStealth = { "the Snow-Ghost", "the White Shadow" },
    invisible = { "the Unseen", "the Vanished", "Whose Banner Marches Alone" },

    -- Movement
    fast = { "the Wind-Footed", "the League-Eater", "Who Was Born a Month Before the Prophecy" },
    swampMove = { "the Bog-Walker", "the Marsh-Born", "the Muddy" },
    mountainMove = { "the Crag-Strider", "the Peak-Born" },
    desertMove = { "the Sand-Strider", "the Dune-Born" },
    snowMove = { "the Snow-Strider", "the Drift-Walker" },
    waterMove = { "the Tide-Born", "the Deep-Swimmer" },
    floating = { "the Wave-Walker", "the Unsinkable" },
    wallClimbing = { "the Wall-Crawler", "the Rampart-Climber" },
    flying = { "the Sky-Strider", "the Cloud-Treader", "Who Looks Down on Gargoyles" },
    trample = { "the Stampede", "the Wall-Shatterer" },
    passWall = { "the Stone-Strider", "the Wall-Walker", "Who Never Uses the Drawbridge" },
    teleport = { "the Blink-Footed", "the Far-Stepper", "Who Appears Behind the Warlock" },
    tunnel = { "the Deep-Delver", "the Earth-Swimmer" },
    quickness = { "the Blur", "the Lightning-Limbed" },

    -- Shields and auras
    shield = { "the Bulwark", "the Shield-Wall of One", "Who Blocked a Dragon's Tail" },
    fireShield = { "the Flame-Wreathed", "the Ember-Cloaked", "Who Turned Backstabbers to Ash" },
    frostShield = { "the Frost-Wreathed", "the Rime-Cloaked" },
    shockShield = { "the Storm-Wreathed", "the Spark-Cloaked", "Whom Only Wooden Clubs Dare Strike" },
    poisonStrikeback = { "the Thorn-Skinned", "the Venom-Barbed", "Whom Wyverns Regret Biting" },
    airShield = { "the Wind-Warded", "the Gale-Cloaked" },
    magicShield = { "the Rune-Warded", "the Spell-Turner" },
    awe = { "the Awe-Inspiring", "the Revered", "Whom Ogres Apologise Before Hitting", ", Too Sexy for Their Shirt" },
    fear = { "the Dread", "the Terrible", "Who Made a Demon Check the Door" },
    acidBlood = { "the Acid-Blooded", "the Corrosive", "Who Melted a Vampire's Fangs" },
    poisonAura = { "the Miasma-Wreathed", "the Blight-Breathed", "Whom Goblins Stand Upwind Of" },
    deathExplosion = { "the Doom-Laden", "of the Final Fire", ", Worst to Slay in a Corridor", "Who Will Have the Final Say" },
    diseaseShield = { "the Pox-Skinned", "the Plague-Warded" },
    slimy = { "the Slime-Coated", "the Ooze-Skinned", "Whom Nobody Wants to Grapple" },
    seduction = { "the Beguiling", "the Heart-Thief", ", Loved by Their Enemies" },
    petrification = { "the Gorgon-Eyed", "the Stone-Maker", "Who Left Statues at the Gate" },

    -- Hard to hit
    invulnerable = { "the Unbreakable", "the Iron-Skinned", "Whom Swords Bounce Off" },
    immortal = { "the Undying", "the Immortal" },
    absorbDead = { "the Grave-Glutton", "the Corpse-Swollen" },
    ethereal = { "the Ghost-Walker", "the Wraith-Touched", ", Only Solid on Holy Days" },
    lucky = { "the Fate-Kissed", "the Charmed", "Who Sneezed Beneath a War-Axe" },
    evasion = { "the Eel-Slick", "the Untouchable" },
    displaced = { "the Mirage", "the Phantom-Shifted" },
    mirrorImages = { "the Many-Faced", "of the Hundred Reflections", "Who Outnumbered an Ambush Alone" },
    twistFate = { "the Fate-Twister", "of the Second Chance" },

    -- Hitting harder
    doubleDamage = { "the Twice-Striking", "the Ruin-Striker" },
    slayUndead = { "the Wight-Bane", "the Lich-Breaker", ", the Necromancer's Bad Investment" },
    slayDemons = { "the Demon-Bane", "the Fiend-Slayer", "Who Sent the Demon Back Unpaid" },
    slayAnimals = { "the Beast-Bane", "the Great Hunter", "Whom Even Bears Play Dead For", "Whom Bears Fear" },
    slayHumans = { "the Man-Bane", "the Host-Breaker", ", the Villagers' Bedtime Story" },
    rangeBonus = { "the Far-Shot", "the Horizon-Shot", "the Mostly Accurate" },

    -- Strange gifts
    awakener = { "the Grove-Waker", "the Spore-Caller", "Who Talks to Mushrooms" },
    seaFarsight = { "the Sea-Watcher", "the Far-Sailor" },
    berserker = { "the Berserk", "the Battle-Mad", "Who Bit a Shield in Half" },
    blessed = { "the Blessed", "the Hallowed", "the Fanatically Religious" },

    -- Leading and sieges
    leadership = { "the Banner-Bearer", "the Born Captain", "Whom Even Mercenaries Obey Unpaid" },
    armyTrainer = { "the Drill-Master", "the Maker of Soldiers" },
    siege = { "the Wall-Breaker", "the Gate-Bane", "Whom Castles Surrender to on Sight" },
    gold = { "the Hoard-Finder", "the Coin-Blessed", "Who Taxed a Passing Dragon", "the Tech Bro" },
    scry = { "the Far-Scryer", "of the Crystal Eye" },
    unaging = { "the Ever-Young", "Whom Time Forgot", ", Cursed with Youth" },
    tangleResistance = { "the Vine-Proof", "Whom Roots Release" },
    hadesImmunity = { "the Death-Walker", "the Twice-Crossed" },
    coldHeal = { "the Frost-Mended", "the Ice-Knit" },

    -- Casters
    combatCaster = { "the Battle-Mage", "the War-Witch", "Who Brought a Grimoire to a Brawl" },
    pathBonus = { "the Adept", "the Rune-Wise" },
    spellRange = { "the Far-Caster", "the Long-Hexer" },

    -- Any other trait
    fallback = {
      "the Fate-Plucked", "the Risen", "the Battle-Crowned", "the Fate-Summoned",
      "Whom the Prophecy Misspelled", ", Promoted for Still Standing", "Who Outlived the Real Hero",
    },
  },

  woundEpithets = {
    lostEye = { "the One-Eyed", "the Half-Sighted" },
    blind = { "the Blind", "the Sightless" },
    chestWound = { "the Sore-Wounded", "the Short of Breath" },
    neverHealingWound = { "the Ever-Bleeding", "of the Wound That Never Closes" },
    battleFright = { "the Haunted", "the Shaken" },
    feeblemind = { "the Addled", "the Simple" },
    disease = { "the Plague-Bearer", "the Pox-Marked" },
    curse = { "the Accursed", "the Hexed", "Who Owes a Witch Money" },
    insanity = { "the Mad", "the Moon-Touched" },
    horrormark = { "the Horror-Marked", "the Doomed" },
    debilitatingWound = { "the Lame", "the Maimed" },
    festeringWound = { "the Festering", "the Rot-Touched" },
  },
}
