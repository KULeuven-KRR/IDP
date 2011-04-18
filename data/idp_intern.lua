idp_intern.main = function() 
	if main then return main() end
end

local oldType = type

idp_intern.isIdp = function(obj) 
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

function type(obj) 
	if idp_intern.isIdp(obj) then
		local idptype = getmetatable(obj)["type"]
		return idp_intern.idptype(idptype)
	else
		return oldType(obj)
	end
end

local oldTostring = tostring
function tostring(e,opts) 
	if isIdp(e) then
		return idp_intern.tostring(e,opts)
	else
		return oldTostring(e)
	end
end

local oldPrint = print
local function print_with_options(list,opts)
	local res = ""
	for i=1,#list do
		res = res..tostring(list[i],opts).."\t"
	end
	oldPrint(res)
end

function print(...) 
	local arglist = {...}
	if #arglist > 1 then
		local opts = arglist[#arglist]
		if type(opts) == "options" then
			arglist[#arglist] = nil
			print_with_options(arglist,opts)
		else 
			oldPrint(...) 
		end
	else
		oldPrint(...)
	end
end
