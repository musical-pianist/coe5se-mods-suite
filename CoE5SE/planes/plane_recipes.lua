-- CoE5SE's plane recipes: how the game builds its planes, as steps over the game's own tools
-- (coe5.gen), for a mod's plane declared with coe5.addPlane{like = "<Plane>"}.
--
-- CoE5SE runs this file in the declaring mod's own script state, once, and calls a recipe's
-- build(plane) while the world is built, and its special(plane) after the game's special squares.
-- `plane` holds name, index, x, y, w, h (the plane's rectangle in world squares), like, mod,
-- elysiumW and elysiumH (Elysium's size), and platform ("windows" or "mac").
--
-- Each recipe follows its vanilla plane's builder step by step, in the game's order and with the
-- game's numbers, on the mod's plane's rectangle:
--   - A step that needs a bigger plane than this one is skipped, and the log names its minimum
--     (lua_refused reason=plane_too_small). Nothing is scaled down.
--   - Where the game copies Elysium at the same offset (the Sky, Agartha, Hades), the recipe reads
--     Elysium's square at the same offset, and fills the squares past Elysium's size.
--   - Portals the game numbers itself (Hades' 29 and 34, the Elemental Planes' 5 to 8, 20, 23, 33)
--     would join the vanilla planes' pairs: a pair inside the plane gets a new number instead, and a
--     link to another vanilla plane is left out. CoE5SE gives every mod's plane a way in from Elysium.

local gen = coe5.gen
local get = coe5.get

-- The vanilla terrains the recipes lay or test, by number.
local PLAIN, FOREST_DEAD, MOUNTAINS = 0, 36, 12
local SEA, DEEP_SEA, SHALLOW_SEA, HOT_SEA = 27, 68, 69, 66
local VOID, CAVE_WALL, MAGMA_RIVER, MAGMA = 50, 51, 52, 53
local FROZEN_WASTE, INFERNAL_WASTE, ASH_FIELD = 54, 55, 56
local ROCK_GROUND, ROCK_GROUND_2 = 47, 189
local CLOUDS, FLAMES, SKY = 194, 198, 251
local UNSTABLE_CLOUDS, DISPERSING_CLOUDS = 305, 306
local CLOUD_FOREST, THUNDER_CLOUD = 208, 307
local KATABATIC_CAVE, DVALIN_STATUE = 331, 233
local GATEWAY, INACTIVE_GATEWAY, NEXUS = 204, 321, 223

-- Terrain flag bits (coe5.gen.terrainFlags).
local FLAG_WATER, FLAG_FOREST, FLAG_MOUNTAIN = 1 << 2, 1 << 13, 1 << 16

-- Every square of the plane gets terrain `t`, or terrainAt(i, j) when given (i, j from the corner).
local function fillPlane(p, t, terrainAt)
  for i = 0, p.w - 1 do
    for j = 0, p.h - 1 do
      gen.setTerrain(p.x + i, p.y + j, terrainAt and terrainAt(i, j) or t)
    end
  end
end

-- Elysium's terrain at offset (i, j), or nil past Elysium's size.
local function elysiumAt(p, i, j)
  if i < p.elysiumW and j < p.elysiumH then return get("squareTerrain", i, j) end
  return nil
end

-- The squares of the rectangle (x, y, w, h), cut to the plane, get terrain t.
local function fillRect(p, x, y, w, h, t)
  for sx = math.max(x, p.x), math.min(x + w, p.x + p.w) - 1 do
    for sy = math.max(y, p.y), math.min(y + h, p.y + p.h) - 1 do gen.setTerrain(sx, sy, t) end
  end
end

-- Terrain t at (x, y) when the square is on the plane.
local function put(p, x, y, t)
  if x >= p.x and y >= p.y and x < p.x + p.w and y < p.y + p.h then gen.setTerrain(x, y, t) end
end

-- The plane's temperatures and ground kinds, the steps every builder ends with.
local function finish(p, fixed)
  if fixed then gen.setFixedTemperature(p.x, p.y, p.w, p.h) else gen.setTemperatures(p.x, p.y, p.w, p.h) end
  gen.setGround(p.x, p.y, p.w, p.h, -1)
end

local recipes = {}

-- ---- Celestial Plane: a Void field with an island of Plain in a Sea, ringed by Clouds and Flames ----
recipes.Celestial = {
  build = function(p)
    fillPlane(p, VOID)                                       -- C2
    -- C3 to C9: two walks from (24, 24); the start must lie 1 in from the plane's edge, and 500 marks
    -- need room around it.
    if gen.refuseSmallPlane("celestial walks", 26, 26) then
      local sx, sy = p.x + 24, p.y + 24
      gen.markWalk(sx, sy, VOID, 500, 0x65)                  -- C3
      gen.markWalk(sx, sy, VOID, 250, 0x66)                  -- C4
      gen.marksToTerrain(0x65, VOID, SEA)                    -- C5
      gen.marksToTerrain(0x66, VOID, PLAIN)                  -- C6
      gen.markNearTerrain(VOID, PLAIN, 0x66)                 -- C7
      gen.marksToTerrain(0x66, VOID, PLAIN)
      gen.markVoidNear(SEA, 0x67, 2)                         -- C8
      gen.marksToTerrain(0x67, VOID, CLOUDS)
      gen.markVoidNear(CLOUDS, 0x68, 1)                      -- C9
      gen.marksToTerrain(0x68, VOID, FLAMES)
    end
    finish(p, true)                                          -- C10, C11
  end,
}

-- ---- Sky: Sky over Elysium, cloud banks with Cloud Forests, Thunder Clouds ----
recipes.Sky = {
  build = function(p)
    -- S2: Void where Elysium is Void, else Sky.
    fillPlane(p, SKY, function(i, j) return elysiumAt(p, i, j) == VOID and VOID or SKY end)
    -- S3: the cloud banks: walks 5 in from the edge.
    if gen.refuseSmallPlane("sky cloud banks", 12, 12) then
      local walks = gen.random(4) + 1 + (p.w * p.h) // 130
      for _ = 1, walks do
        -- The game passes the three draws as one call's arguments, and the two builds evaluate them
        -- in another order: Windows draws the size, then y, then x; Mac x, then y, then the size.
        -- @platform-split sky_cloud_draw_order: Windows draws a cloud walk's size first, Mac its x first
        local x, y, size
        if p.platform == "mac" then
          x = p.x + 5 + gen.random(p.w - 10)
          y = p.y + 5 + gen.random(p.h - 10)
          size = gen.random(25) + 2
        else
          size = gen.random(25) + 2
          y = p.y + 5 + gen.random(p.h - 10)
          x = p.x + 5 + gen.random(p.w - 10)
        end
        gen.markWalk(x, y, SKY, size, UNSTABLE_CLOUDS)
        gen.marksToTerrainChance(UNSTABLE_CLOUDS, CLOUDS, 100)
      end
    end
    -- S4: no cloud over Elysium's desert ground (ground kind 4).
    for i = 0, p.w - 1 do
      for j = 0, p.h - 1 do
        local t = get("squareTerrain", p.x + i, p.y + j)
        if (t == CLOUDS or t == UNSTABLE_CLOUDS or t == DISPERSING_CLOUDS) and i < p.elysiumW and j < p.elysiumH
            and get("squareGround", i, j) == 4 then
          coe5.set("squareTerrain", p.x + i, p.y + j, SKY)
        end
      end
    end
    -- S5: Cloud Forests in the banks, Thunder Clouds in the open sky.
    for i = 0, p.w - 1 do
      for j = 0, p.h - 1 do
        local x, y = p.x + i, p.y + j
        local t = get("squareTerrain", x, y)
        if t == CLOUDS then
          if gen.random(100) < 10 then
            gen.setTerrain(x, y, CLOUD_FOREST)
            gen.spreadTerrain(x, y, gen.random(2) + 1, CLOUDS)
          end
        elseif t == SKY and gen.random(1000) < 3 then
          gen.setTerrain(x, y, THUNDER_CLOUD)
        end
      end
    end
    finish(p, true)                                          -- S6, S7
  end,
}

-- ---- Agartha: Cave Wall under Elysium, a cavern, magma, tunnels, the statue of Dvalin, flooding ----

-- A Cave Wall spot: need 25 first when `first25`, else 9 (findCaveSpot's need).
local function caveSpot(first25)
  if first25 then
    local x, y = gen.findCaveSpot(25)
    if x and y then return x, y end
  end
  return gen.findCaveSpot(9)
end

recipes.Agartha = {
  build = function(p)
    -- A2: Cave Wall, keeping Elysium's Void and Deep Sea.
    fillPlane(p, CAVE_WALL, function(i, j)
      local t = elysiumAt(p, i, j)
      return (t == VOID or t == DEEP_SEA) and t or CAVE_WALL
    end)
    -- A3: the cavern.
    if gen.refuseSmallPlane("agartha cavern", 8, 8) then
      local x, y = gen.findCaveSpot(9)
      if x and y then
        gen.markWalk(x, y, CAVE_WALL, 27 + gen.random(4) + gen.random(4), 0xc9)
        gen.marksToTerrain(0xc9, CAVE_WALL, ROCK_GROUND_2)
      end
    end
    -- A4: magma pockets, one each for more than 500, 1000 and 2000 squares (three draws).
    local squares = p.w * p.h
    local pockets = 0
    if gen.random(squares) > 500 then pockets = pockets + 1 end
    if gen.random(squares) > 1000 then pockets = pockets + 1 end
    if gen.random(squares) > 2000 then pockets = pockets + 1 end
    for _ = 1, pockets do
      local x, y = gen.findCaveSpot(9)
      if x and y then gen.markWalk(x, y, -1, gen.random(8) + 1, 0xcb) end
    end
    gen.marksToTerrain(0xcb, CAVE_WALL, MAGMA)
    -- A5: tunnels.
    for _ = 1, gen.random(4) + 1 + squares // 80 do
      local x, y = gen.findCaveSpot(9)
      if x and y then gen.markWalk(x, y, -1, 2 + gen.random(4) + gen.random(4), 200) end
    end
    gen.marksToTerrain(200, CAVE_WALL, ROCK_GROUND)
    -- A6: a Katabatic Cave, when the world has none yet.
    local hasCave = false
    local depth, height = get("mapDepth"), get("mapH")
    for x = 0, depth - 1 do
      for y = 0, height - 1 do
        if get("squareTerrain", x, y) == KATABATIC_CAVE then hasCave = true break end
      end
      if hasCave then break end
    end
    if not hasCave and gen.random(100) < 20 and gen.refuseSmallPlane("agartha katabatic cave", 5, 5) then
      local x, y = caveSpot(true)
      if x and y then
        gen.markRect(x - 1, y - 1, 3, 3, 0xca)
        gen.marksToTerrain(0xca, CAVE_WALL, ROCK_GROUND)
        gen.setTerrain(x, y, KATABATIC_CAVE)
      end
    end
    -- A7: the Ancient Statue of Dvalin, with 2 to 6 items.
    if gen.refuseSmallPlane("agartha statue", 3, 3) then
      local x, y = caveSpot(true)
      if x and y then
        gen.markRect(x - 1, y - 1, 3, 3, 0xca)
        gen.marksToTerrain(0xca, CAVE_WALL, ROCK_GROUND)
        gen.setTerrain(x, y, DVALIN_STATUE)
        gen.addGroundItems(x, y, 2 + gen.random(3) + gen.random(3), 25)
      end
    end
    gen.fixDiagonals()                                       -- A8
    gen.setTemperatures(p.x, p.y, p.w, p.h)                  -- A9
    -- A10: flooding from the sea, at most 100 passes, then the moist walls.
    local flooded = false
    for _ = 1, 100 do
      if (gen.floodPlane() or 0) <= 0 then break end
      flooded = true
    end
    if flooded then gen.updateMoistWalls() end
    gen.updateMoistWalls()
    gen.setGround(p.x, p.y, p.w, p.h, -1)                    -- A11
  end,
}

-- ---- Inferno: a cave world around Kokytos, Pandemonium and the Inferno, framed in Void ----
-- The game builds Elysium's size, then grows the plane by a 2-square Void frame. Here the plane is
-- the size declared: the frame is its outer 2 squares, and the rest is built as the game's plane.
recipes.Inferno = {
  build = function(p)
    if not gen.refuseSmallPlane("inferno", 28, 28) then
      fillPlane(p, VOID)
      finish(p, false)
      return
    end
    local X, Y, w, h = p.x + 2, p.y + 2, p.w - 4, p.h - 4
    local cw, ch = math.min(w, 44), math.min(h, 44)
    local top = Y + h // 2 - ch // 2
    local cx = X + w // 2
    local mid = top + ch // 2
    local left = X + w // 2 - cw // 2
    gen.markRect(X, Y, w, h, 13)                             -- 2: Deep Sea for now
    fillRect(p, X, Y, w, h, DEEP_SEA)
    gen.markRect(left, top, cw, ch, 1)                       -- 3: the core, Cave Wall
    fillRect(p, left, top, cw, ch, CAVE_WALL)
    -- 4: Kokytos, Pandemonium and the Inferno.
    local regions = {{top + 5, 2, FROZEN_WASTE}, {mid, 3, INFERNAL_WASTE}, {top + ch - 5, 4, MAGMA}}
    for _, r in ipairs(regions) do
      gen.markBlob(cx, r[1], 125, r[2])
      gen.marksToTerrainChance(r[2], r[3], 100)
    end
    -- 5: 1 to 6 pillars of Cave Wall in the regions.
    for _ = 1, gen.random(6) + 1 do
      for _ = 1, 1000 do
        local x, y = X + gen.random(w), Y + gen.random(h)
        local m = gen.markAt(x, y)
        if m == 2 or m == 3 or m == 4 then
          fillRect(p, x, y, 2, 2, CAVE_WALL)
          break
        end
      end
    end
    -- 6: two magma rivers.
    gen.markRiver(cx, top + 10, cx, mid - 6, 11, 1)
    gen.markRiver(cx, mid + 7, cx, top + ch - 5, 11, 1)
    gen.marksToTerrain(11, CAVE_WALL, MAGMA_RIVER)
    -- 7: magma at the rivers' ends, a lake, ash in the Inferno.
    gen.markBlob(cx, mid - 6, 15, 5)
    gen.markBlob(cx, mid + 7, 25, 5)
    gen.marksToTerrainChance(5, MAGMA, 100)
    gen.markBlob(cx, top + 10, 20, 6)
    gen.marksToTerrainChance(6, SHALLOW_SEA, 100)
    gen.marksToTerrainChance(4, ASH_FIELD, 30)
    -- 8: the caves.
    local count = math.max(5, math.min(500, gen.random(4) + 1 + (cw * ch - 600) // 50))
    local caves = {}
    for k = 1, count do
      local x, y = gen.findCaveSpot(9)
      if not x or not y then break end
      local size = k == 1 and 15 or 6 + gen.random(4) + gen.random(4)
      gen.markWalk(x, y, CAVE_WALL, size, 9)
      caves[#caves + 1] = {x, y}
    end
    -- 9: each cave joined to its nearest, once per pair.
    local joined = {}
    for a, c in ipairs(caves) do
      local best, bestD
      for b, d in ipairs(caves) do
        local dist = math.abs(c[1] - d[1]) + math.abs(c[2] - d[2])
        if b ~= a and (not bestD or dist < bestD) then best, bestD = b, dist end
      end
      if best and not joined[best .. ":" .. a] then
        joined[a .. ":" .. best] = true
        gen.markRiver(c[1], c[2], caves[best][1], caves[best][2], 10, 1)
      end
    end
    gen.marksToTerrain(9, CAVE_WALL, ASH_FIELD)              -- 10
    gen.marksToTerrain(10, CAVE_WALL, ROCK_GROUND)
    for _, c in ipairs(caves) do gen.markRiver(c[1], c[2], cx, mid, 12, 2) end   -- 11
    gen.marksToTerrain(12, CAVE_WALL, ROCK_GROUND)
    gen.markNearTerrain(SHALLOW_SEA, MAGMA_RIVER, 8)         -- 12
    gen.markNearTerrain(SHALLOW_SEA, MAGMA, 8)
    gen.marksToTerrain(8, SHALLOW_SEA, HOT_SEA)
    gen.marksToTerrain(13, DEEP_SEA, CAVE_WALL)              -- 13
    fillRect(p, X, Y, w, 1, CAVE_WALL)                       -- 14
    fillRect(p, X, Y + h - 1, w, 1, CAVE_WALL)
    gen.setTemperatures(X, Y, w, h)                          -- 15
    for i = 0, p.w - 1 do                                    -- 16: the Void frame
      for j = 0, p.h - 1 do
        if i < 2 or j < 2 or i >= p.w - 2 or j >= p.h - 2 then gen.setTerrain(p.x + i, p.y + j, VOID) end
      end
    end
    gen.setGround(p.x, p.y, p.w, p.h, -1)                    -- 17
  end,
  -- The Inferno's special squares S1 to S3, row by row as the game walks them, each roll on the
  -- square's terrain before any of them. S4 to S9 (the Pit, the Infernal Pit, the cities and the
  -- citadels) find their squares by the builder's region marks, which only the vanilla Inferno keeps.
  special = function(p)
    local mapW, mapH = get("mapW"), get("mapH")
    local function terrainAt(x, y)
      if x < 0 or y < 0 or x >= mapW or y >= mapH then return -1 end
      return get("squareTerrain", x, y)
    end
    local WALL_OR_FLOW = {[CAVE_WALL] = true, [MAGMA_RIVER] = true, [MAGMA] = true, [HOT_SEA] = true, [SHALLOW_SEA] = true}
    local FLOWS = {[MAGMA_RIVER] = true, [MAGMA] = true, [HOT_SEA] = true, [SHALLOW_SEA] = true}
    for y = p.y, p.y + p.h - 1 do
      for x = p.x, p.x + p.w - 1 do
        local t = get("squareTerrain", x, y)
        if t >= FROZEN_WASTE and t <= ASH_FIELD then                       -- S1
          if gen.random(100) < 20 then
            gen.setTerrain(x, y, MOUNTAINS)
            if gen.random(100) < 25 then gen.setTerrain(x, y, 332)             -- Abandoned Mine
            elseif gen.random(100) < 15 then gen.setTerrain(x, y, 14)          -- Iron Mine
            elseif gen.random(100) < 5 then gen.setTerrain(x, y, 15) end       -- Silver Mine
          end
          if gen.random(100) < 5 then
            gen.setTerrain(x, y, FOREST_DEAD)
            if gen.random(100) < 3 then gen.setTerrain(x, y, 37) end           -- Shadow Wood
          end
          if t == FROZEN_WASTE and gen.random(100) < 4 then gen.setTerrain(x, y, 107) end   -- Swamp
          if gen.random(100) < 6 then gen.setTerrain(x, y, 59) end             -- Village of the Damned
          if gen.random(100) < 6 then gen.setTerrain(x, y, 270) end            -- Half-buried Sinners
          if gen.random(100) < 3 then gen.setTerrain(x, y, 336) end            -- Oven of Sins
          if gen.random(100) < 3 then gen.setTerrain(x, y, 57) end             -- Infernal Tower
          if gen.random(100) < 2 then gen.setTerrain(x, y, 16) end             -- Graveyard
          if gen.random(100) < 4 then gen.setTerrain(x, y, 19) end             -- Old Battlefield
        end
        if t == INFERNAL_WASTE then                                          -- S2: a Gem Deposit
          if gen.random(100) < 3 then gen.setTerrain(x, y, 40 + gen.random(4)) end
        elseif FLOWS[t] and gen.random(100) < 35 then                        -- S3: Tower Bridges
          if not WALL_OR_FLOW[terrainAt(x - 1, y)] and not WALL_OR_FLOW[terrainAt(x + 1, y)]
              and terrainAt(x, y - 1) == t and terrainAt(x, y + 1) == t then
            gen.setTerrain(x, y, 168)
          end
          if not WALL_OR_FLOW[terrainAt(x, y - 1)] and not WALL_OR_FLOW[terrainAt(x, y + 1)]
              and terrainAt(x - 1, y) == t and terrainAt(x + 1, y) == t then
            gen.setTerrain(x, y, 169)
          end
        end
      end
    end
  end,
}

-- ---- Hades: Elysium in shadow ----
-- Elysium's terrains that Hades turns into another (the builder's step 2).
local HADES_BASE = {
  [0] = 150, [44] = 150, [119] = 150,   -- Plain, Tundra, Savanna: Shade Land
  [115] = 116,                           -- Jungle: Destroyed Jungle
  [67] = 77,                             -- River: Black River
  [117] = 338,                           -- Desert: Ash Waste
  [50] = 50, [97] = 97, [107] = 107,     -- Void, Iron Bog, Swamp stay
}
local SHADE_LAND = 150

-- A Hades terrain for Elysium's terrain t.
local function hadesBase(t)
  if HADES_BASE[t] then return HADES_BASE[t] end
  local flags = gen.terrainFlags(t) or 0
  if flags & FLAG_WATER ~= 0 then return 76 end      -- Sea of the Dead
  if flags & FLAG_FOREST ~= 0 then return FOREST_DEAD end
  if flags & FLAG_MOUNTAIN ~= 0 then return MOUNTAINS end
  return PLAIN
end

-- Elysium's sites that Hades' special squares copy as they are, or turn into their ghost.
local HADES_SITES = {}
for _, t in ipairs({12, 13, 14, 15, 19, 29, 30, 73, 74, 88, 104, 105, 153, 332, 97, 107}) do HADES_SITES[t] = t end
local function hadesSites(list, to) for _, t in ipairs(list) do HADES_SITES[t] = to end end
hadesSites({4}, 37)                                -- Ancient Forest: Shadow Wood
hadesSites({5, 6}, 144)                            -- Farms: Ghost Farm
hadesSites({7, 184}, 145)                          -- Hamlet, House Ruins: Ghost Hamlet
hadesSites({78}, 146)                              -- Coastal Hamlet
hadesSites({18, 206}, 147)                         -- Village, Market Village
hadesSites({17, 185}, 148)                         -- Town, City Ruins
hadesSites({9, 207, 273, 274, 275, 276, 277, 278, 279}, 149)   -- the cities: City of the Dead
hadesSites({20, 21}, 151)                          -- Guard towers: Dark Tower
hadesSites({22, 38, 354}, 152)                     -- Castle, Old Castle Ruin, Mausoleum: Dark Citadel
for k = 0, 3 do
  HADES_SITES[79 + k], HADES_SITES[316 + k] = 323 + k, 323 + k   -- ports and pirate ports: Ghost Ports
  HADES_SITES[83 + k], HADES_SITES[135 + k], HADES_SITES[243 + k] = 180 + k, 180 + k, 180 + k   -- Capitolium districts
end

-- A random open land square of the plane (terrain code -42), or nil.
local function landSquare()
  return gen.randomSquareOfTerrain(-42)
end

recipes.Hades = {
  build = function(p)
    fillPlane(p, SHADE_LAND, function(i, j)                  -- 2
      local t = elysiumAt(p, i, j)
      return t and hadesBase(t) or SHADE_LAND
    end)
    finish(p, false)                                         -- 3, 4
  end,
  special = function(p)
    -- S2: Elysium's sites at the same offset, as Hades has them.
    local kings, temples = {}, {}
    for i = 0, math.min(p.w, p.elysiumW) - 1 do
      for j = 0, math.min(p.h, p.elysiumH) - 1 do
        local t = get("squareTerrain", i, j)
        local site = HADES_SITES[t]
        if site then coe5.set("squareTerrain", p.x + i, p.y + j, site) end
        if t == 124 then kings[#kings + 1] = {i, j} end      -- King's Castle
        if t == 39 then temples[#temples + 1] = {i, j} end   -- Ancient Temple
      end
    end
    -- S3: the Citadel of the Dead under the King's Castle, or an Ancient Temple, or on open land.
    local under = #kings > 0 and kings or temples
    local x, y
    if #under > 0 then
      local s = under[gen.random(#under) + 1]
      x, y = p.x + s[1], p.y + s[2]
    else
      x, y = landSquare()
    end
    if x and y then coe5.set("squareTerrain", x, y, 65) end
    -- S4: the Sheol Temple.
    x, y = landSquare()
    if x and y then coe5.set("squareTerrain", x, y, 224) end
  end,
}

-- ---- Elemental Planes: four elemental quarters around the Nexus ----
-- Fixed offsets from the plane's corner, as the game's 64 x 64 plane has them. Each part needs the
-- plane to reach its furthest square.
recipes.Elemental = {
  build = function(p)
    local X, Y = p.x, p.y
    fillPlane(p, VOID)                                       -- 2
    local gates = {}                                         -- name -> {x, y}
    local function gate(name, x, y)
      put(p, x, y, GATEWAY)
      gates[name] = {x, y}
    end
    if gen.refuseSmallPlane("elemental fire", 27, 34) then   -- 3: fire, north-west
      gen.markWalk(X + 20, Y + 20, VOID, 200, 1)
      gen.marksToTerrain(1, VOID, FLAMES)
      gen.markBlob(X + 20, Y + 26, 33, 2)
      for _ = 1, gen.random(6) + 4 do
        local bx, by = X + 13 + gen.random(14), Y + 13 + gen.random(14)
        gen.markBlob(bx, by, gen.random(4) + 1, 2)
      end
      gen.marksToTerrainChance(2, MAGMA, 100)
      gate("fire", X + 20, Y + 20)
    end
    if gen.refuseSmallPlane("elemental air", 45, 31) then    -- 4: air, north-east
      gen.markWalk(X + 44, Y + 20, VOID, 200, 3)
      gen.marksToTerrain(3, VOID, CLOUDS)
      gate("air", X + 44, Y + 20)
    end
    if gen.refuseSmallPlane("elemental earth", 31, 51) then  -- 5: earth, south-west
      gen.markWalk(X + 20, Y + 44, VOID, 200, 4)
      gen.marksToTerrain(4, VOID, CAVE_WALL)
      for _ = 1, 15 do
        local bx, by = X + 13 + gen.random(14), Y + 37 + gen.random(14)
        gen.markBlob(bx, by, gen.random(6) + 2, 5)
      end
      gen.marksToTerrainChance(5, ROCK_GROUND, 100)
      gate("earth", X + 20, Y + 44)
    end
    if gen.refuseSmallPlane("elemental water", 55, 55) then  -- 6: water, south-east
      gen.markWalk(X + 44, Y + 44, VOID, 200, 6)
      gen.marksToTerrain(6, VOID, DEEP_SEA)
      gen.markWalk(X + 46, Y + 46, DEEP_SEA, 33, 7)
      gen.marksToTerrain(7, DEEP_SEA, FROZEN_WASTE)
      gate("water", X + 44, Y + 44)
    end
    if gen.refuseSmallPlane("elemental nexus", 40, 35) then  -- 7: the Nexus
      local cx, cy = X + 32, Y + 32
      put(p, cx, cy, NEXUS)
      put(p, cx - 1, cy, ASH_FIELD) put(p, cx + 1, cy, ASH_FIELD)
      put(p, cx, cy - 1, ASH_FIELD) put(p, cx, cy + 1, ASH_FIELD)
      gate("nexus_fire", cx - 1, cy - 1) gate("nexus_air", cx + 1, cy - 1)
      gate("nexus_earth", cx - 1, cy + 1) gate("nexus_water", cx + 1, cy + 1)
      put(p, cx, cy - 2, INACTIVE_GATEWAY)
      gate("west_in", cx - 2, cy) gate("west_out", cx - 5, cy)
      put(p, cx - 6, cy, 24)                                 -- Dark Citadel
      put(p, cx - 7, cy, 320)                                -- Prison of Ba'al
      gate("east_in", cx + 2, cy) gate("east_out", cx + 5, cy)
      put(p, cx + 6, cy, ASH_FIELD)
      put(p, cx + 7, cy, 322)                                -- Eternal Vault
    end
    -- The plane's own pairs, each with a new number: the quarters to the Nexus, west and east.
    for _, pair in ipairs({{"fire", "nexus_fire"}, {"air", "nexus_air"}, {"earth", "nexus_earth"},
                           {"water", "nexus_water"}, {"west_in", "west_out"}, {"east_in", "east_out"}}) do
      local a, b = gates[pair[1]], gates[pair[2]]
      if a and b then gen.linkNewPortal(a[1], a[2], b[1], b[2], true) end
    end
    finish(p, false)                                         -- 8, 9
  end,
  special = function(p)
    -- S1: the citadels, each on a random square of its terrain.
    for _, c in ipairs({{ROCK_GROUND, {210, 211}}, {FLAMES, {212, 213}}, {MAGMA, {214}}, {DEEP_SEA, {218, 219, 220}},
                        {FROZEN_WASTE, {139}}, {CLOUDS, {215, 216, 217}}}) do
      for _, citadel in ipairs(c[2]) do
        local x, y = gen.randomSquareOfTerrain(c[1])
        if x and y then coe5.set("squareTerrain", x, y, citadel) end
      end
    end
    -- S2: the elemental sites.
    for i = 0, p.w - 1 do
      for j = 0, p.h - 1 do
        local x, y = p.x + i, p.y + j
        local t = get("squareTerrain", x, y)
        if t == CLOUDS then
          if gen.random(100) < 10 then
            coe5.set("squareTerrain", x, y, CLOUD_FOREST)
            gen.spreadTerrain(x, y, gen.random(3) + 1, CLOUDS)
          elseif gen.random(1000) < 15 then
            coe5.set("squareTerrain", x, y, THUNDER_CLOUD)
          end
        elseif t == ROCK_GROUND then
          if gen.random(100) < 10 then
            coe5.set("squareTerrain", x, y, 196)                       -- Crystal Forest
          elseif gen.random(100) < 2 then
            coe5.set("squareTerrain", x, y, 174 + gen.random(4))       -- a Deep Earth Gem Deposit
          elseif gen.random(100) < 1 then
            coe5.set("squareTerrain", x, y, gen.random(100) < 2 and 186 + gen.random(2) or 179)
          end
        elseif t == CAVE_WALL then
          if gen.random(100) < 4 then coe5.set("squareTerrain", x, y, 333) end   -- Giant Geode
        elseif t == MAGMA then
          if gen.random(100) < 5 then coe5.set("squareTerrain", x, y, 334) end   -- Perpetual Eruption
        elseif t == DEEP_SEA then
          if gen.random(1000) < 15 then coe5.set("squareTerrain", x, y, 335) end -- Enchanted Current
        end
      end
    end
    gen.setGround(p.x, p.y, p.w, p.h, THUNDER_CLOUD)         -- S4
  end,
}

-- ---- The Void: all Void, then its sites ----
local FOREST_OF_CUBES, ASTRAL_MARSHES, GLOWING_GROVE = 229, 258, 259
local COLOR_OUT_OF_SPACE, CORKSCREW_TOWERS = 250, 232

recipes.Void = {
  build = function(p)
    gen.markRect(p.x, p.y, p.w, p.h, 100)                    -- V2
    gen.marksToTerrainChance(100, VOID, 100)                 -- V3
    finish(p, false)                                         -- V4, V5
  end,
  -- The Void's special squares, row by row as the game walks them: on each Void square 2% a Forest
  -- of Cubes, else 2% Astral Marshes, else 2% a Glowing Grove, else 0.8% a Color out of Space while
  -- the world holds at most one; then the Corkscrew Towers on a land square. The game's links from
  -- Elysium's and Agartha's void gates and the Elemental Nexus (portal 4) stay with the vanilla Void.
  special = function(p)
    local colors = 0
    for x = 0, get("mapDepth") - 1 do
      for y = 0, get("mapH") - 1 do
        if get("squareTerrain", x, y) == COLOR_OUT_OF_SPACE then colors = colors + 1 end
      end
    end
    for y = p.y, p.y + p.h - 1 do
      for x = p.x, p.x + p.w - 1 do
        if get("squareTerrain", x, y) == VOID then
          if gen.random(100) < 2 then gen.setTerrain(x, y, FOREST_OF_CUBES)
          elseif gen.random(100) < 2 then gen.setTerrain(x, y, ASTRAL_MARSHES)
          elseif gen.random(100) < 2 then gen.setTerrain(x, y, GLOWING_GROVE)
          elseif gen.random(1000) < 8 and colors <= 1 then
            gen.setTerrain(x, y, COLOR_OUT_OF_SPACE)
            colors = colors + 1
          end
        end
      end
    end
    -- The game draws the towers' square over the world until one of the Void's is open land, which
    -- a plain Void square is; the land code (-42) takes no Void square, so the recipe draws a Void one.
    local x, y = gen.randomSquareOfTerrain(VOID)
    if x and y then gen.setTerrain(x, y, CORKSCREW_TOWERS) end
  end,
}

return recipes
