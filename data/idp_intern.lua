local function idpprocedure(name,longname,argnums,code) 
	return { idptype = "procedure", name = name, longname = longname, argnums = argnums, code = code }
end

local function idptheory(name,longname)
	return { idptype = "theory", name = name, longname = longname }
end

local function idpoptions(name,longname)
	return { idptype = "options", name = name, longname = longname }
end

local function idpstructure(name,longname)
	return { idptype = "structure", name = name, longname = longname }
end

local function idpsort(name,longname)
	return { idptype = "sort", name = name, longname = longname }
end

local function idppredicate(name,longname)
	return { idptype = "predicate", name = name, longname = longname }
end

local function idpfunction(name,longname)
	return { idptype = "function", name = name, longname = longname }
end

local function idpvocabulary(name,longname) 
	return { idptype = "vocabulary", name = name, longname = longname }
end

local function idpnamespace(name,longname)
	return { idptype = "namespace", name = name, longname = longname }
end

local function newnodedata(name) 
	return	{ 
		name			= name,
		children		= { },
		runprocs		= { },
		procedure		= nil,
		theory			= nil,
		options			= nil,
		structure		= nil,
		sort			= nil,
		predicate		= nil,	-- TODO
		["function"]	= nil,	-- TODO
		vocabulary		= nil,
		namespace		= nil 
	}
end

local descend			= { }
local getchildren		= { }
local getrunprocs		= { }
local getprocedure		= { }
local getname			= { }
local gettheory			= { }
local getoptions		= { }
local getstructure		= { }
local getsort			= { }
local getpredicate		= { }
local getfunction		= { }
local getnamespace		= { }
local getvocabulary		= { }
local doinsert			= { }
local isidp				= { }

local function createnode(nodedata)
	return function(...) 
		local argtable = {...}
		if #argtable > 0  then
			argone = argtable[1]
			if argone == descend then
				return (nodedata.children)[argtable[2]]
			elseif argone == isidp then
				return isidp
			elseif argone == getchildren then
				return nodedata.children
			elseif argone == getrunprocs then
				return nodedata.runprocs
			elseif argone == getprocedure then
				return nodedata.procedure
			elseif argone == getname then
				return nodedata.name
			elseif argone == gettheory then
				return nodedata.theory
			elseif argone == getoptions then
				return nodedata.options
			elseif argone == getstructure then
				return nodedata.structure
			elseif argone == getsort then
				return nodedata.sort
			elseif argone == getpredicate then
				return nodedata.predicate
			elseif argone == getfunction then
				return nodedata["function"]
			elseif argone == getnamespace then
				return nodedata.namespace
			elseif argone == getvocabulary then
				return nodedata.vocabulary
			elseif argone == doinsert then
				if (nodedata.children)[argtable[2](getname)] then
					(nodedata.children)[argtable[2](getname)] = 
						mergenodes((nodedata.children)[argtable[2](getname)],argtable[2])
				else
					(nodedata.children)[argtable[2](getname)] = argtable[2]
				end
			elseif (nodedata.runprocs)[#argtable] then
				return (nodedata.runprocs)[#argtable](...)
			else
				print("ERROR: Procedure "..nodedata.name.."/"..#argtable.." does not exist.")
				return nil
			end
		elseif (nodedata.runprocs)[0] then
			return (nodedata.runprocs)[0]()
		else 
			print("ERROR: Procedure "..nodedata.name.."/0 does not exist.")
			return nil
		end
	end
end

local function mergenodes(node1,node2) 

	if type(node1) == "nil" then return node2 end
	if type(node2) == "nil" then return node1 end

	local nodedata = newnodedata(node1(getname))

	-- merge children
	local temptable1 = node1(getchildren)
	local temptable2 = node2(getchildren)
	for k,v in pairs(temptable1) do
		if temptable2[k] then
			(nodedata.children)[k] = mergenodes(v,temptable2[k])
		else
			(nodedata.children)[k] = v
		end
	end

	-- merge runprocs
	nodedata.runprocs = node1(getrunprocs)
	temptable2 = node2(getrunprocs)
	for k,v in pairs(temptable2) do
		(nodedata.runprocs)[k] = v
	end

	-- merge rest
	nodedata.procedure = node1(getprocedure) or node2(getprocedure)
	nodedata.theory = node1(gettheory) or node2(gettheory)
	nodedata.options = node1(getoptions) or node2(getoptions)
	nodedata.structure = node1(getstructure) or node2(getstructure)
	nodedata.sort = node1(getsort) or node2(getsort)
	nodedata.predicate = node1(getpredicate) or node2(getpredicate)
	nodedata["function"] = node1(getfunction) or node2(getfunction)
	nodedata.vocabulary = node1(getvocabulary) or node2(getvocabulary)
	nodedata.namespace = node1(getnamespace) or node2(getnamespace)

	return createnode(nodedata)
end


local function newnode(object) 

	-- initialize data structures
	local nodedata = newnodedata(object.name) 
	nodedata[object.idptype] = object.longname
	if object.idptype == "procedure" then
		nodedata["runprocs"][object.argnums] = object.code
	end

	return createnode(nodedata)

end

function using(obj) 
	local longnamesp = obj(getnamespace)
	local longnamevoc = obj(getvocabulary)
	if longnamesp then
		-- TODO
	elseif longnamevoc then
		-- TODO
	else
		print("ERROR: expected a namespace or vocabulary as argument for procedure using.")
	end
end

idp_intern = {
	descend			= descend,			
    getchildren		= getchildren,
    getrunprocs		= getrunprocs,		
    getprocedure	= getprocedure,	
    getname			= getname,		
    gettheory		= gettheory,
    getoptions		= getoptions,		
    getstructure	= getstructure,
    getsort			= getsort,			
    getpredicate	= getpredicate,
    getfunction		= getfunction,		
    getnamespace	= getnamespace,	
    getvocabulary	= getvocabulary,	
    doinsert		= doinsert,

	mergenodes		= mergenodes,

	idpprocedure	= idpprocedure,
	idptheory		= idptheory,
	idpoptions		= idpoptions,
	idpstructure	= idpstructure,
	idpsort			= idpsort,
	idppredicate	= idppredicate,
	idpfunction		= idpfunction,
	idpnamespace	= idpnamespace,
	idpvocabulary	= idpvocabulary,

	newnode			= newnode,
	idpcall			= idpcall
}

local oldTostring = tostring
function tostring(e) 
	if type(e) == "function" then
		if e(isidp) == isidp then
			return idptostring(e)
		else
			return oldTostring(e)
		end
	elseif type(e) == "userdata" then
		return idptostring(e)
	else
		return oldTostring(e)
	end
end
