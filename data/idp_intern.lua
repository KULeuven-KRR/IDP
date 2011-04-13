function idp_intern_main()
	if main then main() end
end

local oldType = type
function type(obj) 
	if oldType(obj) == "userdata" then
		local idptype = getmetatable(obj)["type"]
		return idp_intern.idptype(idptype)
	else
		return oldType(obj)
	end
end

local oldTostring = tostring
function tostring(e) 
	local t = getmetatable(e)
	if t then
		if getmetatable(e).idptype then
			return idptostring(e)
		else
			return oldTostring(e)
		end
	else
		return oldTostring(e)
	end
end

local oldPrint = print
local function print_with_options(list,opts)
	local res = ""
	for i=1,#list do
		if getmetatable(list[i]).idptype then
			res = res..idptostring(list[i],opts)
		else
			res = res..oldTostring(list[i])
		end
		res = res.."\t"
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
