/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

/**************************
	Connection with Lua
**************************/

#include "luaconnection.hpp"
#include <set>
#include <iostream>
#include <cstdlib>
#include "common.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "options.hpp"
#include "namespace.hpp"
#include "error.hpp"
#include "print.hpp"
#include "ground.hpp"
#include "ecnf.hpp"
#include "fobdd.hpp"
#include "propagate.hpp"
#include "generator.hpp"
#include "internalluaargument.hpp"
using namespace std;
using namespace LuaConnection;

extern void parsefile(const string&);

template<class Arg>
int addUserData(lua_State* l, Arg arg, const std::string& name){
	Arg* ptr = (Arg*)lua_newuserdata(l,sizeof(Arg));
	(*ptr) = arg;
	luaL_getmetatable(l,name.c_str());
	lua_setmetatable(l,-2);
	return 1;
}

template<class Arg>
int addUserData(lua_State* l, Arg arg, ArgType type){
	return addUserData(l, arg, toCString(type));
}

int ArgProcNumber = 0;						//!< Number to create unique registry indexes
const char* _typefield = "type";		//!< Field index containing the type of userdata

const char* getTypeField(){
	return _typefield;
}
int& argProcNumber(){
	return ArgProcNumber;
}

std::map<ArgType, const char*> argType2Name;
bool init = false;

const char* toCString(ArgType type){
	if(!init){
		map_init(argType2Name)
			(AT_SORT, "type")
			(AT_PREDICATE, "predicate_symbol")
			(AT_FUNCTION, "function_symbol")
			(AT_SYMBOL, "symbol")
			(AT_VOCABULARY, "vocabulary")
			(AT_COMPOUND, "compound")
			(AT_TUPLE, "tuple")
			(AT_DOMAIN, "domain")
			(AT_PREDTABLE, "predicate_table")
			(AT_PREDINTER, "predicate_interpretation")
			(AT_FUNCINTER, "function_interpretation")
			(AT_STRUCTURE, "structure")
			(AT_TABLEITERATOR, "predicate_table_iterator")
			(AT_DOMAINITERATOR, "domain_iterator")
			(AT_DOMAINATOM, "domain_atom")
			(AT_QUERY, "query")
			(AT_FORMULA, "formula")
			(AT_THEORY, "theory")
			(AT_OPTIONS, "options")
			(AT_NAMESPACE, "namespace")
			(AT_NIL, "nil")
			(AT_INT, "number")
			(AT_DOUBLE, "number")
			(AT_BOOLEAN, "boolean")
			(AT_STRING, "string")
			(AT_TABLE, "table")
			(AT_PROCEDURE, "function")
			(AT_OVERLOADED, "overloaded")
			(AT_MULT, "mult")
			(AT_REGISTRY, "registry")
		;
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
	switch(lua_type(L,arg)) {
		case LUA_TNIL:
			return 0;
		case LUA_TSTRING:
			return DomainElementFactory::instance()->create(StringPointer(lua_tostring(L,arg)));
		case LUA_TNUMBER:
			return DomainElementFactory::instance()->create(lua_tonumber(L,arg));
		case LUA_TUSERDATA:
		{
			lua_getmetatable(L,arg);
			lua_getfield(L,-1,_typefield);
			ArgType type = (ArgType)lua_tointeger(L,-1); assert(type != AT_NIL);
			lua_pop(L,2);
			switch(type) {
				case AT_COMPOUND:
					return DomainElementFactory::instance()->create(*(Compound**)lua_touserdata(L,arg));
				default:
					return 0;
			}
		}
		default:
			return 0;
	}
}

namespace LuaConnection {

	void compile(UserProcedure* procedure, lua_State* state){
		//FIXME
/*		if(!iscompiled()) {
			// Compose function header, body, and return statement
			stringstream ss;
			ss << "local function " << _name << "(";
			if(!_argnames.empty()) {
				vector<string>::iterator it = _argnames.begin();
				ss << *it; ++it;
				for(; it != _argnames.end(); ++it) ss << ',' << *it;
			}
			ss << ')' << _code.str() << " end\n";
			ss << "return " << _name << "(...)\n";

			// Compile
			//cerr << "compiling:\n" << ss.str() << endl;
			int err = luaL_loadstring(L,ss.str().c_str());
			if(err) {
				Error::error(_pi);
				cerr << string(lua_tostring(L,-1)) << endl;
				lua_pop(L,1);
			}
			else {
				_registryindex = "idp_compiled_procedure_" + toString(UserProcedure::_compilenumber);
				++UserProcedure::_compilenumber;
				lua_setfield(L,LUA_REGISTRYINDEX,_registryindex.c_str());
			}
		}*/
	}

	lua_State* _state;

	lua_State* getState() { return _state; }

	// garbage collection
	map<AbstractStructure*,unsigned int>	_luastructures;
	map<AbstractTheory*,unsigned int>		_luatheories;
	map<Options*,unsigned int>				_luaoptions;
	set<SortTable*>							_luadomains;

	/**
	 * Push a domain element to the lua stack
	 */
	int convertToLua(lua_State* L, const DomainElement* d) {
		switch(d->type()) {
			case DET_INT:
				lua_pushinteger(L,d->value()._int);
				return 1;
			case DET_DOUBLE:
				lua_pushnumber(L,d->value()._double);
				return 1;
			case DET_STRING:
				lua_pushstring(L,d->value()._string->c_str());
				return 1;
			case DET_COMPOUND:{
				return addUserData(L, d->value()._compound, AT_COMPOUND);
			}
			default:
				assert(false);
				return 0;
		}
	}

	/**
	 * Push an internal argument to the lua stack
	 */
	int convertToLua(lua_State* L,InternalArgument arg) {
		switch(arg._type) {
			case AT_SORT:{
				return addUserData(L, arg._value._sort, arg._type);
			}
			case AT_PREDICATE:{
				return addUserData(L, arg._value._predicate, arg._type);
			}
			case AT_FUNCTION:{
				return addUserData(L, arg._value._function, arg._type);
			}
			case AT_SYMBOL:{
				return addUserData(L, arg._value._symbol, arg._type);
			}
			case AT_VOCABULARY:{
				return addUserData(L, arg._value._vocabulary, arg._type);
			}
			case AT_COMPOUND:{
				return addUserData(L, arg._value._compound, arg._type);
			}
			case AT_DOMAINATOM:{
				return addUserData(L, arg._value._domainatom, arg._type);
			}
			case AT_TUPLE:{
				return addUserData(L, arg._value._tuple, arg._type);
			}
			case AT_DOMAIN:{
				return addUserData(L, arg._value._domain, arg._type);
			}
			case AT_PREDTABLE:{
				return addUserData(L, arg._value._predtable, arg._type);
			}
			case AT_PREDINTER:{
				return addUserData(L, arg._value._predinter, arg._type);
			}
			case AT_FUNCINTER:{
				return addUserData(L, arg._value._funcinter, arg._type);
			}
			case AT_STRUCTURE:{
				AbstractStructure** ptr = (AbstractStructure**)lua_newuserdata(L,sizeof(AbstractStructure*));
				(*ptr) = arg._value._structure;
				luaL_getmetatable(L,"structure");
				lua_setmetatable(L,-2);
				if(arg._value._structure->pi().line() == 0) {
					if(_luastructures.find(arg._value._structure) != _luastructures.end())
						++_luastructures[arg._value._structure];
					else
						_luastructures[arg._value._structure] = 1;
				}
				return 1;
			}
			case AT_TABLEITERATOR:{
				return addUserData(L, arg._value._tableiterator, arg._type);
			}
			case AT_DOMAINITERATOR:{
				return addUserData(L, arg._value._sortiterator, arg._type);
			}
			case AT_THEORY:{
				AbstractTheory** ptr = (AbstractTheory**)lua_newuserdata(L,sizeof(AbstractTheory*));
				(*ptr) = arg._value._theory;
				luaL_getmetatable(L,toCString(arg._type));
				lua_setmetatable(L,-2);
				if(arg._value._theory->pi().line() == 0) {
					if(_luatheories.find(arg._value._theory) != _luatheories.end())
						++_luatheories[arg._value._theory];
					else
						_luatheories[arg._value._theory] = 1;
				}
				return 1;
			}
			case AT_FORMULA:{
				return addUserData(L, arg._value._formula, arg._type);
			}
			case AT_QUERY:{
				return addUserData(L, arg._value._query, arg._type);
			}
			case AT_OPTIONS:{
				Options** ptr = (Options**)lua_newuserdata(L,sizeof(Options*));
				(*ptr) = arg._value._options;
				luaL_getmetatable(L,toCString(arg._type));
				lua_setmetatable(L,-2);
				if(arg._value._options->pi().line() == 0) {
					if(_luaoptions.find(arg._value._options) != _luaoptions.end())
						++_luaoptions[arg._value._options];
					else
						_luaoptions[arg._value._options] = 1;
				}
				return 1;
			}
			case AT_NAMESPACE:{
				return addUserData(L, arg._value._namespace, arg._type);
			}
			case AT_NIL:{
				lua_pushnil(L);
				return 1;
			}
			case AT_INT:{
				lua_pushinteger(L,arg._value._int);
				return 1;
			}
			case AT_DOUBLE:{
				lua_pushnumber(L,arg._value._double);
				return 1;
			}
			case AT_BOOLEAN:{
				lua_pushboolean(L,arg._value._boolean);
				return 1;
			}
			case AT_STRING:{
				lua_pushstring(L,arg._value._string->c_str());
				return 1;
			}
			case AT_TABLE:{
				lua_newtable(L);
				for(unsigned int n = 0; n < arg._value._table->size(); ++n) {
					lua_pushinteger(L,n+1);
					convertToLua(L,(*(arg._value._table))[n]);
					lua_settable(L,-3);
				}
				return 1;
			}
			case AT_PROCEDURE:{
				lua_getfield(L,LUA_REGISTRYINDEX,arg._value._string->c_str());
				return 1;
			}
			case AT_OVERLOADED:{
				return addUserData(L, arg._value._overloaded, arg._type);
			}
			case AT_MULT:{
				int nrres = 0;
				for(unsigned int n = 0; n < arg._value._table->size(); ++n) {
					nrres += convertToLua(L,(*(arg._value._table))[n]);
				}
				return nrres;
			}
			case AT_REGISTRY:{
				lua_getfield(L,LUA_REGISTRYINDEX,arg._value._string->c_str());
				return 1;
			}
			default:
				assert(false); return 0;
		}
	}

	InternalArgument createArgument(int arg, lua_State* L) {
		InternalArgument ia;
		ia._type = AT_NIL;
		switch(lua_type(L,arg)) {
			case LUA_TNIL:
				ia._type = AT_NIL;
				break;
			case LUA_TBOOLEAN:
				ia._type = AT_BOOLEAN;
				ia._value._boolean = lua_toboolean(L,arg);
				break;
			case LUA_TSTRING:
				ia._type = AT_STRING;
				ia._value._string = StringPointer(lua_tostring(L,arg));
				break;
			case LUA_TTABLE:
				if(arg < 0) {
					arg = lua_gettop(L) + arg + 1;
				}
				ia._type = AT_TABLE;
				ia._value._table = new std::vector<InternalArgument>();
				lua_pushnil(L);
				while(lua_next(L,arg) != 0) {
					ia._value._table->push_back(createArgument(-1,L));
					lua_pop(L,1);
				}
				break;
			case LUA_TFUNCTION:
			{
				ia._type = AT_PROCEDURE;
				std::string* registryindex = StringPointer(std::string("idp_argument_procedure_" + toString(argProcNumber())));
				++argProcNumber();
				lua_pushvalue(L,arg);
				lua_setfield(L,LUA_REGISTRYINDEX,registryindex->c_str());
				ia._value._string = registryindex;
				break;
			}
			case LUA_TNUMBER: {
				if(isInt(lua_tonumber(L,arg))) {
					ia._type = AT_INT;
					ia._value._int = lua_tointeger(L,arg);
				}
				else {
					ia._type = AT_DOUBLE;
					ia._value._double = lua_tonumber(L,arg);
				}
				break;
			}
			case LUA_TUSERDATA: {
				lua_getmetatable(L,arg);
				lua_getfield(L,-1,getTypeField());
				ia._type = (ArgType)lua_tointeger(L,-1); assert(_type != AT_NIL);
				lua_pop(L,2);
				switch(ia._type) {
					case AT_SORT:
						ia._value._sort = *(std::set<Sort*>**)lua_touserdata(L,arg);
						break;
					case AT_PREDICATE:
						ia._value._predicate = *(std::set<Predicate*>**)lua_touserdata(L,arg);
						break;
					case AT_FUNCTION:
						ia._value._function = *(std::set<Function*>**)lua_touserdata(L,arg);
						break;
					case AT_SYMBOL:
						ia._value._symbol = *(OverloadedSymbol**)lua_touserdata(L,arg);
						break;
					case AT_VOCABULARY:
						ia._value._vocabulary = *(Vocabulary**)lua_touserdata(L,arg);
						break;
					case AT_COMPOUND:
						ia._value._compound = *(Compound**)lua_touserdata(L,arg);
						break;
					case AT_DOMAINATOM:
						ia._value._domainatom = *(DomainAtom**)lua_touserdata(L,arg);
						break;
					case AT_TUPLE:
						ia._value._tuple = *(ElementTuple**)lua_touserdata(L,arg);
						break;
					case AT_DOMAIN:
						ia._value._domain = *(SortTable**)lua_touserdata(L,arg);
						break;
					case AT_PREDTABLE:
						ia._value._predtable = *(PredTable**)lua_touserdata(L,arg);
						break;
					case AT_PREDINTER:
						ia._value._predinter = *(PredInter**)lua_touserdata(L,arg);
						break;
					case AT_FUNCINTER:
						ia._value._funcinter = *(FuncInter**)lua_touserdata(L,arg);
						break;
					case AT_STRUCTURE:
						ia._value._structure = *(AbstractStructure**)lua_touserdata(L,arg);
						break;
					case AT_TABLEITERATOR:
						ia._value._tableiterator = *(TableIterator**)lua_touserdata(L,arg);
						break;
					case AT_DOMAINITERATOR:
						ia._value._sortiterator = *(SortIterator**)lua_touserdata(L,arg);
						break;
					case AT_THEORY:
						ia._value._theory = *(AbstractTheory**)lua_touserdata(L,arg);
						break;
					case AT_FORMULA:
						ia._value._formula = *(Formula**)lua_touserdata(L,arg);
						break;
					case AT_QUERY:
						ia._value._query = *(Query**)lua_touserdata(L,arg);
						break;
					case AT_OPTIONS:
						ia._value._options = *(Options**)lua_touserdata(L,arg);
						break;
					case AT_NAMESPACE:
						ia._value._namespace = *(Namespace**)lua_touserdata(L,arg);
						break;
					case AT_OVERLOADED:
						ia._value._overloaded = *(OverloadedObject**)lua_touserdata(L,arg);
						break;
						break;
					default:
						assert(false);
				}
				break;
			}
			case LUA_TTHREAD: assert(false); break;
			case LUA_TLIGHTUSERDATA: assert(false); break;
			case LUA_TNONE: assert(false); break;
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
		switch(lua_type(L,arg)) {
			case LUA_TNIL: result.push_back(AT_NIL); break;
			case LUA_TBOOLEAN: result.push_back(AT_BOOLEAN); break;
			case LUA_TSTRING: result.push_back(AT_STRING); break;
			case LUA_TTABLE: result.push_back(AT_TABLE); break;
			case LUA_TFUNCTION: result.push_back(AT_PROCEDURE); break;
			case LUA_TNUMBER: {
				if(isInt(lua_tonumber(L,arg))) result.push_back(AT_INT);
				else result.push_back(AT_DOUBLE);
				break;
			}
			case LUA_TUSERDATA: {
				lua_getmetatable(L,arg);
				lua_getfield(L,-1,_typefield);
				ArgType type = (ArgType)lua_tointeger(L,-1);
				if(type == AT_OVERLOADED) {
					OverloadedObject* oo = *(OverloadedObject**)lua_touserdata(L,arg);
					result = oo->types();
				}
				else if(type == AT_SYMBOL) {
					OverloadedSymbol* os = *(OverloadedSymbol**)lua_touserdata(L,arg);
					result = os->types();
				}
				else  result.push_back(type);
				lua_pop(L,2);
				break;
			}
			case LUA_TTHREAD: assert(false); break;
			case LUA_TLIGHTUSERDATA: assert(false); break;
			case LUA_TNONE: assert(false); break;
		}
		return result;
	}

	/**
	 *	Call to internal procedure
	 */
	int internalCall(lua_State* L) {
		// get the list of possible procedures
		map<vector<ArgType>,InternalProcedure*>* procs = *(map<vector<ArgType>,InternalProcedure*>**)lua_touserdata(L,1);
		lua_remove(L,1);

		// get the list of possible argument types
		vector<vector<ArgType> > argtypes(lua_gettop(L));
		for(int arg = 1; arg <= lua_gettop(L); ++arg)
			argtypes[arg-1] = getArgTypes(L,arg);

		// find the right procedure
		InternalProcedure* proc = 0;
		vector<vector<ArgType>::iterator> carry(argtypes.size());
		for(unsigned int n = 0; n < argtypes.size(); ++n) carry[n] = argtypes[n].begin();
		while(true) {
			vector<ArgType> currtypes(argtypes.size());
			for(unsigned int n = 0; n < argtypes.size(); ++n) currtypes[n] = *(carry[n]);
			if(procs->find(currtypes) != procs->end()) {
				if(!proc) {
					proc = (*procs)[currtypes];
				}
				else { Error::ambigcommand(proc->name()); return 0; }
			}
			unsigned int c = 0;
			for(; c < argtypes.size(); ++c) {
				++(carry[c]);
				if(carry[c] != argtypes[c].end()) break;
				else carry[c] = argtypes[c].begin();
			}
			if(c == argtypes.size()) break;
		}

		// Execute the procedure
		if(proc) return (*proc)(L);
		else {
			assert(!procs->empty());
			Error::wrongcommandargs(procs->begin()->second->name()); return 0;
		}
	}

	/*************************
		Garbage collection
	*************************/

	template<class T> int garbageCollect(T obj){
		delete(obj);
		return 0;
	}

	int gcInternProc(lua_State* L) { return garbageCollect(*(map<vector<ArgType>,InternalProcedure*>**)lua_touserdata(L,1)); }
	int gcSort(lua_State* L) { return garbageCollect(*(set<Sort*>**)lua_touserdata(L,1)); }
	int gcPredicate(lua_State* L) { return garbageCollect(*(set<Predicate*>**)lua_touserdata(L,1)); }
	int gcFunction(lua_State* L) { return garbageCollect(*(set<Function*>**)lua_touserdata(L,1)); }
	int gcSymbol(lua_State* L) { return garbageCollect(*(OverloadedSymbol**)lua_touserdata(L,1)); }
	int gcVocabulary(lua_State* ) { return 0; }
	int gcCompound(lua_State* ) { return 0; }
	int gcDomainAtom(lua_State* ) { return 0; }
	int gcTuple(lua_State* ) { return 0; }
	int gcTableIterator(lua_State* L) {return garbageCollect(*(TableIterator**)lua_touserdata(L,1)); }
	int gcDomainIterator(lua_State* L) {return garbageCollect(*(SortIterator**)lua_touserdata(L,1)); }
	int gcPredTable(lua_State* ) { return 0; }
	int gcPredInter(lua_State* ) { return 0; }
	int gcFuncInter(lua_State* ) { return 0; }
	int gcNamespace(lua_State* ) { return 0; }
	int gcOverloaded(lua_State* L) { return garbageCollect(*(OverloadedObject**)lua_touserdata(L,1)); }

	int gcDomain(lua_State* L) {
		SortTable* tab = *(SortTable**)lua_touserdata(L,1);
		set<SortTable*>::iterator it = _luadomains.find(tab);
		if(it != _luadomains.end()) {
			delete(*it);
			_luadomains.erase(tab);
		}
		return 0;
	}

	int gcStructure(lua_State* ) {
		/* TODO: uncomment
		AbstractStructure* s = *(AbstractStructure**)lua_touserdata(L,1);
		map<AbstractStructure*,unsigned int>::iterator it = _luastructures.find(s);
		if(it != _luastructures.end()) {
			--(it->second);
			if((it->second) == 0) {
				_luastructures.erase(s);
				delete(s);
			}
		}
		*/
		return 0;
	}

	/**
	 * Garbage collection for theories
	 */
	int gcTheory(lua_State* L) {
		AbstractTheory* t = *(AbstractTheory**)lua_touserdata(L,1);
		map<AbstractTheory*,unsigned int>::iterator it = _luatheories.find(t);
		if(it != _luatheories.end()) {
			--(it->second);
			if((it->second) == 0) {
				_luatheories.erase(t);
				t->recursiveDelete();
			}
		}
		return 0;
	}

	int gcFormula(lua_State* ) {
		// TODO
		return 0;
	}

	int gcQuery(lua_State*) {
		// TODO
		return 0;
	}

	/**
	 * Garbage collection for options
	 */
	int gcOptions(lua_State* L) {
		Options* opts = *(Options**)lua_touserdata(L,1);
		map<Options*,unsigned int>::iterator it = _luaoptions.find(opts);
		if(it != _luaoptions.end()) {
			--(it->second);
			if(it->second == 0) {
				_luaoptions.erase(opts);
				delete(opts);
			}
		}
		return 0;
	}

	/**
	 * Index function for predicate symbols
	 */
	int predicateIndex(lua_State* L) {
		set<Predicate*>* pred = *(set<Predicate*>**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2,L);
		if(index._type == AT_SORT) {
			set<Sort*>* sort = index.sort();
			set<Predicate*>* newpred = new set<Predicate*>();
			for(set<Sort*>::const_iterator it = sort->begin(); it != sort->end(); ++it) {
				for(set<Predicate*>::const_iterator jt = pred->begin(); jt != pred->end(); ++jt) {
					if((*jt)->arity() == 1) {
						if((*jt)->resolve(vector<Sort*>(1,(*it)))) newpred->insert(*jt);
					}
				}
			}
			InternalArgument np(newpred);
			return convertToLua(L,np);
		}
		else if(index._type == AT_TABLE) {
			vector<InternalArgument>* table = index._value._table;
			for(vector<InternalArgument>::const_iterator it =table->begin(); it != table->end(); ++it) {
				if(it->_type != AT_SORT) {
					lua_pushstring(L,"A predicate can only be indexed by a tuple of types");
					return lua_error(L);
				}
			}
			set<Predicate*>* newpred = new set<Predicate*>();
			vector<set<Sort*>::iterator> carry(table->size());
			for(unsigned int n = 0; n < table->size(); ++n) carry[n] = (*table)[n].sort()->begin();
			while(true) {
				vector<Sort*> currsorts(table->size());
				for(unsigned int n = 0; n < table->size(); ++n) currsorts[n] = *(carry[n]);
				for(set<Predicate*>::const_iterator it = pred->begin(); it != pred->end(); ++it) {
					if((*it)->arity() == table->size()) {
						if((*it)->resolve(currsorts)) newpred->insert(*it);
					}
				}
				unsigned int c = 0;
				for(; c < table->size(); ++c) {
					++(carry[c]);
					if(carry[c] != (*table)[c].sort()->end()) break;
					else carry[c] = (*table)[c].sort()->begin();
				}
				if(c == table->size()) break;
			}
			InternalArgument np(newpred);
			return convertToLua(L,np);
		}
		else {
			lua_pushstring(L,"A predicate can only be indexed by a tuple of types");
			return lua_error(L);
		}
	}

	/**
	 * Index function for function symbols
	 */
	int functionIndex(lua_State* L) {
		set<Function*>* func = *(set<Function*>**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2,L);
		if(index._type == AT_TABLE) {
			vector<InternalArgument>* table = index._value._table;
			vector<InternalArgument> newtable;
			if(table->size() < 1) {
				lua_pushstring(L,"Invalid function symbol index");
				return lua_error(L);
			}
			for(unsigned int n = 0; n < table->size(); ++n) {
				if((*table)[n]._type == AT_SORT) {
					newtable.push_back((*table)[n]);
				}
				else {
					lua_pushstring(L,"A function symbol can only be indexed by a tuple of types");
					return lua_error(L);
				}
			}
			set<Function*>* newfunc = new set<Function*>();
			vector<set<Sort*>::iterator> carry(newtable.size());
			for(unsigned int n = 0; n < newtable.size(); ++n) carry[n] = newtable[n].sort()->begin();
			while(true) {
				vector<Sort*> currsorts(newtable.size());
				for(unsigned int n = 0; n < newtable.size(); ++n) currsorts[n] = *(carry[n]);
				for(set<Function*>::const_iterator it = func->begin(); it != func->end(); ++it) {
					if((*it)->arity() == newtable.size()) {
						if((*it)->resolve(currsorts)) newfunc->insert(*it);
					}
				}
				unsigned int c = 0;
				for(; c < newtable.size(); ++c) {
					++(carry[c]);
					if(carry[c] != newtable[c].sort()->end()) break;
					else carry[c] = newtable[c].sort()->begin();
				}
				if(c == newtable.size()) break;
			}
			InternalArgument nf(newfunc);
			return convertToLua(L,nf);
		}
		else {
			lua_pushstring(L,"A function can only be indexed by a tuple of sorts");
			return lua_error(L);
		}
	}

	/**
	 * Index function for symbols
	 */
	int symbolIndex(lua_State* L) {
		OverloadedSymbol* symb = *(OverloadedSymbol**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2,L);
		if(index._type == AT_TABLE) {
			if(index._value._table->size() > 2 &&
				(*(index._value._table))[index._value._table->size()-2]._type == AT_PROCEDURE) {
				convertToLua(L,InternalArgument(new set<Function*>(*(symb->funcs()))));
				lua_replace(L,1);
				return functionIndex(L);
			}
			else {
				convertToLua(L,InternalArgument(new set<Predicate*>(*(symb->preds()))));
				lua_replace(L,1);
				return predicateIndex(L);
			}
		}
		else if(index._type == AT_STRING) {
			string str = *(index._value._string);
			if(str == "type") {
				convertToLua(L,InternalArgument(new set<Sort*>(*(symb->sorts()))));
				return 1;
			}
			else if(str == "predicate") {
				convertToLua(L,InternalArgument(new set<Predicate*>(*(symb->preds()))));
				return 1;
			}
			else if(str == "function") {
				convertToLua(L,InternalArgument(new set<Function*>(*(symb->funcs()))));
				return 1;
			}
			else {
				lua_pushstring(L,"A symbol can only be indexed by a tuple of sorts or the strings \"type\", \"predicate\", or \"function\".");
				return lua_error(L);
			}
		}
		else {
			lua_pushstring(L,"A symbol can only be indexed by a tuple of sorts or the strings \"type\", \"predicate\", or \"function\".");
			return lua_error(L);
		}
	}

	/**
	 * Index function for vocabularies
	 */
	int vocabularyIndex(lua_State* L) {
		Vocabulary* voc = *(Vocabulary**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2,L);
		if(index._type == AT_STRING) {
			unsigned int emptycounter = 0;
			const set<Sort*>* sorts = voc->sort(*(index._value._string));
			if((!sorts) || sorts->empty()) ++emptycounter;
			set<Predicate*> preds = voc->pred_no_arity(*(index._value._string));
			if(preds.empty()) ++emptycounter;
			set<Function*> funcs = voc->func_no_arity(*(index._value._string));
			if(funcs.empty()) ++emptycounter;
			if(emptycounter == 3) return 0;
			else if(emptycounter == 2) {
				if(sorts && !sorts->empty()) {
					set<Sort*>* newsorts = new set<Sort*>(*sorts);
					InternalArgument ns(newsorts);
					return convertToLua(L,ns);
				}
				else if(!preds.empty()) {
					set<Predicate*>* newpreds = new set<Predicate*>(preds);
					InternalArgument np(newpreds);
					return convertToLua(L,np);
				}
				else {
					assert(!funcs.empty());
					set<Function*>* newfuncs = new set<Function*>(funcs);
					InternalArgument nf(newfuncs);
					return convertToLua(L,nf);
				}
			}
			else {
				OverloadedSymbol* os = new OverloadedSymbol();
				if(sorts) {
					for(set<Sort*>::const_iterator it = sorts->begin(); it != sorts->end(); ++it) os->insert(*it);
				}
				for(set<Predicate*>::const_iterator it = preds.begin(); it != preds.end(); ++it) os->insert(*it);
				for(set<Function*>::const_iterator it = funcs.begin(); it != funcs.end(); ++it) os->insert(*it);
				InternalArgument s(os);
				return convertToLua(L,s);
			}
		}
		else {
			lua_pushstring(L,"A vocabulary can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * Index function for domain atoms
	 */
	int domainatomIndex(lua_State* L) {
		const DomainAtom* atom = *(const DomainAtom**) lua_touserdata(L,1);
		InternalArgument index = createArgument(2,L);
		if(index._type == AT_STRING) {
			string str = *index._value._string;
			if(str == "symbol") {
				PFSymbol* s = atom->symbol();
				if(typeid(*s) == typeid(Predicate)) {
					set<Predicate*>* sp = new set<Predicate*>();
					sp->insert(dynamic_cast<Predicate*>(s));
					return convertToLua(L,InternalArgument(sp));
				}
				else {
					set<Function*>* sf = new set<Function*>();
					sf->insert(dynamic_cast<Function*>(s));
					return convertToLua(L,InternalArgument(sf));
				}
			}
			else if(str == "args") {
				ElementTuple* tuple = new ElementTuple(atom->args());
				InternalArgument ia; ia._type = AT_TUPLE;
				ia._value._tuple = tuple;
				return convertToLua(L,ia);
			}
			else {
				lua_pushstring(L,"A domain atom can only be indexed by the strings \"symbol\" and \"args\"");
				return lua_error(L);
			}
		}
		else {
			lua_pushstring(L,"A domain atom can only be indexed by the strings \"symbol\" and \"args\"");
			return lua_error(L);
		}
	}

	/**
	 * Index function for tuples
	 */
	int tupleIndex(lua_State* L) {
		ElementTuple* tuple = *(ElementTuple**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2,L);
		if(index._type == AT_INT) {
			const DomainElement* element = (*tuple)[index._value._int - 1];
			return convertToLua(L,element);
		}
		else {
			lua_pushstring(L,"A tuple can only be indexed by an integer");
			return lua_error(L);
		}
	}

	/**
	 * Length operator for tuples
	 */
	int tupleLen(lua_State* L) {
		ElementTuple* tuple = *(ElementTuple**)lua_touserdata(L,1);
		int length = tuple->size();
		lua_pushinteger(L,length);
		return 1;
	}

	/**
	 * Index function for predicate interpretations
	 */
	int predinterIndex(lua_State* L) {
		PredInter* predinter = *(PredInter**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2, L);
		if(index._type == AT_STRING) {
			string str = *(index._value._string);
			if(str == "ct") {
				InternalArgument tab(predinter->ct());
				return convertToLua(L,tab);
			}
			else if(str == "pt") {
				InternalArgument tab(predinter->pt());
				return convertToLua(L,tab);
			}
			else if(str == "cf") {
				InternalArgument tab(predinter->cf());
				return convertToLua(L,tab);
			}
			else if(str == "pf") {
				InternalArgument tab(predinter->pf());
				return convertToLua(L,tab);
			}
			else {
				lua_pushstring(L,"A predicate interpretation can only be indexed by \"ct\", \"cf\", \"pt\", and \"pf\"");
				return lua_error(L);
			}
		}
		else {
			lua_pushstring(L,"A predicate interpretation can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * Index function for function interpretations
	 */
	int funcinterIndex(lua_State* L) {
		FuncInter* funcinter = *(FuncInter**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2, L);
		if(index._type == AT_STRING) {
			if(*(index._value._string) == "graph") {
				InternalArgument predinter(funcinter->graphinter());
				return convertToLua(L,predinter);
			}
			else {
				lua_pushstring(L,"A function interpretation can only be indexed by \"graph\"");
				return lua_error(L);
			}
		}
		else {
			lua_pushstring(L,"A function interpretation can only be indexed by a string");
			return lua_error(L);
		}
	}
	/**
	 * Index function for structures
	 */
	int structureIndex(lua_State* L) {
		AbstractStructure* structure = *(AbstractStructure**)lua_touserdata(L,1);
		InternalArgument symbol = createArgument(2,L);
		switch(symbol._type) {
			case AT_SORT:
			{
				set<Sort*>* ss = symbol._value._sort;
				if(ss->size() == 1) {
					Sort* s = *(ss->begin());
					SortTable* result = structure->inter(s);
					return convertToLua(L,InternalArgument(result));
				}
				break;
			}
			case AT_PREDICATE:
			{
				set<Predicate*>* sp = symbol._value._predicate;
				if(sp->size() == 1) {
					Predicate* p = *(sp->begin());
					PredInter* result = structure->inter(p);
					return convertToLua(L,InternalArgument(result));
				}
				break;
			}
			case AT_FUNCTION:
			{
				set<Function*>* sf = symbol._value._function;
				if(sf->size() == 1) {
					Function* f = *(sf->begin());
					FuncInter* result = structure->inter(f);
					return convertToLua(L,InternalArgument(result));
				}
				break;
			}
			default:
				break;
		}
		lua_pushstring(L,"A structure can only be indexed by a single type, predicate, or function symbol");
		return lua_error(L);
	}

	/**
	 * Index function for options
	 */
	int optionsIndex(lua_State* L) {
		Options* opts = *(Options**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2,L);
		if(index._type == AT_STRING) {
			return convertToLua(L,opts->getvalue(*(index._value._string)));
		}
		else {
			lua_pushstring(L,"Options can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * Index function for namespaces
	 */
	int namespaceIndex(lua_State* L) {
		Namespace* ns = *(Namespace**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2,L);
		if(index._type == AT_STRING) {
			string str = *(index._value._string);
			unsigned int counter = 0;
			Namespace* subsp = 0;
			if(ns->isSubspace(str)) { subsp = ns->subspace(str); ++counter; }
			Vocabulary* vocab = 0;
			if(ns->isVocab(str)) { vocab = ns->vocabulary(str); ++counter; }
			AbstractTheory* theo = 0;
			if(ns->isTheory(str)) { theo = ns->theory(str); ++counter; }
			AbstractStructure* structure = 0;
			if(ns->isStructure(str)) { structure = ns->structure(str); ++counter; }
			Options* opts = 0;
			if(ns->isOptions(str)) { opts = ns->options(str); ++counter; }
			UserProcedure* proc = 0;
			if(ns->isProc(str)) { proc = ns->procedure(str); ++counter; }
			Query* query = 0;
			if(ns->isQuery(str)) { query = ns->query(str); ++counter;	}

			if(counter == 0) return 0;
			else if(counter == 1) {
				if(subsp) return convertToLua(L,InternalArgument(subsp));
				if(vocab) return convertToLua(L,InternalArgument(vocab));
				if(theo) return convertToLua(L,InternalArgument(theo));
				if(structure) return convertToLua(L,InternalArgument(structure));
				if(opts) return convertToLua(L,InternalArgument(opts));
				if(proc) {
					compile(proc, L);
					lua_getfield(L,LUA_REGISTRYINDEX,proc->registryindex().c_str());
					return 1;
				}
				if(query) {
					return convertToLua(L,InternalArgument(query));
				}
				assert(false);
				return 0;
			}
			else {
				OverloadedObject* oo = new OverloadedObject();
				oo->insert(subsp);
				oo->insert(vocab);
				oo->insert(theo);
				oo->insert(structure);
				oo->insert(opts);
				oo->insert(proc);
				oo->insert(query);
				return convertToLua(L,InternalArgument(oo));
			}
		}
		else {
			lua_pushstring(L,"Namespaces can only be indexed by strings");
			return lua_error(L);
		}
	}

	SortTable* toDomain(vector<InternalArgument>* table, lua_State* L) {
		EnumeratedInternalSortTable* ist = new EnumeratedInternalSortTable();
		SortTable* st = new SortTable(ist);
		for(vector<InternalArgument>::const_iterator it = table->begin(); it != table->end(); ++it) {
			switch(it->_type) {
				case AT_INT:
					st->add(DomainElementFactory::instance()->create(it->_value._int)); break;
				case AT_DOUBLE:
					st->add(DomainElementFactory::instance()->create(it->_value._double)); break;
				case AT_STRING:
					st->add(DomainElementFactory::instance()->create(it->_value._string)); break;
				case AT_COMPOUND:
					st->add(DomainElementFactory::instance()->create(it->_value._compound)); break;
				default:
					delete(st);
					lua_pushstring(L,"Only numbers, strings, and compounds are allowed in a predicate table");
					lua_error(L);
					return 0;
			}
		}
		return st;
	}

	PredTable* toPredTable(vector<InternalArgument>* table, lua_State* L, const Universe& univ) {
		EnumeratedInternalPredTable* ipt = new EnumeratedInternalPredTable();
		for(vector<InternalArgument>::const_iterator it = table->begin(); it != table->end(); ++it) {
			if(it->_type == AT_TABLE) {
				ElementTuple tuple;
				for(vector<InternalArgument>::const_iterator jt = it->_value._table->begin();
					jt != it->_value._table->end(); ++jt) {
					switch(jt->_type) {
						case AT_INT:
							tuple.push_back(DomainElementFactory::instance()->create(jt->_value._int)); break;
						case AT_DOUBLE:
							tuple.push_back(DomainElementFactory::instance()->create(jt->_value._double)); break;
						case AT_STRING:
							tuple.push_back(DomainElementFactory::instance()->create(jt->_value._string)); break;
						case AT_COMPOUND:
							tuple.push_back(DomainElementFactory::instance()->create(jt->_value._compound)); break;
						default:
							lua_pushstring(L,"Only numbers, strings, and compounds are allowed in a predicate table");
							lua_error(L);
							return 0;
					}
				}
				ipt->add(tuple);
			}
			else if(it->_type == AT_TUPLE) ipt->add(*(it->_value._tuple));
			else {
				lua_pushstring(L,"Expected a two-dimensional table");
				lua_error(L);
				return 0;
			}
		}
		PredTable* pt = new PredTable(ipt,univ);
		return pt;
	}

	/**
	 * NewIndex function for predicate interpretations
	 */
	int predinterNewIndex(lua_State* L) {
		PredInter* predinter = *(PredInter**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2, L);
		InternalArgument value = createArgument(3, L);
		const Universe& univ = predinter->ct()->universe();
		if(index._type == AT_STRING) {
			const PredTable* pt = 0;
			if(value._type == AT_PREDTABLE) {
				pt = value._value._predtable;
			}
			else if(value._type == AT_TABLE) {
				pt = toPredTable(value._value._table,L,univ);
			}
			else {
				lua_pushstring(L,"Wrong argument to __newindex procedure of a predicate interpretation");
				return lua_error(L);
			}
			assert(pt);
			string str = *(index._value._string);
			if(str == "ct") predinter->ct(new PredTable(pt->interntable(),univ));
			else if(str == "pt") predinter->pt(new PredTable(pt->interntable(),univ));
			else if(str == "cf") predinter->cf(new PredTable(pt->interntable(),univ));
			else if(str == "pf") predinter->pf(new PredTable(pt->interntable(),univ));
			else {
				lua_pushstring(L,"A predicate interpretation can only be indexed by \"ct\", \"cf\", \"pt\", and \"pf\"");
				return lua_error(L);
			}
			return 0;
		}
		else {
			lua_pushstring(L,"A predicate interpretation can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * NewIndex function for function interpretations
	 */
	int funcinterNewIndex(lua_State* L) {
		FuncInter* funcinter = *(FuncInter**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2, L);
		InternalArgument value = createArgument(3, L);
		if(index._type == AT_STRING) {
			if((*(index._value._string)) == "graph") {
				if(value._type == AT_PREDINTER) {
					funcinter->graphinter(value._value._predinter);
					return 0;
				}
				else {
					lua_pushstring(L,"Expected a predicate interpretation");
					return lua_error(L);
				}
			}
			else {
				lua_pushstring(L,"A function interpretation can only be indexed by \"graph\"");
				return lua_error(L);
			}
		}
		else {
			lua_pushstring(L,"A function interpretation can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * NewIndex function for structures
	 */
	int structureNewIndex(lua_State* L) {
		AbstractStructure* structure = *(AbstractStructure**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2, L);
		InternalArgument value = createArgument(3, L);
		switch(index._type) {
			case AT_SORT:
			{
				set<Sort*>* ss = index._value._sort;
				if(ss->size() == 1) {
					Sort* s = *(ss->begin());
					if(value._type == AT_DOMAIN) {
						SortTable* st = structure->inter(s);
						st->interntable(value._value._domain->interntable());
						return 0;
					}
					else if(value._type == AT_TABLE) {
						SortTable* dom = toDomain(value._value._table,L);
						SortTable* st = structure->inter(s);
						st->interntable(dom->interntable());
						delete(dom);
						return 0;
					}
					else {
						lua_pushstring(L,"Expected a table or a domain");
						return lua_error(L);
					}
				}
				break;
			}
			case AT_PREDICATE:
			{
				set<Predicate*>* sp = index._value._predicate;
				if(sp->size() == 1) {
					Predicate* p = *(sp->begin());
					if(value._type == AT_PREDINTER) {
						structure->inter(p,value._value._predinter->clone(structure->universe(p)));
						return 0;
					}
					else {
						lua_pushstring(L,"Expected a predicate interpretation");
						return lua_error(L);
					}
				}
				break;
			}
			case AT_FUNCTION:
			{
				set<Function*>* sf = index._value._function;
				if(sf->size() == 1) {
					Function* f = *(sf->begin());
					if(value._type == AT_FUNCINTER) {
						structure->inter(f,value._value._funcinter->clone(structure->universe(f)));
						return 0;
					}
					else {
						lua_pushstring(L,"Expected a function interpretation");
						return lua_error(L);
					}
				}
				break;
			}
			default:
				break;
		}
		lua_pushstring(L,"A structure can only be indexed by a single type, predicate, or function symbol");
		return lua_error(L);
	}

	/**
	 * NewIndex function for options
	 */
	int optionsNewIndex(lua_State* L) {
		Options* opts = *(Options**)lua_touserdata(L,1);
		InternalArgument index = createArgument(2,L);
		InternalArgument value = createArgument(3,L);
		if(index._type == AT_STRING) {
			string str = *(index._value._string);
			switch(value._type) {
				case AT_INT: opts->setvalue(str,value._value._int); break;
				case AT_DOUBLE: opts->setvalue(str,value._value._double); break;
				case AT_STRING: opts->setvalue(str,*(value._value._string)); break;
				case AT_BOOLEAN: opts->setvalue(str,value._value._boolean); break;
				default:
					lua_pushstring(L,"Wrong option value");
					return lua_error(L);
			}
			return 0;
		}
		else {
			lua_pushstring(L,"Options can only be indexed by a string");
			return lua_error(L);
		}
	}

	/**
	 * Call function for predicate tables
	 */
	int predtableCall(lua_State* L) {
		PredTable* predtable = *(PredTable**)lua_touserdata(L,1);
		lua_remove(L,1);
		ElementTuple tuple;
		for(int n = 1; n <= lua_gettop(L); ++n) {
			InternalArgument currarg = createArgument(n,L);
			switch(currarg._type) {
				case AT_INT:
					tuple.push_back(DomainElementFactory::instance()->create(currarg._value._int)); break;
				case AT_DOUBLE:
					tuple.push_back(DomainElementFactory::instance()->create(currarg._value._double)); break;
				case AT_STRING:
					tuple.push_back(DomainElementFactory::instance()->create(currarg._value._string)); break;
				case AT_COMPOUND:
					tuple.push_back(DomainElementFactory::instance()->create(currarg._value._compound)); break;
				case AT_TUPLE:
					if(n == 1 && lua_gettop(L) == 1) {
						tuple = *(currarg._value._tuple);
						break;
					}
				default:
					lua_pushstring(L,"Only numbers, strings, and compounds can be arguments of a predicate table");
					lua_error(L);
					return 0;
			}
		}
		lua_pushboolean(L,predtable->contains(tuple));
		return 1;
	}

	/**
	 * Call function for function interpretations
	 */
	int funcinterCall(lua_State* L) {
		FuncInter* funcinter = *(FuncInter**)lua_touserdata(L,1);
		if(funcinter->approxtwovalued()) {
			FuncTable* ft = funcinter->functable();
			lua_remove(L,1);
			unsigned int nrargs = lua_gettop(L);
			if(nrargs == 1) {
				InternalArgument argone = createArgument(1,L);
				if(argone._type == AT_TUPLE) {
					ElementTuple tuple = *(argone._value._tuple);
					while(tuple.size() > ft->arity()) tuple.pop_back();
					while(tuple.size() < ft->arity()) tuple.push_back(0);
					const DomainElement* d = (*ft)[tuple];
					return convertToLua(L,d);
				}
			}
			ElementTuple tuple;
			for(unsigned int n = 1; n <= nrargs; ++n) {
				InternalArgument arg = createArgument(n,L);
				switch(arg._type) {
					case AT_INT:
						tuple.push_back(DomainElementFactory::instance()->create(arg._value._int)); break;
					case AT_DOUBLE:
						tuple.push_back(DomainElementFactory::instance()->create(arg._value._double)); break;
					case AT_STRING:
						tuple.push_back(DomainElementFactory::instance()->create(arg._value._string)); break;
					case AT_COMPOUND:
						tuple.push_back(DomainElementFactory::instance()->create(arg._value._compound)); break;
					default:
						lua_pushstring(L,"Only numbers, strings, and compounds can be arguments of a function interpretation");
						lua_error(L);
						return 0;
				}
			}
			while(tuple.size() > ft->arity()) tuple.pop_back();
			while(tuple.size() < ft->arity()) tuple.push_back(0);
			const DomainElement* d = (*ft)[tuple];
			return convertToLua(L,d);
		}
		else {
			lua_pushstring(L,"Only two-valued function interpretations can be called");
			return lua_error(L);
		}
	}

	/**
	 * Arity function for predicate symbols
	 */
	int predicateArity(lua_State* L) {
		set<Predicate*>* pred = *(set<Predicate*>**)lua_touserdata(L,1);
		InternalArgument arity = createArgument(2,L);
		if(arity._type == AT_INT) {
			set<Predicate*>* newpred = new set<Predicate*>();
			for(set<Predicate*>::const_iterator it = pred->begin(); it != pred->end(); ++it) {
				if((int)(*it)->arity() == arity._value._int) newpred->insert(*it);
			}
			InternalArgument np(newpred);
			return convertToLua(L,np);
		}
		else {
			lua_pushstring(L,"The arity of a predicate must be an integer");
			return lua_error(L);
		}
	}

	/**
	 * Arity function for function symbols
	 */
	int functionArity(lua_State* L) {
		set<Function*>* func = *(set<Function*>**)lua_touserdata(L,1);
		InternalArgument arity = createArgument(2,L);
		if(arity._type == AT_INT) {
			set<Function*>* newfunc = new set<Function*>();
			for(set<Function*>::const_iterator it = func->begin(); it != func->end(); ++it) {
				if((int)(*it)->arity() == arity._value._int) newfunc->insert(*it);
			}
			InternalArgument nf(newfunc);
			return convertToLua(L,nf);
		}
		else {
			lua_pushstring(L,"The arity of a function must an integer");
			return lua_error(L);
		}
	}

	/**
	 * Arity function for symbols
	 */
	int symbolArity(lua_State* L) {
		OverloadedSymbol* symb = *(OverloadedSymbol**)lua_touserdata(L,1);
		InternalArgument arity = createArgument(2,L);
		if(arity._type == AT_TABLE) {
			convertToLua(L,InternalArgument(new set<Function*>(*(symb->funcs()))));
			lua_replace(L,1);
			return functionArity(L);
		}
		else if(arity._type == AT_INT) {
			convertToLua(L,InternalArgument(new set<Predicate*>(*(symb->preds()))));
			lua_replace(L,1);
			return predicateIndex(L);
		}
		else {
			lua_pushstring(L,"The arity of a symbol must be an integer or of the form \'integer : 1\'");
			return lua_error(L);
		}
	}

	typedef pair<int (*)(lua_State*), string> tablecolheader;

	void createNewTable(lua_State* L, ArgType type, vector<tablecolheader> elements){
		bool newtable = luaL_newmetatable(L,toCString(type))!=0;
		assert(newtable);
		if(newtable){
			lua_pushinteger(L,type);
			lua_setfield(L,-2,_typefield);
			for(auto i=elements.begin(); i<elements.end(); ++i){
				lua_pushcfunction(L,(*i).first);
				lua_setfield(L,-2,(*i).second.c_str());
			}
			lua_pop(L,1);
		}
	}

	void internProcMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcInternProc, "__gc"));
		elements.push_back(tablecolheader(&internalCall, "__call"));

		bool newtable = luaL_newmetatable(L,"internalprocedure")!=0;
		assert(newtable);
		if(newtable){
			for(auto i=elements.begin(); i<elements.end(); ++i){
				lua_pushcfunction(L,(*i).first);
				lua_setfield(L,-2,(*i).second.c_str());
			}
			lua_pop(L,1);
		}
	}

	void sortMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcSort, "__gc"));
		createNewTable(L, AT_SORT, elements);
	}

	void predicateMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcPredicate, "__gc"));
		elements.push_back(tablecolheader(&predicateIndex, "__index"));
		elements.push_back(tablecolheader(&predicateArity, "__div"));
		createNewTable(L, AT_PREDICATE, elements);
	}

	void functionMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcFunction, "__gc"));
		elements.push_back(tablecolheader(&functionIndex, "__index"));
		elements.push_back(tablecolheader(&functionArity, "__div"));
		createNewTable(L, AT_FUNCTION, elements);
	}

	void symbolMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcInternProc, "__gc"));
		elements.push_back(tablecolheader(&symbolIndex, "__index"));
		elements.push_back(tablecolheader(&symbolArity, "__div"));
		createNewTable(L, AT_SYMBOL, elements);
	}

	void vocabularyMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcVocabulary, "__gc"));
		elements.push_back(tablecolheader(&vocabularyIndex, "__index"));
		createNewTable(L, AT_VOCABULARY, elements);
	}

	void compoundMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcCompound, "__gc"));
		createNewTable(L, AT_COMPOUND, elements);
	}

	void domainatomMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcDomainAtom, "__gc"));
		elements.push_back(tablecolheader(&domainatomIndex, "__index"));
		createNewTable(L, AT_DOMAINATOM, elements);
	}

	void tupleMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcTuple, "__gc"));
		elements.push_back(tablecolheader(&tupleIndex, "__index"));
		elements.push_back(tablecolheader(&tupleLen, "__len"));
		createNewTable(L, AT_TUPLE, elements);
	}

	void domainMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcDomain, "__gc"));
		createNewTable(L, AT_DOMAIN, elements);
	}

	void predtableMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcPredTable, "__gc"));
		elements.push_back(tablecolheader(&predtableCall, "__call"));
		createNewTable(L, AT_PREDTABLE, elements);
	}

	void predinterMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcPredInter, "__gc"));
		elements.push_back(tablecolheader(&predinterIndex, "__index"));
		elements.push_back(tablecolheader(&predinterNewIndex, "__newindex"));
		createNewTable(L, AT_PREDINTER, elements);
	}

	void funcinterMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcFuncInter, "__gc"));
		elements.push_back(tablecolheader(&funcinterIndex, "__index"));
		elements.push_back(tablecolheader(&funcinterNewIndex, "__newindex"));
		elements.push_back(tablecolheader(&funcinterCall, "__call"));
		createNewTable(L, AT_FUNCINTER, elements);
	}

	void structureMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcStructure, "__gc"));
		elements.push_back(tablecolheader(&structureIndex, "__index"));
		elements.push_back(tablecolheader(&structureNewIndex, "__newindex"));
		createNewTable(L, AT_STRUCTURE, elements);
	}

	void tableiteratorMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcTableIterator, "__gc"));
		createNewTable(L, AT_TABLEITERATOR, elements);
	}

	void domainiteratorMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcDomainIterator, "__gc"));
		createNewTable(L, AT_DOMAINITERATOR, elements);
	}

	void theoryMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcTheory, "__gc"));
		createNewTable(L, AT_THEORY, elements);
	}

	void formulaMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcFormula, "__gc"));
		createNewTable(L, AT_FORMULA, elements);
	}

	void queryMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcQuery, "__gc"));
		createNewTable(L, AT_QUERY, elements);
	}

	void optionsMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcOptions, "__gc"));
		elements.push_back(tablecolheader(&optionsIndex, "__index"));
		elements.push_back(tablecolheader(&optionsNewIndex, "__newindex"));
		createNewTable(L, AT_OPTIONS, elements);
	}

	void namespaceMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcNamespace, "__gc"));
		elements.push_back(tablecolheader(&namespaceIndex, "__index"));
		createNewTable(L, AT_NAMESPACE, elements);
	}

	void overloadedMetaTable(lua_State* L) {
		vector<tablecolheader> elements;
		elements.push_back(tablecolheader(&gcOverloaded, "__gc"));
		createNewTable(L, AT_OVERLOADED, elements);
	}

	InternalArgument createrange(const vector<InternalArgument>& args, lua_State* ) {
		int n1 = args[0]._value._int;
		int n2 = args[1]._value._int;
		InternalArgument ia; ia._type = AT_DOMAIN;
		if(n1 <= n2) {
			ia._value._domain = new SortTable(new IntRangeInternalSortTable(n1,n2));
		}
		else {
			ia._value._domain = new SortTable(new EnumeratedInternalSortTable());
		}
		_luadomains.insert(ia._value._domain);
		return ia;
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
		domainatomMetaTable(L);

		theoryMetaTable(L);
		formulaMetaTable(L);
		queryMetaTable(L);
		optionsMetaTable(L);
		namespaceMetaTable(L);

		overloadedMetaTable(L);
	}

	/**
	 * map internal procedure names to the actual procedures
	 */
	typedef map<vector<ArgType>,InternalProcedure*> internalprocargmap;
	typedef map<string,internalprocargmap > internalproclist;
	internalproclist _internalprocedures;

	/**
	 *	\brief Add a new internal procedure
	 *
	 *	\param name			the name of the internal procedure
	 *	\param argtypes		the type of the arguments of the procedure
	 *	\param execute		the implementation of the procedure
	 *
	 */
/*	void addInternalProcedure(const string& name,
					   const vector<ArgType>& argtypes,
					   InternalArgument (*execute)(const vector<InternalArgument>&, lua_State*)) {
		InternalProcedure* proc = new InternalProcedure(name,argtypes,execute);
		_internalprocedures[name][argtypes] = proc;
	}*/

	/**
	 * Adds all internal procedures
	 */
	void addInternProcs(lua_State* L) {
		// arguments of internal procedures
/*		FIXME vector<ArgType> vempty(0);
		vector<ArgType> vint(1,AT_INT);
		vector<ArgType> vtheo(1,AT_THEORY);
		vector<ArgType> vstruct(1,AT_STRUCTURE);
		vector<ArgType> vspace(1,AT_NAMESPACE);
		vector<ArgType> vvoc(1,AT_VOCABULARY);
		vector<ArgType> vpredtable(1,AT_PREDTABLE);
		vector<ArgType> vdomain(1,AT_DOMAIN);
		vector<ArgType> vform(1,AT_FORMULA);
		vector<ArgType> vtabitertuple(2); vtabitertuple[0] = AT_TABLEITERATOR; vtabitertuple[1] = AT_TUPLE;
		vector<ArgType> vtheoopt(2); vtheoopt[0] = AT_THEORY; vtheoopt[1] = AT_OPTIONS;
		vector<ArgType> vformopt(2); vformopt[0] = AT_FORMULA; vformopt[1] = AT_OPTIONS;
		vector<ArgType> vstructopt(2); vstructopt[0] = AT_STRUCTURE; vstructopt[1] = AT_OPTIONS;
		vector<ArgType> vdomainatomopt(2); vdomainatomopt[0] = AT_DOMAINATOM; vdomainatomopt[1] = AT_OPTIONS;
		vector<ArgType> vstructvoc(2); vstructvoc[0] = AT_STRUCTURE; vstructvoc[1] = AT_VOCABULARY;
		vector<ArgType> vvocopt(2); vvocopt[0] = AT_VOCABULARY; vvocopt[1] = AT_OPTIONS;
		vector<ArgType> voptopt(2); voptopt[0] = AT_OPTIONS; voptopt[1] = AT_OPTIONS;
		vector<ArgType> vspaceopt(2); vspaceopt[0] = AT_NAMESPACE; vspaceopt[1] = AT_OPTIONS;
		vector<ArgType> vintint(2); vintint[0] = AT_INT; vintint[1] = AT_INT;
		vector<ArgType> vdomiterint(2); vdomiterint[0] = AT_DOMAINITERATOR; vdomiterint[1] = AT_INT;
		vector<ArgType> vdomiterdouble(2); vdomiterdouble[0] = AT_DOMAINITERATOR; vdomiterdouble[1] = AT_DOUBLE;
		vector<ArgType> vdomiterstring(2); vdomiterstring[0] = AT_DOMAINITERATOR; vdomiterstring[1] = AT_STRING;
		vector<ArgType> vdomitercomp(2); vdomitercomp[0] = AT_DOMAINITERATOR; vdomitercomp[1] = AT_COMPOUND;
		vector<ArgType> vpritab(2); vpritab[0] = AT_PREDINTER; vpritab[1] = AT_TABLE;
		vector<ArgType> vpritup(2); vpritup[0] = AT_PREDINTER; vpritup[1] = AT_TUPLE;
		vector<ArgType> vquerystruct(2); vquerystruct[0] = AT_QUERY; vquerystruct[1] = AT_STRUCTURE;
		vector<ArgType> vtheostructopt(3);
			vtheostructopt[0] = AT_THEORY;
			vtheostructopt[1] = AT_STRUCTURE;
			vtheostructopt[2] = AT_OPTIONS;
		vector<ArgType> vtheotheo(2); vtheotheo[0] = AT_THEORY; vtheotheo[1] = AT_THEORY;



		// Create internal procedures
		addInternalProcedure("idptype",vint,&idptype);
		addInternalProcedure("tostring",vtheoopt,&printtheory);
		addInternalProcedure("tostring",vformopt,&printformula);
		addInternalProcedure("tostring",vstructopt,&printstructure);
		addInternalProcedure("tostring",vvocopt,&printvocabulary);
		addInternalProcedure("tostring",vdomainatomopt,&printdomainatom);
		addInternalProcedure("namespace",vspaceopt,&printnamespace);
		addInternalProcedure("tostring",voptopt,&printoptions);
		addInternalProcedure("help",vspace,&help);
		addInternalProcedure("newstructure",vvoc,&newstructure);
		addInternalProcedure("newtheory",vvoc,&newtheory);
		addInternalProcedure("newoptions",vempty,&newoptions);
		addInternalProcedure("clone",vtheo,&clonetheory);
		addInternalProcedure("clone",vstruct,&clonestructure);
		addInternalProcedure("push_negations",vtheo,&pushnegations);
		addInternalProcedure("flatten",vtheo,&flatten);
		addInternalProcedure("mx",vtheostructopt,&modelexpand);
		addInternalProcedure("ground",vtheostructopt,&ground);
		addInternalProcedure("propagate",vtheostructopt,&propagate);
		addInternalProcedure("range",vintint,&createrange);
		addInternalProcedure("dummytuple",vempty,&createtuple);
		addInternalProcedure("deref_and_increment",vtabitertuple,&derefandincrement);
		addInternalProcedure("deref_and_increment",vdomiterint,&domderefandincrement);
		addInternalProcedure("deref_and_increment",vdomiterstring,&domderefandincrement);
		addInternalProcedure("deref_and_increment",vdomiterdouble,&domderefandincrement);
		addInternalProcedure("deref_and_increment",vdomitercomp,&domderefandincrement);
		addInternalProcedure("tableiterator",vpredtable,&tableiterator);
		addInternalProcedure("domainiterator",vdomain,&domainiterator);
		addInternalProcedure("changevocabulary",vstructvoc,&changevocabulary);
		addInternalProcedure("maketrue",vpritab,&maketabtrue);
		addInternalProcedure("maketrue",vpritup,&maketrue);
		addInternalProcedure("makefalse",vpritab,&maketabfalse);
		addInternalProcedure("makefalse",vpritup,&makefalse);
		addInternalProcedure("makeunknown",vpritab,&maketabunknown);
		addInternalProcedure("makeunknown",vpritup,&makeunknown);
		addInternalProcedure("completion",vtheo,&completion);
		addInternalProcedure("estimate_nr_ans",vquerystruct,&estimatenrans);
		addInternalProcedure("estimate_cost",vquerystruct,&estimatecost);
		addInternalProcedure("query",vquerystruct,&query);
		addInternalProcedure("bddstring",vform,&tobdd);
		addInternalProcedure("clean",vstruct,&clean);
		addInternalProcedure("merge",vtheotheo,&mergetheories);*/

		// Add the internal procedures to lua
		lua_getglobal(L,getLibraryName().c_str());
		for(internalproclist::iterator it =	_internalprocedures.begin(); it != _internalprocedures.end(); ++it) {
			addUserData(L, new internalprocargmap(it->second), "internalprocedure");
			lua_setfield(L,-2,it->first.c_str());
		}
		lua_pop(L,1);
	}

	/**
	 * Establish the connection with lua.
	 */
	void makeLuaConnection() {
		// Create the lua state
		_state = lua_open();
		luaL_openlibs(_state);

		// Create all metatables
		createMetaTables(_state);

		// Create the global table for the internal library
		lua_newtable(_state);
		lua_setglobal(_state,getLibraryName().c_str());

		// Add internal procedures
		addInternProcs(_state);

		// Overwrite some standard lua procedures
		stringstream ss;
		ss << DATADIR << getLuaLibraryFilename();
		int err = luaL_dofile(_state,ss.str().c_str());
		if(err) {
			cerr << lua_tostring(_state,-1) << endl;
			cerr << "Error in " <<getLuaLibraryFilename() <<".\n"; exit(1);
		}

		// Add the global namespace and standard options
		addGlobal(Namespace::global());
		addGlobal(Namespace::global()->options("stdoptions"));

		// Parse standard input file
		stringstream ss1;
		ss1 << DATADIR <<getIDPLibraryFilename();
		parsefile(ss1.str());

		// Parse configuration file
		stringstream ss2;
		ss2 << RCDIR <<getConfigFilename();
		err = luaL_dofile(_state,ss2.str().c_str());
		if(err) { cerr << "Error in configuration file\n"; exit(1); }

	}

	/**
	 * End the connection with lua
	 */
	void closeLuaConnection() {
		lua_close(_state);
		for(map<string,map<vector<ArgType>,InternalProcedure*> >::iterator it =	_internalprocedures.begin();
			it != _internalprocedures.end(); ++it) {
			for(map<vector<ArgType>,InternalProcedure*>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
				delete(jt->second);
			}
		}
		delete(Namespace::global());
		delete(DomainElementFactory::instance());
	}

	void execute(stringstream* chunk) {
		int err = luaL_dostring(_state,chunk->str().c_str());
		if(err) {
			Error::error();
			cerr << string(lua_tostring(_state,-1)) << endl;
			lua_pop(_state,1);
		}
	}

	void pushglobal(const vector<string>& name, const ParseInfo& pi) {
		lua_getglobal(_state,name[0].c_str());
		for(unsigned int n = 1; n < name.size(); ++n) {
			if(lua_istable(_state,-1)) {
				lua_getfield(_state,-1,name[n].c_str());
				lua_remove(_state,-2);
			}
			else {
				Error::error(pi);
				cerr << "unknown object" << endl;
			}
		}
	}

	InternalArgument* call(const vector<string>& proc, const vector<vector<string> >& args, const ParseInfo& pi) {
		pushglobal(proc,pi);
		for(unsigned int n = 0; n < args.size(); ++n) pushglobal(args[n],pi);
		int err = lua_pcall(_state,args.size(),1,0);
		if(err) {
			Error::error(pi);
			cerr << lua_tostring(_state,-1) << endl;
			lua_pop(_state,1);
			return 0;
		}
		else {
			InternalArgument* ia = new InternalArgument(createArgument(-1, _state));
			lua_pop(_state,1);
			return ia;
		}
	}

	const DomainElement* funccall(string* procedure, const ElementTuple& input) {
		lua_getfield(_state,LUA_REGISTRYINDEX,procedure->c_str());
		for(ElementTuple::const_iterator it = input.begin(); it != input.end(); ++it) {
			convertToLua(_state,*it);
		}
		int err = lua_pcall(_state,input.size(),1,0);
		if(err) {
			Error::error();
			cerr << string(lua_tostring(_state,-1)) << endl;
			lua_pop(_state,1);
			return 0;
		}
		else {
			const DomainElement* d = convertToElement(-1,_state);
			lua_pop(_state,1);
			return d;
		}
	}

	bool predcall(string* procedure, const ElementTuple& input) {
		lua_getfield(_state,LUA_REGISTRYINDEX,procedure->c_str());
		for(ElementTuple::const_iterator it = input.begin(); it != input.end(); ++it) {
			convertToLua(_state,*it);
		}
		int err = lua_pcall(_state,input.size(),1,0);
		if(err) {
			Error::error();
			cerr << string(lua_tostring(_state,-1)) << endl;
			lua_pop(_state,1);
			return 0;
		}
		else {
			bool b = lua_toboolean(_state,-1);
			lua_pop(_state,1);
			return b;
		}
	}

	void compile(UserProcedure* proc) {
		compile(proc, _state);
	}

	template<> void addGlobal(UserProcedure* p){
		InternalArgument ia;
		ia._type = AT_PROCEDURE;
		ia._value._string = StringPointer(p->registryindex());
		convertToLua(getState(),ia);
		lua_setglobal(getState(),p->name().c_str());
	}

	AbstractStructure* structure(InternalArgument* arg) { return arg->_type==AT_STRUCTURE?arg->_value._structure:NULL; }
	AbstractTheory* theory(InternalArgument* arg) { return arg->_type==AT_THEORY?arg->_value._theory:NULL; }
	Vocabulary* vocabulary(InternalArgument* arg) { return arg->_type==AT_VOCABULARY?arg->_value._vocabulary:NULL; }

	string*	getProcedure(const std::vector<std::string>& name, const ParseInfo& pi) {
		pushglobal(name,pi);
		InternalArgument ia = createArgument(-1,_state);
		lua_pop(_state,1);
		if(ia._type == AT_PROCEDURE) {
			return ia._value._string;
		}
		else return 0;
	}

}