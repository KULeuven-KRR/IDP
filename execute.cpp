/************************************
	execute.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "execute.hpp"
#include "ground.hpp"
#include "ecnf.hpp"
#include "print.hpp"
#include "data.hpp"
#include "lua.hpp"
#include "error.hpp"
#include "fobdd.hpp"


#include "external/MonitorInterface.hpp"

/*
	Connection with lua
*/

namespace BuiltinProcs {

	map<string,vector<Inference*> >	_inferences;	// All inference methods

	void initialize() {
		_inferences["print"].push_back(new PrintTheory());
		_inferences["print"].push_back(new PrintVocabulary());
		_inferences["print"].push_back(new PrintStructure());
		_inferences["print"].push_back(new PrintNamespace());
		_inferences["push_negations"].push_back(new PushNegations());
		_inferences["remove_equivalences"].push_back(new RemoveEquivalences());
		_inferences["remove_eqchains"].push_back(new RemoveEqchains());
		_inferences["flatten"].push_back(new FlattenFormulas());
		_inferences["convert_to_theory"].push_back(new StructToTheory());
		_inferences["move_quantifiers"].push_back(new MoveQuantifiers());
		_inferences["tseitin"].push_back(new ApplyTseitin());
		_inferences["reduce"].push_back(new GroundReduce());
		_inferences["move_functions"].push_back(new MoveFunctions());
		_inferences["load_file"].push_back(new LoadFile());
		_inferences["clone"].push_back(new CloneStructure());
		_inferences["clone"].push_back(new CloneTheory());
		_inferences["fastground"].push_back(new FastGrounding());
		_inferences["fastmx"].push_back(new FastMXInference());

		_inferences["newoptions"].push_back(new NewOption());

		_inferences["forcetwovalued"].push_back(new ForceTwoValued());
		_inferences["changevoc"].push_back(new ChangeVoc());
		_inferences["getbdds"].push_back(new BDDPrinter());

		_inferences["delete"].push_back(new DeleteData(IAT_THEORY));
		_inferences["delete"].push_back(new DeleteData(IAT_STRUCTURE));
		_inferences["delete"].push_back(new DeleteData(IAT_VOCABULARY));
		_inferences["delete"].push_back(new DeleteData(IAT_NAMESPACE));
		_inferences["delete"].push_back(new DeleteData(IAT_OPTIONS));

		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_PREDICATE));
		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_FUNCTION));
		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_SORT));
		_inferences["index"].push_back(new GetIndex(IAT_NAMESPACE,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_VOCABULARY,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_OPTIONS,IAT_STRING));

		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_PREDICATE,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_FUNCTION,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_SORT,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_PREDICATE,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_FUNCTION,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_SORT,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_DOUBLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_BOOLEAN));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_STRING));
	}

	void cleanup() {
		for(map<string,vector<Inference*> >::iterator it = _inferences.begin(); it != _inferences.end(); ++it) {
			for(unsigned int n = 0; n < it->second.size(); ++n) {
				delete(it->second[n]);
			}
		}
	}

	string typestring(InfArgType t) {
		switch(t) {
			case IAT_THEORY: 
				return("theory");
			case IAT_STRUCTURE: 
				return("structure");
			case IAT_VOCABULARY: 
				return("vocabulary");
			case IAT_NAMESPACE: 
				return("namespace");
			case IAT_OPTIONS: 
				return("options");
			case IAT_NIL: 
				return("nil");
			case IAT_INT: 
				return("number");
			case IAT_DOUBLE:
				return("number");
			case IAT_BOOLEAN: 
				return("boolean");
			case IAT_STRING: 
				return("string");
			case IAT_TABLE: 
				return("table");
			case IAT_PROCEDURE: 
				return("function");
			case IAT_OVERLOADED: 
				return("overloaded");
			case IAT_SORT:
				return("sort");
			case IAT_PREDICATE: 
				return("predicate");
			case IAT_FUNCTION:
				return("idpfunction");
			case IAT_MULT:
				assert(false); return "mult";
			case IAT_REGISTRY:
				assert(false); return "registry";
			default:
				assert(false);
		}
	}

	string typestring(lua_State* L, int index) {
		lua_getglobal(L,"type");
		lua_pushvalue(L,index);
		lua_call(L,1,1);
		string str = string(lua_tostring(L,-1));
		lua_pop(L,1);
		return str;
	}

	InfArgType typeenum(lua_State* L, int index) {
		string strtype = typestring(L,index);
		if(strtype == "theory") return IAT_THEORY;
		if(strtype == "structure") return IAT_STRUCTURE;
		if(strtype == "vocabulary") return IAT_VOCABULARY;
		if(strtype == "namespace") return IAT_NAMESPACE;
		if(strtype == "options") return IAT_OPTIONS;
		if(strtype == "nil") return IAT_NIL;
		if(strtype == "number") return IAT_DOUBLE;
		if(strtype == "boolean") return IAT_BOOLEAN;
		if(strtype == "string") return IAT_STRING;
		if(strtype == "table") return IAT_TABLE;
		if(strtype == "function") return IAT_PROCEDURE;
		if(strtype == "overloaded") return IAT_OVERLOADED;
		if(strtype == "sort") return IAT_SORT;
		if(strtype == "predicate") return IAT_PREDICATE;
		if(strtype == "idpfunction") return IAT_FUNCTION;
		if(strtype == "mult") return IAT_MULT;
		if(strtype == "registry") return IAT_REGISTRY;
		assert(false); return IAT_INT;
	}

	bool checkarg(lua_State* L, int n, InfArgType t) {
		string typestr = typestring(t);
		string nstr = typestring(L,n);
		if(nstr == typestr) return true;
		else if(nstr == "overloaded") {
			OverloadedObject* obj = *((OverloadedObject**)lua_touserdata(L,n));
			switch(t) {
				case IAT_THEORY:
					return obj->isTheory();
				case IAT_STRUCTURE:
					return obj->isStructure();
				case IAT_VOCABULARY:
					return obj->isVocabulary();
				case IAT_NAMESPACE:
					return obj->isNamespace();
				case IAT_OPTIONS:
					return obj->isOptions();
				case IAT_INT:
					return obj->isInt();
				case IAT_DOUBLE:
					return obj->isDouble();
				case IAT_BOOLEAN:
					return (obj->isBool());
				case IAT_STRING:
					return (obj->isString());
				case IAT_PROCEDURE:
					return (obj->isProcedure());
				case IAT_SORT:
					return (obj->isSort());
				case IAT_PREDICATE:
					return (obj->isPredicate());
				case IAT_FUNCTION:
					return (obj->isFunction());
				case IAT_TABLE:
					return (obj->isTable());
				default:
					return false;
			}
		}
		else return false;
	}


	int converttolua(lua_State* L, InfArg res, InfArgType t) {
		switch(t) {
			case IAT_THEORY:
			{
				AbstractTheory** ptr = (AbstractTheory**) lua_newuserdata(L,sizeof(AbstractTheory*));
				(*ptr) = res._theory;
				luaL_getmetatable (L,"theory");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_VOCABULARY:
			{
				Vocabulary** ptr = (Vocabulary**) lua_newuserdata(L,sizeof(Vocabulary*));
				(*ptr) = res._vocabulary;
				luaL_getmetatable (L,"vocabulary");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_STRUCTURE:
			{
				AbstractStructure** ptr = (AbstractStructure**) lua_newuserdata(L,sizeof(AbstractStructure*));
				(*ptr) = res._structure;
				luaL_getmetatable (L,"structure");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_NAMESPACE:
			{
				Namespace** ptr = (Namespace**) lua_newuserdata(L,sizeof(Namespace*));
				(*ptr) = res._namespace;
				luaL_getmetatable (L,"namespace");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_OPTIONS:
			{
				InfOptions** ptr = (InfOptions**) lua_newuserdata(L,sizeof(InfOptions*));
				(*ptr) = res._options;
				luaL_getmetatable (L,"options");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_NIL:
				lua_pushnil(L);
				break;
			case IAT_INT:
				lua_pushnumber(L,double(res._int));
				break;
			case IAT_DOUBLE:
				lua_pushnumber(L,res._double);
				break;
			case IAT_BOOLEAN:
				lua_pushboolean(L,res._boolean);
				break;
			case IAT_STRING:
				lua_pushstring(L,res._string->c_str());
				break;
			case IAT_TABLE:
				lua_newtable(L);
				for(unsigned int n = 0; n < res._table->size(); ++n) {
					lua_pushinteger(L,n+1);
					converttolua(L,(*(res._table))[n]._value,(*(res._table))[n]._type);
					lua_settable(L,-3);
				}
				break;
			case IAT_PROCEDURE:
			{
				lua_getfield(L,LUA_REGISTRYINDEX,res._procedure->c_str());
				break;
			}
			case IAT_MULT:
			{
				int nrres = 0;
				for(unsigned int n = 0; n < res._table->size(); ++n) {
					nrres += converttolua(L,(*(res._table))[n]._value,(*(res._table))[n]._type);
				}
				return nrres;
			}
			case IAT_REGISTRY:
			{
				lua_getfield(L,LUA_REGISTRYINDEX,res._string->c_str());
				break;
			}
			case IAT_OVERLOADED:
			{
				OverloadedObject* obj = res._overloaded;
				if(obj->single()) {
					InfArg newarg;
					if(obj->isNamespace())	{ 
						newarg._namespace = obj->getNamespace(); 
						converttolua(L,newarg,IAT_NAMESPACE);	
					}
					if(obj->isVocabulary()) { 
						newarg._vocabulary = obj->getVocabulary(); 
						converttolua(L,newarg,IAT_VOCABULARY);	
					}
					if(obj->isTheory()) { 
						newarg._theory = obj->getTheory(); 
						converttolua(L,newarg,IAT_THEORY);	
					}
					if(obj->isStructure()) { 
						newarg._structure = obj->getStructure(); 
						converttolua(L,newarg,IAT_STRUCTURE);	
					}
					if(obj->isOptions()) { 
						newarg._options = obj->getOptions(); 
						converttolua(L,newarg,IAT_OPTIONS);	
					}
					if(obj->isProcedure()) {
						newarg._procedure = obj->getProcedure();
						converttolua(L,newarg,IAT_PROCEDURE);	
					}
					if(obj->isPredicate()) {
						newarg._predicate = obj->getPredicate();
						converttolua(L,newarg,IAT_PREDICATE);	
					}
					if(obj->isFunction()) {
						newarg._function = obj->getFunction();
						converttolua(L,newarg,IAT_FUNCTION);	
					}
					if(obj->isSort()) {
						newarg._sort = obj->getSort();
						converttolua(L,newarg,IAT_SORT);
					}
					if(obj->isString()) {
						newarg._string = obj->getString();
						converttolua(L,newarg,IAT_STRING);
					}
					if(obj->isBool()) {
						newarg._boolean = obj->getBool();
						converttolua(L,newarg,IAT_BOOLEAN);
					}
					if(obj->isDouble()) {
						newarg._double = obj->getDouble();
						converttolua(L,newarg,IAT_DOUBLE);
					}
					if(obj->isInt()) {
						newarg._int = obj->getInt();
						converttolua(L,newarg,IAT_INT);
					}
				}
				else {
					OverloadedObject** ptr = (OverloadedObject**) lua_newuserdata(L,sizeof(OverloadedObject*));
					(*ptr) = obj;
					luaL_getmetatable(L,"overloaded");
					lua_setmetatable(L,-2);
				}
				break;
			}
			default:
				assert(false);
		}
		return 1;
	}

	InfArg convertarg(lua_State* L, int n, InfArgType t);

	TypedInfArg convertarg(lua_State* L, int n) {
		TypedInfArg result;
		result._type = typeenum(L,n);
		if(result._type == IAT_DOUBLE) {
			if(isInt(lua_tonumber(L,n))) result._type = IAT_INT;
		}
		result._value = convertarg(L,n,result._type);
		return result;
	}

	InfArg convertarg(lua_State* L, int n, InfArgType t) {
		string nstr = typestring(L,n);
		InfArg a;
		if(nstr != "overloaded") {
			switch(t) {
				case IAT_THEORY:
					a._theory = *((AbstractTheory**)lua_touserdata(L,n));
					break;
				case IAT_STRUCTURE:
					a._structure = *((AbstractStructure**)lua_touserdata(L,n));
					break;
				case IAT_VOCABULARY:
					a._vocabulary = *((Vocabulary**)lua_touserdata(L,n));
					break;
				case IAT_NAMESPACE:
					a._namespace = *((Namespace**)lua_touserdata(L,n));
					break;
				case IAT_OPTIONS:
					a._options = *((InfOptions**)lua_touserdata(L,n));
					break;
				case IAT_INT:
					a._int = int(lua_tonumber(L,n));
					break;
				case IAT_DOUBLE:
					a._double = lua_tonumber(L,n);
					break;
				case IAT_BOOLEAN:
					a._boolean = lua_toboolean(L,n);
					break;
				case IAT_STRING:
					a._string = IDPointer(lua_tostring(L,n));
					break;
				case IAT_SORT:
					a._sort = *((Sort**)lua_touserdata(L,n));
					break;
				case IAT_PREDICATE:
					a._predicate = *((Predicate**)lua_touserdata(L,n));
					break;
				case IAT_FUNCTION:
					a._function = *((Function**)lua_touserdata(L,n));
					break;
				case IAT_TABLE:
				{
					a._table = new vector<TypedInfArg>;
					for(unsigned int i = 1; ; ++i) {
						lua_pushinteger(L,i);
						lua_gettable(L,n);
						if(lua_isnil(L,-1)) {
							lua_pop(L,1);
							break;
						}
						else {
							a._table->push_back(convertarg(L,-1));
							lua_pop(L,1);
						}
					}
					break;
				}
				case IAT_PROCEDURE:
					// TODO
					assert(false);
				default:
					assert(false);
			}
		}
		else {
			OverloadedObject* obj = *((OverloadedObject**)lua_touserdata(L,n)); 
			switch(t) {
				case IAT_THEORY:
					a._theory = obj->getTheory();
					break;
				case IAT_STRUCTURE:
					a._structure = obj->getStructure();
					break;
				case IAT_VOCABULARY:
					a._vocabulary = obj->getVocabulary();
					break;
				case IAT_NAMESPACE:
					a._namespace = obj->getNamespace();
					break;
				case IAT_OPTIONS:
					a._options = obj->getOptions();
					break;
				case IAT_INT:
					a._int = obj->getInt();
					break;
				case IAT_DOUBLE:
					a._double = obj->getDouble();
					break;
				case IAT_BOOLEAN:
					a._boolean = obj->getBool();
					break;
				case IAT_STRING:
					a._string = obj->getString();
					break;
				case IAT_SORT:
					a._sort = obj->getSort();
					break;
				case IAT_PREDICATE:
					a._predicate = obj->getPredicate();
					break;
				case IAT_FUNCTION:
					a._function = obj->getFunction();
					break;
				case IAT_PROCEDURE:
					a._procedure = obj->getProcedure();
					break;
				case IAT_TABLE:
					a._table = obj->getTable();
					break;
				case IAT_OVERLOADED:
					a._overloaded = obj;
					break;
				default:
					assert(false);
			}

		}
		return a;
	}

}

int overloadcall(lua_State* L) {
	InfArg a = BuiltinProcs::convertarg(L,1,IAT_OVERLOADED);
	if(a._overloaded->isProcedure()) {
		lua_getfield(L,LUA_REGISTRYINDEX,a._overloaded->getProcedure()->c_str());
		lua_replace(L,1);
		int nrargs = lua_gettop(L) -1; 
		lua_call(L,nrargs,LUA_MULTRET);
		return lua_gettop(L);
	}
	else {
		Error::notcommand();
		return 0;
	}
}

int idpcall(lua_State* L) {
	// Collect name
	string name = string(lua_tostring(L,1));
	lua_remove(L,1);

	// Try to find correct inference 
	vector<Inference*> pvi = BuiltinProcs::_inferences[name];
	unsigned int nrargs = lua_gettop(L); 
	vector<Inference*> vi;
	for(unsigned int n = 0; n < pvi.size(); ++n) {
		if(nrargs == pvi[n]->arity()) vi.push_back(pvi[n]);
	}
	if(vi.empty()) {
		name += "/" + itos(nrargs);
		Error::unkncommand(name);
		return 0;
	}
	vector<Inference*> vi2;
	for(unsigned int n = 0; n < vi.size(); ++n) {
		bool ok = true;
		for(unsigned int m = 1; m <= nrargs; ++m) {
			ok = BuiltinProcs::checkarg(L,m,(vi[n]->intypes())[m-1]);
			if(!ok) break;
		}
		if(ok) vi2.push_back(vi[n]);
	}
	if(vi2.empty()) {
		Error::wrongcommandargs(name + '/' + itos(nrargs));
		return 0;
	}
	else if(vi2.size() == 1) {
		vector<InfArg> via;
		for(unsigned int m = 1; m <= nrargs; ++m)
			via.push_back(BuiltinProcs::convertarg(L,m,(vi2[0]->intypes())[m-1]));
		TypedInfArg res = vi2[0]->execute(via,L);
		return BuiltinProcs::converttolua(L,res._value,res._type);
	}
	else {
		Error::ambigcommand(name + '/' + itos(nrargs));
		return 0;
	}
}



/*
	Built-in procedures
*/

extern void parsefile(const string&);
TypedInfArg LoadFile::execute(const vector<InfArg>& args, lua_State* L) const {
	parsefile(*(args[0]._string));
	Namespace::global()->toLuaGlobal(L);
	TypedInfArg a; a._type = IAT_NIL; 
	return a;
}

TypedInfArg DeleteData::execute(const vector<InfArg>& args, lua_State*) const {
	switch(_intypes[0]) {
		case IAT_THEORY:
			if(!(args[0]._theory->pi().line())) delete(args[0]._theory);
			break;
		case IAT_STRUCTURE:
			if(!(args[0]._structure->pi().line())) delete(args[0]._structure);
			break;
		case IAT_NAMESPACE:
			if(!(args[0]._namespace->pi().line())) delete(args[0]._namespace);
			break;
		case IAT_VOCABULARY:
			if(!(args[0]._vocabulary->pi().line())) {
				if(args[0]._vocabulary != Namespace::global()->vocabulary("std")) delete(args[0]._vocabulary);
			}
			break;
		case IAT_OPTIONS:
			if(!(args[0]._options->pi().line())) delete(args[0]._options);
			break;
		default:
			assert(false);
	}
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg NewOption::execute(const vector<InfArg>&, lua_State*) const {
	TypedInfArg a; a._type = IAT_OPTIONS;
	a._value._options = new InfOptions("",ParseInfo());
	return a;
}

TypedInfArg PrintTheory::execute(const vector<InfArg>& args, lua_State*) const {
	InfOptions* opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	AbstractTheory* t = args[0]._theory;
	string str = printer->print(t);
	delete(printer);
	TypedInfArg a; a._type = IAT_STRING; a._value._string = IDPointer(str);
	return a;
}

TypedInfArg PrintVocabulary::execute(const vector<InfArg>& args, lua_State*) const {
	InfOptions* opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	string str = printer->print(args[0]._vocabulary);
	delete(printer);
	TypedInfArg a; a._type = IAT_STRING; a._value._string = IDPointer(str);
	return a;
}

TypedInfArg PrintStructure::execute(const vector<InfArg>& args, lua_State*) const {
	InfOptions* opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	AbstractStructure* s = args[0]._structure;
	string str = printer->print(s);
	delete(printer);
	TypedInfArg a; a._type = IAT_STRING; a._value._string = IDPointer(str);
	return a;
}

TypedInfArg PrintNamespace::execute(const vector<InfArg>&, lua_State*) const {
	// TODO
	assert(false);
	string str = string("printing of namespaces is not yet implemented");
	TypedInfArg a; a._type = IAT_STRING; a._value._string = IDPointer(str);
	return a;
}

TypedInfArg PushNegations::execute(const vector<InfArg>& args, lua_State*) const {
	assert(args.size() == 1);
	TheoryUtils::push_negations(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg RemoveEquivalences::execute(const vector<InfArg>& args, lua_State*) const {
	assert(args.size() == 1);
	TheoryUtils::remove_equiv(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg FlattenFormulas::execute(const vector<InfArg>& args, lua_State*) const {
	assert(args.size() == 1);
	TheoryUtils::flatten(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg RemoveEqchains::execute(const vector<InfArg>& args, lua_State*) const {
	assert(args.size() == 1);
	TheoryUtils::remove_eqchains(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

FastMXInference::FastMXInference() {
	_intypes = vector<InfArgType>(3);
	_intypes[0] = IAT_THEORY;
	_intypes[1] = IAT_STRUCTURE;
	_intypes[2] = IAT_OPTIONS;
	_description = "Performs model expansion on the structure given the theory it should satisfy.";
}

class TraceWriter {
	private:
		GroundTranslator*	_translator;
		lua_State*			L;
		string*				_registryindex;
		static int			_tracenr;
		int					_timepoint;

	public:
		TraceWriter(GroundTranslator* trans, lua_State* Ls) : _translator(trans), L(Ls), _timepoint(1) { 
			++_tracenr;
			_registryindex = IDPointer(string("trace") + itos(_tracenr));
			lua_newtable(L);
			lua_setfield(L,LUA_REGISTRYINDEX,_registryindex->c_str());
		}

		void backtrack(int a){
			lua_getfield(L,LUA_REGISTRYINDEX,_registryindex->c_str());
			lua_pushinteger(L,_timepoint);
			++_timepoint;
			lua_newtable(L);
			lua_pushstring(L,"backtrack");
			lua_setfield(L,-2,"type");
			lua_pushinteger(L,a);
			lua_setfield(L,-2,"dl");
			lua_settable(L,-3);
			lua_pop(L,1);
		}

		void propagate(MinisatID::Literal a, int b){
			lua_getfield(L,LUA_REGISTRYINDEX,_registryindex->c_str());
			lua_pushinteger(L,_timepoint);
			++_timepoint;
			lua_newtable(L);
			lua_pushstring(L,"assign");
			lua_setfield(L,-2,"type");
			lua_pushinteger(L,b);
			lua_setfield(L,-2,"dl");
			lua_pushboolean(L,!a.hasSign());
			lua_setfield(L,-2,"value");
			// TODO: change next two lines to push real atoms
			lua_pushstring(L,_translator->printatom(a.getAtom().getValue()).c_str());
			lua_setfield(L,-2,"atom");
			lua_settable(L,-3);
			lua_pop(L,1);
		}

		TypedInfArg trace() const {
			TypedInfArg trace; trace._type = IAT_REGISTRY; trace._value._string = _registryindex;	
			return trace;
		}
};

int TraceWriter::_tracenr = 0;

TypedInfArg FastMXInference::execute(const vector<InfArg>& args, lua_State* L) const {

	// Convert arguments
	AbstractTheory* theory = args[0]._theory;
	AbstractStructure* structure = args[1]._structure;
	InfOptions* opts = args[2]._options;
	
	// Create solver
	MinisatID::SolverOption modes;
	modes.nbmodels = opts->_nrmodels;
	modes.verbosity = opts->_satverbosity;
	modes.remap = false;
	SATSolver* solver = new SATSolver(modes);

	// Create grounder
	GrounderFactory gf(structure);
	TopLevelGrounder* grounder = gf.create(theory,solver);

	// Ground
	grounder->run();
	assert(typeid(*(grounder->grounding())) == typeid(SolverTheory));
	SolverTheory* grounding = dynamic_cast<SolverTheory*>(grounder->grounding());

	// Create monitor
	TraceWriter tracewriter(grounding->translator(),L);
	if(opts->_trace) {
		cb::Callback1<void, int> callbackback(&tracewriter, &TraceWriter::backtrack);
		cb::Callback2<void, MinisatID::Literal, int> callbackprop(&tracewriter, &TraceWriter::propagate);
		MinisatID::Monitor* m = new MinisatID::Monitor();
		m->setBacktrackCB(callbackback);
		m->setPropagateCB(callbackprop);
		solver->addMonitor(m);
	}

	// Add function constraints
	grounding->addFuncConstraints();
	grounding->addFalseDefineds();

	// Solve
	vector<MinisatID::Literal> assumpts;
	MinisatID::ModelExpandOptions options;
	options.nbmodelstofind = modes.nbmodels;
	options.printmodels = MinisatID::PRINT_NONE;
	options.savemodels = MinisatID::SAVE_ALL;
	options.search = MinisatID::MODELEXPAND;
	MinisatID::Solution* sol = new MinisatID::Solution(options);
	solver->solve(sol);

	// Translate
	TypedInfArg a; a._type = IAT_TABLE; a._value._table = new vector<TypedInfArg>();
	if(sol->isSat()){
		for(unsigned int i=0; i<sol->getModels().size(); i++){
			AbstractStructure* mod = structure->clone();
			set<PredInter*>	tobesorted1;
			set<FuncInter*>	tobesorted2;
			for(unsigned int j=0; j<sol->getModels()[i].size(); j++) {
				PFSymbol* pfs = grounding->translator()->symbol((sol->getModels()[i][j].getAtom().getValue()));
				if(pfs && mod->vocabulary()->contains(pfs)) {
					vector<domelement> vd = grounding->translator()->args(sol->getModels()[i][j].getAtom().getValue());
					vector<TypedElement> args = ElementUtil::convert(vd);
					if(pfs->ispred()) {
						mod->inter(pfs)->add(args,!(sol->getModels()[i][j].hasSign()),true);
						tobesorted1.insert(mod->inter(pfs));
					}
					else {
						Function* f = dynamic_cast<Function*>(pfs);
						mod->inter(f)->add(args,!(sol->getModels()[i][j].hasSign()),true);
						tobesorted2.insert(mod->inter(f));
					}
				}
			}
			for(set<PredInter*>::const_iterator it=tobesorted1.begin(); it != tobesorted1.end(); ++it)
				(*it)->sortunique();
			for(set<FuncInter*>::const_iterator it=tobesorted2.begin(); it != tobesorted2.end(); ++it)
				(*it)->sortunique();

			if(opts->_modelformat == MF_TWOVAL) {
				mod->forcetwovalued();
				TypedInfArg b; b._value._structure = mod; b._type = IAT_STRUCTURE;
				a._value._table->push_back(b);
			}
			else if(opts->_modelformat == MF_ALL) {
				// TODO
				TypedInfArg b; b._value._structure = mod; b._type = IAT_STRUCTURE;
				a._value._table->push_back(b);
			}
			else {
				TypedInfArg b; b._value._structure = mod; b._type = IAT_STRUCTURE;
				a._value._table->push_back(b);
			}
		}
	}
	if(opts->_trace) {
		TypedInfArg b; b._type = IAT_MULT; b._value._table = new vector<TypedInfArg>(1,a);
		b._value._table->push_back(tracewriter.trace());
		return b;
	}
	else {
		return a;
	}
}

TypedInfArg StructToTheory::execute(const vector<InfArg>& args, lua_State*) const {
	assert(args.size() == 1);
	AbstractTheory* t = StructUtils::convert_to_theory(args[0]._structure);
	TypedInfArg a; a._type = IAT_THEORY; a._value._theory = t;
	return a;
}

TypedInfArg MoveQuantifiers::execute(const vector<InfArg>& args, lua_State*) const {
	TheoryUtils::move_quantifiers(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg ApplyTseitin::execute(const vector<InfArg>& args, lua_State*) const {
	TheoryUtils::tseitin(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

GroundReduce::GroundReduce() {
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY;
	_intypes[1] = IAT_STRUCTURE;
	_description = "Replace ground atoms in the theory by their truth value in the structure";
}

TypedInfArg GroundReduce::execute(const vector<InfArg>& args, lua_State*) const {
	TheoryUtils::reduce(args[0]._theory,args[1]._structure);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg MoveFunctions::execute(const vector<InfArg>& args, lua_State*) const {
	TheoryUtils::move_functions(args[0]._theory);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg CloneStructure::execute(const vector<InfArg>& args, lua_State*) const {
	TypedInfArg a; 
	a._type = IAT_STRUCTURE; a._value._structure = args[0]._structure->clone();
	return a;
}

TypedInfArg CloneTheory::execute(const vector<InfArg>& args, lua_State*) const {
	TypedInfArg a;
	a._type = IAT_THEORY; a._value._theory = args[0]._theory->clone();
	return a;
}

FastGrounding::FastGrounding() {
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY; 
	_intypes[1] = IAT_STRUCTURE;
	_description = "Ground the theory and structure and store the grounding";
}

TypedInfArg FastGrounding::execute(const vector<InfArg>& args, lua_State*) const {
	GrounderFactory factory(args[1]._structure);
	TopLevelGrounder* g = factory.create(args[0]._theory);
	g->run();
	TypedInfArg a; a._type = IAT_THEORY;
	a._value._theory = g->grounding();
	return a;
}

TypedInfArg ForceTwoValued::execute(const vector<InfArg>& args, lua_State*) const {
	args[0]._structure->forcetwovalued();
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

ChangeVoc::ChangeVoc() {
	_intypes.push_back(IAT_STRUCTURE);
	_intypes.push_back(IAT_VOCABULARY);
	_description = "Change the vocabulary of a structure";
}

TypedInfArg ChangeVoc::execute(const vector<InfArg>& args, lua_State*) const {
	AbstractStructure* str = args[0]._structure;
	Vocabulary* v = args[1]._vocabulary;
	StructUtils::changevoc(str,v);
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

TypedInfArg GetIndex::execute(const vector<InfArg>& args, lua_State* L) const {
	TypedInfArg a;
	switch(_intypes[0]) {
		case IAT_STRUCTURE:
			// TODO
			break;
		case IAT_NAMESPACE:
		{
			Namespace* nsp = args[0]._namespace;
			string str = *(args[1]._string);
			a = nsp->getObject(str,L);
			break;
		}
		case IAT_VOCABULARY:
			// TODO
			break;
		case IAT_OPTIONS:
			a = args[0]._options->get(*(args[1]._string));
			break;
		default:
			assert(false);
	}
	return a;
}

TypedInfArg BDDPrinter::execute(const vector<InfArg>& args, lua_State*) const {
	FOBDDManager manager;
	FOBDDFactory factory(&manager);
	AbstractTheory* theory = args[0]._theory;
	FOBDD* result = manager.truebdd();
	for(unsigned int n = 0; n < theory->nrSentences(); ++n) {
		theory->sentence(n)->accept(&factory);
		result = manager.conjunction(result,factory.bdd());
	}
	// TODO: assert that there are no definitions and no fixpoint definitions
	TypedInfArg a; a._type = IAT_STRING;
	a._value._string = new string(manager.to_string(result));
	return a;
}

SetIndex::SetIndex(InfArgType table, InfArgType key, InfArgType value) {
	_intypes = vector<InfArgType>(3);
	_intypes[0] = table; _intypes[1] = key; _intypes[2] = value;
	_description = "newindex";
}

TypedInfArg SetIndex::execute(const vector<InfArg>& args, lua_State*) const {
	switch(_intypes[0]) {
		case IAT_OPTIONS:
			switch(_intypes[2]) {
				case IAT_DOUBLE:
					if(isInt(args[2]._double)) {
						args[0]._options->set(*(args[1]._string),int(args[2]._double));
					}
					else {
						args[0]._options->set(*(args[1]._string),args[2]._double);
					}
					break;
				case IAT_STRING:
					args[0]._options->set(*(args[1]._string),*(args[2]._string));
					break;
				case IAT_BOOLEAN:
					args[0]._options->set(*(args[1]._string),args[2]._boolean);
					break;
				default:
					assert(false);
			}
			break;
		case IAT_STRUCTURE:
			// TODO
			assert(false);
			break;
		default:
			assert(false);
	}
	TypedInfArg a; a._type = IAT_NIL;
	return a;
}

/*************************
	Overloaded objects
*************************/

bool OverloadedObject::single() const {
	unsigned int count = 0;
	if(isNamespace()) ++count;
	if(isVocabulary()) ++count;
	if(isTheory()) ++count;	
	if(isStructure()) ++count;
	if(isOptions()) ++count;		
	if(isProcedure()) ++count;	
	if(isPredicate()) ++count;	
	if(isFunction()) ++count;	
	if(isSort()) ++count;		
	if(isString()) ++count;
	if(isInt()) ++count;
	if(isBool()) ++count;
	if(isDouble()) ++count;
	if(isTable()) ++count;
	return count <= 1;
}

/*********************
	Lua Procedures
*********************/

int LuaProcCompileNumber = 0;

void LuaProcedure::compile(lua_State* L) {
	if(!iscompiled()) {
		stringstream ss;
		ss << "local " << _name << " = ";
		ss << code() << "\n";
		ss << "return " << _name << "(...)\n";
		luaL_loadstring(L,ss.str().c_str());
		_registryindex = "idp_compiled_procedure_" + itos(LuaProcCompileNumber);
		++LuaProcCompileNumber;
		lua_setfield(L,LUA_REGISTRYINDEX,_registryindex.c_str());
	}
}

TypedInfArg InfOptions::get(const string& opt) const {
	TypedInfArg tia;
	if(opt == "language") {
		tia._type = IAT_STRING;
		switch(_format) {
			case OF_IDP: tia._value._string = IDPointer(string("idp")); break;
			case OF_TXT: tia._value._string = IDPointer(string("txt")); break;
			default: assert(false); tia._type = IAT_NIL; break;
		}
	}
	else if(opt == "modelformat") {
		tia._type = IAT_STRING;
		switch(_modelformat) {
			case MF_ALL: tia._value._string = IDPointer(string("all")); break;
			case MF_TWOVAL: tia._value._string = IDPointer(string("twovalued")); break;
			case MF_THREEVAL: tia._value._string = IDPointer(string("threevalued")); break;
			default: assert(false); tia._type = IAT_NIL; break;
		}
	}
	else if(opt == "nrmodels") {
		tia._type = IAT_INT;
		tia._value._int = _nrmodels;
	}
	else if(opt == "satverbosity") {
		tia._type = IAT_INT;
		tia._value._int = _satverbosity;
	}
	else if(opt == "printtypes") {
		tia._type = IAT_BOOLEAN;
		tia._value._boolean = _printtypes ? true : false;
	}
	else {
		Error::unknopt(opt,0);
		tia._type = IAT_NIL;
	}
	return tia;
}

TypedInfArg Namespace::getObject(const string& str, lua_State* L) const {
	TypedInfArg a;
	OverloadedObject* obj = new OverloadedObject();
	if(isSubspace(str)) obj->makenamespace(subspace(str));
	if(isVocab(str)) obj->makevocabulary(vocabulary(str));
	if(isTheory(str)) obj->maketheory(theory(str));
	if(isStructure(str)) obj->makestructure(structure(str));
	if(isOption(str)) obj->makeoptions(option(str));
	if(isProc(str)) {
		LuaProcedure* proc = procedure(str);
		proc->compile(L);
		obj->makeprocedure(&(proc->registryindex()));
	}
	if(obj->single()) {
		if(obj->isTheory()) {
			a._type = IAT_THEORY;
			a._value._theory = obj->getTheory();
		}
		if(obj->isVocabulary()) {
			a._type = IAT_VOCABULARY;
			a._value._vocabulary = obj->getVocabulary();
		}
		if(obj->isStructure()) {
			a._type = IAT_STRUCTURE;
			a._value._structure = obj->getStructure();
		}
		if(obj->isNamespace()) {
			a._type = IAT_NAMESPACE;
			a._value._namespace = obj->getNamespace();
		}
		if(obj->isOptions()) {
			a._type = IAT_OPTIONS;
			a._value._options = obj->getOptions();
		}
		if(obj->isProcedure()) {
			a._type = IAT_PROCEDURE;
			a._value._procedure = obj->getProcedure();
		}
		delete(obj);
	}
	else {
		a._type = IAT_OVERLOADED;
		a._value._overloaded = obj;
	}
	return a;
}
