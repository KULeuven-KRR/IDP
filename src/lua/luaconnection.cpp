/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "luaconnection.hpp"
#include <set>
#include <iostream>
#include <cstdlib>
#include "IncludeComponents.hpp"
#include "options.hpp"
#include "errorhandling/error.hpp"
#include "printers/print.hpp"
#include "GlobalData.hpp"
#include "external/commands/allcommands.hpp"
#include "monitors/luainteractiveprintmonitor.hpp"
#include "inferences/modelexpansion/LuaTraceMonitor.hpp"
#include "utils/ListUtils.hpp"
#include "insert.hpp"
#include "structure/StructureComponents.hpp"
#include "external/runidp.hpp"
#include "lstate.h"
#include "inferences/makeTwoValued/TwoValuedStructureIterator.hpp"

using namespace std;
using namespace LuaConnection;

extern void parsefile(const string&);

const std::string& InternalProcedure::getName() const {
	return inference_->getName();
}
const std::string& InternalProcedure::getNameSpace() const {
	return inference_->getNamespace();
}
const std::vector<ArgType>& InternalProcedure::getArgumentTypes() const {
	return inference_->getArgumentTypes();
}

int UserProcedure::_compilenumber = 0;
int LuaTraceMonitor::_tracenr = 0;

template<class Arg>
int addUserData(lua_State* l, Arg arg, const std::string& name) {
	Arg* ptr = (Arg*) lua_newuserdata(l, sizeof(Arg));
	(*ptr) = arg;
	luaL_getmetatable(l, name.c_str());
	lua_setmetatable(l, -2);
	return 1;
}

template<class Arg>
int addUserData(lua_State* l, Arg arg, ArgType type) {
	return addUserData(l, arg, toCString(type));
}

int ArgProcNumber = 0; //!< Number to create unique registry indexes
const char* _typefield = "type"; //!< Field index containing the type of userdata

const char* getTypeField() {
	return _typefield;
}

int& argProcNumber() {
	return ArgProcNumber;
}

std::map<ArgType, const char*> argType2Name;
bool init = false;

const char* toCString(ArgType type) {
	if (not init) {
		map_init(argType2Name)(AT_SORT, "type")(AT_PREDICATE, "predicate_symbol")(AT_FUNCTION, "function_symbol")(AT_SYMBOL, "symbol")(AT_VOCABULARY,
				"vocabulary")(AT_COMPOUND, "compound")(AT_TUPLE, "tuple")(AT_DOMAIN, "domain")(AT_PREDTABLE, "predicate_table")(AT_PREDINTER,
				"predicate_interpretation")(AT_FUNCINTER, "function_interpretation")(AT_STRUCTURE, "structure")(AT_TABLEITERATOR, "predicate_table_iterator")(
				AT_DOMAINITERATOR, "domain_iterator")(AT_QUERY, "query")(AT_TERM, "term")(AT_FOBDD, "fobdd")(AT_FORMULA, "formula")(AT_THEORY,
				"theory")(AT_OPTIONS, "options")(AT_NAMESPACE, "namespace")(AT_NIL, "nil")(AT_INT, "number")(AT_DOUBLE, "number")(AT_BOOLEAN, "boolean")(
				AT_STRING, "string")(AT_TABLE, "table")(AT_PROCEDURE, "function")(AT_OVERLOADED, "overloaded")(AT_MULT, "mult")(AT_REGISTRY, "registry")(
				AT_TRACEMONITOR, "tracemonitor")(AT_MODELITERATOR, "mxIterator")(AT_TWOVALUEDITERATOR, "twoValuedIterator");
		init = true;
	}
	return argType2Name.at(type);
}

InternalArgument nilarg() {
	InternalArgument ia;
	ia._type = AT_NIL;
	return ia;
}

/**
 * Get a domain element from the lua stack
 */
const DomainElement* convertToElement(int arg, lua_State* L) {
	switch (lua_type(L, arg)) {
	case LUA_TNIL:
		return NULL;
	case LUA_TSTRING:
		return createDomElem(lua_tostring(L,arg));
	case LUA_TNUMBER:
		return createDomElem(lua_tonumber(L, arg));
	case LUA_TUSERDATA: {
		lua_getmetatable(L, arg);
		lua_getfield(L, -1, _typefield);
		ArgType type = (ArgType) lua_tointeger(L, -1);
		Assert(type != AT_NIL);
		lua_pop(L, 2);
		return type == AT_COMPOUND ? createDomElem(*(Compound**) lua_touserdata(L, arg)) : NULL;
	}
	default:
		return NULL;
	}
}

namespace LuaConnection {

lua_State* _state = NULL;

lua_State* getState() {
	if(_state==NULL){
		cerr <<"THROWING" <<"\n";
		throw IdpException("Lua was not initialized");
	}
	return _state;
}

// Reference counters
map<Structure*, unsigned int> _luastructures;
map<AbstractTheory*, unsigned int> _luatheories;
map<Options*, unsigned int> _luaoptions;

template<typename T>
map<T, unsigned int>& get();

template<>
map<AbstractTheory*, unsigned int>& get<AbstractTheory*>() {
	return _luatheories;
}
template<>
map<Structure*, unsigned int>& get<Structure*>() {
	return _luastructures;
}
template<>
map<Options*, unsigned int>& get<Options*>() {
	return _luaoptions;
}

int InternalProcedure::operator()(lua_State* L) const {
	std::vector<InternalArgument> args;
	for (int arg = 1; arg <= lua_gettop(L); ++arg) {
		args.push_back(createArgument(arg, L));
	}
	if (inference_->needPrintMonitor()) {
		inference_->addPrintMonitor(new LuaInteractivePrintMonitor(L));
	}
	InternalArgument result = inference_->execute(args);
	inference_->clean();
	int out = LuaConnection::convertToLua(L, result);
	if (result._type == AT_STRING) {
		delete(result._value._string);
	}
	return out;
}

std::map<const void*, UserProcedure*> p2name;

void compile(UserProcedure* procedure, lua_State* state) {
	if (not procedure->iscompiled()) {
		// Compose function header, body, and return statement
		stringstream ss;
		ss << "local function " << procedure->name() << "(";
		bool begin = true;
		for (auto it = procedure->args().cbegin(); it != procedure->args().cend(); ++it) {
			if (not begin) {
				ss << ",";
			}
			begin = false;
			ss << *it;
		}
		ss << ')' << procedure->getProcedurecode() << " end\n";
		ss << "return " << procedure->name() << "(...)\n";

		// Compile
		int err = luaL_loadstring(state, ss.str().c_str());
		if (err) {
			stringstream ss;
			ss << string(lua_tostring(state,-1));
			//The error message  is supposed to be of the form ".*\]:(.*):"
			// where the capture group is the line number of the error within
			// the procedure.
			//The following code extracts that line number from the string.
			//As regex are not properly supported in C++, a DFA-style matching
			// is appropriate
			auto s = ss.str();
			auto i = 0;
			while(s.at(i) != ']'){
				i++;
			}
			i+=2;
			auto start = i;
			while(s.at(i) != ':'){
				i++;
			}
			// - 1 cause the start of the procedure is line 1 and already counted in the parsing location
			int lineNb = stoi(s.substr(start,i-start)) - 1;
			 
			
			Error::error(ss.str(), ParseInfo(procedure->pi().linenumber()+lineNb,1,procedure->pi().filename()));
			return;
		}
		procedure->setRegistryIndex("idp_compiled_procedure_" + convertToString(UserProcedure::getCompileNumber()));
		UserProcedure::increaseCompileNumber();
		p2name[lua_topointer(state, -1)]=procedure;
		lua_setfield(state, LUA_REGISTRYINDEX, procedure->registryindex().c_str());
	}
}

/**
 * Push a domain element to the lua stack
 */
int convertToLua(lua_State* L, const DomainElement* d) {
	if (d == NULL) {
		lua_pushnil(L);
		return 1;
	}
	int result = -1;
	switch (d->type()) {
	case DET_INT:
		lua_pushinteger(L, d->value()._int);
		result = 1;
		break;
	case DET_DOUBLE:
		lua_pushnumber(L, d->value()._double);
		result = 1;
		break;
	case DET_STRING:
		lua_pushstring(L, d->value()._string->c_str());
		result = 1;
		break;
	case DET_COMPOUND:
		result = addUserData(L, d->value()._compound, AT_COMPOUND);
		break;
	}
	return result;
}

/**
 * Push an internal argument to the lua stack
 */
int convertToLua(lua_State* L, InternalArgument arg) {
	int result = -1;
	switch (arg._type) {
	case AT_SORT: {
		result = addUserData(L, arg._value._sort, arg._type);
		break;
	}
	case AT_PREDICATE: {
		result = addUserData(L, arg._value._predicate, arg._type);
		break;
	}
	case AT_FUNCTION: {
		result = addUserData(L, arg._value._function, arg._type);
		break;
	}
	case AT_SYMBOL: {
		result = addUserData(L, arg._value._symbol, arg._type);
		break;
	}
	case AT_VOCABULARY: {
		result = addUserData(L, arg._value._vocabulary, arg._type);
		break;
	}
	case AT_COMPOUND: {
		result = addUserData(L, arg._value._compound, arg._type);
		break;
	}
	case AT_TUPLE: {
		result = addUserData(L, arg._value._tuple, arg._type);
		break;
	}
	case AT_DOMAIN: {
		result = addUserData(L, arg._value._domain, arg._type);
		break;
	}
	case AT_PREDTABLE: {
		result = addUserData(L, arg._value._predtable, arg._type);
		break;
	}
	case AT_PREDINTER: {
		result = addUserData(L, arg._value._predinter, arg._type);
		break;
	}
	case AT_FUNCINTER: {
		result = addUserData(L, arg._value._funcinter, arg._type);
		break;
	}
	case AT_STRUCTURE: {
		auto ptr = (Structure**) lua_newuserdata(L, sizeof(Structure*));
		(*ptr) = arg._value._structure;
		luaL_getmetatable(L, toCString(arg._type));
		lua_setmetatable(L, -2);
		if (arg._value._structure->pi().linenumber() == 0) {
			if (_luastructures.find(arg._value._structure) != _luastructures.cend()) {
				++_luastructures[arg._value._structure];
			} else {
				_luastructures[arg._value._structure] = 1;
			}
		}
		result = 1;
		break;
	}
	case AT_TABLEITERATOR: {
		result = addUserData(L, arg._value._tableiterator, arg._type);
		break;
	}
	case AT_DOMAINITERATOR: {
		result = addUserData(L, arg._value._sortiterator, arg._type);
		break;
	}
	case AT_THEORY: {
		auto ptr = (AbstractTheory**) lua_newuserdata(L, sizeof(AbstractTheory*));
		(*ptr) = arg._value._theory;
		luaL_getmetatable(L, toCString(arg._type));
		lua_setmetatable(L, -2);
		if (arg._value._theory->pi().linenumber() == 0) {
			if (_luatheories.find(arg._value._theory) != _luatheories.cend()) {
				++_luatheories[arg._value._theory];
			} else {
				_luatheories[arg._value._theory] = 1;
			}
		}
		result = 1;
		break;
	}
	case AT_FORMULA: {
		result = addUserData(L, arg._value._formula, arg._type);
		break;
	}
	case AT_QUERY: {
		result = addUserData(L, arg._value._query, arg._type);
		break;
	}
	case AT_TERM: {
		result = addUserData(L, arg._value._term, arg._type);
		break;
	}
	case AT_FOBDD: {
		result = addUserData(L, arg._value._fobdd, arg._type);
		break;
	}
	case AT_OPTIONS: {
		auto ptr = (Options**) lua_newuserdata(L, sizeof(Options*));
		(*ptr) = arg._value._options;
		luaL_getmetatable(L, toCString(arg._type));
		lua_setmetatable(L, -2);
		auto options = arg._value._options;
		if (_luaoptions.find(options) != _luaoptions.cend()) {
			_luaoptions[options] += 1;
		} else {
			_luaoptions[options] = 1;
		}
		for(auto suboption: options->getSubOptionBlocks()){ // Sub option blocks have at least as many refs as their parent, but might be passed around separately
			if (_luaoptions.find(suboption) != _luaoptions.cend()) {
				_luaoptions[suboption] += 1;
			} else {
				_luaoptions[suboption] = 1;
			}
		}
		result = 1;
		break;
	}
	case AT_NAMESPACE: {
		result = addUserData(L, arg._value._namespace, arg._type);
		break;
	}
	case AT_NIL: {
		lua_pushnil(L);
		result = 1;
		break;
	}
	case AT_INT: {
		lua_pushinteger(L, arg._value._int);
		result = 1;
		break;
	}
	case AT_DOUBLE: {
		lua_pushnumber(L, arg._value._double);
		result = 1;
		break;
	}
	case AT_BOOLEAN: {
		lua_pushboolean(L, arg._value._boolean);
		result = 1;
		break;
	}
	case AT_STRING: {
		Assert(arg._value._string!=NULL);
		lua_pushstring(L, arg._value._string->c_str());
		result = 1;
		break;
	}
	case AT_TABLE: {
		Assert(arg._value._table!=NULL);
		lua_newtable(L);
		for (size_t n = 0; n < arg._value._table->size(); ++n) {
			lua_pushinteger(L, n + 1);
			convertToLua(L, (*(arg._value._table))[n]);
			lua_settable(L, -3);
		}
		result = 1;
		break;
	}
	case AT_PROCEDURE: {
		Assert(arg._value._string!=NULL);
		lua_getfield(L, LUA_REGISTRYINDEX, arg._value._string->c_str());
		result = 1;
		break;
	}
	case AT_OVERLOADED: {
		result = addUserData(L, arg._value._overloaded, arg._type);
		break;
	}
	case AT_MULT: {
		Assert(arg._value._table!=NULL);
		int nrres = 0;
		for (size_t n = 0; n < arg._value._table->size(); ++n) {
			nrres += convertToLua(L, (*(arg._value._table))[n]);
		}
		result = nrres;
		break;
	}
	case AT_REGISTRY:
		Assert(arg._value._string!=NULL);
		lua_getfield(L, LUA_REGISTRYINDEX, arg._value._string->c_str());
		result = 1;
		break;
	case AT_MODELITERATOR:
		Assert(arg._value._modelIterator!=NULL);
		result = addUserData(L, arg._value._modelIterator, arg._type);
		break;
	case AT_TWOVALUEDITERATOR:
		Assert(arg._value._twoValuedIterator!=NULL);
		result = addUserData(L, arg._value._twoValuedIterator, arg._type);
		break;
	case AT_TRACEMONITOR:
		throw IdpException("Tracemonitors cannot be passed to lua.");
	}
	return result;
}

InternalArgument createArgument(int arg, lua_State* L) {
	InternalArgument ia;
	ia._type = AT_NIL;
	switch (lua_type(L, arg)) {
	case LUA_TNIL:
		ia._type = AT_NIL;
		break;
	case LUA_TBOOLEAN:
		ia._type = AT_BOOLEAN;
		ia._value._boolean = lua_toboolean(L, arg);
		break;
	case LUA_TSTRING:
		ia._type = AT_STRING;
		ia._value._string = new std::string(lua_tostring(L,arg));
		break;
	case LUA_TTABLE:
		if (arg < 0) {
			arg = lua_gettop(L) + arg + 1;
		}
		ia._type = AT_TABLE;
		ia._value._table = new std::vector<InternalArgument>();
		addToGarbageCollection(ia._value._table);
		lua_pushnil(L);
		while (lua_next(L, arg) != 0) {
			ia._value._table->push_back(createArgument(-1, L));
			lua_pop(L, 1);
		}
		break;
	case LUA_TFUNCTION: {
		ia._type = AT_PROCEDURE;
		auto proc = p2name[lua_topointer(L, -1)];
		ia._value._procedure = NULL;
		if(proc){
			ia._value._procedure = proc;
		}
		break;
	}
	case LUA_TNUMBER: {
		if (isInt(lua_tonumber(L, arg))) {
			ia._type = AT_INT;
			ia._value._int = lua_tointeger(L, arg);
		} else {
			ia._type = AT_DOUBLE;
			ia._value._double = lua_tonumber(L, arg);
		}
		break;
	}
	case LUA_TUSERDATA: {
		lua_getmetatable(L, arg);
		lua_getfield(L, -1, getTypeField());
		ia._type = (ArgType) lua_tointeger(L, -1);
		Assert(ia._type != AT_NIL);
		lua_pop(L, 2);
		switch (ia._type) {
		case AT_SORT:
			ia._value._sort = *(std::set<Sort*>**) lua_touserdata(L, arg);
			break;
		case AT_PREDICATE:
			ia._value._predicate = *(std::set<Predicate*>**) lua_touserdata(L, arg);
			break;
		case AT_FUNCTION:
			ia._value._function = *(std::set<Function*>**) lua_touserdata(L, arg);
			break;
		case AT_SYMBOL:
			ia._value._symbol = *(OverloadedSymbol**) lua_touserdata(L, arg);
			break;
		case AT_VOCABULARY:
			ia._value._vocabulary = *(Vocabulary**) lua_touserdata(L, arg);
			break;
		case AT_COMPOUND:
			ia._value._compound = *(Compound**) lua_touserdata(L, arg);
			break;
		case AT_TUPLE:
			ia._value._tuple = *(ElementTuple**) lua_touserdata(L, arg);
			break;
		case AT_DOMAIN:
			ia._value._domain = *(SortTable**) lua_touserdata(L, arg);
			break;
		case AT_PREDTABLE:
			ia._value._predtable = *(PredTable**) lua_touserdata(L, arg);
			break;
		case AT_PREDINTER:
			ia._value._predinter = *(PredInter**) lua_touserdata(L, arg);
			break;
		case AT_FUNCINTER:
			ia._value._funcinter = *(FuncInter**) lua_touserdata(L, arg);
			break;
		case AT_STRUCTURE:
			ia._value._structure = *(Structure**) lua_touserdata(L, arg);
			break;
		case AT_TABLEITERATOR:
			ia._value._tableiterator = *(TableIterator**) lua_touserdata(L, arg);
			break;
		case AT_DOMAINITERATOR:
			ia._value._sortiterator = *(SortIterator**) lua_touserdata(L, arg);
			break;
		case AT_THEORY:
			ia._value._theory = *(AbstractTheory**) lua_touserdata(L, arg);
			break;
		case AT_FORMULA:
			ia._value._formula = *(Formula**) lua_touserdata(L, arg);
			break;
		case AT_QUERY:
			ia._value._query = *(Query**) lua_touserdata(L, arg);
			break;
		case AT_TERM:
			ia._value._term = *(Term**) lua_touserdata(L, arg);
			break;
		case AT_FOBDD:
			ia._value._fobdd = *(FOBDD**) lua_touserdata(L,arg);
			break;
		case AT_OPTIONS:
			ia._value._options = *(Options**) lua_touserdata(L, arg);
			break;
		case AT_NAMESPACE:
			ia._value._namespace = *(Namespace**) lua_touserdata(L, arg);
			break;
		case AT_OVERLOADED:
			ia._value._overloaded = *(OverloadedObject**) lua_touserdata(L, arg);
			break;
		case AT_MODELITERATOR:
			ia._value._modelIterator = *(WrapModelIterator**) lua_touserdata(L, arg);
			break;
		case AT_TWOVALUEDITERATOR:
			ia._value._twoValuedIterator = *(TwoValuedStructureIterator**) lua_touserdata(L, arg);
			break;
		default:
			throw IdpException("Encountered a lua USERDATA for which not internal type exists (or it is not handled correctly).");
		}
		break;
	}
	case LUA_TTHREAD:
		throw IdpException("Invalid request to create a lua object from a THREAD object.");
	case LUA_TLIGHTUSERDATA:
		throw IdpException("Invalid request to create a lua object from a LIGHTUSERDATA object.");
	case LUA_TNONE:
		throw IdpException("Invalid request to create a lua object from an empty object.");
	}
	return ia;
}

/**
 * Get all argument types of an object on the lua stack
 *
 * \param arg	the index in the lua stack
 */
vector<ArgType> getArgTypes(lua_State* L, unsigned int arg) {
	vector<ArgType> result;
	switch (lua_type(L, arg)) {
	case LUA_TNIL:
		result.push_back(AT_NIL);
		break;
	case LUA_TBOOLEAN:
		result.push_back(AT_BOOLEAN);
		break;
	case LUA_TSTRING:
		result.push_back(AT_STRING);
		break;
	case LUA_TTABLE:
		result.push_back(AT_TABLE);
		break;
	case LUA_TFUNCTION:
		result.push_back(AT_PROCEDURE);
		break;
	case LUA_TNUMBER: {
		if (isInt(lua_tonumber(L, arg))) {
			result.push_back(AT_INT);
		} else {
			result.push_back(AT_DOUBLE);
		}
		break;
	}
	case LUA_TUSERDATA: {
		lua_getmetatable(L, arg);
		lua_getfield(L, -1, _typefield);
		ArgType type = (ArgType) lua_tointeger(L, -1);
		if (type == AT_OVERLOADED) {
			OverloadedObject* oo = *(OverloadedObject**) lua_touserdata(L, arg);
			result = oo->types();
		} else if (type == AT_SYMBOL) {
			OverloadedSymbol* os = *(OverloadedSymbol**) lua_touserdata(L, arg);
			result = os->types();
		} else {
			result.push_back(type);
		}
		lua_pop(L, 2);
		break;
	}
	case LUA_TTHREAD:
		throw IdpException("Invalid request to create a lua object from a THREAD object.");
	case LUA_TLIGHTUSERDATA:
		throw IdpException("Invalid request to create a lua object from a LIGHTUSERDATA object.");
	case LUA_TNONE:
		throw IdpException("Invalid request to create a lua object from an empty object.");
	}
	return result;
}

void errorNoSuchProcedure(const vector<vector<ArgType> >& passedtypes, map<vector<ArgType>, InternalProcedure*> const * const procs) {
	string name = procs->begin()->second->getName();
	stringstream ss;
	ss << "There is no procedure " << name << " with the provided arguments <";
	bool begini = true;
	for (auto i = passedtypes.cbegin(); i < passedtypes.cend(); ++i) {
		if (not begini) {
			ss << ",";
		}
		begini = false;
		bool beginj = true;
		for (auto j = i->cbegin(); j < i->cend(); ++j) {
			if (not beginj) {
				ss << "/";
			}
			beginj = false;
			ss << toCString(*j);
		}
	}
	ss << ">\n";
	ss << "Did you intend to use:\n";
	for (auto i = procs->begin(); i != procs->end(); ++i) {
		ss << "\t" << name << "(";
		bool begin = true;
		for (auto j = (*i).second->getArgumentTypes().cbegin(); j != (*i).second->getArgumentTypes().cend(); ++j) {
			if (not begin) {
				ss << ", ";
			}
			begin = false;
			ss << toCString(*j);
		}
		ss << ")\n";
	}
	ss << "\n";
	Error::error(ss.str());
}

/*
 "call": called when Lua calls a value.
 function function_event (func, ...)
 if type(func) == "function" then
 return func(...)   -- primitive call
 else
 local h = metatable(func).__call
 if h then
 return h(func, ...)
 else
 error(...)
 end
 end
 end
 */

int internalCall(lua_State* L) {
	// get the list of possible procedures (with the associated name?)
	map<vector<ArgType>, InternalProcedure*>* procs = *(map<vector<ArgType>, InternalProcedure*>**) lua_touserdata(L, 1);
	Assert(not procs->empty());
	//otherwise lua should have thrown an exception

	lua_remove(L, 1); // The function itself is the first argument

	// get the list of possible argument types
	vector<vector<ArgType> > argtypes;
	for (int luaindex = 1; luaindex <= lua_gettop(L); ++luaindex) {
		argtypes.push_back(getArgTypes(L, luaindex));
	}

	// find the right procedure
	InternalProcedure* proc = NULL;
	vector<vector<ArgType>::iterator> carry;
	for (auto i = argtypes.begin(); i < argtypes.end(); ++i) {
		carry.push_back((*i).begin());
	}
	while (true) {
		vector<ArgType> currtypes;
		for (auto i = carry.cbegin(); i < carry.cend(); ++i) {
			currtypes.push_back(**i);
		}

		if (procs->find(currtypes) != procs->end()) {
			if (proc != NULL) {
				Error::ambigcommand(proc->getName());
				return 0;
			}
			proc = (*procs)[currtypes];
		}

		//Code generates all possible combinations over argument types
		bool newcombination = false;
		for (size_t i = 0; not newcombination && i < argtypes.size(); ++i) {
			++carry[i];
			if (carry[i] != argtypes[i].cend()) {
				newcombination = true;
			} else {
				carry[i] = argtypes[i].begin();
			}
		}
		if (not newcombination) {
			break;
		}
	}

	if (proc == NULL) {
		errorNoSuchProcedure(argtypes, procs);
		throw NoSuchProcedureException();
	}

	return (*proc)(L);
}

/**********************
 * Garbage collection
 **********************/

template<class T, class List >
void reduceCounter(T t, List& list){
	auto it2 = list.find(t);
	if (it2 != list.cend()) {
		--(it2->second);
		if ((it2->second) == 0) {
			list.erase(t);
			delete (t);
		}
	}
}

template<typename T>
int garbageCollect(lua_State* L) {
	auto t = *(T*) lua_touserdata(L, 1);
	auto& list = get<T>();
	reduceCounter(t, list);
	return 0;
}

template<>
int garbageCollect<Options*>(lua_State* L) {
	auto options = *(Options**) lua_touserdata(L, 1);
	auto& list = get<Options*>();
	for(auto suboptions: options->getSubOptionBlocks()){
		reduceCounter(suboptions, list);
	}
	reduceCounter(options, list);
	return 0;
}

template<>
int garbageCollect<AbstractTheory*>(lua_State* L) {
	auto t = *(AbstractTheory**) lua_touserdata(L, 1);
	auto& list = get<AbstractTheory*>();
	auto it = list.find(t);
	if (it != list.cend()) {
		--(it->second);
		if ((it->second) == 0) {
			list.erase(t);
			t->recursiveDelete();
		}
	}
	return 0;
}

template<typename T>
int garbageCollect(T obj) {
	delete (obj);
	return 0;
}

// FIXME commented garbage collection?
// TODO cleanup garbage collection
int gcInternProc(lua_State* L) {
	auto t = *(map<vector<ArgType>, InternalProcedure*>**) lua_touserdata(L, 1);
	deleteList(*t);
	delete (t);
	return 0;
}
int gcSort(lua_State* L) {
	return garbageCollect(*(set<Sort*>**) lua_touserdata(L, 1));
}
int gcPredicate(lua_State* L) {
	return garbageCollect(*(set<Predicate*>**) lua_touserdata(L, 1));
}
int gcFunction(lua_State* L) {
	return garbageCollect(*(set<Function*>**) lua_touserdata(L, 1));
}
int gcSymbol(lua_State* L) {
	return garbageCollect(*(OverloadedSymbol**) lua_touserdata(L, 1));
}
int gcVocabulary(lua_State*) {
	return 0;
}
int gcCompound(lua_State*) {
	return 0;
}
int gcTuple(lua_State*) {
	return 0;
}
int gcTableIterator(lua_State* L) {
	return garbageCollect(*(TableIterator**) lua_touserdata(L, 1));
}
int gcDomainIterator(lua_State* L) {
	return garbageCollect(*(SortIterator**) lua_touserdata(L, 1));
}
int gcPredTable(lua_State*) {
	return 0;
}
int gcPredInter(lua_State*) {
	return 0;
}
int gcFuncInter(lua_State*) {
	return 0;
}
int gcNamespace(lua_State*) {
	return 0;
}
int gcOverloaded(lua_State* L) {
	return garbageCollect(*(OverloadedObject**) lua_touserdata(L, 1));
}

int gcDomain(lua_State*) {
	// TODO return garbageCollect(*(SortTable**) lua_touserdata(L, 1));
	return 0;
}
int gcStructure(lua_State* L) {
	return garbageCollect<Structure*>(L);
}

/**
 * Garbage collection for theories
 */
int gcTheory(lua_State* L) {
	return garbageCollect<AbstractTheory*>(L);
}

int gcFormula(lua_State*) {
	// TODO
	return 0;
}

int gcQuery(lua_State*) {
	// TODO
	return 0;
}

int gcTerm(lua_State*) {
	// TODO
	return 0;
}
int gcFobdd(lua_State*) {
	// TODO
	return 0;
}

int gcMXIterator(lua_State* L) {
	return garbageCollect(*(WrapModelIterator**) lua_touserdata(L, 1));
}

int gcTwoValuedIterator(lua_State* L) {
	return garbageCollect(*(TwoValuedStructureIterator**) lua_touserdata(L, 1));
}

/**
 * Garbage collection for options
 */
int gcOptions(lua_State* L) {
	return garbageCollect<Options*>(L);
}

/**
 * Index function for predicate symbols
 */
int predicateIndex(lua_State* L) {
	set<Predicate*>* pred = *(set<Predicate*>**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	if (index._type == AT_SORT) {
		set<Sort*>* sort = index.sort();
		set<Predicate*>* newpred = new set<Predicate*>();
		for (auto it = sort->begin(); it != sort->end(); ++it) {
			for (auto jt = pred->begin(); jt != pred->end(); ++jt) {
				if ((*jt)->arity() == 1) {
					auto newp = (*jt)->resolve(vector<Sort*>(1, (*it)));
					if (newp!=NULL) {
						newpred->insert(newp);
					}
				}
			}
		}
		InternalArgument np(newpred);
		return convertToLua(L, np);
	} else if (index._type == AT_TABLE) {
		vector<InternalArgument>* table = index._value._table;
		for (auto it = table->begin(); it != table->end(); ++it) {
			if (it->_type != AT_SORT) {
				lua_pushstring(L, "A predicate can only be indexed by a tuple of types");
				return lua_error(L);
			}
		}
		set<Predicate*>* newpred = new set<Predicate*>();
		vector<set<Sort*>::iterator> carry(table->size());
		for (size_t n = 0; n < table->size(); ++n) {
			carry[n] = (*table)[n].sort()->begin();
		}
		while (true) {
			vector<Sort*> currsorts(table->size());
			for (size_t n = 0; n < table->size(); ++n) {
				currsorts[n] = *(carry[n]);
			}
			for (auto it = pred->begin(); it != pred->end(); ++it) {
				if ((*it)->arity() == table->size()) {
					auto newp = (*it)->resolve(currsorts);
					if (newp!=NULL) {
						newpred->insert(newp);
					}
				}
			}
			size_t c = 0;
			for (; c < table->size(); ++c) {
				++(carry[c]);
				if (carry[c] != (*table)[c].sort()->end()) {
					break;
				} else {
					carry[c] = (*table)[c].sort()->begin();
				}
			}
			if (c == table->size()) {
				break;
			}
		}
		InternalArgument np(newpred);
		return convertToLua(L, np);
	} else {
		lua_pushstring(L, "A predicate can only be indexed by a tuple of types");
		return lua_error(L);
	}
}

/**
 * Index function for function symbols
 */
int functionIndex(lua_State* L) {
	set<Function*>* func = *(set<Function*>**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	if (index._type == AT_TABLE) {
		vector<InternalArgument>* table = index._value._table;
		vector<InternalArgument> newtable;
		if (table->size() < 1) {
			lua_pushstring(L, "Invalid function symbol index");
			return lua_error(L);
		}
		for (size_t n = 0; n < table->size(); ++n) {
			if ((*table)[n]._type == AT_SORT) {
				newtable.push_back((*table)[n]);
			} else {
				lua_pushstring(L, "A function symbol can only be indexed by a tuple of types");
				return lua_error(L);
			}
		}
		set<Function*>* newfunc = new set<Function*>();
		vector<set<Sort*>::iterator> carry(newtable.size());
		for (size_t n = 0; n < newtable.size(); ++n) {
			carry[n] = newtable[n].sort()->begin();
		}
		while (true) {
			vector<Sort*> currsorts(newtable.size());
			for (size_t n = 0; n < newtable.size(); ++n) {
				currsorts[n] = *(carry[n]);
			}
			for (auto f : *func) {
				if (f->arity() == newtable.size()-1) {
					auto newf = f->resolve(currsorts);
					if (newf!=NULL) {
						newfunc->insert(newf);
					}
				}
			}
			size_t c = 0;
			for (; c < newtable.size(); ++c) {
				++(carry[c]);
				if (carry[c] != newtable[c].sort()->end()) {
					break;
				} else {
					carry[c] = newtable[c].sort()->begin();
				}
			}
			if (c == newtable.size()) {
				break;
			}
		}
		InternalArgument nf(newfunc);
		return convertToLua(L, nf);
	} else {
		lua_pushstring(L, "A function can only be indexed by a tuple of sorts");
		return lua_error(L);
	}
}

/**
 * Index function for symbols
 */
int symbolIndex(lua_State* L) {
	OverloadedSymbol* symb = *(OverloadedSymbol**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	if (index._type == AT_TABLE) {
		if (index._value._table->size() > 2 && (*(index._value._table))[index._value._table->size() - 2]._type == AT_PROCEDURE) {
			convertToLua(L, InternalArgument(new set<Function*>(*(symb->funcs()))));
			lua_replace(L, 1);
			return functionIndex(L);
		} else {
			convertToLua(L, InternalArgument(new set<Predicate*>(*(symb->preds()))));
			lua_replace(L, 1);
			return predicateIndex(L);
		}
	} else if (index._type == AT_STRING) {
		string str = *(index._value._string);
		delete(index._value._string);
		if (str == "type") {
			convertToLua(L, InternalArgument(new set<Sort*>(*(symb->sorts()))));
			return 1;
		} else if (str == "pred") {
			convertToLua(L, InternalArgument(new set<Predicate*>(*(symb->preds()))));
			return 1;
		} else if (str == "func") {
			convertToLua(L, InternalArgument(new set<Function*>(*(symb->funcs()))));
			return 1;
		} else {
			lua_pushstring(L, "A symbol can only be indexed by a tuple of sorts or the strings \"type\", \"pred\", or \"func\".");
			return lua_error(L);
		}
	} else {
		lua_pushstring(L, "A symbol can only be indexed by a tuple of sorts or the strings \"type\", \"pred\", or \"func\".");
		return lua_error(L);
	}
}

/**
 * Index function for vocabularies
 */
int vocabularyIndex(lua_State* L) {
	auto voc = *(Vocabulary**) lua_touserdata(L, 1);
	auto index = createArgument(2, L);
	if (index._type != AT_STRING) {
		lua_pushstring(L, "A vocabulary can only be indexed by a string");
		return lua_error(L);
	}
	unsigned int emptycounter = 0;
	auto sort = voc->sort(*(index._value._string));
	if (sort == NULL) {
		++emptycounter;
	}
	auto preds = voc->pred_no_arity(*(index._value._string));
	if (preds.empty()) {
		++emptycounter;
	}
	auto funcs = voc->func_no_arity(*(index._value._string));
	if (funcs.empty()) {
		++emptycounter;
	}
	if (emptycounter == 3) {
		return 0;
	} else if (emptycounter == 2) {
		if (sort != NULL) {
			auto newsorts = new set<Sort*> { sort };
			return convertToLua(L, InternalArgument(newsorts));
		} else if (not preds.empty()) {
			auto newpreds = new set<Predicate*>(preds);
			return convertToLua(L, InternalArgument(newpreds));
		} else {
			Assert(not funcs.empty());
			auto newfuncs = new set<Function*>(funcs);
			return convertToLua(L, InternalArgument(newfuncs));
		}
	} else {
		auto os = new OverloadedSymbol();
		if (sort != NULL) {
			os->insert(sort);
		}
		for (auto it = preds.cbegin(); it != preds.cend(); ++it) {
			os->insert(*it);
		}
		for (auto it = funcs.cbegin(); it != funcs.cend(); ++it) {
			os->insert(*it);
		}
		return convertToLua(L, InternalArgument(os));
	}
}

/**
 * Index function for tuples
 */
int tupleIndex(lua_State* L) {
	ElementTuple* tuple = *(ElementTuple**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	if (index._type == AT_INT) {
		const DomainElement* element = (*tuple)[index._value._int - 1];
		return convertToLua(L, element);
	} else {
		lua_pushstring(L, "A tuple can only be indexed by an integer");
		return lua_error(L);
	}
}

/**
 * Length operator for tuples
 */
int tupleLen(lua_State* L) {
	ElementTuple* tuple = *(ElementTuple**) lua_touserdata(L, 1);
	int length = tuple->size();
	lua_pushinteger(L, length);
	return 1;
}

/**
 * Index function for predicate interpretations
 */
int predinterIndex(lua_State* L) {
	PredInter* predinter = *(PredInter**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	if (index._type == AT_STRING) {
		string str = *(index._value._string);
		if (str == "ct") {
			InternalArgument tab(predinter->ct());
			return convertToLua(L, tab);
		} else if (str == "pt") {
			InternalArgument tab(predinter->pt());
			return convertToLua(L, tab);
		} else if (str == "cf") {
			InternalArgument tab(predinter->cf());
			return convertToLua(L, tab);
		} else if (str == "pf") {
			InternalArgument tab(predinter->pf());
			return convertToLua(L, tab);
		} else {
			lua_pushstring(L, "A predicate interpretation can only be indexed by \"ct\", \"cf\", \"pt\", and \"pf\"");
			return lua_error(L);
		}
	} else {
		lua_pushstring(L, "A predicate interpretation can only be indexed by a string");
		return lua_error(L);
	}
}

/**
 * Index function for function interpretations
 */
int funcinterIndex(lua_State* L) {
	FuncInter* funcinter = *(FuncInter**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	if (index._type == AT_STRING) {
		if (*(index._value._string) == "graph") {
			InternalArgument predinter(funcinter->graphInter());
			return convertToLua(L, predinter);
		} else {
			lua_pushstring(L, "A function interpretation can only be indexed by \"graph\"");
			return lua_error(L);
		}
	} else {
		lua_pushstring(L, "A function interpretation can only be indexed by a string");
		return lua_error(L);
	}
}
/**
 * Index function for structures
 */
int structureIndex(lua_State* L) {
	Structure* structure = *(Structure**) lua_touserdata(L, 1);
	InternalArgument symbol = createArgument(2, L);
	switch (symbol._type) {
	case AT_SORT: {
		set<Sort*>* ss = symbol._value._sort;
		if (ss->size() == 1) {
			Sort* s = *(ss->begin());
			SortTable* result = structure->inter(s);
			return convertToLua(L, InternalArgument(result));
		}
		break;
	}
	case AT_PREDICATE: {
		set<Predicate*>* sp = symbol._value._predicate;
		if (sp->size() == 1) {
			Predicate* p = *(sp->begin());
			PredInter* result = structure->inter(p);
			return convertToLua(L, InternalArgument(result));
		}
		break;
	}
	case AT_FUNCTION: {
		set<Function*>* sf = symbol._value._function;
		if (sf->size() == 1) {
			Function* f = *(sf->begin());
			FuncInter* result = structure->inter(f);
			return convertToLua(L, InternalArgument(result));
		}
		break;
	}
	default:
		break;
	}
	lua_pushstring(L, "A structure can only be indexed by a single type, predicate, or function symbol");
	return lua_error(L);
}

InternalArgument getValue(Options* opts, const string& name) {
	if (opts->isOptionOfType<int>(name)) {
		return InternalArgument(opts->getValueOfType<int>(name));
	} else if (opts->isOptionOfType<std::string>(name)) {
		return InternalArgument(new std::string(opts->getValueOfType<std::string>(name)));
	} else if (opts->isOptionOfType<bool>(name)) {
		return InternalArgument(opts->getValueOfType<bool>(name));
	} else if (opts->isOptionOfType<double>(name)) {
		return InternalArgument(opts->getValueOfType<double>(name));
	} else if (opts->isOptionOfType<Options*>(name)) {
		return InternalArgument(opts->getValueOfType<Options*>(name));
	} else {
		throw IdpException("Requesting non-existing option " + name);
	}
}

/**
 * Index function for options
 */
int optionsIndex(lua_State* L) {
	Options* opts = *(Options**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	if (index._type == AT_STRING) {
		// TODO remove getvalue returning internalargument from options
		// instead, add options isIntOption, getIntValue, ...
		return convertToLua(L, getValue(opts, *(index._value._string)));
	} else {
		lua_pushstring(L, "Options can only be indexed by a string");
		return lua_error(L);
	}
}

/**
 * Index function for namespaces
 */
int namespaceIndex(lua_State* L) {
	Namespace* ns = *(Namespace**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	if (index._type != AT_STRING) {
		lua_pushstring(L, "Namespaces can only be indexed by strings");
		return lua_error(L);
	}
	string str = *(index._value._string);
	delete(index._value._string);
	unsigned int counter = 0;
	Namespace* subsp = NULL;
	if (ns->isSubspace(str)) {
		subsp = ns->subspace(str);
		++counter;
	}
	Vocabulary* vocab = NULL;
	if (ns->isVocab(str)) {
		vocab = ns->vocabulary(str);
		++counter;
	}
	AbstractTheory* theo = NULL;
	if (ns->isTheory(str)) {
		theo = ns->theory(str);
		++counter;
	}
	Structure* structure = NULL;
	if (ns->isStructure(str)) {
		structure = ns->structure(str);
		++counter;
	}
	Options* opts = NULL;
	UserProcedure* proc = NULL;
	if (ns->isProc(str)) {
		proc = ns->procedure(str);
		++counter;
	}
	Query* query = NULL;
	if (ns->isQuery(str)) {
		query = ns->query(str);
		++counter;
	}
	Term* term = NULL;
	if (ns->isTerm(str)) {
		term = ns->term(str);
		++counter;
	}

	if (counter == 0) {
		return 0;
	}
	if (counter > 1) {
		OverloadedObject* oo = new OverloadedObject();
		oo->insert(subsp);
		oo->insert(vocab);
		oo->insert(theo);
		oo->insert(structure);
		oo->insert(opts);
		oo->insert(proc);
		oo->insert(query);
		oo->insert(term);
		return convertToLua(L, InternalArgument(oo));
	}

	Assert(counter==1);
	// Only one element on the stack
	if (subsp) {
		return convertToLua(L, InternalArgument(subsp));
	}
	if (vocab) {
		return convertToLua(L, InternalArgument(vocab));
	}
	if (theo) {
		return convertToLua(L, InternalArgument(theo));
	}
	if (structure) {
		return convertToLua(L, InternalArgument(structure));
	}
	if (opts) {
		return convertToLua(L, InternalArgument(opts));
	}
	if (proc) {
		compile(proc, L);
		lua_getfield(L, LUA_REGISTRYINDEX, proc->registryindex().c_str());
		return 1;
	}
	if (query) {
		return convertToLua(L, InternalArgument(query));
	}
	if (term) {
		return convertToLua(L, InternalArgument(term));
	}
	throw IdpException("Some element could not be transformed into a lua object.");
}

SortTable* toDomain(vector<InternalArgument>* table, lua_State* L) {
	auto st = TableUtils::createSortTable();
	for (auto it = table->begin(); it != table->end(); ++it) {
		switch (it->_type) {
		case AT_INT:
			st->add(createDomElem(it->_value._int));
			break;
		case AT_DOUBLE:
			st->add(createDomElem(it->_value._double));
			break;
		case AT_STRING:
			st->add(createDomElem(*it->_value._string));
			break;
		case AT_COMPOUND:
			st->add(createDomElem(it->_value._compound));
			break;
		default:
			delete (st);
			lua_pushstring(L, "Only numbers, strings, and compounds are allowed in a predicate table");
			lua_error(L);
			return 0;
		}
	}
	return st;
}

PredTable* toPredTable(vector<InternalArgument>* table, lua_State* L, const Universe& univ) {
	auto pt = TableUtils::createPredTable(univ);
	for (auto it = table->begin(); it != table->end(); ++it) {
		if (it->_type == AT_TABLE) {
			ElementTuple tuple;
			for (auto jt = it->_value._table->begin(); jt != it->_value._table->end(); ++jt) {
				switch (jt->_type) {
				case AT_INT:
					tuple.push_back(createDomElem(jt->_value._int));
					break;
				case AT_DOUBLE:
					tuple.push_back(createDomElem(jt->_value._double));
					break;
				case AT_STRING:
					tuple.push_back(createDomElem(*jt->_value._string));
					break;
				case AT_COMPOUND:
					tuple.push_back(createDomElem(jt->_value._compound));
					break;
				default:
					lua_pushstring(L, "Only numbers, strings, and compounds are allowed in a predicate table");
					lua_error(L);
					return 0;
				}
			}
			pt->add(tuple);
		} else if (it->_type == AT_TUPLE) {
			pt->add(*(it->_value._tuple));
		} else {
			lua_pushstring(L, "Expected a two-dimensional table");
			lua_error(L);
			return 0;
		}
	}
	return pt;
}

/**
 * NewIndex function for predicate interpretations
 */
int predinterNewIndex(lua_State* L) {
	PredInter* predinter = *(PredInter**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	InternalArgument value = createArgument(3, L);
	const Universe& univ = predinter->ct()->universe();
	if (index._type == AT_STRING) {
		const PredTable* pt = 0;
		if (value._type == AT_PREDTABLE) {
			pt = value._value._predtable;
		} else if (value._type == AT_TABLE) {
			pt = toPredTable(value._value._table, L, univ);
		} else {
			lua_pushstring(L, "Wrong argument to __newindex procedure of a predicate interpretation");
			return lua_error(L);
		}
		Assert(pt);
		string str = *(index._value._string);
		if (str == "ct") {
			predinter->ct(new PredTable(pt->internTable(), univ));
		} else if (str == "pt") {
			predinter->pt(new PredTable(pt->internTable(), univ));
		} else if (str == "cf") {
			predinter->cf(new PredTable(pt->internTable(), univ));
		} else if (str == "pf") {
			predinter->pf(new PredTable(pt->internTable(), univ));
		} else {
			lua_pushstring(L, "A predicate interpretation can only be indexed by \"ct\", \"cf\", \"pt\", and \"pf\"");
			return lua_error(L);
		}
		return 0;
	} else {
		lua_pushstring(L, "A predicate interpretation can only be indexed by a string");
		return lua_error(L);
	}
}

/**
 * NewIndex function for function interpretations
 */
int funcinterNewIndex(lua_State* L) {
	FuncInter* funcinter = *(FuncInter**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	InternalArgument value = createArgument(3, L);
	if (index._type == AT_STRING) {
		if ((*(index._value._string)) == "graph") {
			if (value._type == AT_PREDINTER) {
				funcinter->graphInter(value._value._predinter);
				return 0;
			} else {
				lua_pushstring(L, "Expected a predicate interpretation");
				return lua_error(L);
			}
		} else {
			lua_pushstring(L, "A function interpretation can only be indexed by \"graph\"");
			return lua_error(L);
		}
	} else {
		lua_pushstring(L, "A function interpretation can only be indexed by a string");
		return lua_error(L);
	}
}

/**
 * NewIndex function for structures
 */
int structureNewIndex(lua_State* L) {
	auto structure = *(Structure**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	InternalArgument value = createArgument(3, L);
	switch (index._type) {
	case AT_SORT: {
		auto ss = index._value._sort;
		if (ss->size() == 1) {
			auto s = *(ss->begin());
			if (value._type == AT_DOMAIN) {
				auto st = structure->inter(s);
				st->internTable(value._value._domain->internTable());
				return 0;
			} else if (value._type == AT_TABLE) {
				auto dom = toDomain(value._value._table, L);
				auto st = structure->inter(s);
				st->internTable(dom->internTable());
				delete (dom);
				return 0;
			} else {
				lua_pushstring(L, "Expected a table or a domain");
				return lua_error(L);
			}
		}
		break;
	}
	case AT_PREDICATE: {
		auto sp = index._value._predicate;
		if (sp->size() == 1) {
			auto p = *(sp->begin());
			if (value._type == AT_PREDINTER) {
				structure->changeInter(p, value._value._predinter->clone(structure->universe(p)));
				return 0;
			} else {
				lua_pushstring(L, "Expected a predicate interpretation");
				return lua_error(L);
			}
		}
		break;
	}
	case AT_FUNCTION: {
		auto sf = index._value._function;
		if (sf->size() == 1) {
			auto f = *(sf->begin());
			if (value._type == AT_FUNCINTER) {
				structure->changeInter(f, value._value._funcinter->clone(structure->universe(f)));
				return 0;
			} else {
				lua_pushstring(L, "Expected a function interpretation");
				return lua_error(L);
			}
		}
		break;
	}
	default:
		break;
	}
	lua_pushstring(L, "A structure can only be indexed by a single type, predicate, or function symbol");
	return lua_error(L);
}

int invalidOption(Options* options, lua_State* L, const string& option, const string& value) {
	stringstream ss;
	ss << "\"" << value << "\" is not a valid value for option " << option << ".\n";
	ss << options->printAllowedValues(option) << ".\n";
	lua_pushstring(L, ss.str().c_str());
	return lua_error(L);
}

template<class T>
int attempToSetValue(lua_State* L, Options* opts, const string& option, const T& value) {
	if (not opts->isAllowedValue(option, value)) {
		stringstream ss;
		ss << value;
		return invalidOption(opts, L, option, ss.str());
	}
	opts->setValue(option, value);
	return 0;
}

/**
 * FIXME what does it do?
 */
int optionsNewIndex(lua_State* L) {
	Options* opts = *(Options**) lua_touserdata(L, 1);
	InternalArgument index = createArgument(2, L);
	InternalArgument value = createArgument(3, L);
	if (index._type != AT_STRING) {
		lua_pushstring(L, "Options can only be indexed by a string");
		return lua_error(L);
	}

	string option = *(index._value._string);
	delete(index._value._string);
	if (not opts->isOption(option)) {
		stringstream ss;
		ss << "There is no option named " << option << ".\n";
		lua_pushstring(L, ss.str().c_str());
		return lua_error(L);
	}
	switch (value._type) {
	case AT_INT:
		return attempToSetValue(L, opts, option, value._value._int);
		/*case AT_DOUBLE: // TODO currently there are no float options
		 return attempToSetValue(L, opts, option, value._value._double);*/
	case AT_STRING:
	{
		string str2 = *(value._value._string);
		delete(value._value._string);
		return attempToSetValue(L, opts, option, str2);
	}
	case AT_BOOLEAN:
		return attempToSetValue(L, opts, option, value._value._boolean);
	default:
		stringstream ss;
		ss << "Wrong option value type for option " << option << ".\n";
		ss << opts->printAllowedValues(option) << ".\n";
		lua_pushstring(L, ss.str().c_str());
		return lua_error(L);
	}
}

/**
 * Call function for predicate tables
 */
int predtableCall(lua_State* L) {
	PredTable* predtable = *(PredTable**) lua_touserdata(L, 1);
	lua_remove(L, 1);
	ElementTuple tuple;
	for (int n = 1; n <= lua_gettop(L); ++n) {
		InternalArgument currarg = createArgument(n, L);
		switch (currarg._type) {
		case AT_INT:
			tuple.push_back(createDomElem(currarg._value._int));
			break;
		case AT_DOUBLE:
			tuple.push_back(createDomElem(currarg._value._double));
			break;
		case AT_STRING:
			tuple.push_back(createDomElem(*currarg._value._string));
			break;
		case AT_COMPOUND:
			tuple.push_back(createDomElem(currarg._value._compound));
			break;
		case AT_TUPLE:
			if (n == 1 && lua_gettop(L) == 1) {
				tuple = *(currarg._value._tuple);
				break;
			}
			break;
		default:
			lua_pushstring(L, "Only numbers, strings, and compounds can be arguments of a predicate table");
			lua_error(L);
			return 0;
		}
	}
	lua_pushboolean(L, predtable->contains(tuple));
	return 1;
}

/**
 * Call function for function interpretations
 */
int funcinterCall(lua_State* L) {
	FuncInter* funcinter = *(FuncInter**) lua_touserdata(L, 1);
	if (!funcinter->approxTwoValued()) {
		lua_pushstring(L, "Only two-valued function interpretations can be called");
		return lua_error(L);
	}
	FuncTable* ft = funcinter->funcTable();
	lua_remove(L, 1);
	unsigned int nrargs = lua_gettop(L);
	ElementTuple tuple;
	if (nrargs == 1 && createArgument(1, L)._type == AT_TUPLE) {
		tuple = *(createArgument(1, L)._value._tuple);
	} else {
		for (unsigned int n = 1; n <= nrargs; ++n) {
			InternalArgument arg = createArgument(n, L);
			switch (arg._type) {
			case AT_INT:
				tuple.push_back(createDomElem(arg._value._int));
				break;
			case AT_DOUBLE:
	  			tuple.push_back(createDomElem(arg._value._double));
				break;
			case AT_STRING:
				tuple.push_back(createDomElem(*arg._value._string));
				break;
			case AT_COMPOUND:
				tuple.push_back(createDomElem(arg._value._compound));
				break;
			default:
				lua_pushstring(L, "Only numbers, strings, and compounds can be arguments of a function interpretation");
				return lua_error(L);
			}
		}
	} 
	if (tuple.size() != ft->arity()) {
		lua_pushstring(L, "Call the function interpretation with the correct number of arguments");
		return lua_error(L);
	}
	const DomainElement* d = (*ft)[tuple];
	return convertToLua(L, d);
}

/**
 * Arity function for predicate symbols
 */
int predicateArity(lua_State* L) {
	set<Predicate*>* pred = *(set<Predicate*>**) lua_touserdata(L, 1);
	InternalArgument arity = createArgument(2, L);
	if (arity._type == AT_INT) {
		set<Predicate*>* newpred = new set<Predicate*>();
		for (auto it = pred->begin(); it != pred->end(); ++it) {
			if ((int) (*it)->arity() == arity._value._int) {
				newpred->insert(*it);
			}
		}
		InternalArgument np(newpred);
		return convertToLua(L, np);
	} else {
		lua_pushstring(L, "The arity of a predicate must be an integer");
		return lua_error(L);
	}
}

/**
 * Arity function for function symbols
 */
int functionArity(lua_State* L) {
	set<Function*>* func = *(set<Function*>**) lua_touserdata(L, 1);
	InternalArgument arity = createArgument(2, L);
	if (arity._type == AT_INT) {
		set<Function*>* newfunc = new set<Function*>();
		for (auto it = func->begin(); it != func->end(); ++it) {
			if ((int) (*it)->arity() == arity._value._int) {
				newfunc->insert(*it);
			}
		}
		InternalArgument nf(newfunc);
		return convertToLua(L, nf);
	} else {
		lua_pushstring(L, "The arity of a function must an integer");
		return lua_error(L);
	}
}

/**
 * Arity function for symbols
 */
int symbolArity(lua_State* L) {
	OverloadedSymbol* symb = *(OverloadedSymbol**) lua_touserdata(L, 1);
	InternalArgument arity = createArgument(2, L);
	if (arity._type == AT_TABLE) {
		convertToLua(L, InternalArgument(new set<Function*>(*(symb->funcs()))));
		lua_replace(L, 1);
		return functionArity(L);
	} else if (arity._type == AT_INT) {
		convertToLua(L, InternalArgument(new set<Predicate*>(*(symb->preds()))));
		lua_replace(L, 1);
		return predicateIndex(L);
	} else {
		lua_pushstring(L, "The arity of a symbol must be an integer or of the form \'integer : 1\'");
		return lua_error(L);
	}
}

int mxNext(lua_State* L) {
	if(lua_type(L, 1) == LUA_TNONE) {
		lua_pushstring(L, "next expects an MXiterator. Use the \":\" operator.");
		return lua_error(L);
	}
	InternalArgument ia = createArgument(1, L);
	if(ia._type != AT_MODELITERATOR) {
		lua_pushstring(L, "next expects an MXiterator. Use the \":\" operator.");
		return lua_error(L);
	}
	WrapModelIterator* iter = ia._value._modelIterator;
	shared_ptr<ModelIterator> it = iter->get();
	auto result = it->calculateMonitor();
	if(result.unsat) {
		lua_pushnil(L);
		return 1;
	} else {
		InternalArgument ia(result._models[0]);
		return convertToLua(L, ia);
	}
}

int twoValuedNext(lua_State* L) {
	if (lua_type(L, 1) == LUA_TNONE) {
		lua_pushstring(L, "next expects an twoValuedIterator. Use the \":\" operator.");
		return lua_error(L);
	}
	InternalArgument ia = createArgument(1, L);
	if (ia._type != AT_TWOVALUEDITERATOR) {
		lua_pushstring(L, "next expects an twoValuedIterator. Use the \":\" operator.");
		return lua_error(L);
	}
	TwoValuedStructureIterator* iter = ia._value._twoValuedIterator;
	auto result = iter->next();
	if (result == NULL) {
		lua_pushnil(L);
		return 1;
	} else {
		InternalArgument ia(result);
		return convertToLua(L, ia);
	}
}

typedef pair<int (*)(lua_State*), string> tablecolheader;

void createNewTable(lua_State* L, ArgType type, vector<tablecolheader> elements) {
	bool newtable = luaL_newmetatable(L, toCString(type)) != 0;
	Assert(newtable);
	if (newtable) {
		lua_pushinteger(L, type);
		lua_setfield(L, -2, _typefield);
		for (auto i = elements.cbegin(); i < elements.cend(); ++i) {
			lua_pushcfunction(L, (*i).first);
			lua_setfield(L, -2, (*i).second.c_str());
		}
		lua_pop(L, 1);
	}
}

const char* getInternalProcedureMetaTableName() {
	return "internalprocedure";
}
/*
 * GC means garbage collect
 * If lua calls procedure __gc, we need to call &gc... (such as for example &gcInternProc, &gcSort, ...)
 */

void internProcMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcInternProc, "__gc" });
	elements.push_back(tablecolheader { &internalCall, "__call" });

	bool newtable = luaL_newmetatable(L, getInternalProcedureMetaTableName()) != 0;
	Assert(newtable);
	if (newtable) {
		for (auto i = elements.cbegin(); i < elements.cend(); ++i) {
			lua_pushcfunction(L, (*i).first);
			lua_setfield(L, -2, (*i).second.c_str());
		}
		lua_pop(L, 1);
	}
}

void sortMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcSort, "__gc" });
	createNewTable(L, AT_SORT, elements);
}

void predicateMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcPredicate, "__gc" });
	elements.push_back(tablecolheader { &predicateIndex, "__index" });
	elements.push_back(tablecolheader { &predicateArity, "__div" });
	createNewTable(L, AT_PREDICATE, elements);
}

void functionMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcFunction, "__gc" });
	elements.push_back(tablecolheader { &functionIndex, "__index" });
	elements.push_back(tablecolheader { &functionArity, "__div" });
	createNewTable(L, AT_FUNCTION, elements);
}

void symbolMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcSymbol, "__gc" });
	elements.push_back(tablecolheader { &symbolIndex, "__index" });
	elements.push_back(tablecolheader { &symbolArity, "__div" });
	createNewTable(L, AT_SYMBOL, elements);
}

void vocabularyMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcVocabulary, "__gc" });
	elements.push_back(tablecolheader { &vocabularyIndex, "__index" });
	createNewTable(L, AT_VOCABULARY, elements);
}

void compoundMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcCompound, "__gc" });
	createNewTable(L, AT_COMPOUND, elements);
}

void tupleMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcTuple, "__gc" });
	elements.push_back(tablecolheader { &tupleIndex, "__index" });
	elements.push_back(tablecolheader { &tupleLen, "__len" });
	createNewTable(L, AT_TUPLE, elements);
}

void domainMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcDomain, "__gc" });
	createNewTable(L, AT_DOMAIN, elements);
}

void predtableMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcPredTable, "__gc" });
	elements.push_back(tablecolheader { &predtableCall, "__call" });
	createNewTable(L, AT_PREDTABLE, elements);
}

void predinterMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcPredInter, "__gc" });
	elements.push_back(tablecolheader { &predinterIndex, "__index" });
	elements.push_back(tablecolheader { &predinterNewIndex, "__newindex" });
	createNewTable(L, AT_PREDINTER, elements);
}

void funcinterMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcFuncInter, "__gc" });
	elements.push_back(tablecolheader { &funcinterIndex, "__index" });
	elements.push_back(tablecolheader { &funcinterNewIndex, "__newindex" });
	elements.push_back(tablecolheader { &funcinterCall, "__call" });
	createNewTable(L, AT_FUNCINTER, elements);
}

void structureMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcStructure, "__gc" });
	elements.push_back(tablecolheader { &structureIndex, "__index" });
	elements.push_back(tablecolheader { &structureNewIndex, "__newindex" });
	createNewTable(L, AT_STRUCTURE, elements);
}

void tableiteratorMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcTableIterator, "__gc" });
	createNewTable(L, AT_TABLEITERATOR, elements);
}

void domainiteratorMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcDomainIterator, "__gc" });
	createNewTable(L, AT_DOMAINITERATOR, elements);
}

void theoryMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcTheory, "__gc" });
	createNewTable(L, AT_THEORY, elements);
}

void formulaMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcFormula, "__gc" });
	createNewTable(L, AT_FORMULA, elements);
}

void queryMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcQuery, "__gc" });
	createNewTable(L, AT_QUERY, elements);
}

void termMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcTerm, "__gc" });
	createNewTable(L, AT_TERM, elements);
}
void fobddMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcFobdd, "__gc" });
	createNewTable(L, AT_FOBDD, elements);
}

void optionsMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcOptions, "__gc" });
	elements.push_back(tablecolheader { &optionsIndex, "__index" });
	elements.push_back(tablecolheader { &optionsNewIndex, "__newindex" });
	createNewTable(L, AT_OPTIONS, elements);
}

void namespaceMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcNamespace, "__gc" });
	elements.push_back(tablecolheader { &namespaceIndex, "__index" });
	createNewTable(L, AT_NAMESPACE, elements);
}

void overloadedMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcOverloaded, "__gc" });
	createNewTable(L, AT_OVERLOADED, elements);
}

void mxIteratorMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcMXIterator, "__gc" });
	elements.push_back(tablecolheader { &mxNext, "next" });
	createNewTable(L, AT_MODELITERATOR, elements);
	
	//Make metatable own table:
	//mt.__index = mt
	string name = toCString(AT_MODELITERATOR);
	luaL_getmetatable(L, name.c_str());
	lua_pushvalue(L, -1);
	string index = "__index";
	lua_setfield(L, -2, index.c_str());
	lua_pop(L, 1);
}

void twoValuedIteratorMetaTable(lua_State* L) {
	vector<tablecolheader> elements;
	elements.push_back(tablecolheader { &gcTwoValuedIterator, "__gc" });
	elements.push_back(tablecolheader { &twoValuedNext, "next" });
	createNewTable(L, AT_TWOVALUEDITERATOR, elements);

	//Make metatable own table:
	//mt.__index = mt
	string name = toCString(AT_TWOVALUEDITERATOR);
	luaL_getmetatable(L, name.c_str());
	lua_pushvalue(L, -1);
	string index = "__index";
	lua_setfield(L, -2, index.c_str());
	lua_pop(L, 1);
}

/**
 * Create all metatables
 */
void createMetaTables(lua_State* L) {
	internProcMetaTable(L);

	sortMetaTable(L);
	predicateMetaTable(L);
	functionMetaTable(L);
	symbolMetaTable(L);
	vocabularyMetaTable(L);

	compoundMetaTable(L);
	tupleMetaTable(L);
	domainMetaTable(L);
	predtableMetaTable(L);
	predinterMetaTable(L);
	funcinterMetaTable(L);
	structureMetaTable(L);
	tableiteratorMetaTable(L);
	domainiteratorMetaTable(L);

	theoryMetaTable(L);
	formulaMetaTable(L);
	queryMetaTable(L);
	termMetaTable(L);
	optionsMetaTable(L);
	namespaceMetaTable(L);
	fobddMetaTable(L);

	overloadedMetaTable(L);
	mxIteratorMetaTable(L);
	twoValuedIteratorMetaTable(L);
}

std::set<Namespace*> _checkedAddToGlobal;
std::set<Namespace*>& getCheckedGlobals(){
	return _checkedAddToGlobal;
}

void checkedAddToGlobal(Namespace* ns){
	auto& checkedglobals = getCheckedGlobals();
	if(ns->name()==getInternalNamespaceName()){
		return;
	}
	if (checkedglobals.find(ns) == checkedglobals.cend()) {
		bool add = false;
		if (ns == getGlobal()->getStdNamespace()) {
			add = true;
		} else if (ns == getGlobal()->getGlobalNamespace()) {
			add = true;
		} else if (ns->hasParent(getGlobal()->getStdNamespace())) {
			add = true;
		} else {
			for (auto i = getGlobal()->getGlobalNamespace()->subspaces().cbegin(); i != getGlobal()->getGlobalNamespace()->subspaces().cend(); ++i) {
				if (ns == (*i).second) {
					add = true;
				}
			}
		}
		if (add) {
			addGlobal(ns);
		}
		checkedglobals.insert(ns);
	}
}

/**
 * map internal procedure names to the actual procedures
 */
typedef map<vector<ArgType>, InternalProcedure*> internalprocargmap;

map<string, internalprocargmap*> procname2globalprocmap;
map<string, map<string, internalprocargmap*> > ns2proc2procmap;
set<string> addednamespaces;

void addInternalProcedure(Inference* inf, lua_State* state) {
	if (addednamespaces.find(inf->getNamespace()) == addednamespaces.cend()) {
		//	cerr <<"Adding global ns table " <<nsspace.c_str() <<"\n";
		lua_newtable(state);
		lua_setglobal(state, inf->getNamespace().c_str());
		addednamespaces.insert(inf->getNamespace());
	}

	if (inf->getNamespace() != getInternalNamespaceName()) {
		auto mappingglobal = procname2globalprocmap.find(inf->getName());
		if(mappingglobal!=procname2globalprocmap.cend()){
			mappingglobal->second->insert({inf->getArgumentTypes(), new InternalProcedure(inf)});
		}else{
			auto possiblearguments = new internalprocargmap();
			possiblearguments->insert({inf->getArgumentTypes(), new InternalProcedure(inf)});
			addUserData(state, possiblearguments, getInternalProcedureMetaTableName());
			//cerr <<"Setting global " <<inf->getName().c_str() <<"\n";
			lua_setglobal(state, inf->getName().c_str());
			procname2globalprocmap[inf->getName()] = possiblearguments;
		}
	}

	if (inf->getNamespace() != getGlobalNamespaceName()) {
		bool exists = true;
		auto mappingns = ns2proc2procmap.find(inf->getNamespace());
		if(mappingns!=ns2proc2procmap.cend()){
			auto mappingname = mappingns->second.find(inf->getName());
			if(mappingname!=mappingns->second.cend()){
				mappingname->second->insert({inf->getArgumentTypes(), new InternalProcedure(inf)});
			}else{
				exists = false;
			}
		}else{
			exists = false;
		}

		if(not exists){
			lua_getglobal(state, inf->getNamespace().c_str());
			auto possiblearguments = new internalprocargmap();
			possiblearguments->insert({inf->getArgumentTypes(), new InternalProcedure(inf)});
			addUserData(state, possiblearguments, getInternalProcedureMetaTableName());
			//cerr <<"Setting field " <<inf->getName().c_str() <<"in namespace " <<inf->getNamespace() <<"\n";
			lua_setfield(state, -2, inf->getName().c_str());
			lua_pop(state, 1);
			ns2proc2procmap[inf->getNamespace()][inf->getName()] = possiblearguments;
		}
	}
}

void addInternalProcedures(lua_State*) {
	addednamespaces.clear();
	procname2globalprocmap.clear();
	ns2proc2procmap.clear();

	// The mapping of all possible procedure names to a map with all their possible arguments and associated effective internal procedures
	for (auto i = getAllInferences().cbegin(); i != getAllInferences().cend(); ++i) {
		addInternalProcedure((*i).get(), getState());
	}
}

/**
 * Establish the connection with lua.
 */
void makeLuaConnection() {
	// Create the lua state
	_state = lua_open();
	luaL_openlibs(getState());

	_checkedAddToGlobal.clear(); // TODO belongs to some singleton?

	// Create all metatables
	createMetaTables(getState());

	// Add internal procedures
	addInternalProcedures(getState());

	// Overwrite some standard lua procedures
	int err = luaL_dofile(getState(),getPathOfLuaInternals().c_str());
	if (err) {
		clog << lua_tostring(getState(),-1) << "\n";
		clog << "Error in " << getPathOfLuaInternals() << ".\n";
		exit(1);
	}

	// Add the global namespace and standard options
	// IMPORTANT only add namespaces as global after inferences have been added
	addGlobal(GlobalData::getGlobalNamespace());
	addGlobal(GlobalData::getStdNamespace());
	addGlobal(Vocabulary::std());
	addGlobal("stdoptions", GlobalData::instance()->getOptions()); // TODO string "stdoptions" used twice, also in data/idp_intern.lua

	// Parse standard input file
	parsefile(getPathOfIdpInternals());

	for (auto i = GlobalData::getGlobalNamespace()->subspaces().cbegin(); i != GlobalData::getGlobalNamespace()->subspaces().cend(); ++i) {
		checkedAddToGlobal(i->second);
	}
	// NOTE: nested std namespaces are NOT added!
	for (auto i = GlobalData::getStdNamespace()->subspaces().cbegin(); i != GlobalData::getStdNamespace()->subspaces().cend(); ++i) {
		checkedAddToGlobal(i->second);
	}

	// Parse and run configuration file
	auto errornb = run({getPathOfConfigFile()}, false, false, "stdspace.configIDP()");
	if(errornb!=0){
		clog << lua_tostring(getState(),-1) << "\n";
		clog << "Error in " << getPathOfConfigFile() << ".\n";
		exit(1);
	}
}

/**
 * End the connection with lua
 */
void closeLuaConnection() {
	_checkedAddToGlobal.clear(); // TODO belongs to some singleton?
	lua_close(getState());
	_state = NULL;
	//FIXME it seems that all of the Internal Procedures have been deleted by gc.
	//FIXME no, they are not, just some are deleted...
//	for (auto i = ns2name2procedures.cbegin(); i != ns2name2procedures.cend(); ++i) {
//		for (auto j = i->second.cbegin(); j != i->second.cend(); ++j) {
//			for (auto k = j->second.cbegin(); k != j->second.cend(); ++k) {
//				if (k->second != NULL) {
//					delete (k->second);
//				}
//			}
//		}
//	}
}

const DomainElement* execute(const std::string& chunk) {
	try {
		int err = luaL_dostring(getState(),chunk.c_str());
		if (err) {
			stringstream ss;
			auto result = lua_tostring(getState(),-1);
			ss <<result;
			lua_pop(getState(), 1);
			Error::error(ss.str());
			return NULL;
		}
	} catch (NoSuchProcedureException& e) {
		// Stops execution of further commands, as expected
	}

	return convertToElement(-1, getState());
}

void pushglobal(const vector<string>& name, const ParseInfo& pi) {
	lua_getglobal(getState(), name[0].c_str());
	for (size_t n = 1; n < name.size(); ++n) {
		if (lua_istable(getState(),-1)) {
			lua_getfield(getState(), -1, name[n].c_str());
			lua_remove(getState(), -2);
		} else {
			stringstream ss;
			ss << "Unknown object.";
			Error::error(ss.str(), pi);
		}
	}
}

InternalArgument* call(const vector<string>& proc, const vector<vector<string>>& args, const ParseInfo& pi) {
	pushglobal(proc, pi);
	for (size_t n = 0; n < args.size(); ++n) {
		pushglobal(args[n], pi);
	}
	int err = lua_pcall(getState(), args.size(), 1, 0);
	if (err) {
		stringstream ss;
		ss << lua_tostring(getState(),-1);
		lua_pop(getState(), 1);
		Error::error(ss.str(), pi);
		return NULL;
	} else {
		InternalArgument* ia = new InternalArgument(createArgument(-1, getState()));
		lua_pop(getState(), 1);
		return ia;
	}
}

const DomainElement* funccall(string* procedure, const ElementTuple& input) {
	lua_getfield(getState(), LUA_REGISTRYINDEX, procedure->c_str());
	for (auto it = input.cbegin(); it != input.cend(); ++it) {
		convertToLua(getState(), *it);
	}
	int err = lua_pcall(getState(), input.size(), 1, 0);
	if (err) {
		stringstream ss;
		ss << string(lua_tostring(getState(),-1));
		lua_pop(getState(), 1);
		Error::error(ss.str());
		return NULL;
	} else {
		auto d = convertToElement(-1, getState());
		lua_pop(getState(), 1);
		return d;
	}
}

bool predcall(string* procedure, const ElementTuple& input) {
	lua_getfield(getState(), LUA_REGISTRYINDEX, procedure->c_str());
	for (auto it = input.cbegin(); it != input.cend(); ++it) {
		convertToLua(getState(), *it);
	}
	int err = lua_pcall(getState(), input.size(), 1, 0);
	if (err) {
		stringstream ss;
		ss << string(lua_tostring(getState(),-1)) << "\n";
		lua_pop(getState(), 1);
		throw IdpException(ss.str());
	} else {
		bool b = lua_toboolean(getState(), -1);
		lua_pop(getState(), 1);
		return b;
	}
}

void compile(UserProcedure* proc) {
	compile(proc, getState());
}

template<>
void addGlobal(UserProcedure* p) {
	InternalArgument ia;
	ia._type = AT_PROCEDURE;
	ia._value._string = new std::string(p->registryindex());
	convertToLua(getState(), ia);
	delete(ia._value._string);
	lua_setglobal(getState(), p->name().c_str());
}

Structure* structure(InternalArgument* arg) {
	return arg->_type == AT_STRUCTURE ? arg->_value._structure : NULL;
}
AbstractTheory* theory(InternalArgument* arg) {
	return arg->_type == AT_THEORY ? arg->_value._theory : NULL;
}
const FOBDD* fobdd(InternalArgument* arg){
	return arg->_type == AT_FOBDD ? arg->_value._fobdd : NULL;
}

Vocabulary* vocabulary(InternalArgument* arg) {
	return arg->_type == AT_VOCABULARY ? arg->_value._vocabulary : NULL;
}

string* getProcedure(const std::vector<std::string>& name, const ParseInfo& pi) {
	pushglobal(name, pi);
	InternalArgument ia = createArgument(-1, getState());
	lua_pop(getState(), 1);
	if (ia._type == AT_PROCEDURE) {
		return ia._value._string;
	} else {
		return 0;
	}
}

LuaTraceMonitor* getLuaTraceMonitor() {
	if (getState() == NULL) {
		return NULL;
	}
	return new LuaTraceMonitor(getState());
}

} /* namespace LuaConnection */
