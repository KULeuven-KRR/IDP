local oldType = type

-- return true iff the given object is a userdatum created by idp
idpintern.isIdp = function(obj) 
	if oldType(obj) == "userdata" then
		local t = getmetatable(obj)
		if t then
			if t.type then return true
			else return false
			end
		else
			return false
		end
	else 
		return false
	end
end

-- overwrites standard type function so that it returns the correct idp types instead of 'userdatum'
function type(obj) 
	if idpintern.isIdp(obj) then
		local t = getmetatable(obj)["type"]
		return idptype(t)
	else
		return oldType(obj)
	end
end

-- overwrite standard pairs function so that it returns an iterator also on idp tables
local oldPairs = pairs
function pairs(table) 
	if idpintern.isIdp(table) then
		return 
			function(s,var) 
				var = var+1
				if var > #table then
					return nil
				else
					return var, table[var]
				end
			end,
			0,
			0
	else
		return oldPairs(table)
	end
end

-- overwrite standard ipairs function so that it returns an iterator also on idp tables
local oldIpairs = ipairs
function ipairs(table) 
	if idpintern.isIdp(table) then
		return pairs(table)
	else
		return oldIpairs(table)
	end
end

-- overwrite standard tostring function so that it works correctly on idp types
local oldTostring = tostring
function tostring(e) 
	if idpintern.isIdp(e) then
		return idpintern.tostring(e)
	else
		return oldTostring(e)
	end
end

