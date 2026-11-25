-- CoE5SE calls

local C = {}

local loggedAsMissing = {}

local function coe5Call(name)
  local call = rawget(coe5, name)
  if not call and not loggedAsMissing[name] then
    loggedAsMissing[name] = true
    coe5.log("heroes_call_missing call=coe5." .. name)
  end
  return call
end

function C.promote(unit, monster)
  local promoteUnit = coe5Call("promoteUnit")
  if not promoteUnit then return nil end
  local commander = promoteUnit(unit, monster, true)
  local gameRefused = type(commander) ~= "number" or commander < 0
  if gameRefused then return nil end
  return commander
end

function C.message(player, title, text, goTo)
  local sendMessage = coe5Call("sendMessage")
  if not sendMessage then return false end
  if goTo then
    return sendMessage(player, title, text, {x = goTo.x, y = goTo.y, unit = goTo.unit})
  end
  return sendMessage(player, title, text)
end

function C.renameCommander(commander)
  local renameCommander = coe5Call("renameCommander")
  if not renameCommander then return nil end
  return renameCommander(commander)
end

function C.cardLine(affliction, labelAndTextOf)
  local afflictionCardLine = coe5Call("afflictionCardLine")
  if not afflictionCardLine then return false end
  return afflictionCardLine(affliction, labelAndTextOf)
end

return C
