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


-- Returns a structure that is a solution to the model expansion problem with input theory T and structure S 
function onemodel(T,S) 
        Opts = getoptions()
        local oldnbmodels = Opts.nbmodels
        Opts.nbmodels = 1
        local solutions, trace = modelexpand(T,S)
        Opts.nbmodels = oldnbmodels
        local solution = nil
        if solutions then
                solution = solutions[1]
        end
        if trace then
                return solution, trace
        else
                return solution
        end
end

-- Returns a structure that is a solution to the model expansion problem with input theory T, structure S and output vocabularium V 
function onemodel(T,S,V) 
        Opts = getoptions()
        local oldnbmodels = Opts.nbmodels
        Opts.nbmodels = 1
        local solutions, trace = modelexpand(T,S,V)
        Opts.nbmodels = oldnbmodels
        local solution = nil
        if solutions then
                solution = solutions[1]
        end
        if trace then
                return solution, trace
        else
                return solution
        end
end

-- Returns all solutions to the model expansion problem with input theory T and structure S 
function allmodels(T,S) 
        Opts = getoptions()
        local oldnbmodels = Opts.nbmodels
        Opts.nbmodels = 0
        local solutions, trace = modelexpand(T,S)
        Opts.nbmodels = oldnbmodels
        if trace then
                return solutions, trace
        else
                return solutions
        end
end

-- Returns all solutions to the model expansion problem with input theory T, structure S and output vocabularium V 
function allmodels(T,S,V) 
        Opts = getoptions()
        local oldnbmodels = Opts.nbmodels
        Opts.nbmodels = 0
        local solutions, trace = modelexpand(T,S,V)
        Opts.nbmodels = oldnbmodels
        if trace then
                return solutions, trace
        else
                return solutions
        end
end

-- Prints a given list of models or unsatisfiable if the list is empty 
function printmodels(sols) 
        if #sols == 0 then
                print("Unsatisfiable")
        end

        print("Number of models: "..#sols)
        for k,v in ipairs(sols) do
                print("Model "..k)
                local kstr = tostring(k)
                local str = "======"
                for i=1,string.len(kstr) do str = str.."=" end
                print(str)
                print(v)
        end
end

