---@meta
-- What CoE5SE gives a mod's Lua script, written for your editor. CoE5SE never runs this file: it runs
-- only <ModName>.lua, and the files it requires. Copy it next to your script, and VS Code or Cursor
-- with the Lua extension (LuaLS) completes coe5.* as you type and shows these notes on hover.
--
-- How a script runs: CoE5SE loads it on the first main menu frame, after every DLL has started,
-- and runs its top-level code once. It has Lua's base library (without load, dofile, loadfile, and
-- collectgarbage, and with CoE5SE's own require, below), string, table, math, utf8, and coroutine;
-- there is no io, os, or package, and its only files are the ones it requires from its own mod's
-- folder and its own data files (coe5.readIni and the rest). print and warn
-- write lua_print and lua_warning lines to coe5mod.log, as coe5.log does; coe5.log's and print's
-- lines only while the player has CoE5SE's script_log setting on (off by default). One call into
-- the script may run 20,000,000 Lua steps, counting the coroutines it resumes, its __gc
-- finalizers, and library loops (table.move, concat, unpack, sort, insert, remove, and string
-- find, match, gmatch, gsub: one step per element, byte, or match step), and 10,000 steps for each
-- coe5.route call (about 2,000 routes in one call). The count starts again
-- when the game draws a frame (coe5.ui.wait, a next() that ran one, or a call that comes after
-- one, even inside the script's own wrap). A call that spends it stops
-- the script for good: pcall, a __close handler, or a new coroutine can't catch the stop, and
-- xpcall's handler doesn't run for it (lua_refused reason=step_limit). A script's Lua state may
-- hold 1.5 GB, and all scripts' together 6 GB: past the first the script gets "not enough memory"
-- and is stopped the same way (memory_cap); past the second the script holding the most is
-- (memory_total). A coe5 call made outside a
-- call into the script (a __gc finalizer while CoE5SE closes a stopped script) returns nil
-- (reason=not_running).
--
-- Cheat sheet:
--   Run another file of your mod   local safety = require("gatherer.safety")   (gatherer/safety.lua in your mod's folder)
--   Log a line                     coe5.log("hello")
--   Add a main menu button         coe5.addMainMenuButton("My Mod", openScreen)
--   ... right after Mods           coe5.addMainMenuButton("My Mod", openScreen, coe5.mainMenu.mods)
--   Add a Game Menu line           coe5.addGameMenuLine{label = "Resign now", onClick = function() return coe5.gameMenu.resign end}
--   ... in its Settings section    coe5.addGameMenuLine{label = "My Options", onClick = openOptions, section = coe5.gameMenuSection.settings}
--   ... in a section of its own    coe5.addGameMenuLine{label = "Resources", onClick = openResources, section = "Cheats"}
--   End the game, start a new one  coe5.restartGame()   (in a Game Menu line's onClick)
--   Pick a save, end, and load it  coe5.loadGame()      (in a Game Menu line's onClick)
--   Read or change a game value    coe5.get("society"), coe5.set("society", 3)
--   Read a player slot's value     coe5.get("slotName", 0)
--   Give a player 10 Gold          coe5.set("slotResource", p, 0, coe5.get("slotResource", p, 0) + 10)
--   Run code around a game call    coe5.wrap("PLAY_GAME_NEW_WORLD", function(next, mode) return next(mode) end)
--   ... outside other scripts' wraps  coe5.wrap("PLAY_GAME_NEW_WORLD", function(next, mode) return next(mode) end, 100)
--   Watch a call you may not change  coe5.observe("MONTH_START", nil, function(result, kind, text) coe5.log(coe5.turn()) end)
--   Send players a message         coe5.sendMessage(coe5.message.everyHuman, "Hello", "Welcome back.")
--   A game number by name          coe5.controller.human, coe5.gameOverMode.plain, ... (the tables at the end)
--   Read a setting your .c5se has  coe5.setting("welcome")   (on/off: true or false; a choice: its index from 0; a key: its key value)
--   Add a screen to your settings page  coe5.addScreen("Open My Mod", openScreen)
--   Change a key of the game's own coe5.rebindKey(coe5.keyAction.mapEndTurn, string.byte("k"))
--   Hide the Read Tip button       coe5.hideMainMenuButton(coe5.mainMenu.readTip, function() return true end)
--   Keep data in a file            coe5.writeIni("last", {Player1 = {Class = 3}}), coe5.readIni("last").Player1.Class
--   Build a screen                 coe5.ui.screenBegin("My Mod") ... coe5.ui.wait()
--   A row with a monster's picture coe5.ui.monsterRow(20, coe5.findMonster("Spearman"), "Spearman", "Recruit")
--   A commander's rituals          coe5.get("commanderRitual", c, 0), coe5.get("ritualString", r, 0)   (nil past the last)
--   What a ritual needs            coe5.get("ritualCostType", r, 0), coe5.get("ritualTerrainName", r), coe5.get("ritualEffect", r, 0)
--   Can a caster pay a cost        coe5.get("commanderCostAvailable", c, coe5.get("ritualCostType", r, 0)) >= amount
--   Time something                 local start = coe5.get("ticksMs") ... coe5.get("ticksMs") - start   (milliseconds)
--   A class's recruits             coe5.get("classRecruitName", coe5.get("slotClass", p), 0)   (nil past the last)
--   Make a unit on a square        coe5.spawnUnit(p, coe5.monsterVariant(coe5.findMonster("Spearman")), x, y)
--   The square under the pointer   coe5.get("mapPointerX"), coe5.get("mapPointerY")   (in a MAP_SCREEN_WAIT wrap, after next)
--   Keep the map from a click      coe5.set("mouseButtons", 0)   (the same wrap)
--   Send an AI explorer elsewhere  coe5.wrap("AI_EXPLORE", function(next, c, comstr) local s, x, y = next(c, comstr) return s, x, y end)   (Windows only)
--   A square's plane and portal    coe5.get("squarePlane", x, y), coe5.get("squarePortal", x, y), coe5.portalExits(x, y)
--   A commander's square           local u = coe5.get("commanderUnit", c); coe5.get("unitX", u), coe5.get("unitY", u)
--   Can an AI commander get there  coe5.route(c, x, y)   (nil for no route; only inside an AI site's wrap)
--   A square's army, unit by unit  for _, u in ipairs(coe5.squareUnits(x, y)) do local s = coe5.get("unitStrength", coe5.get("unitOwner", u), u) end   (Windows only for the strength)
--   Does a player see a unit        coe5.get("unitVisibleTo", u, p)   (stealth, invisibility, spiritsight; squareSeen for the fog; Windows only)
--   Write the AI's trace lines only  coe5.set("traceLevel", 2); coe5.keepTraceLines({"ailog", "aiship"})   (Windows only)
--   Keep an AI from an attack      coe5.wrap("AI_CONQUER", function(next, c, comstr) local s, x, y = next(c, comstr) if risky(x, y) then return 0 end return s, x, y end)
--   Move a resting AI commander    coe5.wrap("AI_RALLY", function(next, c, comstr) local s, x, y = next(c, comstr) if s <= 0 then return 1, hx, hy end return s, x, y end)
--   Drop an AI caster's ritual plan  coe5.scrapRitualPlan(c)   (only inside an AI site, AI_TURN's observers included)
--   Free one AI unit from its commander  coe5.releaseFollower(u)   (only inside an AI site, a unit of the moving AI; Windows only)
--   Move one AI unit to another commander  coe5.transferFollower(u, c)   (same, both of the moving AI on one square)
--   Plan a ritual an AI caster knows  coe5.planRitual(c, coe5.get("commanderRitual", c, 0))   (same; the commander has no plan)
--   A player's income next turn    coe5.get("slotIncome", p, 3)   (Fungus; Windows only)
--   What a terrain pays            coe5.get("terrainIncome", coe5.get("squareTerrain", x, y), 3)
--   Does a square count for a terrain  coe5.terrainMatch(coe5.get("ritualTerrain", r), x, y)
--   Keep the AI from planning a ritual  coe5.wrap("AI_RITUAL_CANDIDATE", function(next, c, ritual) if blocked(c, ritual) then return false end return next(c, ritual) end)   (Windows only)
--   A player's recruitment offers  for _, o in ipairs(coe5.shopOffers(p)) do coe5.log(o.name .. " " .. o.gold) end
--   Buy for the AI yourself        coe5.wrap("AI_RECRUIT", function(next, p) local sq = coe5.recruitSquares(p, offer)[1] if sq then coe5.aiBuyOffer(p, offer, sq.x, sq.y) end end)   (Windows only; skipping next skips the game's buying)
--   Leave troops for another AI commander  coe5.wrap("AI_GATHER_TROOPS", function(next, c) if not leader(c) then next(c) end end)   (Windows only)
--   Keep an AI commander from being picked up  coe5.wrap("AI_PICK_UP_LONER", function(next, c, unit) if busy(unit) then return 0 end return next(c, unit) end)   (Windows only)
--   Split AI armies of 30 units or more  coe5.wrap("AI_SPLIT_ARMY", function(next, c, minUnits) next(c, 30) end)   (Windows only)
--   Route onto an army with a group's strength  coe5.wrap("AI_MOVE_COMSTR", function(next, c) return next(c) + helpers(c) end)   (Windows only)
--   The commander a unit follows   coe5.get("unitCommander", u)   (-1 for none)
--   Every army a player sees now   for _, a in ipairs(coe5.armies(p)) do coe5.log(a.x .. "," .. a.y .. " " .. a.owner .. " " .. a.units) end
--   Does a monster fly             coe5.get("monsterMoveFlags", coe5.get("unitMonster", u)) & 1 ~= 0   (Windows only; unitMoveFlags adds its items)
--   Can an AI army take a sea square  coe5.get("commanderMoveFlags", c) & 192 ~= 0   (Windows only)
--   A caster's ritual level        coe5.get("commanderRitualLevel", c)   (its school: commanderRitpow; Windows only)
--   A square's library level       coe5.get("squareLibraryLevel", x, y)   (Windows only)
--   Did the player press Space on a commander  coe5.get("commanderDone", c)   ("Commander is done for this turn"; Windows only)
--   Block or add to a Voice of El ceremony  coe5.wrap("CEREMONY_PERFORM", function(next, ceremony, c, target) if coe5.get("ceremonyName", ceremony) ~= "Call the Inquisition" then next(ceremony, c, target) end end)   (Windows only)
--   Change a ceremony's cost       coe5.wrap("CEREMONY_COSTS", function(next, c) next(c) coe5.set("ceremonyCost", n, 30) end)   (n: the ceremony whose ceremonyName you matched; Windows only)
--   Keep a seal from breaking      coe5.wrap("SEAL_BREAK", function(next, slot, seal) if seal ~= 3 then return next(slot, seal) end coe5.set("eventVar", 300, 1) end)   (Windows only)
--   A line in the map editor's toolbox  local id = coe5.addMapEditorLine{label = "Show grid", onClick = function() on = true end}
--   Draw over the map editor's map  coe5.wrap("MAP_EDITOR_DRAW", function(next) local v = coe5.mapView() coe5.ui.drawLine(v.originX, 0, v.originX, 600, 1, 1, 1, 0.3) return next() end)
--   Replace a seal's event         coe5.wrap("SEAL_EVENT", function(next, slot, seal) if seal ~= 3 then return next(slot, seal) end coe5.set("eventVar", 300, 1) end)   (Windows only)
--
-- When CoE5SE refuses a call, the call returns false or coe5.message.notAdded (each note below
-- says which), and coe5mod.log says why with reason=<word>: api_refused (mod=<your mod>) for calls
-- that go through CoE5SE's plugin API, lua_refused for the rest. Search the log for "_refused".
--
-- A wrong argument type, an unknown name, a read-only value, or a call that runs too long is an
-- error instead: logged as lua_error, it stops the script until the game restarts. Its functions
-- don't run again, and its wraps pass the game's calls straight on (a wrap that errors gives the
-- game what next returned). A call of the script still running further out (a wrap waiting in
-- next) finishes, but adds no wrap, observer, or callback (lua_refused reason=script_stopped), and
-- CoE5SE turns setupAutomated off if the script left it on. Put a risky part in pcall to keep the script running (the step limit
-- is the one error pcall can't catch). An error names
-- the place as <ModName>.lua:<line> (or a required file's gatherer/safety.lua:<line>), never the
-- file's full path.
--
-- No table fills up: any number of mods can add buttons, Game Menu lines, and wraps.

---Runs `<name>.lua` from your mod's own folder, the one holding <ModName>.lua, and returns what it
---returns (true when it returns nothing). Each dot is a subfolder: require("gatherer.safety") runs
---gatherer/safety.lua. A file runs once, in your script's Lua state (its globals, step budget, and
---memory), and gets its name as `...`; a later require of the same name returns the first one's
---value. It works at load time and later. A name is 1 to 200 bytes of letters, digits, and _, a
---single dot between folders, in the case the files are stored in (on Windows too). Refused, with
---lua_refused call=require and an error to the caller: any other name (empty_name, name_too_long,
---bad_character, bad_name, reserved_name for a Windows device name such as con), a file or folder
---in another case (wrong_case), a link or junction on the way (link), and your own <ModName>
---(main_script). A missing file and a loop (a file that requires itself through others) are errors
---too, and a file of bytecode doesn't load. There is no package table.
---@param name string
---@return any
function require(name) end

---The game calls a script can run code around with coe5.wrap. Each note gives the function you
---pass and what `next` returns. A "frame" site runs once per frame of its screen, and its `next`
---returns the id clicked in that frame (0 for none). Lines a script draws with coe5.ui.text inside
---a wrap go onto the game's own screen. A site marked "Windows only" isn't found on the Mac build
---yet: coe5.wrap returns false there.
---@alias coe5.Site
---| "MAIN_MENU_WAIT"            # fn(next, block, mode): each frame of the main menu. next returns the clicked id (coe5.mainMenu).
---| "MAIN_MENU_PLAY_GAME"       # fn(next, server): Start New Game or Load Game (server 0 in single player). next returns after the Game Over screen, or at once if the player cancels setup. Not called when the player backs out of a loaded save whose spells changed.
---| "PLAY_GAME_NEW_WORLD"       # fn(next, mode): a new game's setup and world. Mode coe5.newGame.normal, .network, or .skipParticipants. next returns 0 on Cancel. Not called for a loaded game.
---| "NEW_WORLD_MAP_SCREEN"      # fn(next): Random Map Settings. next returns coe5.newGame.mapScreenOk on Ok, 0 on Cancel.
---| "NEW_WORLD_INIT_PARTICIPANTS"  # fn(next, arg): the default single-player slots, set before Setup Participants draws; after next returns, the slot values are what the screen opens with. Pass arg on unchanged. next returns nothing. Windows only.
---| "NEW_WORLD_CREATING_WORLD"  # fn(next, kind, text): "Creating World", before the classes are rolled. next(kind, text) shows the text; other text is shown as it is (a % in it isn't read as a format). next returns nothing.
---| "MAP_SETTINGS_WAIT"         # fn(next, block, mode): each frame of Random Map Settings. next returns the clicked id; coe5.newGame.mapSettingsOk is Ok.
---| "PARTICIPANTS_WAIT"         # fn(next, block, mode): each frame of Setup Participants. next returns the clicked id; coe5.newGame.participantsOk is Ok.
---| "PARTICIPANTS_DIFFICULTY_WAIT"  # fn(next, block, mode): each frame of Setup Participants' AI difficulty popup. next returns the clicked id. "newWorldUniqueRandom" is nil here (the popup is a call of its own).
---| "PARTICIPANTS_BUTTON_ROW"   # fn(next, style): starts Setup Participants' Ok row. Lines drawn before next go above the row; lines drawn after it go into the row, above Ok. next returns an integer. On Mac CoE5SE makes the row itself (the game has no call for it there), and next works the same.
---| "GAME_MENU_SAVING_SECTION_END"  # fn(next): closes the Game Menu's "Saving & Exiting" section. Lines drawn before next go at the end of the section. next returns an integer.
---| "GAME_MENU_WAIT"            # fn(next, block, mode): each frame of the in-game Game Menu (F10). next returns the clicked id; coe5.gameMenu.resign resigns. CoE5SE's own wrapper, which runs the Game Menu lines, runs first.
---| "RITUAL_SCREEN_ROW"         # fn(next, id, a, b, name, costType1, cost1, costType2, cost2, costType3, cost3, costType4, cost4, hover, apCost, castable, mirrorMonster, mirrorText): one row of the ritual screen. id is the slot + coe5.ritual.rowCastable, or + coe5.ritual.rowGreyed when greyed out; hover is the reason text, and the costs are already scaled for the caster; pass a and b on unchanged. hover and mirrorText may be nil; name may not. In next, a mirrorMonster past the last monster or a costType above 50 is refused (lua_refused reason=out_of_range) and the game's own value kept. next returns nothing the game uses (nil).
---| "MAP_SCREEN_WAIT"           # fn(next, block, mode): each frame of the map screen, after the game has bound its shortcuts. next returns the clicked id; coe5.mapAction.gameMenu opens the Game Menu. A commander's button is 20000 + its index (negated too); an id for a commander past the game's last becomes 0 (lua_refused, reason=commander_past_table). After next, "mapPointerX" and "mapPointerY" hold the square under the pointer and "mouseButtons" the frame's buttons (set it to 0 to keep a click from the map). A line drawn here isn't shown: the map lays out its lines before this wait.
---| "MAP_MENU_BUTTON_TEXT"      # fn(next, text): the map's "(F10) Menu" button text. next(text) draws it; pass other text to draw that. next returns nothing the game uses (nil).
---| "SQUARE_INFO_SCREEN"        # fn(next, player, x, y, mode): the square info window, "(i) terrain info", for square (x, y). In next, a player that isn't a slot (0-31) is refused (lua_refused reason=out_of_range) and the game's own kept. next returns nothing the game uses (nil).
---| "SQUARE_INFO_BUTTON_ROW"    # fn(next, style): starts the square info window's Ok row. Lines drawn before next go above the row. next returns an integer. On Mac CoE5SE makes the row itself (the game has no call for it there), and next works the same.
---| "SQUARE_INFO_WAIT"          # fn(next, block, mode): each frame of the square info window. next returns the clicked id; the window uses coe5.screen.squareInfoOk and 2.
---| "LOAD_GAME_BROWSER"         # fn(next, title): Load Game's save list (the game's file browser, which other screens use too). next returns 1 when a save was picked, 0 on Cancel. The browser's other arguments go on unchanged.
---| "FILE_BROWSER_ROW_DATE"     # fn(next, id, text): one row's date column in the file browser. Row n's id is coe5.screen.fileBrowserRowFirst + n; the row is 450 wide. next(id, text) draws it; draw more after it with coe5.ui.columnX and coe5.ui.text. next returns an integer.
---| "FILE_BROWSER_WAIT"         # fn(next, block, mode): each frame of the file browser. Returning coe5.screen.fileBrowserDelete deletes the row the "hoveredId" value names (the browser reads it right after the wait), after the game's own question. A row id past the browser's last listed row becomes 0, no click (lua_refused, reason=row_past_list or hovered_row_past_list).
---| "FILE_BROWSER_HOVERED"      # Windows only until CoE5 5.41, which inlined it as Mac always had: on neither build now, so coe5.wrap returns false. The browser reads "hoveredId" right after its wait; set it in a FILE_BROWSER_WAIT wrap to pick the row to delete.
---| "MOD_BANNER_DRAW"           # fn(next, path, x, y, w, h, alpha): one enabled mod's banner in the column on the right of the menu screens. x is the screen width - 210, y the screen height - 60 - 60 * row, w 200. Skip next to hide the banner. Pass path to next as it came: another path is refused (lua_refused reason=path_changed) and the game's own banner drawn. next returns nothing.
---| "GAME_OVER_WAIT"            # fn(next, block, mode): each frame of the Game Over screen. The game passes coe5.gameOverMode.fog (fog and falling particles; mode 8 draws them too); any other mode clears to black, and coe5.gameOverMode.plain draws nothing behind the screen. next returns the clicked id.
---| "ARMY_WAIT"                 # fn(next, block, mode): each frame of Transfer troops. next returns the clicked id.
---| "BATTLE_WAIT"               # fn(next, block, mode): each frame of a battle's real-time loop. next returns the clicked id.
---| "MAP_EDITOR_WAIT"           # fn(next, block, mode): each frame of the map editor. next returns the clicked id (coe5.mapEditor, or a coe5.addMapEditorLine id). A key bound with coe5.ui.bindKey before next counts for that frame, toolbox hidden or not.
---| "MAP_EDITOR"                # fn(next): the whole map editor, opened from the Mods screen's Start Map Editor or the --mapeditor launch option; next returns when the player quits it. With --mapeditor the editor opens before the main menu: the scripts load just before it, and the game exits when it closes.
---| "MAP_EDITOR_DRAW"           # fn(next): once per frame of the map editor's map, after the map's squares and before the toolbox, the plane buttons, and the hover box. Draw with coe5.ui.drawLine and coe5.ui.drawText: before next lands over the map and under the toolbox, after next over everything but the hover box. coe5.mapView says where each square is. next returns nothing.
---| "BATTLE_EDITOR_WAIT"        # fn(next, block, mode): each frame of the battle map editor. next returns the clicked id.
---| "RECRUIT_WAIT"              # fn(next, block, mode): each frame of Recruit. next returns the clicked id.
---| "VIEW_ARMY_WAIT"            # fn(next, block, mode): each frame of an army on the battlefield, before a battle. next returns the clicked id.
---| "UNIT_LIST_WAIT"            # fn(next, block, mode): each frame of a battle's unit list (F1). next returns the clicked id. A unit's row is 100000 + its unit index (negated too); an id for a unit past the game's last becomes 0 (lua_refused, reason=unit_past_table).
---| "KEYBOARD_SHORTCUTS_WAIT"   # fn(next, block, mode): each frame of the Keyboard Shortcuts screen. next returns the clicked id.
---| "TOOLTIP_LINE_DRAW"         # fn(next, text, index, r, g, b, alpha): one line of the hover text box at the bottom of the screen, on every frame it shows. text is the line as the game cut the hover text to the box's width, index its place (0 the top line; the game draws them from the last up), and r, g, b, alpha its colour, 0 to 1 (the game's white, 1 1 1 1). next(text, index, r, g, b, alpha) draws it in the colour you pass. Pass text and index as they came: other ones are refused (lua_refused reason=text_changed or index_changed) and the line's own kept. next returns nothing.
---| "AI_EXPLORE"                # fn(next, commander, comstr): an AI player's exploration target for one commander, during its moves; comstr is the commander's strength. next(commander, comstr) returns the game's choice: score, x, y (x and y coe5.aiMove.noSquare for none). Return score, x, y to send the commander to any square of the world, on any plane (the game routes it, through portals too); return score alone to keep next's square; return nothing to keep next's choice. The game runs this only while the commander's earlier steps (rally, friendly move, conquest) scored below coe5.aiMove.exploreBelow, and keeps the square only when the score is above their best (commanderAiMoveKind is coe5.aiMove.target when one of them picked a square, at commanderAiTargetX and Y); a score of coe5.aiMove.farExploreBelow or more also skips the game's far exploration. A square off the world or on no plane, a score above 0 with no square, or values that aren't whole numbers are refused (lua_refused reason=off_map, no_plane, no_square, not_a_score, not_a_square, out_of_range) and next's choice kept; another commander in next is refused (commander_changed). Not in network games. Windows only: the Mac AI stays the game's own.
---| "AI_RALLY"                  # fn(next, commander, comstr): the AI's rally step for one commander, the first of its steps: a square of the player's where its other commanders' armies could join this one. Takes and returns what AI_EXPLORE does, with the same refusals; the game keeps the square when the score is above 0, and the move is then coe5.aiMove.target. Windows only.
---| "AI_FRIENDLY_MOVE"          # fn(next, commander, comstr): the AI's move to a square a teammate owns, after AI_RALLY; the game drops a square where an enemy army near it is much stronger than comstr. Kept when the score is above the rally's; as AI_RALLY otherwise. Windows only.
---| "AI_CONQUER"                # fn(next, commander, comstr): the AI's conquest step, after AI_FRIENDLY_MOVE: a square to attack or take, among those the player has seen; -999 for none. Kept when the score is above the earlier steps' best; as AI_RALLY otherwise. Windows only.
---| "AI_FAR_EXPLORE"            # fn(next, commander, comstr): the AI's far exploration, after AI_EXPLORE, run only while the best score is below coe5.aiMove.farExploreBelow: an unseen square anywhere. Kept when the score is above the best, and the move is then coe5.aiMove.explore. As AI_RALLY otherwise. Windows only.
---| "AI_RITUAL_CANDIDATE"       # fn(next, commander, ritual): one candidate of the AI's ritual plan choice for a commander with no plan (commanderRitualPlan 0), after the game's income, "rare", "never plan this" and other data reasons passed it. next(commander, ritual) runs the game's remaining checks of it, the pointless check first, and returns true when they keep it, false when they drop it. Return false to veto the candidate (the game drops it as "pointless" and draws the next); return true or nothing to keep it, which leaves the decision to the game: a kept candidate always gets next's answer, called for you if the wrap didn't, so a wrap can veto but never plan what the game drops. Another value is refused (lua_refused reason=not_a_boolean) and keeps; another commander or ritual in next is refused (commander_changed, ritual_changed). A wrap that errors keeps. coe5.scrapRitualPlan works inside it. Not in network games. Windows only.
---| "AI_RECRUIT"               # fn(next, player): an AI player's recruiting step, once a turn after AI_TURN and before its commanders move. next(player) runs the game's own buying: from the player's offers (coe5.shopOffers) at its citadels, keeping gold for the game's save target (10 turns of gold income, at most 250). Skip next and the game buys nothing this turn. coe5.aiBuyOffer buys inside it, before or after next. next returns nothing. Not in network games. Windows only.
---| "AI_GATHER_TROOPS"         # fn(next, commander): the AI's troop pick-up for one commander, before each of its move steps: next(commander) takes the free troops on its square that may join it, leaving a share there (20% on a citadel, 60% with under 2 citadels), or, with none free, leaves some of its own ranged troops behind. Don't call next to skip it: the commander takes and leaves nothing this step, and the troops stay free for another commander there. Another commander in next is refused (commander_changed). A wrap that errors runs it. coe5.scrapRitualPlan works inside it. Not in network games. Windows only.
---| "AI_PICK_UP_LONER"          # fn(next, commander, unit): the AI's pick-up of one lone commander (unit is its unit) into the army of commander, a caster of a higher ritual level on its square, after the game's checks passed it: from then on the lone commander and its followers follow commander and never move on their own again. next(commander, unit) joins them and returns how many units joined; don't call it (return 0) to keep the lone commander free. Another commander or unit in next is refused (commander_changed, unit_changed). A wrap that errors runs it. coe5.scrapRitualPlan works inside it. Not in network games. Windows only.
---| "AI_SPLIT_ARMY"             # fn(next, commander, minUnits): the AI's split of a commander's army, at the end of its routine before its moves, in 25% of the calls: with minUnits units or more following it (the game's own is 240), the best commander that follows it leaves with some of its troops (each troop of strength up to 100 to 299 with a 10% chance, in two passes, until the larger of a fifth and 40 to 99 units went). minUnits is the game's minimum for this call; next(commander, minUnits) runs the split with the minimum given, 1 or more, for this call only (lua_refused reason=out_of_range keeps the game's). Don't call next to skip it. No commander follows another unless the AI picked it up (AI_PICK_UP_LONER), so with none nothing splits. Another commander in next is refused (commander_changed). A wrap that errors runs it. coe5.scrapRitualPlan works inside it. Not in network games. Windows only.
---| "AI_MOVE_COMSTR"            # fn(next, commander): the strength the AI commander's route takes for one move step, right after AI_GATHER_TROOPS and before the game routes it to the square its steps kept. next(commander) returns the game's own, the strength of the commander's army. The route won't end on a square whose army is stronger than the strength allows (times 100, divided by aiAttackDivOther, aiAttackDivAi or aiAttackDivHuman by that army's owner, less 15, aiAttackMargin's value), so return a larger one, such as several commanders' strength together, to let it route onto a stronger army; the square itself was picked by the steps. Returns a whole number from -1 to 21474836, as coe5.route's comstr; another value is refused (lua_refused reason=not_a_number or out_of_range) and next's kept. Return nothing for next's. Another commander in next is refused (commander_changed). coe5.route works inside it. A wrap that errors gives the game's. Not in network games. Windows only.
---| "CEREMONY_PERFORM"          # fn(next, ceremony, commander, target): one of the Voice of El's ceremonies, a player's (the Ceremonies screen, a map order) or the AI's. ceremony is the game's number for it: match it by coe5.get("ceremonyName", ceremony), never by the number. target is the player a Start Crusade goes against, -1 for every other ceremony. next(ceremony, commander, target) is the game's own: it checks the rank, limits and cost (and does nothing when one fails), charges the cost and action points, and does the ceremony. Skip next and none of it happens: no cost, no action points, no effect, and no message. Code after next adds effects of your own. Another ceremony, commander or target in next is refused (ceremony_changed, commander_changed, target_changed) and the game's kept. A wrap that errors runs it. Not in network games. Windows only.
---| "CEREMONY_COSTS"            # fn(next, commander): the game works out every ceremony's cost for a priest of El: at the top of the Ceremonies screen, before a map order's ceremony, and before the AI's. After next(commander), coe5.get("ceremonyCost", ceremony) holds each one and coe5.set("ceremonyCost", ceremony, value) changes it: the screen shows that cost, the ceremony checks and charges it, and the AI weighs it, until the next time the game works them out. The game multiplies some: Break Seal's by the seal's number (1 for the first), Bolster Devotion's by one more than the player's earlier Bolster Devotions, Increase Divine Favor's by the level it raises the favor to, Convert Settlement's by a fifth of the square's gold income (its action points too). Each is gold, or relics for Break Seal, Bolster Devotion and Increase Divine Favor. Another commander in next is refused (commander_changed). Not in network games. Windows only.
---| "SEAL_BREAK"                # fn(next, slot, seal): one of the Seven Seals breaking, by a player's Break Seal ceremony or an item, slot that player; seal is the seal it breaks, 1 to 7. Inside the wrap the seals broken are still seal - 1; next(slot, seal) breaks it, as the game does. Skip next and the seal stays unbroken, so none of what follows from it happens (its event, SEAL_EVENT; El's minions for the Voice of El players get no stronger), and the next Break Seal ceremony breaks the same seal again. The ceremony's cost and action points are spent either way. To break the seal but replace its event, wrap SEAL_EVENT instead. The seals the game breaks on its own at a month's end ("Cardinal Intervenes", "Heavenly Surge of Power") don't come here. Another slot or seal in next is refused (slot_changed, seal_changed). A wrap that errors breaks it. Not in network games. Windows only.
---| "SEAL_EVENT"                # fn(next, slot, seal): a broken seal's event, at a month's end: the month after it broke, one seal a month in order when several broke, whoever broke it (a ceremony, an item, or the game at a month's end), once per seal. seal is 1 to 7; the game's events: none for the first two, an Apocalyptic Rider for the third to the sixth ("The apocalypse is coming closer"), the Apocalypse for the seventh. slot is always -1: the game keeps no record of who broke a seal. next(slot, seal) lets the game run its event, which it does once every wrap has returned (so code after next runs before the event); next returns nothing. Skip next to replace the event with your own: the seal stays broken, its event counts as done and never comes back, and the next seal can be broken as before. What the wrap returns isn't read. Another slot or seal in next is refused (slot_changed, seal_changed). A wrap that errors runs it. Not in network games. Windows only.

---The game calls a script can watch with coe5.observe but not change: each decides or applies game
---content. before(...) gets a copy of the arguments before the call, and after(result, ...) the
---call's result and the arguments after it (result nil where the call has none). What they return
---doesn't reach the game.
---@alias coe5.ObservedSite
---| "RITUAL_SCREEN_LOCATION_CHECK"  # (commander, slot, a, b): the ritual screen's location check for one row, right before the row is drawn; a and b are -999. A result of 0 or below is a reason code.
---| "RITUAL_SCREEN_CAN_CAST"    # (commander, slot): the ritual screen's cast check for one row. A result of 0 or below is a reason (coe5.ritual.reasonMirror, ...).
---| "GAME_START"                # (a, b): once before the first turn of a new or loaded game; both 0 in single player. After a load, "turn" holds the saved month. No result.
---| "MONTH_START"               # (kind, text): the "New Month" text, after the game's monthly update; "turn" already holds the new month. The result is nil.
---| "PLAYER_TURN_START"         # (player, quick): one player's start-of-turn step, humans, AI and independents (slots 24-30) alike; quick is 1 for AI and independents. No result.
---| "HUMAN_TURN"                # (player): a human player's turn; after runs when they end it. The result is nil.
---| "AI_TURN"                   # (player): an AI player's moves. No result.
---| "PLAYER_TURN_END"           # (player): the first end-of-turn step after any player has moved.
---| "GAME_OVER"                 # (winner): the game over screens (not in autorun games). No result.
---| "GAME_SAVE"                 # (path, name): the Game Menu's Save and both autosaves. A result above 0 means the game was saved; after can write the mod's own data next to it.
---| "GAME_LOAD"                 # (path, a): Load Game, and a load from the command line. GAME_START follows.

---The game values coe5.get and coe5.set reach.
---@alias coe5.Variable
---| "turn"                  # integer: 0 on the first turn, -1 during setup. Read-only.
---| "networkGame"           # boolean. Read-only.
---| "loadedGame"            # integer: nonzero for a game loaded from a save. Setting it nonzero logs lua_loaded_game_set with your mod.
---| "loadedMapName"         # string: the map file the game started on, empty for a random map. Read-only.
---| "society"               # integer: 0 Random, 1 Dark Ages, 2 Agricultural, 3 Empire, 4 Fallen Empire, 5 Monarchy, 6 Dawn of a New Empire, 7-11 sub-societies; nothing else (the game's society table). Changes the moment the player clicks it.
---| "commonCause"           # integer: the Advanced Options checkbox. Changes the moment the player clicks it. Read during play and saved, so a change in a game counts.
---| "scoreGraphs"           # integer: the Advanced Options checkbox. Changes the moment the player clicks it. Read during play and saved, so a change in a game counts.
---| "battleReports"         # integer: the Advanced Options checkbox. Changes the moment the player clicks it. Read during play and saved, so a change in a game counts.
---| "mapW"                  # integer: the map's width in squares, written at Ok (with a random ±1). At most 1000, the game's square arrays.
---| "mapH"                  # integer: the map's height in squares, written at Ok (with a random ±1). At most 160, the game's square arrays.
---| "startFlags"            # integer: start positions, written at Ok: 8 clustered, 16 west/east, 0 random.
---| "wilder"                # integer: Wilder, written at Ok: 2 on, 1 off, 0 before the first game. 0 to 2, what the game stores.
---| "mapDepth"              # integer: despite the name, the width of the whole world: the map's width, raised as planes are added beside the map. At most 1000.
---| "northPart"             # integer: Random Map Settings' North/South box, the north part in percent; -1 is the default, 25. -1 to 100, what the game stores. Not saved.
---| "southPart"             # integer: the south part in percent; -1 is the default, 35. -1 to 100. Not saved.
---| "slotController"        # per slot, integer: coe5.controller.human, .ai, or .removed; .independent once the world is built.
---| "slotClass"             # per slot, integer: the class's `newclass` number (1 Baron, 2 Necromancer, ...). Below 1 is Random until the classes are rolled. At most 199, the class table's last record.
---| "slotAi"                # per slot, integer: AI difficulty, 2 (Jester) by default. Read at each turn start and saved, so a change in a game counts. At most 10: the game stops on a level outside 1 to 10 at an AI's turn.
---| "slotTeam"              # per slot, integer: below 0 is no team.
---| "slotName"              # per slot, string, up to 31 characters. Humans' names are filled in after PLAY_GAME_NEW_WORLD's next.
---| "slotColorRed"          # per slot, number 0 to 1: the red of the player's colour, the square on Setup Participants. Each new game's setup starts every slot on the palette's colour (playercolor). Takes what the game stores: a finite number from -8421505 to 8421504 (playercolor's whole numbers over 255); NaN or an infinity is refused (reason=not_finite).
---| "slotColorGreen"        # per slot, number 0 to 1: the colour's green.
---| "slotColorBlue"         # per slot, number 0 to 1: the colour's blue.
---| "slotResource"          # per slot and resource, integer: how much of resource 0-13 (resourceName's order, 0 Gold) the player has. coe5.get("slotResource", slot, resource), coe5.set("slotResource", slot, resource, value).
---| "slotIncome"            # per slot and resource 0-13, integer: what the player gets of the resource at its next turn start, as the game works it out then: each special resource (all but Gold, Iron and Trade) only while the player has a commander who gathers it, then an AI's difficulty percent (a human's is 100; an AI's 100 plus its level's bonus, a quarter of a positive bonus more for the special resources, and that quarter less for Gold, Iron and Trade while the AI has any special income). Trade is the trade points. 0 for a slot that is neither a human nor an AI. Nil on Mac. Read-only.
---| "newWorldUniqueRandom"  # integer: Setup Participants' "unique random players" box, a local of the new game's setup call. Nil, and not writable, outside a wrap of a NEW_WORLD_* or PARTICIPANTS_* site, and inside PARTICIPANTS_DIFFICULTY_WAIT.
---| "cliMapW"               # integer: the command line's --mapw, below 1 when not given. Random Map Settings opens on exactly cliMapW x cliMapH when either is set. At most 1000.
---| "cliMapH"               # integer: the command line's --maph, below 1 when not given. At most 160.
---| "cliSociety"            # integer: the command line's --society, below 0 when not given. Read-only.
---| "subSociety"            # boolean: the command line's --subsoc, which lets the player pick a sub-society. Read-only.
---| "startDefault"          # integer: the start positions Random Map Settings opens with: 0 normal, 1 clustered, 2 west/east; nothing else.
---| "wilderDefault"         # boolean: whether Random Map Settings opens with Wilder on.
---| "setupNetwork"          # boolean: true while a network game is set up. Read-only.
---| "aiLevelName"           # index 0-10, string: the AI difficulty names that slotAi indexes. Read-only.
---| "screenW"               # integer: the game's UI width. Read-only.
---| "screenH"               # integer: the game's UI height. UI y counts up from the bottom. Read-only.
---| "hoveredId"             # integer: the id of the line under the pointer, as the file browser reads it right after its wait. A row id past the browser's last listed file doesn't reach its Delete (see FILE_BROWSER_WAIT).
---| "bannerFade"            # number: the mod banner column's opacity, 0 to 1. The game lowers it every frame and draws the banners only while it is above 0.
---| "activePlayer"          # integer: the slot whose turn it is, -1 outside a game. Read-only.
---| "season"                # integer: 0 Summer, 1 Autumn, 2 Winter, 3 Spring. Read-only.
---| "seasonName"            # index 0-3, string: the game's name for a season ("Winter"). Read-only.
---| "date"                  # string: the date as the map shows it, "Mid Summer of year 0". Read-only.
---| "worldW"                # integer: the world's width in squares. Read-only.
---| "worldH"                # integer: the world's height in squares. Read-only.
---| "uniqueRandomDefault"   # boolean: whether Setup Participants' "unique random players" box starts ticked.
---| "slotLockClass"         # per slot, integer: the class a loaded map locks the slot to. Read-only.
---| "slotLockController"    # per slot, integer: the controller a loaded map locks. Read-only.
---| "slotLockAi"            # per slot, integer: the AI difficulty a loaded map locks. Read-only.
---| "slotLockTeam"          # per slot, integer: the team a loaded map locks. Read-only.
---| "eventVar"              # index 0-9999, integer: the .c5m event variables that setvar and +varequal use.
---| "modLastIndex"          # integer: the last index in the game's mod list. Read-only.
---| "modName"               # index 0 to modLastIndex, string: the mod's .c5m name. Read-only.
---| "modFolder"             # index 0 to modLastIndex, string: the mod's folder. Read-only.
---| "modEnabled"            # index 0 to modLastIndex, integer: nonzero when enabled on the Mods screen. Read-only.
---| "monsterName"           # index 0 to monsterLastIndex, string (empty for an unused index). Read-only.
---| "monsterLastIndex"      # integer. Read-only.
---| "monsterSprite"         # index 0 to monsterLastIndex, integer: the monster's picture, for coe5.ui.nameBox. Read-only.
---| "itemName"              # index 0 to itemLastIndex, string. Read-only.
---| "itemLastIndex"         # integer. Read-only.
---| "className"             # index 0 to classLastIndex, string; 0 is "no class". Read-only.
---| "classLeader"           # index 0 to classLastIndex, string: the class leader's monster name. Read-only.
---| "classLastIndex"        # integer. Read-only.
---| "terrainName"           # index 0 to terrainLastIndex, string. Read-only.
---| "fortPartWord"          # index 0 to 31, a fort part bit, string: the terrain word that sets it ("irongate" for 11, the bit of 2048), from the game's terrain reader; nil for a bit none sets. Read-only.
---| "terrainLastIndex"      # integer. Read-only.
---| "terrainIncome"         # index: a terrain (as terrainName), then a resource 0-13, integer: the terrain's own income of the resource per month, as its .c5m gives it (gold, iron, trade, herbs, fungus, weed, sacr, hands), before seasons, class bonuses and enchantments; nil for Gems (7-10, a square's squareGems), Relics and Lifeforce, which no terrain field gives. Read-only.
---| "ritualName"            # index 0 to ritualLastIndex, string. Read-only.
---| "ritualLastIndex"       # integer. Read-only.
---| "resourceName"          # index 0-13, string: Gold, Iron, ... Lifeforce. Read-only.
---| "incomeName"            # index 0-18, string: the game's income names, "Gold Income" ... "Lifeforce Income", then the cost types past 13: "Humanoid Corpses", "Gem Income", "Expendable Units Income", "Corpses", "Goblin Income". Read-only.
---| "classRecruitName"      # index: a class (0 to classLastIndex), then an offer 0-159, string: the monster name of the class's recruitment offer (addunitrec, addmercrec, addcomrec); nil at and past the list's end. Read-only.
---| "classRecruitCount"     # index: a class, then an offer 0-159, integer: units per offer, -1 for a commander (addcomrec); nil at and past the list's end. Read-only.
---| "classStartArmy"        # index: a class, then a line 0-15, string: a start army line, "6 Spearman" (addstartunits) or "High Lord" for a commander (addstartcom); nil at and past the last line. Read-only.
---| "classStandardTroops"   # index: a class, boolean: the class has stdtroops, so it also recruits Spearman, Swordsman, Archer, Crossbowman, Heavy Infantry, and Catapult. The game holds those six names in its code, not in a table, so a script lists them itself. Read-only.
---| "classAbility"          # index: a class, then an ability id (coe5.classAbility), integer: the class's value of it, through the game's own lookup (the first match), 0 when the class has none. The game's random class roll reads aiClass, randomSkip, aiLevelMin, and aiLevelMax. Read-only.
---| "mapPointerX"          # integer: the map square under the pointer in this map frame, -1 when none (it is a square of the world, planes side by side). Nil outside a wrap of MAP_SCREEN_WAIT; read it after next. Read-only.
---| "mapPointerY"           # integer: that square's y, -1 when none. Nil outside a wrap of MAP_SCREEN_WAIT. Read-only.
---| "mouseButtons"          # integer: this frame's mouse buttons, 1 left, 2 right, 4 middle, added; the first frame of a press, then each frame it is held. In a wrap of MAP_SCREEN_WAIT, after next, set it to 0 to keep the map from acting on the click (a left click moves the selected commander), and keep setting it to 0 while the button stays down. 0 to 7.
---| "squareSeen"            # index: a slot, then x 0-999, then y 0-159, integer: what that player sees of the square: 1 darkened, 2 seen, 3 scried, 0 none of these. The map acts on a click only on a square above 1. Read-only.
---| "squareArmy"            # index: x 0-999, then y 0-159, integer: the square's army size, 0 for none, -1 while two players at war share it. Read-only.
---| "squareArmyOwner"       # index: x 0-999, then y 0-159, integer: the player whose army stands on the square (the one with the most units), -1 for none or a square outside the world. Read-only.
---| "squareOwner"           # index: x 0-999, then y 0-159, integer: the player who owns the square, -1 for none. Read-only.
---| "squareTerrain"         # index: x 0-999, then y 0-159, integer: the square's terrain type (terrainName takes it). Read-only.
---| "squarePortal"          # index: x 0-999, then y 0-159, integer: the square's portal number, 0 for none. Above 0 it is an entrance, and coe5.portalExits lists where it leads; below 0 only an exit. Read-only.
---| "squarePlane"           # index: x 0-999, then y 0-159, integer: the plane the square lies on (planeName takes it), -1 for none, as the game finds it. Read-only.
---| "commanderLastIndex"    # integer: the last commander record in use or once used; a loop runs 0 to it. Read-only.
---| "commanderUnit"         # index 0-9989, integer: the commander's unit (unitOwner, unitHp, unitMonster take it), below 0 for an empty record. Read-only.
---| "commanderRitual"       # index: a commander 0-9989, then 0-63, integer: a ritual the commander knows (ritualName, ritualSummons take it); nil at and past the first slot below 1, the list's end. Read-only.
---| "commanderCostAvailable" # index: a commander 0-9989, then a cost type 0-18 (as ritualCostType), integer: what the owner of the commander's unit has of it where the unit stands, as the game counts it when a ritual is cast: 0-13 the stock, 14 humanoid corpses on the square, 15 gems, 16 the player's expendable units there, 17 all corpses there, 18 the player's goblins there; a cost is paid when this is at least its amount. Nil for a commander with no unit or a unit no player owns. Read-only.
---| "commanderAiTargetX"    # index 0-9989, integer: x of the square an AI commander's move heads for, coe5.aiMove.noSquare for none. Read-only.
---| "commanderAiTargetY"    # index 0-9989, integer: that square's y. Read-only.
---| "commanderAiMoveKind"   # index 0-9989, integer: what the move is: coe5.aiMove.none, .target (from the rally, friendly move, or conquest step), or .explore. Read-only.
---| "commanderRitualPlan"   # index 0-9989, integer: the ritual an AI commander plans to cast (ritualName takes it), as the game's AI keeps it from turn to turn until it casts or scraps it; 0 for none, -1 a spawnbooster (a commander the AI keeps on its square to raise a spawn, which never moves). coe5.scrapRitualPlan drops a plan. Read-only.
---| "commanderDone"         # index 0-9989, boolean: true once the player gave the commander the map's "Commander is done for this turn" (Space; the game also marks the other commanders of its group), which the map's next-commander key then skips and End Turn's "commanders have AP left" warning leaves out; no action points spent. The game clears it at its player's next turn start, for a new commander, and on load (saves don't hold it). Nil on Mac. Read-only.
---| "slotLeader"            # per slot, integer: the player's leader, a commander (commanderUnit takes it), -1 once the leader is dead. Read-only.
---| "unitLastIndex"         # integer: the last unit record; a loop runs 0 to it. Read-only.
---| "unitOwner"             # index 0-98999, integer: the unit's player (slot). Read-only.
---| "unitHp"                # index 0-98999, integer: the unit's hit points; 0 or below is a dead unit or an empty record. Read-only.
---| "unitMonster"           # index 0-98999, integer: the unit's monster (monsterName takes it), below 0 for an empty record. Read-only.
---| "unitCommander"         # index 0-98999, integer: the commander the unit follows (commanderUnit takes it), -1 for none: a commander's own unit too, unless it follows another. commanderStrength adds the commander's unit and every unit following it. The AI's troop pick-up and its pick-up of lone commanders set it. Read-only.
---| "unitStationary"        # index 0-98999, boolean: true when the unit never moves on the world map ("Cannot move on world map", the monster word stationary or an item's or ability's), as the game's AI checks it before it plans a commander's move; false for an empty record. Read-only.
---| "ritualSummons"         # index 0 to ritualLastIndex, boolean: the ritual has the summoning effect: it makes one of its ritualString results, at random. Read-only.
---| "ritualString"          # index: a ritual, then 0-47, string: one of its addstring results, as the .c5m wrote it ("c*Mound King", "1d3*Shade", "2*Cannon Car & 1*Mortar Car", "(167)Anakite"); nil at and past the first empty one. Read-only.
---| "ritualRitpow"          # index 0 to ritualLastIndex, integer: the ritual's school (ritpow). Read-only.
---| "ritualLevel"           # index 0 to ritualLastIndex, integer: its level, the ritual screen's "Level" group. Read-only.
---| "ritualCostType"        # index: a ritual, then 0-3, integer: a cost's resource (resourceName takes 0-13); nil at and past the first type below 0, the list's end. Read-only.
---| "ritualCost"            # index: a ritual, then 0-3, integer: that cost's base amount, before the caster's discounts (RITUAL_SCREEN_ROW's costs are the charged ones); nil where ritualCostType is. Read-only.
---| "ritualTerrain"         # index 0 to ritualLastIndex, integer: the terrain it needs: a terrain type (terrainName takes it), a group below -1, or -1 for none. Read-only.
---| "ritualEffect"          # index: a ritual, then 0-13, integer: an effect id (effectName gives its .c5m word); nil at and past the first id below 1, the list's end. Read-only.
---| "ritualEffectValue"     # index: a ritual, then 0-13, integer: that effect's value; nil where ritualEffect is. Read-only.
---| "ritualFlags"           # index 0 to ritualLastIndex, integer: the ritual's 64 flag bits. Read-only.
---| "ritualTerrainName"     # index 0 to ritualLastIndex, string: the game's own name for ritualTerrain, a group's included ("forest or jungle"). Read-only.
---| "ritualTerrainAt"       # index: a ritual, then x 0-999, then y 0-159, boolean: the square counts for the ritual's terrain, as the ritual screen checks it (true when it needs none); nil for a terrain code the game can't check. Read-only.
---| "ritualEffectMet"       # index: a ritual, then an effect 0-13 (as ritualEffect), then a commander 0-9989, boolean: the effect's rule holds for the commander where it stands, as the game's own checks decide it: nearby1req to nearby99req and nonearby1req to nonearby99req (the terrain within 1, 3, 5, 7 squares or on the plane, or none there); the unit rules minmonreq, monworldreq, monplayerreq, nomonreq, fewmonreq, nomonhomereq, nomonworldreq, nomonplayerreq, nomonelysiumreq, nomonmasteryreq (the ritual's "(&)", "(-)", "(+)", "(|)" strings, counted in order); promotion (an owned unit here, the caster too, named by the first line of one of the ritual's string pairs; a lone "(-)" line is skipped); levelreq, resrestrict, resreq, benrestrict, fxreq, nofxherereq, enchherereq, fortreq, nofortreq, hasportalreq, noportalreq, noplanereq, seasonreq, eventvarreq, noeventvarreq, homereq, homecitreq. An effect repeated after the first of its word is true (the game reads only the first). Nil for any other effect, or a commander with no unit, no owner, or (for the terrain rules) no square on the map. Read-only.
---| "ritualEffectTerrainName" # index: a ritual, then an effect 0-13, string: the game's own name for a nearby or nonearby effect's terrain, a group's included ("forest or jungle"); nil for any other effect. Read-only.
---| "effectName"            # index 0-1501, string: the .c5m word of a ritual effect id (ritualEffect), "maxcast" for 1057; nil for an id no word sets. Read-only.
---| "slotRitualCasts"       # per slot, then a ritual, integer: how many times that player has cast the ritual (what maxcast counts). Read-only.
---| "unitX"                 # index 0-98999, integer: the unit's square's x (ritualTerrainAt takes it). Read-only.
---| "unitY"                 # index 0-98999, integer: the unit's square's y. Read-only.
---| "unitApLeft"            # index 0-98999, integer: the unit's action points left this turn, as the game reckons them (3, 4 for fast, 2 for slow, less those used; 0 for an empty record). The ritual screen casts while it is above 0. Read-only.
---| "unitApUsed"            # index 0-98999, integer: the unit's action points used this turn, the number unitApLeft subtracts; moving and the game's other actions add to it. Set it to 0 to give the unit every action point back.
---| "unitAbility"           # index: a unit, then an ability 0-1501, integer: the unit's total of that ability, its monster's, items', and temporary ones, as the game adds them. A ritual's ritualRitpow is the ability that gives the caster's mastery level in its school. Read-only.
---| "tooltipLineHeight"     # number: the hover box's line height in UI units (14). The box's bottom sits at 16 less 2 a line, so a text past about 8 lines starts below the screen. Read-only.
---| "planeName"             # index 0-11, string: a plane of this world ("Elysium"), by the number a ritual's planereq names (squarePlane gives it); nil past planeLastIndex. Read-only.
---| "planeLastIndex"        # integer: the world's last plane, as the game counts them; planeName runs 0 to it. Read-only.
---| "ticksMs"               # integer: the game's millisecond clock. Only the difference between two reads means anything, and it can jump back when it wraps. Read-only.
---| "traceLevel"            # integer: the game's debug trace level, 0 in a player's game (each -d on the command line adds 1). At 1 the game writes its trace (log.txt in the game folder on Windows, standard output on Mac), at 2 and 3 more of it; above 0 the game also shows a few debug details (a unit of no one named with its number). coe5.keepTraceLines picks the lines. 0 to 3.
---| "bugChecksOff"          # boolean: true skips the game's bug checks, as its --nochecks switch does. At trace level 2 and 3 the game runs them at many places a turn (each over every unit), and they also fix some of what they find. Nil on Mac, where setting it is refused (lua_refused reason=windows_only).
---| "commanderName"         # index 0-9989, string: the commander's name ("Agrippa"); the trace names it "<name> the <monster>". Read-only.
---| "commanderStrength"     # index 0-9989, integer: the strength of the commander's army as the game counts it for the AI's moves (the comstr its AI sites get): the commander's unit and every unit following it. Nil for a commander with no living unit, and on Mac. Read-only.
---| "commanderGarrisonLeft" # index 0-9989, integer: the strength that stays on the commander's square when the commander leaves as the AI moves it: its army, the free troops the AI's gather adds to it there, less the units the gather leaves behind, by the game's own rule, measured as squareStrength measures the owner's units (the fort and ranged raise worked out on the staying units). The gather's one random drop (a follower with unit flag 8 that is a tenth to a quarter of the army, 50%) counts as leaving. Nil for a commander with no living unit on a square, and on Mac. Read-only.
---| "slotHomeX"             # per slot, integer: the player's home citadel's x, -1 for none. Read-only.
---| "slotHomeY"             # per slot, integer: the player's home citadel's y. Read-only.
---| "squareGems"            # index: x 0-999, then y 0-159, then a gem kind 0-3, integer: the square's gem income of that kind; the game lets a player with a gem gatherer own a square with any above 0. Read-only.
---| "squareStrength"        # index: a slot (or -1 for every player), then x 0-999, then y 0-159, integer: the strength of the units on the square of that player and its teammates, now, as the game measures it for the AI: each living unit's strength as that player sees it, the sum raised by (100 + 60 x the square's fort level)% when over a tenth of the units are ranged. Sees through fog. Nil on Mac. Read-only.
---| "squareArmyStationary"  # index: x 0-999, then y 0-159, boolean: true when the game never moves the army on the square on the world map: every living unit there is stationary ("Cannot move on world map"), as the independents' moves check it. Also true for a square with no living unit. Sees through fog. Nil on Mac. Read-only.
---| "squareArmyHolds"       # index: x 0-999, then y 0-159, boolean: true when the independents' moves hold the army on the square, by the game's two hold tests before its stationary one: the square's hold flag (the event command holdit 1, a gold mine's attackers) or the hold flag on its top unit, the unit the move finds for the army's owner. False for a square with no army. Sees through fog. Nil on Mac. Read-only.
---| "squareCitadel"         # index: a slot (or -1 for any class), then x 0-999, then y 0-159, boolean: true when the square is a citadel for that player, as the game counts a player's citadels and its AI's conquest scores them: the terrain's citadel flag, or a citadel only the player's class has (its forest, wooden or mine citadels, a class's citadel terrain or enchantment there). Whoever owns it. Sees through fog. Nil on Mac. Read-only.
---| "squareRememberedStrength" # index: a slot, then x 0-999, then y 0-159, integer: the strength that player's AI remembers on the square, what the game's conquest and threat checks read: every unit's strength there (as squareStrength with -1) when the player's AI turn last saw it, or, once forgotten (5% a turn on a square out of view), the strength there now, which the read doesn't store; 0 off the world. Nil on Mac. Read-only.
---| "unitStrength"          # index: a slot (or -1 for no player), then a unit 0-98999, integer: the unit's strength as that player counts it, the per-unit strength squareStrength and commanderStrength add up: the player's own unit with its armor, items and afflictions (pass unitOwner for the strength its owner sees), any other unit, allies' too, as the bare monster it shows, without armor or items. 0 for a unit that isn't alive. Nil on Mac. Read-only.
---| "unitArmor"             # index 0-98999, integer: the unit's armor as its unit screen shows it: its monster's or items' armor, protection, and the states that add to it. Nil for a unit that isn't alive, and on Mac. Read-only.
---| "unitVisibleTo"         # index: a unit 0-98999, then a slot, boolean: whether that player sees the unit, as the game decides what the map shows: its own unit and any unit without stealth on any square (squareSeen tells the fog); a stealthy one only scried, or in view with a unit of that player next to it with spiritsight (acutesenses against plain stealth). False for a unit that isn't alive. Nil on Mac. Read-only.
---| "monsterAttackType"     # index: a monster 0-5999, then a row 0-15, integer: the kind of the monster's attack in that row, by its .c5m word: 1 meleeweapon, 2 rangedweapon, 3 spellweapon, 4 meleeweaponbonus, 5 rangedweaponbonus, 6 spellweaponbonus, 7 rangedweapon25, 8 meleeweaponspec, 10 prebatweapon, 11 rangedweapon50, 12 assassinweapon, 13 siegeweapon, 14 spellweapon50, 15 spellweaponsingle, 17 meleeweapon50s, 18 rangedweapon50s, 19 spellweapon50s, 21 rangedweapon50x; nil at and past the list's end (the first row whose type is below 1 or weapon below 0, as the game ends it). Nil on Mac. Read-only.
---| "monsterAttackWeapon"   # index: a monster, then a row 0-15, integer: the row's weapon (weaponDmgType takes it), or for a spell attack (types 3, 6, 14, 15, 19) its spell path; nil where monsterAttackType is. Nil on Mac. Read-only.
---| "monsterAttackValue"    # index: a monster, then a row 0-15, integer: the row's number as the .c5m wrote it: the damage value, a special damage weapon's kind (a bitmask: 262144 Confusion), or a spell attack's level; nil where monsterAttackType is. Nil on Mac. Read-only.
---| "weaponDmgType"         # index 0-4999, integer: the weapon's damage type (dmgtype): 1 slash, 2 blunt, 3 pierce, 4 fire, 5 cold, 6 shock, 7 magic, 8 fear, 9 poison, 10 acid, 11 phantasm, 12 special damage (its kind in the attack row's value), 13 special benefit, 14 charm, 15 polymorph, 16 sleep, 17 paralyze, 18 summon, 19 enslave, 20 swallow, 21 mirror image, 22 corrupt, 23 banish, 24 petrify, 25 animate, 26 heal. Nil on Mac. Read-only.
---| "ceremonyName"          # index 0 to ceremonyLastIndex, string: the game's name for a Voice of El ceremony number, its label on the Ceremonies screen: "Convert Settlement", "Break Seal", "Bolster Devotion", "Start Crusade", "Call the Inquisition", "Ceremony of Mastery" (three numbers, one per rank it raises a priest to), "Increase Divine Favor"; nil for a number with none. Match ceremonies by it, never by their numbers. Nil on Mac. Read-only.
---| "ceremonyLastIndex"     # integer: the last ceremony number the game takes; ceremonyName runs 0 to it. Nil on Mac. Read-only.
---| "ceremonyCost"          # index 0 to ceremonyLastIndex, integer: a ceremony's cost as the game worked it out for the commander of the running CEREMONY_COSTS wrap (see there for the ceremonies the game multiplies it for), after that wrap's next (before it, the last costs the game worked out); nil outside the wrap, and for a number with no cost. Writable in it, 0 to 100000: the screen shows, the ceremony charges, and the AI weighs the new one (lua_refused reason=no_cost for a number with none, out_of_range past the bounds). Nil on Mac.
---| "weaponAoe"             # index 0-4999, integer: the weapon's area (aoe): 0 a single target, n a burst of n, -n n strikes, 1000s beams and cones. Nil on Mac. Read-only.
---| "unitMoveFlags"         # index 0-98999, integer: the unit's move flags, the game's sum of its monster's move words and its items' (boots of flying): flying 1, immobile 4, slow 8, fast 16, swamp 32, water 64 (swims or sails), float 128, mountain 256, noland 512, passwall 2^32, stationary 2^35 (also for a unit the game holds in place), teleport 2^39, shipmove 2^40, tunnel 2^42, and the terrain words (forest1 2^23, snow 2^37 and the rest). Test a bit with &: coe5.get("unitMoveFlags", u) & 1 ~= 0 flies. It shows another player's unit with its items, which the game's AI never weighs: for what the AI may know, read monsterFlags or monsterMoveFlags of its unitMonster. Nil for an empty record. Read-only.
---| "unitFlags"             # index 0-98999, integer: the unit's flag words, its monster's and items' added as the game adds them (resrestrict's test): human 2^3, undead 2^4, spiritsight 2^5, stealth 2^17, invisible 2^22, ethereal 2^26, demonic 2^33, foreststealth 2^44, desertstealth 2^41, snowstealth 2^56, and the rest of the monster flag words. Shows another player's items, as unitMoveFlags does. Nil for an empty record. Read-only.
---| "monsterFlags"          # index 0-5999, integer: the monster's own flag words, as unitFlags' bits, as the .c5m set them: what anyone who knows the monster knows. Read-only.
---| "monsterMoveFlags"      # index 0-5999, integer: the monster's own move words, as unitMoveFlags' bits. Nil on Mac. Read-only.
---| "monsterAbility"        # index: a monster 0-5999, then an ability 0-1501, integer: the monster's own value of that ability as the game's lookup reads it (its first match), 0 for none; no items or temporary ones (unitAbility adds those for a living unit). Reads a monster no unit is yet: what a Ritual of Mastery makes (mastery 0x32b, an offset to the next form; montag 0x320), a school's level (ritualRitpow's ability), libmastery 0x32d. Read-only.
---| "commanderMoveFlags"    # index 0-9989, integer: the move flags of the commander's army as the AI's routes test them: its unit's move flags, widened (flying or float adds water 64), with each living unit following it (a bit stays only when every unit has it, noland and slow when any has). Either bit of 192 (water 64, float 128) lets the AI's conquest take a sea square; either bit of 129 (flying, float) lets an army into the Sky. Shows the army's items. Nil for a commander with no living unit, and on Mac. Read-only.
---| "commanderRitpow"       # index 0-9989, integer: the commander's ritual school, the first ritpow ability (501 up, as ritualRitpow names a school) its unit has; 0 for none. The AI ranks casters by its level (commanderRitualLevel): the pick-up of lone commanders, a split's new leader. Nil for a commander with no living unit, and on Mac. Read-only.
---| "commanderRitualLevel"  # index 0-9989, integer: the commander's level in that school (unitAbility of it), 0 for none; the AI's apprentice spam runs for level 1. Nil for a commander with no living unit, and on Mac. Read-only.
---| "commanderSpellLevel"   # index 0-9989, integer: the best level of the commander's spell attacks, with its magic and path boosts, any path, as the game's library and memorized spells reckon it; 0 for none. Shows the boosts of another player's items. Nil for a commander with no living unit. Read-only.
---| "terrainFlags"          # index: a terrain (as terrainName), integer: its 64 flag bits, the ones the AI reads: 1 a citadel, 2 burnable (the AI won't step a burnforest army onto it), 4 water (sea), 2^17 frozen water that thaws (the AI won't step onto it from other terrain in Winter or Spring with seasonTurnsLeft 2 or less), 2^21, 2^23 and 2^26 weigh in conquest's score (2^23 with a mine citadel class), 2^34 and 2^35 a library of level 1 and 2. Nil on Mac. Read-only.
---| "squareLibraryLevel"    # index: x 0-999, then y 0-159, integer: the square's library level as the AI's conquest scores it (100 a level): its terrain's library bits and the square owner's library enchantments there. 0 off the world. Nil on Mac. Read-only.
---| "seasonTurnsLeft"       # integer: the months left in this season, this one included (+seasonleft): 3 in its Early month, 2 in Mid, 1 in Late; in Summer, Autumn and Winter the weather may add one more month. Nil on Mac. Read-only.
---| "aiAttackDivOther"      # integer: the moving AI's attack divisor for an army of an independent or no one, which its turn sets before it recruits: 200 (175 while its team is over 4 times its enemies' strength), 50 less when desperate (fewer than 2 citadels after more than 2), 25 less on its attack turns (10%). Its conquest attacks a square whose remembered strength is at most comstr * 100 / divisor - 15 (aiAttackMargin), and its routes won't end on a stronger army (AI_MOVE_COMSTR). Nil on Mac. Read-only.
---| "aiAttackDivAi"         # integer: the same for an AI's army: 200, 125 while over 4 times, else 150 when 2 or more enemies and the team at most 2 times them. Nil on Mac. Read-only.
---| "aiAttackDivHuman"      # integer: the same for a human's army: 175, 125, or 150 as aiAttackDivAi. Nil on Mac. Read-only.
---| "aiAttackMargin"        # integer: 15, taken off after the division. The game's conquest and routes hold their own 15 in their code and don't read this one: it shows the margin, it doesn't set it. Nil on Mac. Read-only.

---@class coe5.GameMenuLine
---@field label string cut to 63 characters (logged as api_cut)
---@field tooltip? string cut to 255 characters
---@field visible? fun(): boolean Shows the line while it returns true. In Lua, 0 counts as true: return a boolean.
---@field onClick fun(): integer? coe5.gameMenu.stay (or nothing, or false) keeps the menu open, coe5.gameMenu.close (or true) closes it, coe5.gameMenu.resign resigns. To end the game and start a new one, call coe5.restartGame() here instead of resigning.
---@field section? integer|string coe5.gameMenuSection.saving (the default), coe5.gameMenuSection.settings, or a section name, cut to 63 characters

---A line of the map editor's toolbox, for coe5.addMapEditorLine.
---@class coe5.MapEditorLine
---@field label string cut to 63 characters (logged as api_cut)
---@field tooltip? string cut to 255 characters
---@field visible? fun(): boolean Shows the line while it returns true, checked every frame. In Lua, 0 counts as true: return a boolean.
---@field onClick fun() runs when the line, or a key bound to its id, is clicked

---The map view, from coe5.mapView. UI units: x from the left, y up from the bottom.
---@class coe5.MapView
---@field zoom number UI units per square (8 to 100 in the map editor, 12 to 100 on the map)
---@field cameraX number the world x at the view's centre column, in squares
---@field cameraY number the y at the view's centre row, in squares (the centre shows row cameraY + 1)
---@field originX number the UI x of world square x = 0's left edge; square x's is originX + x * zoom
---@field originY number the UI y of row y = 0's top edge; row y's is originY - y * zoom
---@field screenW integer the UI's width
---@field screenH integer the UI's height
---@field plane integer the shown plane (planeName takes it)
---@field planeX integer the plane's first world x
---@field planeY integer the plane's first y
---@field planeW integer the plane's width in squares
---@field planeH integer the plane's height in squares
---@field firstX integer the first world x of the plane on screen
---@field lastX integer the last
---@field firstY integer the first y of the plane on screen
---@field lastY integer the last

---A recruitment offer, from coe5.shopOffers (a player's offers this turn) or coe5.classRecruits (a
---class's list). The fields marked shop or class come only from that one.
---@class coe5.Offer
---@field offer integer shop: the offer's number, for coe5.recruitSquares and coe5.aiBuyOffer
---@field monster? integer the monster (monsterName takes it); class: nil for a name no monster has
---@field name string the monster's name
---@field count integer units it buys; class: -1 for a commander (addcomrec)
---@field commander boolean the unit comes as a commander
---@field costs table<integer, integer> cost by type 0-18 (incomeName's order: 0 gold, 1 iron, ...)
---@field gold integer costs[0]; shop: rolled with the die when the offer came
---@field iron integer costs[1]
---@field goldDie? integer class: the open-ended die added to gold each time the offer comes
---@field chance? integer class: percent a month (before temples, libraries, and the AI level)
---@field flags integer the offer's flag bits
---@field oncePerOffer? boolean shop: bought once, then gone
---@field extra? boolean shop: a buy leaves the square's recruitment for the turn unused (mercenaries, commanders)
---@field aiNever? boolean shop: the game's AI never buys it, and coe5.aiBuyOffer refuses it
---@field terrain integer the terrain it is recruited on (recterr), -1 or 0 for any
---@field limiter? string shop: its "=" limiter ("=Soulless": it upgrades those units)
---@field limiters? string[] class: its reclimiter lines ("-Baron", "+Goblin Chieftain", "=Soulless")
---@field moves integer the monster's action points a turn: 4 fast, 2 slow, else 3
---@field spellLevel integer the highest level of the monster's spell attacks, 0 for none (a caster's measure)

---@class coe5
coe5 = {}

---Writes a lua_log line to coe5mod.log, while the player has CoE5SE's "Enable copious logs" on
---(Mod Settings, CoE5SE's page; off by default, so ask players to turn it on when you need
---their log). print works the same way. CoE5SE's own lines about your script (lua_error,
---lua_refused, lua_script_loaded) are always written. The text stays on that one line: a line break
---or other control character in it is written escaped (\n, \r, \t, \xNN).
---@param text string|number
function coe5.log(text) end

---Adds a main menu button above Quit, or after the game's button `afterId`: coe5.mainMenu.startNewGame,
---.loadGame, .network, .preferences, .mods, or .credits. Buttons several mods put after the same
---game button follow it in the order the mods added them. When the column has no room for every
---mod's button, its last row is More, which lists the rest.
---@param label string cut to 47 characters (logged as api_cut)
---@param onClick fun()
---@param afterId? integer
---@return boolean added false when `label` is empty (logged as api_refused) or memory ran out (lua_refused)
function coe5.addMainMenuButton(label, onClick, afterId) end

---Adds a line to the in-game Game Menu: at the end of its Saving & Exiting section, or with
---`section = coe5.gameMenuSection.settings`, at the end of its Settings section. A name, such as
---`section = "Cheats"`, puts the line in a section of that name after Saving & Exiting. Every mod
---that gives the same name (in any case) shares that section, headed with the first line's
---spelling. Sections come in the order their names were first added, and a section shows only
---while one of its lines does.
---@param line coe5.GameMenuLine
---@return boolean added false when `label` is empty, `section` is an empty name, or `section` is a number that is neither (logged as api_refused)
function coe5.addGameMenuLine(line) end

---Reads a game value. A per-slot value needs its slot (0-31), and a table value its index;
---slotResource needs the slot, then the resource (0-13), and a value with more indexes takes each
---in the order its note gives (squareSeen: the slot, x, then y).
---@param name coe5.Variable
---@param slot? integer slot or index
---@param index? integer slotResource's resource, or a second index
---@param index3? integer a third index (squareSeen's y)
---@return any value an integer, boolean, or string, as the name's note says; nil for a frame value outside its site's wrap, an index past a list's end, or a name the game's table leaves empty
function coe5.get(name, slot, index, index3) end

---Changes a game value. A per-slot value needs its slot (0-31), and a table value its index;
---slotResource needs the slot, then the resource (0-13). A
---read-only value is an error. A boolean value takes only true or false: anything else, a number or
---nil, changes nothing and is refused (lua_refused reason=needs_boolean), because in Lua 0 is true.
---An integer the game itself never holds there (past a table the game indexes by it, or outside what
---the game stores, as the name's note says, or too big for the game's field) changes nothing and is
---refused (reason=out_of_range). A slot's colour takes what the name's note says; bannerFade takes any
---number, and one too big for a float is stored as an infinity.
---@param name coe5.Variable
---@param value integer|string|boolean
---@overload fun(name: coe5.Variable, slot: integer, value: integer|string|boolean)
---@overload fun(name: coe5.Variable, slot: integer, index: integer, value: integer)
function coe5.set(name, value) end

---Runs `fn` around a game call. `fn` gets `next` first, then the call's arguments; call `next` with
---them to go on to the game, and return what the call should return. Among the scripts' wraps, a
---higher `priority` runs first, outside the others, whatever order the mods load in; without one it
---is 0. Among the same priority, the wrap added last runs first. Every DLL's wrap runs outside every
---script's, whatever either's priority: a DLL sees each call before any script and gets back what
---the scripts returned. The log's wrap_order lines list each site's wrappers in the order they run, with
---"plugins:" before the DLLs' and "inner:" before the scripts'. Call `next` only while
---`fn` runs, and not from a coroutine; either is an error. A wrap that returns without calling
---`next` keeps the game's own call from running; the first time it does, the log names it
---(lua_wrap_no_next). An observe-only site (coe5.ObservedSite) is an error here: use coe5.observe.
---@param site coe5.Site
---@param fn fun(next: function, ...): ... the site's result (the AI sites: score, x, y)
---@param priority? integer higher runs first among the scripts' wraps; 0 when left out
---@return boolean wrapped false when the site isn't on this platform yet (logged as api_refused or lua_refused)
function coe5.wrap(site, fn, priority) end

---Watches a game call a script may not change. `before` gets a copy of the call's arguments right
---before it, and `after` the call's result, then the arguments, right after it; either may be nil,
---not both. What they return doesn't reach the game. Every DLL's observers of the site run before
---any script's, in both `before` and `after`. A site that isn't observe-only is an error.
---@param site coe5.ObservedSite
---@param before? fun(...)
---@param after? fun(result: any, ...)
---@return boolean added false when the site isn't on this platform yet (logged as api_refused)
function coe5.observe(site, before, after) end

---The turn: 0 on the first turn, -1 while a game is set up. The same as coe5.get("turn").
---@return integer
function coe5.turn() end

---Adds a message to a player's Messages window. Only human players get messages.
---@param player integer a slot (0-31), or coe5.message.everyHuman
---@param title string cut to 99 characters
---@param text string cut to 2047 characters, the game's buffer for it
---@return integer index the message's index, or coe5.message.notAdded when no message was added. With coe5.message.everyHuman it is always coe5.message.notAdded, even when the message was sent. A bad slot or a network game is logged (api_refused or lua_refused).
function coe5.sendMessage(player, title, text) end

---Reads one of your mod's settings. Every mod declares its settings with newsetting blocks in
---<ModName>.c5se next to its .c5m, and nowhere else (the format is in src/jazzy_api.h, "Mod
---Settings starter"); a script only reads them. Names match ignoring case. The value is the
---player's current pick in Mod Settings, read each time you call it; a declared setting that breaks
---a rule gives its default. Scripts don't run in network games. A setting you read only once, when the script
---loads, takes effect after a restart: give it a restart line, and changing it offers one.
---@param name string the setting's name, as its newsetting line gives it
---@return boolean|integer|nil value on/off: true or false; a choice: the picked option's number from 0; a key: its key value (coe5.key.none for none); nil when your mod declares no such setting (logged as lua_refused)
function coe5.setting(name) end

---Tells other mods that your script is filling in a new game's setup itself: while it is on, mods
---that change the setup screens leave them alone. Turn it on right
---before your code starts a new game's setup, and off once that setup returns. Every mod shares
---the one flag, so always turn it off again. A call that changes it logs setup_automated with your
---mod's name (by=<ModName>.lua).
---@param on boolean
function coe5.setSetupAutomated(on) end

---Whether a mod has said it is filling in a new game's setup itself (coe5.setSetupAutomated). A mod
---that changes the setup screens leaves them alone while it is true.
---@return boolean
function coe5.setupAutomated() end

---Hides the game's main menu button `id` on every frame where `hidden()` returns true. Only Read Tip,
---coe5.mainMenu.readTip, can be hidden; the buttons below it move up. Any number of mods may ask: the
---button is hidden while any of them says so.
---@param id integer coe5.mainMenu.readTip
---@param hidden fun(): boolean runs every main menu frame; keep it short
---@return boolean added false for any other button (logged as lua_refused)
function coe5.hideMainMenuButton(id, hidden) end

---Adds a line to your mod's page in Mod Settings, in a box titled "Screens" after your mod's
---settings. A click runs `open`, your own screen (a coe5.ui loop), then shows the page again. The
---tooltip comes last because it is optional; the C API's addScreen takes it second.
---@param label string 1 to 56 characters
---@param open fun()
---@param tooltip? string up to 255 characters
---@return boolean added false when the label or tooltip breaks a rule (the page lists it; logged as api_refused)
function coe5.addScreen(label, open, tooltip) end

---Sets key action `action`'s key (coe5.keyAction) to `key` (coe5.key, or a character's byte) on its
---screen, in the game's texts, and in the Keyboard Shortcuts list, in place of the game's key.
---coe5.key.none leaves the action with no key. The key lasts until the game closes or the next call;
---nothing is saved, so keep players' picks in [key] settings. Every mod shares one key per action and
---the last call wins: each change logs key_rebound with mod=, and key_rebound_over naming both mods
---when another mod set that key last.
---@param action integer
---@param key integer
---@return boolean changed false for an unknown action or a value that isn't a key (logged as api_refused)
function coe5.rebindKey(action, key) end

---Gives key action `action` a second key beside its own, which runs it too; the texts name both,
---"Esc/F10". coe5.key.none removes it.
---@param action integer
---@param key integer
---@return boolean changed false for an unknown action or a value that isn't a key (logged as api_refused)
function coe5.addActionKey(action, key) end

---Key action `action`'s key now.
---@param action integer
---@return integer|nil key nil for an unknown action (logged as api_refused), where the C API returns -1
function coe5.actionKey(action) end

---The index of the first monster in the game's table with this name, ignoring case: a class
---leader's with coe5.get("classLeader", class). Its picture is coe5.get("monsterSprite", index).
---@param name string
---@return integer|nil index nil when no monster has this name
function coe5.findMonster(name) end

---Makes one unit of `monster` for `player` on square (x, y), following no commander, the way the
---game's recruit purchase makes a troop: the game's newunit, then what the game does after it (the
---unit's land or water shape for the square, the square's army recount and conquest check, the
---player's sight around it). The game checks no terrain, owner, or sight here, and neither does
---CoE5SE: your script picks the square. A unique monster that already has a living unit moves that
---unit here, as the game's own spawns do. For the variant the recruit screen would make, pass
---coe5.monsterVariant(monster).
---@param player integer a slot, 0-31
---@param monster integer a monster's index (coe5.findMonster)
---@param x integer below mapDepth
---@param y integer below mapH
---@return integer|nil unit the unit's index, or nil: outside a game (reason=no_game), a player past the slots (bad_player), a monster index no monster uses (bad_monster), a square outside the world (off_map), or no free unit record (no_free_slot), each logged as lua_refused
function coe5.spawnUnit(player, monster, x, y) end

---Makes one commander of `monster` for `player` on square (x, y), with its unit and a name the game
---gives it, the way the recruit purchase makes a commander, then does what coe5.spawnUnit does after.
---@param player integer a slot, 0-31
---@param monster integer a monster's index
---@param x integer below mapDepth
---@param y integer below mapH
---@return integer|nil commander the commander's index, or nil with the reasons coe5.spawnUnit gives (no_free_slot also when no commander record is free)
function coe5.spawnCommander(player, monster, x, y) end

---The monster the recruit purchase makes for an offer of `monster`: the game's own pick, `monster` or
---one of the up to 9 records right after it with the same name, at random (a class's "Swordsman"
---with three looks).
---@param monster integer a monster's index
---@return integer|nil variant nil for a monster index no monster uses (logged as lua_refused, reason=bad_monster)
function coe5.monsterVariant(monster) end

---Where the portal on square (x, y) leads: the squares the game sends an army through it to. An
---entrance is a square whose squarePortal is above 0, and its exits are the squares with that
---portal number or its negative, the entrance left out, as the game finds them: the first 100 by
---x, then y. The game picks one of them at random each time, so the list is where it can lead.
---@param x integer
---@param y integer
---@return {x: integer, y: integer}[] exits empty for a square with no entrance or outside the world
function coe5.portalExits(x, y) end

---The living units on square (x, y), the units the game counts there (squareStrength, its square
---checks): hp above 0 and standing on the square, in unit order. A square's army is what stands on
---it; unitCommander tells which follow which commander. Sees through fog, as squareStrength does.
---@param x integer
---@param y integer
---@return integer[] units unit indexes (unitHp, unitMonster, unitStrength take them); empty for no living unit there or a square outside the arrays
function coe5.squareUnits(x, y) end

---Every army on the world map: each square of the world (x below mapDepth, y below mapH) with an
---army (squareArmy not 0), by x, then y. With viewer, only the squares that player sees now
---(squareSeen 2 or 3), which the game's AI refreshes its strength memory from; without it, every
---army, through fog. A viewer outside 0-31 is refused (lua_refused reason=bad_player) and the list
---is empty.
---@param viewer? integer a player slot 0-31
---@return {x: integer, y: integer, owner: integer, units: integer, commanders: integer[]}[] armies owner as squareArmyOwner, units as squareArmy (-1 while two players at war share the square), commanders the commanders whose unit stands there alive, in commander order, followers of another commander included (unitCommander tells)
function coe5.armies(viewer) end

---How far a commander's army has to go to reach square (x, y), by the game's own pathfinder, the
---route the AI's moves take: from the commander's square, through portals too, with the whole map
---known (like the AI's routes, it ignores what the player has seen). comstr is the strength the
---route's end may face: a square held by an army stronger than it allows is closed. Left out, it is
---the commander's own, the comstr the AI sites get. Only inside a wrap of an AI move site
---(AI_EXPLORE); anywhere else the call is refused (lua_refused reason=outside_ai_site) and returns
---nil. A route draws nothing from the game's random sequence that the game's later draws see.
---Each call, refused or not, counts 10,000 steps of the script's 20,000,000-step budget before the
---game routes, so about 2,000 routes fit in one call into the script (reason=step_limit past it).
---@param commander integer 0-9989
---@param x integer below mapDepth
---@param y integer below mapH
---@param comstr? integer -1 to 21474836; -1 routes as a player's own move order does
---@return integer|nil length the route's cost in moves, about 1 a square on open ground, 0 on the commander's square; nil for no route, a commander with no living unit on a square, or a refused call (reason=outside_ai_site, bad_commander, off_map, out_of_range)
function coe5.route(commander, x, y, comstr) end

---Drops an AI commander's planned ritual (commanderRitualPlan) the way the game's own "scrapping
---plan" does: the plan becomes 0, and the game's ritual step plans again when it next runs for the
---commander (in 80% of its runs), with each candidate passing AI_RITUAL_CANDIDATE. Only 0 is
---written: a script can't pick the plan. Only inside an AI site: a wrap of an AI move site, of
---AI_RITUAL_CANDIDATE, or of an army site (AI_GATHER_TROOPS, AI_PICK_UP_LONER, AI_SPLIT_ARMY), or
---an observer of AI_TURN (before the player's moves, so its commanders plan again that turn). Refused (lua_refused) and false: outside them (reason=outside_ai_site), a
---commander past the table (bad_commander), one with no living unit (no_unit), one whose player
---isn't an AI (not_ai), and a spawnbooster's plan, -1 (not_ritual). Each scrap logs
---ai_plan_scrapped with the commander, its player, and the ritual. Works on both builds where an AI
---site runs (Mac: AI_TURN).
---@param commander integer 0-9989
---@return boolean ok true when the commander has no plan now (scrapped, or it had none)
function coe5.scrapRitualPlan(commander) end

---Frees one unit of an AI player from the commander it follows (unitCommander becomes -1), as the
---game's own unfollowtarg ritual effect does, and resets that commander's cached action points, since
---its army changed. The unit stays on its square, a free unit of its player, which the AI's troop
---pick-up can take again. Only inside an AI site, as coe5.scrapRitualPlan, and only for a unit of the
---player whose AI turn runs (coe5.get("activePlayer")). Refused (lua_refused) and false: outside an AI
---site (reason=outside_ai_site), in a network game (network_game), a unit past the table (bad_unit),
---one dead or of no player (no_unit), one whose player isn't an AI (not_ai), one of another player
---(not_moving_player), and on Mac (windows_only). Each release logs ai_follower_released with the
---unit, its old commander, and its player.
---@param unit integer 0-98999
---@return boolean ok true when the unit follows no one now (released, or it followed no one)
function coe5.releaseFollower(unit) end

---Moves one unit of an AI player from the commander it follows to another commander of the same
---player on the same square, the game's way: the army screen's own transfer, with its rules (no
---stationary unit, none that moves on its own, none led by a wandering or insane commander) and both
---commanders' cached action points reset. Only inside an AI site, for a unit of the player whose AI
---turn runs, as coe5.releaseFollower. Refused (lua_refused) and false: the reasons of
---coe5.releaseFollower, then a commander past the table (bad_commander), one with no living unit
---(no_commander), one of another player (other_player), the commander's own unit (same_unit), a unit
---that follows no one (not_following), one that follows the commander already (already_following),
---the two on different squares (not_same_square), and one the game's transfer refuses
---(game_refused); windows_only on Mac.
---Each transfer logs ai_follower_transferred.
---@param unit integer 0-98999
---@param commander integer 0-9989
---@return boolean ok true when the unit follows the commander now
function coe5.transferFollower(unit, commander) end

---A player's recruitment offers this turn, those with units to buy, in the shop's order: what
---Recruit lists, the other citadels' terrain-bound offers included. A magic item for sale and an
---offer already bought (once per offer) are left out. Refused (lua_refused reason=bad_player, an
---empty list) for a player outside 0-31.
---@param player integer 0-31
---@return coe5.Offer[] offers
function coe5.shopOffers(player) end

---A class's recruitment list (addunitrec, addmercrec, addcomrec, in its order): every offer the
---class can get, before the month's chance rolls. The standard troops of stdtroops aren't in it
---(classStandardTroops). Refused (lua_refused reason=bad_class, an empty list) for a class outside
---0-199.
---@param class integer slotClass's number
---@return coe5.Offer[] offers
function coe5.classRecruits(class) end

---The squares where the game's AI recruiting can buy for the player: its own citadels (squares
---whose terrain is a citadel for its class), by y then x, at most 100, each {x, y, used}, used true
---when the square's recruitment is used this turn. With an offer (coe5.Offer.offer), only the
---squares where coe5.aiBuyOffer would buy it now. Refused (lua_refused, an empty list):
---reason=bad_player, bad_offer (past the player's last offer), windows_only on Mac.
---@param player integer 0-31
---@param offer? integer
---@return {x: integer, y: integer, used: boolean}[] squares
function coe5.recruitSquares(player, offer) end

---Buys one of an AI player's offers on square (x, y) the way the game's own recruiting step does:
---the same purchase (the units, or a commander, made there and every cost paid), then an offer that
---isn't extra uses the square's recruitment for the turn and a once-per-offer offer is gone. Only
---inside a wrap of AI_RECRUIT, for the player recruiting. The game's own square rules hold: the
---offer must be buyable there (its terrain, costs, limiter), the square one of the player's
---recruit squares, unused this turn, and the monster's .c5m AI words met (aipowcom1-3, aimaxshop,
---aihold, a ship by the sea). Refused (lua_refused) and false: reason=outside_ai_recruit,
---bad_player, other_player, not_ai, bad_offer, off_map, not_recruit_square, item, sold_out,
---not_for_ai, cannot_pay, too_few_to_upgrade, not_here, square_used, aipowcom, aimaxshop, no_sea,
---aihold; windows_only on Mac; the second result is that reason word. False alone: a commander
---offer the game's buy couldn't make (no free commander, or no unit to upgrade), with the square
---and the offer used all the same, as the game's own step leaves them. Each buy logs
---ai_offer_bought with the gold before and after.
---@param player integer 0-31, the player of the AI_RECRUIT call
---@param offer integer coe5.Offer.offer
---@param x integer
---@param y integer
---@return boolean bought
---@return string? reason the lua_refused reason when refused
function coe5.aiBuyOffer(player, offer, x, y) end

---Adds a line to the map editor's toolbox, right under "Create Fancy Terrain". The editor's Tab
---(Hide Toolbox) hides it with the rest. CoE5SE gives the line an id the editor itself never uses
---(44 to 999, in the order lines are added); a click on the line runs onClick, and so does a key
---bound to the id with coe5.ui.bindKey in a MAP_EDITOR_WAIT wrap, toolbox hidden or not.
---@param line coe5.MapEditorLine
---@return integer|false id the line's id, or false when `label` is empty, all 956 ids are taken, or the site isn't on this build (logged as api_refused)
function coe5.addMapEditorLine(line) end

---The map view now (the map screen's or the map editor's), in UI units: x from the left, y up from
---the bottom, screenW x screenH. World square (x, y) is `zoom` units wide, its top left corner at
---(originX + x * zoom, originY - y * zoom). firstX to lastX and firstY to lastY are the squares of
---the shown plane at least partly on the screen (first is above last when none is). A map file
---numbers a square x - planeX, y - planeY.
---@return coe5.MapView|nil view nil when the shown plane isn't one of the game's
function coe5.mapView() end

---Gives an AI commander with no plan (commanderRitualPlan 0) the plan of casting one of the rituals
---it knows (commanderRitual), as the game's own planner writes a candidate it keeps. The game's
---ritual step then treats it as any plan it holds: casts it when the caster can, keeps it while it
---can't cast it right now or can't pay, and scraps it as the game scraps a plan ("cannot cast",
---"pointless", "aibad"). The game's own reasons for drawing a candidate (airare, aialways, the
---income checks) don't apply, as they don't to a plan held from an earlier turn. Set from an
---AI_TURN observer, the commander's ritual step sees it that same turn. Only inside an AI site, as
---coe5.scrapRitualPlan. Refused (lua_refused) and false: outside them (reason=outside_ai_site), a
---commander past the table (bad_commander), one with no living unit (no_unit), one whose player isn't
---an AI (not_ai), a commander with a plan already, a spawnbooster's -1 included (has_plan: scrap it
---first), and a ritual the commander doesn't know (not_known). Each plan logs ai_plan_set with the
---commander, its player, and the ritual.
---@param commander integer 0-9989
---@param ritual integer a ritual the commander knows
---@return boolean ok true when the plan is set
function coe5.planRitual(commander, ritual) end

---Whether square (x, y) counts for a terrain code, as the game checks a ritual's terrain
---(ritualTerrainAt): -1 any terrain, a terrain type (terrainName takes it) by its name, so another
---type of the same name counts, and a group (-2 to -1500, as ritualTerrain and the newlocplanes,
---randnewloc and nearby words give them) by the game's own test.
---@param code integer
---@param x integer
---@param y integer
---@return boolean|nil counts false for a square off the world; nil for a code the game can't check (below -1500, or 2000 and up)
function coe5.terrainMatch(code, x, y) end

---Keeps only some of the game's debug trace lines (log.txt in the game folder, at "traceLevel" 1 to
---3): from now on the game writes a line only when its format text, after its leading spaces,
---starts with one of the words, and drops the rest before it formats them. The words match the
---game's format text, not the finished line: "bt%d:" keeps the battle lines that read "bt3: ...".
---nil or an empty list keeps every line again. CoE5SE keeps one list for every mod: the last call
---wins. The list does nothing in a network game. Windows only.
---@param words? string[] each a word a kept line starts with ("ailog", "removing player")
---@return boolean set false when refused (lua_refused reason=not_a_list, not_a_string, empty_word, leading_space, bad_character, out_of_memory; windows_only on Mac; site_unavailable when the game's trace functions aren't the code CoE5SE knows, logged as trace_filter_installed)
function coe5.keepTraceLines(words) end

---Names a subfolder of your mod's area for your data files. The area is coe5se_data\<your mod's
---name> in the game's user folder (%APPDATA%\coe5 on Windows, ~/.coe5 on Mac); without this call
---your files go in the area itself. The subfolder is always inside the area: no mod reaches
---another's. The area holds 10 MB in all, its subfolders included, or up to 100 MB when your
---mod's .c5se has a maxdata line (maxdata 50).
---@param name string up to 64 characters, no / \ : * ? " < > |, not starting or ending with a space or a dot
---@return boolean named false for a name that isn't one plain folder name (logged as lua_refused)
function coe5.dataFolder(name) end

---Reads data file `name`.ini from your data folder, as t[section][key] = value. Values are strings:
---use tonumber for numbers. Keys before the first [section] line are in t[""]. A line it can't read
---(a [ with no ], a line with no =) is skipped, and the rest still reads (logged as bad_lines).
---@param name string the file's name without .ini: the same rules as coe5.dataFolder's
---@return table<string, table<string, string>>|nil t nil when there is no such file, the file is over 1 MB, or the name breaks a rule (logged as lua_refused)
function coe5.readIni(name) end

---Writes t[section][key] = value as data file `name`.ini in your data folder, in place of the old
---file, sections and keys in sorted order. Values are strings or numbers. A name or value that
---wouldn't read back the same is refused: a line break, ] in a section name, = in a key, a key
---starting with [ ; or #, or a space at either end.
---@param name string the file's name without .ini
---@param t table<string, table<string, string|number>>
---@return boolean written false when something is refused, the file would pass 1 MB, your mod's area would pass 10 MB or its .c5se's maxdata (data_quota), or it can't be written (logged as lua_refused)
function coe5.writeIni(name, t) end

---The names of the data files in your data folder, without .ini, sorted.
---@return string[]
function coe5.listIni() end

---Deletes data file `name`.ini from your data folder.
---@param name string
---@return boolean deleted false when there was no such file, or it can't be deleted (logged as lua_refused)
function coe5.deleteIni(name) end

---Call it in a Game Menu line's onClick, in a single player game. When onClick returns, CoE5SE
---ends the game the way Resign does, and after the Game Over screen it starts a new game's setup
---instead of going back to the main menu. Your wraps can fill that setup in while
---coe5.restartPhase() is "starting".
---@return boolean asked false outside a Game Menu line's onClick, in a network game, or unless exactly one human player is in the game (logged as api_refused)
function coe5.restartGame() end

---Call it in a Game Menu line's onClick, in a single player game. It shows the game's Load Game
---list, with every mod's wraps of LOAD_GAME_BROWSER, while the game goes on underneath. Cancel
---returns false and the game goes on. When a save is picked, it returns true, and when onClick
---returns, CoE5SE ends the game the way Resign does and then loads the save, as the main menu's
---Load Game does. coe5.restartPhase() is "ending" until then. CoE5SE clicks Ok and Exit on the
---screens the end shows, and Ok on the loaded turn's first Messages screen (View messages, key m,
---still shows them).
---@return boolean picked false on Cancel (logged as load_game phase=cancelled), outside a Game Menu line's onClick, in a network game, unless exactly one human player is in the game, or while a Load Game list from coe5.loadGame is already up, as from a wrap of its sites (logged as api_refused, reason=already_picking)
function coe5.loadGame() end

---Where a restart is: "ending" from the onClick that called coe5.restartGame or picked a save with
---coe5.loadGame until the old game's MAIN_MENU_PLAY_GAME returns, "starting" during the new game
---CoE5SE started (its setup and play), and nil otherwise. A game started from the main menu, or
---loaded with coe5.loadGame, is always nil. A MAIN_MENU_PLAY_GAME wrap that calls next() again
---cancels the restart or load: that second game is nil (logged as restart_game phase=cancelled
---reason=play_game_reentered).
---@return "ending"|"starting"|nil phase
function coe5.restartPhase() end

---The game's own screen widgets. A screen is a loop. Each pass calls screenBegin, draws sections,
---then buttonRow and its lines, and calls wait, which returns the clicked id.
---
---    while true do
---        coe5.ui.screenBegin("My Mod")
---        coe5.ui.sectionBegin("Options")
---        coe5.ui.text(10, "Say hello")
---        coe5.ui.sectionEnd()
---        coe5.ui.buttonRow()
---        coe5.ui.text(9, "Back")
---        coe5.ui.bindEscape(9)
---        local id = coe5.ui.wait()
---        if id == 9 then return end
---        if id == 10 then coe5.log("hello") end
---    end
---@class coe5.ui
coe5.ui = {}

---Clears the screen and draws the title. Each text the coe5.ui calls draw is cut to the game's
---buffer for it, the most the game draws (logged as api_cut): 99 characters for a title, 4999 for
---a line, list row, or tooltip, 999 for a checkbox, 998 for a choice.
---@param title string
function coe5.ui.screenBegin(title) end

---Starts a framed section.
---@param header? string
function coe5.ui.sectionBegin(header) end

---Ends the section.
function coe5.ui.sectionEnd() end

---Draws one line. Id 0 makes it plain text; any other id makes it clickable.
---@param id integer
---@param text string
function coe5.ui.text(id, text) end

---Draws one checkbox line. A click on it makes wait return `id`; your script then flips its own
---value, as with choice.
---@param id integer
---@param checked boolean
---@param label string
function coe5.ui.checkbox(id, checked, label) end

---Draws one radio line. Your script tracks which one is selected.
---@param id integer
---@param selected boolean pass a boolean such as `id == picked`: in Lua, 0 counts as true
---@param label string
function coe5.ui.choice(id, selected, label) end

---Hover text for the line drawn just before.
---@param text string
function coe5.ui.tooltip(text) end

---Starts the bottom row of buttons (Ok, Back, ...). Always last.
function coe5.ui.buttonRow() end

---Starts a list, Load Game's layout: call it after screenBegin, then listRow for each item, then
---listEnd. Use it to pick one of several things, each opening its own screen.
function coe5.ui.listBegin() end

---Draws one striped row of the list, 450 wide. A click on it makes wait return `id`.
---@param id integer
---@param text string
function coe5.ui.listRow(id, text) end

---Ends the list, with the gap before the button row.
function coe5.ui.listEnd() end

---Makes the Esc key click line `id`.
---@param id integer
function coe5.ui.bindEscape(id) end

---Makes a key click line `id`: coe5.key.enter, coe5.key.esc (as bindEscape), or a lowercase letter's
---byte, string.byte("s"). Key 0 binds nothing. An id or key that doesn't fit the game's 32-bit int
---binds nothing either (logged as lua_refused, reason=out_of_range), as for bindEscape's id.
---@param id integer
---@param key integer
function coe5.ui.bindKey(id, key) end

---Ends the frame: shows the screen and returns the clicked line's id.
---@return integer id the clicked line's id, 0 when nothing was clicked this frame
function coe5.ui.wait() end

---Draws a section's header: sectionBegin(header) is sectionBegin() then this.
---@param text string
function coe5.ui.sectionHeader(text) end

---Sets the screen's width. screenBegin uses Random Map Settings' width, coe5.ui.setupScreenWidth().
---NaN, an infinity, or a negative width changes nothing (lua_refused reason=not_finite or negative).
---@param width number
function coe5.ui.screenWidth(width) end

---The width Random Map Settings uses.
---@return number
function coe5.ui.setupScreenWidth() end

---Starts the menu screens' music, as the game does once before a screen's loop. Does nothing on Mac.
function coe5.ui.screenMusic() end

---Moves where the next line of the row starts, `x` from the row's left edge: a column of your own
---in a row the game draws, such as the file browser's (FILE_BROWSER_ROW_DATE). NaN or an infinity
---changes nothing (lua_refused reason=not_finite).
---@param x number
function coe5.ui.columnX(x) end

---The game's "Enter name" box, with a monster's picture. It runs its own loop until Ok or Cancel.
---Enter clicks Ok, and Esc clicks Cancel. CoE5SE draws the box itself, as the game draws it, because
---the game's own box binds no key to Esc and, on Mac, always says "Enter name", takes 25 characters,
---and has no Cancel.
---@param title string cut to 99 characters
---@param prompt string cut to 499 characters
---@param sprite integer a monster's picture: coe5.get("monsterSprite", monster)
---@param maxLength? integer 1 to 42; 32 when left out. The game's field takes two characters fewer (32 takes 30)
---@return string|nil text the typed text, or nil on Cancel, or at once for a sprite no monster has (lua_refused reason=bad_sprite)
function coe5.ui.nameBox(title, prompt, sprite, maxLength) end

---Draws one striped row: the monster's picture, then `text`, all clicked as `id`. With `text` only it
---is the row of the game's own monster list ("Select Monster"), the picture as high as the line.
---With `line2` it is two lines under one stripe as wide as the screen's sections, the second one
---subtext (the recruit screen's smaller text, and fainter), and the picture is as high as both
---lines, beside them; a click anywhere on the row is `id`. A monster index no monster uses draws nothing (lua_refused
---reason=bad_monster), and so does an id past the game's 32-bit int (out_of_range).
---@param id integer
---@param monster integer a monster's index (coe5.findMonster)
---@param text string
---@param line2? string a second line under it
function coe5.ui.monsterRow(id, monster, text, line2) end

---Draws one line from (x1, y1) to (x2, y2) at UI coordinates, in r, g, b (each 0 to 1) at `alpha`
---(0 clear, 1 solid), `width` screen pixels wide. Only inside a MAP_EDITOR_DRAW wrap; anywhere
---else it draws nothing (lua_refused reason=not_drawing). NaN or an infinity draws nothing
---(not_finite), and so does a width of 0 or less (out_of_range).
---@param x1 number
---@param y1 number
---@param x2 number
---@param y2 number
---@param r number
---@param g number
---@param b number
---@param alpha number
---@param width? number 1 when left out
function coe5.ui.drawLine(x1, y1, x2, y2, r, g, b, alpha, width) end

---Draws one line of text in the game's GUI font with its bottom left at (x, y), UI coordinates,
---`size` UI units high, in r, g, b at `alpha`. Only inside a MAP_EDITOR_DRAW wrap, as drawLine.
---The text is cut to 499 bytes (logged as api_cut).
---@param x number
---@param y number
---@param size number
---@param text string
---@param r number
---@param g number
---@param b number
---@param alpha number
function coe5.ui.drawText(x, y, size, text, r, g, b, alpha) end

-- ---- Game numbers ------------------------------------------------------------------------------
-- The game's own numbers that a script passes to CoE5SE or gets back, by name. The values are the
-- game's, and the same as the C names in jazzy_api.h (given with each table).

---The game's main menu buttons, for coe5.addMainMenuButton's afterId (C: MAIN_MENU_*). The ids
---the table skips (5, 7, 8) have no button.
---@class coe5.mainMenu
coe5.mainMenu = {
  startNewGame = 1,
  loadGame = 2,
  preferences = 3,
  network = 4,
  credits = 6,
  quit = 9,
  mods = 10,
  readTip = 11,
}

---What a Game Menu line's onClick returns (C: GAME_MENU_*).
---@class coe5.gameMenu
coe5.gameMenu = {
  stay = 0,    -- the menu stays open
  close = 1,   -- the menu closes, as Ok does
  resign = 6,  -- the player resigns, with no question asked
}

---Which section of the Game Menu a line goes in, addGameMenuLine's `section` (C: GAME_MENU_SECTION_*).
---A name instead makes a section of the mod's own.
---@class coe5.gameMenuSection
coe5.gameMenuSection = {
  saving = 0,    -- the end of Saving & Exiting (the default)
  settings = 1,  -- the end of Settings, after the game's own lines
}

---The map editor's ids, as MAP_EDITOR_WAIT's next returns them (C: MAP_EDITOR_*).
---@class coe5.mapEditor
coe5.mapEditor = {
  hideToolbox = 2,  -- "Hide Toolbox" and Tab: the editor shows or hides its toolbox
}

---Map screen actions (C: MAP_ACTION_*).
---@class coe5.mapAction
coe5.mapAction = {
  specialPower = 5,  -- Use Special Power: a caster's ritual screen
  gameMenu = 6,      -- the Game Menu (F10)
  squareInfo = 96,   -- the square info window for the square under the pointer
}

---Key values: the game's key codes, the same on Windows and Mac (C: KEY_*). A letter, digit, or
---punctuation key is its character's byte, string.byte("y"); these are the other keys. Add ctrl,
---alt, or ctrlShift (with a lowercase letter) for a combination: coe5.key.ctrl + string.byte("i")
---is ctrl+i. The map screen already binds F1 to F6 and F9 to F12; F7 and F8 are free.
---@class coe5.key
coe5.key = {
  none = 0,         -- no key
  backspace = 8,
  tab = 9,
  enter = 13,
  esc = 27,
  space = 32,
  delete = 127,
  f1 = 328,
  f2 = 329,
  f3 = 330,
  f4 = 331,
  f5 = 332,
  f6 = 333,
  f7 = 334,
  f8 = 335,
  f9 = 336,
  f10 = 337,
  f11 = 338,
  f12 = 339,
  pageDown = 340,
  pageUp = 341,
  endKey = 342,     -- End
  home = 343,
  up = 344,
  down = 345,
  left = 346,
  right = 347,
  ctrl = 100000,    -- added to a key: ctrl+key
  alt = 200000,     -- added to a key: alt+key
  ctrlShift = 300000,  -- added to a lowercase letter: ctrl+shift+letter
}

---coe5.sendMessage's player and result (C: MESSAGE_*).
---@class coe5.message
coe5.message = {
  everyHuman = -2,  -- every human player, as a .c5m event's `message -2`
  notAdded = -1,    -- the result when no message was added
}

---A player slot's controller, the slotController value (C: CONTROLLER_*).
---@class coe5.controller
coe5.controller = {
  removed = -1,     -- an empty slot
  human = 0,
  ai = 1,
  independent = 2,  -- the independents, slots 24-30, once the world is built
}

---A class ability id, classAbility's second index (C: CLASS_ABILITY_ID_*). The game's random class
---roll reads the first four.
---@class coe5.classAbility
coe5.classAbility = {
  aiClass = 203,          -- aiclass: -1 an AI's random roll never gets the class, 1 only an AI's does
  randomSkip = 204,       -- a percent: the random roll skips the class that often
  aiLevelMin = 210,       -- an AI below this difficulty level never rolls the class; 0 no bound
  aiLevelMax = 211,       -- an AI above this difficulty level never rolls the class; 0 no bound
  standardTroops = 222,   -- stdtroops: 1 when the class also recruits the standard troops
}

---GAME_OVER_WAIT's mode (C: GAME_OVER_MODE_*).
---@class coe5.gameOverMode
coe5.gameOverMode = {
  plain = 4,  -- nothing drawn behind the screen: plain black
  fog = 9,    -- what the game passes: fog and falling particles
}

---A new game's setup: PLAY_GAME_NEW_WORLD's mode, and the setup screens' results and ids (C:
---NEW_WORLD_*, MAP_SCREEN_OK, MAP_SETTINGS_*, PARTICIPANTS_OK).
---@class coe5.newGame
coe5.newGame = {
  normal = 0,
  network = 1,
  skipParticipants = 2,  -- any mode but 0 and 1 skips Setup Participants
  mapScreenOk = 1,       -- what NEW_WORLD_MAP_SCREEN returns on Ok (0 is Cancel)
  mapSettingsOk = 9,     -- Random Map Settings' Ok, as MAP_SETTINGS_WAIT returns it
  mapSizeFirst = 100,    -- its first map size button (Small); size n is 100 + n, to 104
  participantsOk = 1,    -- Setup Participants' Ok
}

---The ritual screen: why a ritual can't be cast, and its rows' ids (C: RITUAL_REASON_*, RITUAL_ROW_*),
---for RITUAL_SCREEN_ROW, RITUAL_SCREEN_CAN_CAST, and RITUAL_SCREEN_LOCATION_CHECK.
---@class coe5.ritual
coe5.ritual = {
  reasonMirror = -2,    -- needs a mirror monster
  reasonMaxcast = -3,   -- cast as often as its maxcast allows
  reasonImmortal = -4,
  reasonMastery = -6,
  reasonPlane = -7,
  rowCastable = 1000,   -- a castable ritual's row id is its slot + 1000
  rowGreyed = 12288,    -- a greyed-out one's is its slot + 12288 (0x3000)
}

---The game's file browser (Load Game's list) and the square info window (C: FILE_BROWSER_*,
---SQUARE_INFO_OK), for FILE_BROWSER_ROW_DATE, FILE_BROWSER_WAIT, and SQUARE_INFO_WAIT.
---@class coe5.screen
coe5.screen = {
  fileBrowserRowFirst = 10000,  -- row n's id is 10000 + n
  fileBrowserDelete = 7,        -- what the browser's Delete key returns
  squareInfoOk = 1,             -- the square info window's Ok
}

---An AI commander's move (C: AI_MOVE_*, AI_NO_SQUARE, AI_EXPLORE_BELOW, AI_FAR_EXPLORE_BELOW), for
---the AI sites (AI_RALLY, AI_FRIENDLY_MOVE, AI_CONQUER, AI_EXPLORE, AI_FAR_EXPLORE) and
---commanderAiMoveKind.
---@class coe5.aiMove
coe5.aiMove = {
  none = 0,              -- no move planned: the commander rests
  target = 1,            -- a square from the rally, friendly move, or conquest step
  explore = 2,           -- an exploration, which the game goes on with next turn while the square stays unseen
  noSquare = -1,         -- x and y of no square
  exploreBelow = 50,     -- AI_EXPLORE runs only while the best score so far is below this
  farExploreBelow = 10,  -- the game's far exploration runs only while the best score is below this
}

-- BEGIN key actions (generated from key_actions.h: change that list, not this block)
---The key actions (C: KEY_ACTION_* in key_actions.h): one for each key of each line of the game's
---Keyboard Shortcuts list, for coe5.rebindKey, coe5.addActionKey, and coe5.actionKey. The
---comment gives the screen, the game's key, and the line.
---@class coe5.keyAction
coe5.keyAction = {
  mapShortcutsList = 1,          -- Map, ?: List keyboard shortcuts
  mapRecruit = 2,                -- Map, r: Recruit
  mapEndTurn = 3,                -- Map, y: End turn
  mapMessages = 4,               -- Map, m: View messages
  mapNextCommander = 5,          -- Map, n: Next commander
  mapPreviousCommander = 6,      -- Map, j: Previous commander
  mapCommanderDone = 7,          -- Map, Space: Commander is done for this turn
  mapTransfer = 8,               -- Map, t: Transfer troops
  mapSpecialPower = 9,           -- Map, p: Use commander's special power
  mapUnitOverview = 10,          -- Map, F1: Unit overview
  mapPlayerOverview = 11,        -- Map, F2: Player overview
  mapClassDescription = 12,      -- Map, F3: Class description
  mapScoreGraphs = 13,           -- Map, F5: View score graphs
  mapActiveMods = 14,            -- Map, F6: View active mods
  mapGameMenu = 15,              -- Map, F10: Settings, save, quit (the Game Menu)
  mapSquareInfo = 16,            -- Map, i: Square terrain info
  mapCommanderStats = 17,        -- Map, v: View commander stats
  mapTrade = 18,                 -- Map, l: Trade administration
  mapSentry = 19,                -- Map, s: Sentry commander
  mapLocationPower = 20,         -- Map, o: Use location's special power
  mapBurnForestScreen = 21,      -- Map, b: Burn forest screen
  mapBurnForestNow = 22,         -- Map, B: Burn forest immediately
  mapHuntSlaves = 23,            -- Map, h: Hunt for slaves
  mapPlaneShift = 24,            -- Map, u: Plane shift
  mapContinue = 25,              -- Map, ,: Continue moving
  mapContinueAll = 26,           -- Map, .: Continue moving for all commanders
  mapSquareDetails = 27,         -- Map, ctrl+i: Detailed square info
  mapAstrology = 28,             -- Map, ctrl+a: Astrology reading
  mapRepeatOrders = 29,          -- Map, ctrl+r: Execute all repeat orders now
  mapScry = 30,                  -- Map, alt+s: Scry
  mapCenter = 31,                -- Map, Home: Center map on starting citadel
  mapHideIcons = 32,             -- Map, Tab: Hide map icons (hold)
  mapZoomIn = 33,                -- Map, PgUp: Zoom in
  mapZoomOut = 34,               -- Map, PgDn: Zoom out
  mapScrollUp = 35,              -- Map, Up: Scroll map up
  mapScrollDown = 36,            -- Map, Down: Scroll map down
  mapScrollLeft = 37,            -- Map, Left: Scroll map left
  mapScrollRight = 38,           -- Map, Right: Scroll map right
  mapMoveUpLeft = 39,            -- Map, q: Move commander up-left
  mapMoveUp = 40,                -- Map, w: Move commander up
  mapMoveUpRight = 41,           -- Map, e: Move commander up-right
  mapMoveLeft = 42,              -- Map, a: Move commander left
  mapMoveRight = 43,             -- Map, d: Move commander right
  mapMoveDownLeft = 44,          -- Map, z: Move commander down-left
  mapMoveDown = 45,              -- Map, x: Move commander down
  mapMoveDownRight = 46,         -- Map, c: Move commander down-right
  mapLookUpLeft = 47,            -- Map, Q: Info on the square up-left
  mapLookUp = 48,                -- Map, W: Info on the square up
  mapLookUpRight = 49,           -- Map, E: Info on the square up-right
  mapLookLeft = 50,              -- Map, A: Info on the square to the left
  mapLookRight = 51,             -- Map, D: Info on the square to the right
  mapLookDownLeft = 52,          -- Map, Z: Info on the square down-left
  mapLookDown = 53,              -- Map, X: Info on the square down
  mapLookDownRight = 54,         -- Map, C: Info on the square down-right
  mapLookHere = 55,              -- Map, S: Info on the commander's square
  armyShortcutsList = 56,        -- Transfer troops, ?: List keyboard shortcuts
  armySelectAll = 57,            -- Transfer troops, a: Select/deselect all units
  armySelectAfflicted = 58,      -- Transfer troops, b: Select units with battle afflictions
  armyDeselectSlowAp = 59,       -- Transfer troops, c: Deselect units with fewer AP left
  armySelectFast = 60,           -- Transfer troops, f: Select all fast units
  armySelectHorror = 61,         -- Transfer troops, h: Select horror marked units
  armyShowItems = 62,            -- Transfer troops, i: Highlight units with magic items
  armyDeselectLandlubbers = 63,  -- Transfer troops, l: Deselect all landlubbers
  armySelectType = 64,           -- Transfer troops, m: Select the type under the pointer
  armySelectSlow = 65,           -- Transfer troops, s: Select all slow units
  armySelectWounded = 66,        -- Transfer troops, w: Select all wounded units
  armySelectTen = 67,            -- Transfer troops, +: Select 10 of the type under the pointer
  armyDeselectTen = 68,          -- Transfer troops, -: Deselect 10 of the type under the pointer
  armyScrollUp = 69,             -- Transfer troops, Up: Scroll battlefield up
  armyScrollDown = 70,           -- Transfer troops, Down: Scroll battlefield down
  armyScrollLeft = 71,           -- Transfer troops, Left: Scroll battlefield left
  armyScrollRight = 72,          -- Transfer troops, Right: Scroll battlefield right
  armySelectMenu = 73,           -- Transfer troops, F2: Select units menu
  armyExit = 74,                 -- Transfer troops, Esc: Exit screen
  battleShortcutsList = 75,      -- Battle, ?: List keyboard shortcuts
  battleScrollLeft = 76,         -- Battle, Left: Scroll battlefield left
  battleScrollRight = 77,        -- Battle, Right: Scroll battlefield right
  battleMessagesUp = 78,         -- Battle, -: Scroll battle messages back
  battleMessagesDown = 79,       -- Battle, +: Scroll battle messages forward
  battleMessagesPageUp = 80,     -- Battle, PgUp: Scroll battle messages a page back
  battleMessagesPageDown = 81,   -- Battle, PgDn: Scroll battle messages a page forward
  battlePause = 82,              -- Battle, Space: Pause battle
  battleTerrainOnly = 83,        -- Battle, Tab: View terrain only (hold)
  battleSlowMotion = 84,         -- Battle, z: Slow motion
  battleNormalSpeed = 85,        -- Battle, n: Normal speed
  battleFastForward = 86,        -- Battle, f: Fast forward
  battleSpeedUp = 87,            -- Battle, s: Speed up one notch
  battleSkip = 88,               -- Battle, q: Skip entire battle
  battleLog = 89,                -- Battle, l: Hide/show battle log
  battleGrid = 90,               -- Battle, g: Hide/show grid
  battleTerrain = 91,            -- Battle, t: Hide/show terrain
  battleUnitList = 92,           -- Battle, F1: View unit list
  mapEditorShortcutsList = 93,   -- Map editor, ?: List keyboard shortcuts
  mapEditorMenu = 94,            -- Map editor, Tab: Hide/show menu
  mapEditorPencil1 = 95,         -- Map editor, 1: Pencil size 1
  mapEditorPencil2 = 96,         -- Map editor, 2: Pencil size 2
  mapEditorPencil3 = 97,         -- Map editor, 3: Pencil size 3
  mapEditorClearSquare = 98,     -- Map editor, c: Clear special attributes for square
  mapEditorNameSquare = 99,      -- Map editor, n: Name square
  mapEditorGateway = 100,        -- Map editor, g: Create gateway
  mapEditorOverlay = 101,        -- Map editor, o: Toggle overlay on/off
  mapEditorQuit = 102,           -- Map editor, q: Quit map editor
  mapEditorRandomWorld = 103,    -- Map editor, r: Recreate random world
  mapEditorTerrainList = 104,    -- Map editor, t: Select terrain from a list
  mapEditorUnflood = 105,        -- Map editor, u: Unflood all squares
  mapEditorFixedTemp = 106,      -- Map editor, E: Set fixed temp on entire map
  mapEditorRemoveFancy = 107,    -- Map editor, F: Remove all fancy terrains
  mapEditorLoadOverlay = 108,    -- Map editor, O: Load overlay image
  mapEditorFixTemps = 109,       -- Map editor, T: Fix temperatures
  mapEditorCircularTemp = 110,   -- Map editor, ctrl+u: Circular temperature on map
  mapEditorLoad = 111,           -- Map editor, ctrl+l: Load map
  mapEditorDrawTemp = 112,       -- Map editor, ctrl+e: Enter temperature to draw with
  mapEditorResize = 113,         -- Map editor, ctrl+r: Resize map (scale)
  mapEditorSave = 114,           -- Map editor, ctrl+s: Save map
  mapEditorRemoveCold = 115,     -- Map editor, ctrl+t: Remove all cold temperatures
  mapEditorReduceSquare = 116,   -- Map editor, alt+r: Reduce square
  mapEditorDestroySquare = 117,  -- Map editor, alt+d: Destroy square
  mapEditorMoveUp = 118,         -- Map editor, U: Move entire map up
  mapEditorMoveLeft = 119,       -- Map editor, H: Move entire map left
  mapEditorMoveDown = 120,       -- Map editor, J: Move entire map down
  mapEditorMoveRight = 121,      -- Map editor, K: Move entire map right
  mapEditorZoomIn = 122,         -- Map editor, PgUp: Zoom in
  mapEditorZoomOut = 123,        -- Map editor, PgDn: Zoom out
  mapEditorScrollUp = 124,       -- Map editor, Up: Scroll map up
  mapEditorScrollDown = 125,     -- Map editor, Down: Scroll map down
  mapEditorScrollLeft = 126,     -- Map editor, Left: Scroll map left
  mapEditorScrollRight = 127,    -- Map editor, Right: Scroll map right
  mapEditorPlaneAbove = 128,     -- Map editor, <: Plane above
  mapEditorPlaneBelow = 129,     -- Map editor, >: Plane below
  mapEditorRefresh = 130,        -- Map editor, F5: Refresh map
  battleEditorShortcutsList = 131, -- Battle map editor, ?: List keyboard shortcuts
  battleEditorGroundMode = 132,  -- Battle map editor, Tab: Switch to/from ground edit mode
  battleEditorCenterLine = 133,  -- Battle map editor, ctrl+w: Show center line
  battleEditorReplacer = 134,    -- Battle map editor, r: Replacer
  battleEditorDraw = 135,        -- Battle map editor, q: Draw normally
  battleEditorLoad = 136,        -- Battle map editor, ctrl+l: Load
  battleEditorSave = 137,        -- Battle map editor, ctrl+s: Save
  battleEditorMonster = 138,     -- Battle map editor, m: Select monster
  battleEditorClear = 139,       -- Battle map editor, C: Clear all
  battleEditorScrollUp = 140,    -- Battle map editor, u: Scroll terrain up
  battleEditorScrollLeft = 141,  -- Battle map editor, h: Scroll terrain left
  battleEditorScrollDown = 142,  -- Battle map editor, j: Scroll terrain down
  battleEditorScrollRight = 143, -- Battle map editor, k: Scroll terrain right
  battleEditorNoDeploy = 144,    -- Battle map editor, alt+b: No deploy mark
  battleEditorCatapult = 145,    -- Battle map editor, alt+c: Catapult mark
  battleEditorEffigy = 146,      -- Battle map editor, alt+e: Effigy mark
  battleEditorMage = 147,        -- Battle map editor, alt+m: Mage mark
  battleEditorMelee = 148,       -- Battle map editor, alt+s: Melee soldier mark
  battleEditorDemonHand = 149,   -- Battle map editor, alt+d: Demon hand mark
  battleEditorDemonHead = 150,   -- Battle map editor, alt+h: Demon head mark
  battleEditorFirewall = 151,    -- Battle map editor, alt+v: Firewall mark
  battleEditorNoTrees = 152,     -- Battle map editor, alt+t: No random trees/bushes here
  battleEditorClearNoTrees = 153, -- Battle map editor, alt+y: Clear no random trees/bushes mark
  recruitClearHere = 154,        -- Recruitment, c: Clear auto-recruitment at this location
  recruitClearAll = 155,         -- Recruitment, v: Clear auto-recruitment everywhere
  recruitLocation1 = 156,        -- Recruitment, 1: Recruitment location 1
  recruitLocation2 = 157,        -- Recruitment, 2: Recruitment location 2
  recruitLocation3 = 158,        -- Recruitment, 3: Recruitment location 3
  recruitLocation4 = 159,        -- Recruitment, 4: Recruitment location 4
  recruitLocation5 = 160,        -- Recruitment, 5: Recruitment location 5
  recruitLocation6 = 161,        -- Recruitment, 6: Recruitment location 6
  recruitLocation7 = 162,        -- Recruitment, 7: Recruitment location 7
  recruitLocation8 = 163,        -- Recruitment, 8: Recruitment location 8
  recruitLocation9 = 164,        -- Recruitment, 9: Recruitment location 9
  recruitPrevious = 165,         -- Recruitment, Left: Previous recruitment location
  recruitNext = 166,             -- Recruitment, Right: Next recruitment location
  recruitShortcutsList = 167,    -- Recruitment, ?: List keyboard shortcuts
  recruitExit = 168,             -- Recruitment, Esc: Exit recruitment
  recruitExitEnter = 169,        -- Recruitment, Enter: Exit recruitment, second key
  participantsAdd = 170,         -- Setup participants, n: Add new player
  participantsColors = 171,      -- Setup participants, r: Randomize player colors by team
  participantsAllHuman = 172,    -- Setup participants, H: Set all players to human controlled
  participantsAllAi = 173,       -- Setup participants, A: Set all players to AI controlled
  participantsAiUp = 174,        -- Setup participants, +: Increase all AI levels by 1
  participantsAiDown = 175,      -- Setup participants, -: Decrease all AI levels by 1
  participantsRemove = 176,      -- Setup participants, Backspace: Remove last player
  participantsOk = 177,          -- Setup participants, Enter: Ok
  participantsCancel = 178,      -- Setup participants, Esc: Cancel
  viewArmyShowItems = 179,       -- Army on the battlefield, i: Highlight units with magic items
  viewArmyShortcutsList = 180,   -- Army on the battlefield, ?: List keyboard shortcuts
  viewArmyScrollLeft = 181,      -- Army on the battlefield, Left: Scroll battlefield left
  viewArmyScrollRight = 182,     -- Army on the battlefield, Right: Scroll battlefield right
  viewArmyTerrainOnly = 183,     -- Army on the battlefield, Tab: View terrain only (hold)
  viewArmyExit = 184,            -- Army on the battlefield, Enter: Exit screen
  viewArmyExitEsc = 185,         -- Army on the battlefield, Esc: Exit screen, second key
  unitListCommanders = 186,      -- Battle unit list, c: Only show commanders
  unitListShowAll = 187,         -- Battle unit list, A: Show all (terrain units too)
  unitListShortcutsList = 188,   -- Battle unit list, ?: List keyboard shortcuts
  unitListExit = 189,            -- Battle unit list, Enter: Exit screen
  unitListExitEsc = 190,         -- Battle unit list, Esc: Exit screen, second key
  fileBrowserDelete = 191,       -- Load game, Delete: Delete save file under pointer
  fileBrowserDeleteCtrl = 192,   -- Load game, ctrl+d: Delete save file, second key
  fileBrowserSortTime = 193,     -- Load game, ctrl+t: Sort by time
  fileBrowserSortName = 194,     -- Load game, ctrl+n: Sort by name
  fileBrowserShortcutsList = 195, -- Load game, ?: List keyboard shortcuts
  fileBrowserCancel = 196,       -- Load game, Esc: Cancel
}
-- END key actions
