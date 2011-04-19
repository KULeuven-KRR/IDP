/************************************
	execute.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "execute.hpp"
#include "namespace.hpp"
#include "element.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "ground.hpp"
#include "ecnf.hpp"
#include "print.hpp"
#include "data.hpp"
#include "options.hpp"
#include "lua.hpp"
#include "error.hpp"
#include "fobdd.hpp"
#include "external/MonitorInterface.hpp"

using namespace std;

/*
	Connection with lua
*/

int LuaProcCompileNumber = 0;
int LuaArgProcNumber = 0;

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
		_inferences["delete"].push_back(new DeleteData(IAT_PREDICATE));
		_inferences["delete"].push_back(new DeleteData(IAT_FUNCTION));
		_inferences["delete"].push_back(new DeleteData(IAT_SORT));
		_inferences["delete"].push_back(new DeleteData(IAT_PREDINTER));
		_inferences["delete"].push_back(new DeleteData(IAT_FUNCINTER));
		_inferences["delete"].push_back(new DeleteData(IAT_PREDTABLE));
		_inferences["delete"].push_back(new DeleteData(IAT_TUPLE));

		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_PREDICATE));
		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_FUNCTION));
		_inferences["index"].push_back(new GetIndex(IAT_STRUCTURE,IAT_SORT));
		_inferences["index"].push_back(new GetIndex(IAT_NAMESPACE,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_VOCABULARY,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_OPTIONS,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_PREDINTER,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_FUNCINTER,IAT_STRING));
		_inferences["index"].push_back(new GetIndex(IAT_PREDTABLE,IAT_INT));
		_inferences["index"].push_back(new GetIndex(IAT_TUPLE,IAT_INT));

		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_PREDICATE,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_FUNCTION,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_SORT,IAT_TABLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_PREDICATE,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_FUNCTION,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_STRUCTURE,IAT_SORT,IAT_PROCEDURE));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_DOUBLE));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_BOOLEAN));
		_inferences["newindex"].push_back(new SetIndex(IAT_OPTIONS,IAT_STRING,IAT_STRING));

		_inferences["len"].push_back(new LenghtOperator(IAT_PREDTABLE));
		_inferences["len"].push_back(new LenghtOperator(IAT_TUPLE));

		_inferences["cast"].push_back(new CastOperator());
		_inferences["aritycast"].push_back(new ArityCastOperator(IAT_PREDICATE));
		_inferences["aritycast"].push_back(new ArityCastOperator(IAT_FUNCTION));
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
				return("predicate_symbol");
			case IAT_FUNCTION:
				return("function_symbol");
			case IAT_PREDTABLE:
				return("predicate_table");
			case IAT_PREDINTER:
				return("predicate_interpretation");
			case IAT_FUNCINTER:
				return("function_interpretation");
			case IAT_TUPLE:
				return("tuple");
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

	InfArgType typeenum(const string& strtype) {
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
		if(strtype == "predicate_table") return IAT_PREDTABLE;
		if(strtype == "function") return IAT_PROCEDURE;
		if(strtype == "overloaded") return IAT_OVERLOADED;
		if(strtype == "sort") return IAT_SORT;
		if(strtype == "predicate_symbol") return IAT_PREDICATE;
		if(strtype == "function_symbol") return IAT_FUNCTION;
		if(strtype == "predicate_interpretation") return IAT_PREDINTER;
		if(strtype == "function_interpretation") return IAT_FUNCINTER;
		if(strtype == "tuple") return IAT_TUPLE;
		if(strtype == "mult") return IAT_MULT;
		if(strtype == "registry") return IAT_REGISTRY;
		assert(false); return IAT_INT;
	}

	InfArgType typeenum(lua_State* L, int index) {
		string strtype = typestring(L,index);
		return typeenum(strtype);
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
				case IAT_PROCEDURE:
					return (obj->isProcedure());
				case IAT_SORT:
					return (obj->isSort());
				case IAT_PREDICATE:
					return (obj->isPredicate());
				case IAT_FUNCTION:
					return (obj->isFunction());
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
			case IAT_SORT:
			{
				set<Sort*>** ptr = (set<Sort*>**) lua_newuserdata(L,sizeof(set<Sort*>*));
				(*ptr) = res._sort;
				luaL_getmetatable (L,"sort");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_PREDICATE:
			{
				set<Predicate*>** ptr = (set<Predicate*>**) lua_newuserdata(L,sizeof(set<Predicate*>*));
				(*ptr) = res._predicate;
				luaL_getmetatable (L,"predicate_symbol");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_FUNCTION:
			{
				set<Function*>** ptr = (set<Function*>**) lua_newuserdata(L,sizeof(set<Function*>*));
				(*ptr) = res._function;
				luaL_getmetatable (L,"function_symbol");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_PREDINTER:
			{
				PredInter** ptr = (PredInter**) lua_newuserdata(L,sizeof(set<PredInter*>*));
				(*ptr) = res._predinter;
				luaL_getmetatable (L,"predicate_interpretation");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_FUNCINTER:
			{
				FuncInter** ptr = (FuncInter**) lua_newuserdata(L,sizeof(set<FuncInter*>*));
				(*ptr) = res._funcinter;
				luaL_getmetatable (L,"function_interpretation");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_PREDTABLE:
			{
				PredTable** ptr = (PredTable**) lua_newuserdata(L,sizeof(set<PredTable*>*));
				(*ptr) = res._predtable;
				luaL_getmetatable (L,"predicate_table");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_TUPLE:
			{
				PredTableTuple** ptr = (PredTableTuple**) lua_newuserdata(L,sizeof(set<PredTableTuple*>*));
				(*ptr) = res._tuple;
				luaL_getmetatable (L,"tuple");
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
					a._sort = *((set<Sort*>**)lua_touserdata(L,n));
					break;
				case IAT_PREDICATE:
					a._predicate = *((set<Predicate*>**)lua_touserdata(L,n));
					break;
				case IAT_FUNCTION:
					a._function = *((set<Function*>**)lua_touserdata(L,n));
					break;
				case IAT_PREDINTER:
					a._predinter = *((PredInter**)lua_touserdata(L,n));
					break;
				case IAT_FUNCINTER:
					a._funcinter = *((FuncInter**)lua_touserdata(L,n));
					break;
				case IAT_PREDTABLE:
					a._predtable = *((PredTable**)lua_touserdata(L,n));
					break;
				case IAT_TUPLE:
					a._tuple = *((PredTableTuple**)lua_touserdata(L,n));
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
				{
					string registryindex = "idp_argument_procedure" + itos(LuaArgProcNumber);
					++LuaArgProcNumber;
					lua_pushvalue(L,n);
					lua_setfield(L,LUA_REGISTRYINDEX,registryindex.c_str());
					break;
				}
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

int overloaddiv(lua_State* L) {
	InfArg a = BuiltinProcs::convertarg(L,1,IAT_OVERLOADED);
	OverloadedObject* obj = a._overloaded;
	InfArg b = BuiltinProcs::convertarg(L,2,IAT_INT);
	unsigned int div = b._int;
	set<Predicate*>* sp = 0;
	set<Function*>* sf = 0;
	if(obj->isPredicate()) {
		for(set<Predicate*>::iterator it = obj->getPredicate()->begin(); it != obj->getPredicate()->end(); ++it) {
			if((*it)->arity() == div) {
				if(!sp) sp = new set<Predicate*>();
				sp->insert(*it);
			}
		}
	}
	if(obj->isFunction()) {
		for(set<Function*>::iterator it = obj->getFunction()->begin(); it != obj->getFunction()->end(); ++it) {
			if((*it)->arity() == div) {
				if(!sf) sf = new set<Function*>();
				sf->insert(*it);
			}
		}
	}
	if(sp) {
		if(sf) {
			OverloadedObject* newobj = new OverloadedObject();
			newobj->setpredicate(sp);
			newobj->setfunction(sf);
			InfArg res; res._overloaded = newobj;
			BuiltinProcs::converttolua(L,res,IAT_OVERLOADED);
		}
		else {
			InfArg res; res._predicate = sp;
			BuiltinProcs::converttolua(L,res,IAT_PREDICATE);
		}
	}
	else if(sf) {
		InfArg res; res._function = sf;
		BuiltinProcs::converttolua(L,res,IAT_FUNCTION);
	}
	else {
		lua_pushnil(L);
	}
	return 1;
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

int idppredcall(lua_State* L) {
	InfArg a = BuiltinProcs::convertarg(L,1,IAT_PREDTABLE);
	PredTable* pt = a._predtable;
	vector<TypedElement> args;
	lua_remove(L,1);
	unsigned int nrargs = lua_gettop(L);
	for(unsigned int n = 1; n <= nrargs; ++n) {
		TypedInfArg tia = BuiltinProcs::convertarg(L,n);
		TypedElement te;
		switch(tia._type) {
			case IAT_INT:
				te._type = ELINT;
				te._element._int = tia._value._int;
				break;
			case IAT_DOUBLE:
				te._type = ELDOUBLE;
				te._element._double = tia._value._double;
				break;
			case IAT_STRING:
				te._type = ELSTRING;
				te._element._string = tia._value._string;
				break;
			default:
				te._type = ELINT;
				te._element = ElementUtil::nonexist(ELINT);
				break;
		}
		args.push_back(te);
		}
		while(args.size() > pt->arity()) args.pop_back();
		while(args.size() < pt->arity()) {
			TypedElement te;
			te._type = ELINT;
			te._element = ElementUtil::nonexist(ELINT);
			args.push_back(te);
		}
		if(pt->contains(args)) lua_pushboolean(L,1);
		else lua_pushboolean(L,0);
		return 1;
}

int idpfunccall(lua_State* L) {
	InfArg a = BuiltinProcs::convertarg(L,1,IAT_FUNCINTER);
	FuncTable* ft = a._funcinter->functable();
	if(ft) {
		vector<TypedElement> args;
		lua_remove(L,1);
		unsigned int nrargs = lua_gettop(L);
		for(unsigned int n = 1; n <= nrargs; ++n) {
			TypedInfArg tia = BuiltinProcs::convertarg(L,n);
			TypedElement te;
			switch(tia._type) {
				case IAT_INT:
					te._type = ELINT;
					te._element._int = tia._value._int;
					break;
				case IAT_DOUBLE:
					te._type = ELDOUBLE;
					te._element._double = tia._value._double;
					break;
				case IAT_STRING:
					te._type = ELSTRING;
					te._element._string = tia._value._string;
					break;
				default:
					te._type = ELINT;
					te._element = ElementUtil::nonexist(ELINT);
					break;
			}
			args.push_back(te);
		}
		while(args.size() > ft->arity()) args.pop_back();
		while(args.size() < ft->arity()) {
			TypedElement te;
			te._type = ELINT;
			te._element = ElementUtil::nonexist(ELINT);
			args.push_back(te);
		}
		Element e = (*ft)[args];
		if(ElementUtil::exists(e,ft->outtype())) {
			switch(ft->outtype()) {
				case ELINT:
					lua_pushinteger(L,e._int);
					break;
				case ELDOUBLE:
					lua_pushnumber(L,e._double);
					break;
				case ELSTRING:
					lua_pushstring(L,e._string->c_str());
					break;
				case ELCOMPOUND:
					lua_pushstring(L,IDPointer(ElementUtil::ElementToString(e,ELCOMPOUND))->c_str());
					break;
				default:
					assert(false);
			}
		}
		else lua_pushnil(L);
	}
	else {
		Error::threevalcall();
		lua_pushnil(L);
	}
	return 1;
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
		case IAT_PREDICATE: case IAT_FUNCTION: case IAT_SORT:
			// do nothing
			break;
		case IAT_TUPLE:
			delete(args[0]._tuple);
			break;
		case IAT_PREDINTER:
			// TODO
			break;
		case IAT_FUNCINTER:
			// TODO
			break;
		case IAT_PREDTABLE:
			// TODO
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
	string str = printer->print(args[0]._theory);
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
	string str = printer->print(args[0]._structure);
	delete(printer);
	TypedInfArg a; a._type = IAT_STRING; a._value._string = IDPointer(str);
	return a;
}

TypedInfArg PrintNamespace::execute(const vector<InfArg>& args, lua_State*) const {
	InfOptions* opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	string str = printer->print(args[0]._namespace);
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
			lua_pushstring(L,_translator->printAtom(a.getAtom().getValue()).c_str());
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
	//TODO pass other solver options from opts->_solveroptions
	SATSolver* solver = new SATSolver(modes);

	// Create grounder
	GrounderFactory gf(structure,opts->_cpsupport);
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
		for(vector<MinisatID::Model*>::const_iterator modelit = sol->getModels().begin(); modelit != sol->getModels().end(); ++modelit) {
//cerr << "---Building new model---" << endl;
			AbstractStructure* mod = structure->clone();
			set<PredInter*>	tobesorted1;
			set<FuncInter*>	tobesorted2;
//cerr << "-Normal SAT part-" << endl;
			for(vector<MinisatID::Literal>::const_iterator literalit = (*modelit)->literalinterpretations.begin();
					literalit != (*modelit)->literalinterpretations.end(); ++literalit) {
				PFSymbol* pfs = grounding->translator()->symbol(((*literalit).getAtom().getValue()));
				if(pfs && mod->vocabulary()->contains(pfs)) {
					vector<domelement> vd = grounding->translator()->args((*literalit).getAtom().getValue());
					vector<TypedElement> args = ElementUtil::convert(vd);
					if(pfs->ispred()) {
						mod->inter(pfs)->add(args,!((*literalit).hasSign()),true);
						tobesorted1.insert(mod->inter(pfs));
					}
					else {
						Function* function = dynamic_cast<Function*>(pfs);
//if(!((*literalit).hasSign())) cerr << "Adding value " << args.back()._element._int << " for function " << function->name() << endl;
//else cerr << "Adding impossible value " << args.back()._element._int << " for function " << function->name() << endl;
						mod->inter(function)->add(args,!((*literalit).hasSign()),true);
						tobesorted2.insert(mod->inter(function));
					}
				}
			}
//cerr << "-CP part-" << endl;
			for(vector<MinisatID::VariableEqValue>::const_iterator cpvarit = (*modelit)->variableassignments.begin();
					cpvarit != (*modelit)->variableassignments.end(); ++cpvarit) {
				Function* function = grounding->termtranslator()->function((*cpvarit).variable);
				if(function && mod->vocabulary()->contains(function)) {
					vector<domelement> vd = grounding->termtranslator()->args((*cpvarit).variable);
					vector<TypedElement> args = ElementUtil::convert(vd);
					TypedElement value((*cpvarit).value);
					args.push_back(value);
//cerr << "Adding value " << args.back()._element._int << " for function " << function->name() << endl;
					mod->inter(function)->add(args,true,true);
					tobesorted2.insert(mod->inter(function));
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

	// Return answer
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
	_intypes = vector<InfArgType>(3);
	_intypes[0] = IAT_THEORY; 
	_intypes[1] = IAT_STRUCTURE;
	_intypes[2] = IAT_OPTIONS;
	_description = "Ground the theory and structure and store the grounding";
}

TypedInfArg FastGrounding::execute(const vector<InfArg>& args, lua_State*) const {
	GrounderFactory factory(args[1]._structure,args[2]._options->_cpsupport);
	TopLevelGrounder* grounder = factory.create(args[0]._theory);
	grounder->run();
	TypedInfArg a; a._type = IAT_THEORY;
	a._value._theory = grounder->grounding();
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
		{
			AbstractStructure* as = args[0]._structure;
			switch(_intypes[1]) {
				case IAT_SORT:
					a = as->getObject(args[1]._sort);
					break;
				case IAT_PREDICATE:
					a = as->getObject(args[1]._predicate);
					break;
				case IAT_FUNCTION:
					a = as->getObject(args[1]._function);
					break;
				default:
					assert(false);
			}
			break;
		}
		case IAT_NAMESPACE:
		{
			Namespace* nsp = args[0]._namespace;
			string str = *(args[1]._string);
			a = nsp->getObject(str,L);
			break;
		}
		case IAT_VOCABULARY:
		{
			Vocabulary* voc = args[0]._vocabulary;
			string str = *(args[1]._string);
			a = voc->getObject(str);
			break;
		}
		case IAT_OPTIONS:
			a = args[0]._options->get(*(args[1]._string));
			break;
		case IAT_PREDINTER:
		{
			PredInter* pinter = args[0]._predinter;
			string str = *(args[1]._string);
			a._type = IAT_PREDTABLE;
			if(str == "ct") {
				if(pinter->ct()) a._value._predtable = pinter->ctpf();
				else {
					// TODO
					assert(false);
				}
			}
			else if(str == "cf") {
				if(pinter->cf()) a._value._predtable = pinter->cfpt();
				else {
					// TODO
					assert(false);
				}
			}
			else if(str == "pt") {
				if(!(pinter->cf())) a._value._predtable = pinter->cfpt();
				else {
					// TODO
					assert(false);
				}
			}
			else if(str == "pf") {
				if(!(pinter->ct())) a._value._predtable = pinter->ctpf();
				else {
					// TODO
					assert(false);
				}
			}
			else {
				// TODO Error message
				a._type = IAT_NIL;
			}
			break;
		}
		case IAT_FUNCINTER:
		{
			FuncInter* finter = args[0]._funcinter;
			string str = *(args[1]._string);
			if(str == "graph") {
				a._type = IAT_PREDINTER;
				a._value._predinter = finter->predinter();
			}
			else {
				// TODO Error message
				a._type = IAT_NIL;
			}
			break;
		}
		case IAT_PREDTABLE:
		{
			PredTable* pt = args[0]._predtable;
			unsigned int index = args[1]._int - 1;
			if(pt->finite() && index < pt->size()) {
				if(pt->arity() == 1) {
					Element e = pt->element(index,0);
					switch(pt->type(0)) {
						case ELINT: 
							a._type = IAT_INT;
							a._value._int = e._int;
							break;
						case ELDOUBLE: 
							a._type = IAT_DOUBLE;
							a._value._int = e._double;
							break;
						case ELSTRING:
							a._type = IAT_STRING;
							a._value._string = e._string;
							break;
						case ELCOMPOUND:
							a._type = IAT_STRING;
							a._value._string = IDPointer(ElementUtil::ElementToString(e,ELCOMPOUND));
							break;
						default:
							assert(false);
					}
				}
				else {
					a._type = IAT_TUPLE;
					a._value._tuple = new PredTableTuple(pt,index);
				}
			}
			else {
				// TODO Error message
				a._type = IAT_NIL;
			}
			break;
		}
		case IAT_TUPLE:
		{
			PredTableTuple* ptt = args[0]._tuple;
			unsigned int column = args[1]._int - 1;
			if(column < ptt->_table->size()) {
				Element e = ptt->_table->element(ptt->_index,column);
				switch(ptt->_table->type(column)) {
					case ELINT: 
						a._type = IAT_INT;
						a._value._int = e._int;
						break;
					case ELDOUBLE: 
						a._type = IAT_DOUBLE;
						a._value._int = e._double;
						break;
					case ELSTRING:
						a._type = IAT_STRING;
						a._value._string = e._string;
						break;
					case ELCOMPOUND:
						a._type = IAT_STRING;
						a._value._string = IDPointer(ElementUtil::ElementToString(e,ELCOMPOUND));
						break;
					default:
						assert(false);
				}
			}
			else {
				// TODO Error message
				a._type = IAT_NIL;
			}
			break;
		}
		default:
			assert(false);
	}
	return a;
}

TypedInfArg BDDPrinter::execute(const vector<InfArg>& args, lua_State*) const {
	AbstractTheory* theory = args[0]._theory;
	FOBDDManager manager;
	FOBDDFactory factory(&manager,theory->vocabulary());
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

TypedInfArg LenghtOperator::execute(const vector<InfArg>& args, lua_State*) const {
	TypedInfArg a;
	a._type = IAT_INT;
	if(_intypes[0] == IAT_PREDTABLE) {
		if(args[0]._predtable->finite()) {
			a._value._int = args[0]._predtable->size();
		}
		else {
			a._value._string = IDPointer(string("infinite"));
			a._type = IAT_STRING;
		}
	}
	else {
		a._value._int = args[0]._tuple->_table->arity();
	}
	return a;
}

TypedInfArg ArityCastOperator::execute(const vector<InfArg>& args, lua_State*) const {
	TypedInfArg result; result._type = IAT_NIL;
	unsigned int arity = args[1]._int;
	switch(_intypes[0]) {
		case IAT_PREDICATE:
		{
			set<Predicate*>* preds = args[0]._predicate;
			set<Predicate*>* newpreds = 0;
			for(set<Predicate*>::iterator it = preds->begin(); it != preds->end(); ++it) {
				if((*it)->arity() == arity) {
					if(!newpreds) {
						newpreds = new set<Predicate*>();
						result._type = IAT_PREDICATE;
						result._value._predicate = newpreds;
					}
					newpreds->insert(*it);
				}
			}
			break;
		}
		case IAT_FUNCTION:
		{
			set<Function*>* funcs = args[0]._function;
			set<Function*>* newfuncs = 0;
			for(set<Function*>::iterator it = funcs->begin(); it != funcs->end(); ++it) {
				if((*it)->arity() == arity) {
					if(!newfuncs) {
						newfuncs = new set<Function*>();
						result._type = IAT_FUNCTION;
						result._value._function = newfuncs;
					}
					newfuncs->insert(*it);
				}
			}
			break;
		}
		default:
			break;
	}
	return result;
}

TypedInfArg CastOperator::execute(const vector<InfArg>& args, lua_State*) const {
	OverloadedObject* obj = args[0]._overloaded;
	InfArgType newtype = BuiltinProcs::typeenum(*(args[1]._string));
	TypedInfArg result;
	result._type = IAT_NIL;
	switch(newtype) {
		case IAT_THEORY:
			if(obj->isTheory()) {
				result._type = IAT_THEORY;
				result._value._theory = obj->getTheory();
			}
			break;
		case IAT_STRUCTURE: 
			if(obj->isTheory()) {
				result._type = IAT_STRUCTURE;
				result._value._structure = obj->getStructure();
			}
			break;
		case IAT_VOCABULARY:
			if(obj->isVocabulary()) {
				result._type = IAT_VOCABULARY;
				result._value._vocabulary = obj->getVocabulary();
			}
			break;
		case IAT_NAMESPACE: 
			if(obj->isNamespace()) {
				result._type = IAT_NAMESPACE;
				result._value._namespace = obj->getNamespace();
			}
			break;
		case IAT_OPTIONS:
			if(obj->isOptions()) {
				result._type = IAT_OPTIONS;
				result._value._options = obj->getOptions();
			}
			break;
		case IAT_PROCEDURE:
			if(obj->isProcedure()) {
				result._type = IAT_PROCEDURE;
				result._value._procedure = obj->getProcedure();
			}
			break;
		case IAT_SORT:
			if(obj->isSort()) {
				result._type = IAT_SORT;
				result._value._sort = obj->getSort();
			}
			break;
		case IAT_PREDICATE:
			if(obj->isPredicate()) {
				result._type = IAT_PREDICATE;
				result._value._predicate = obj->getPredicate();
			}
			break;
		case IAT_FUNCTION:
			if(obj->isFunction()) {
				result._type = IAT_FUNCTION;
				result._value._function = obj->getFunction();
			}
			break;
		default:
			break;
	}
	return result;
}

/*************************
	Overloaded objects
*************************/

void OverloadedObject::makepredicate(Predicate* p) {
	if(!_predicate) _predicate = new set<Predicate*>();
	_predicate->insert(p);
}

void OverloadedObject::makefunction(Function* f) {
	if(!_function) _function = new set<Function*>();
	_function->insert(f);
}

void OverloadedObject::makesort(Sort* s) {
	if(!_sort) _sort = new set<Sort*>();
	_sort->insert(s);
}

bool OverloadedObject::isPredicate() const {
	if(_predicate) return (!(_predicate->empty()));
	else return false;
}

bool OverloadedObject::isFunction() const {
	if(_function) return (!(_function->empty()));
	else return false;
}

bool OverloadedObject::isSort() const {
	if(_sort) return (!(_sort->empty()));
	else return false;
}

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
	return count <= 1;
}

/*********************
	Lua Procedures
*********************/


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

TypedInfArg AbstractStructure::getObject(set<Sort*>* sort) const {
	TypedInfArg a;
	if(sort->size() == 1) {
		a._type = IAT_PREDTABLE;
		a._value._predtable = inter(*(sort->begin()));
	}
	else {
		Error::indexoverloadedsort();
		a._type = IAT_NIL;
	}
	return a;
}

TypedInfArg AbstractStructure::getObject(set<Predicate*>* predicate) const {
	TypedInfArg a;
	if(predicate->size() == 1) {
		a._type = IAT_PREDINTER;
		a._value._predinter = inter(*(predicate->begin()));
	}
	else {
		Error::indexoverloadedpred();
		a._type = IAT_NIL;
	}
	return a;
}

TypedInfArg AbstractStructure::getObject(set<Function*>* function) const {
	TypedInfArg a;
	if(function->size() == 1) {
		a._type = IAT_FUNCINTER;
		a._value._funcinter = inter(*(function->begin()));
	}
	else {
		Error::indexoverloadedfunc();
		a._type = IAT_NIL;
	}
	return a;
}

TypedInfArg Vocabulary::getObject(const string& str) const {
	TypedInfArg a;
	OverloadedObject* obj = new OverloadedObject();
	vector<Predicate*> vp = pred_no_arity(str);
	vector<Function*> vf = func_no_arity(str);
	const set<Sort*>* ss = sort(str);
	for(vector<Predicate*>::const_iterator it = vp.begin(); it != vp.end(); ++it) {
		obj->makepredicate(*it);
	}
	for(vector<Function*>::const_iterator it = vf.begin(); it != vf.end(); ++it) {
		obj->makefunction(*it);
	}
	if(ss) {
		for(set<Sort*>::const_iterator it = ss->begin(); it != ss->end(); ++it) {
			obj->makesort(*it);
		}
	}
	if(obj->single()) {
		if(obj->isPredicate()) {
			a._type = IAT_PREDICATE;
			a._value._predicate = obj->getPredicate();
		}
		else if(obj->isFunction()) {
			a._type = IAT_FUNCTION;
			a._value._function = obj->getFunction();
		}
		else if(obj->isSort()) {
			a._type = IAT_SORT;
			a._value._sort = obj->getSort();
		}
		else {
			a._type = IAT_NIL;
		}
		delete(obj);
	}
	else {
		a._type = IAT_OVERLOADED;
		a._value._overloaded = obj;
	}
	return a;
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
		else if(obj->isVocabulary()) {
			a._type = IAT_VOCABULARY;
			a._value._vocabulary = obj->getVocabulary();
		}
		else if(obj->isStructure()) {
			a._type = IAT_STRUCTURE;
			a._value._structure = obj->getStructure();
		}
		else if(obj->isNamespace()) {
			a._type = IAT_NAMESPACE;
			a._value._namespace = obj->getNamespace();
		}
		else if(obj->isOptions()) {
			a._type = IAT_OPTIONS;
			a._value._options = obj->getOptions();
		}
		else if(obj->isProcedure()) {
			a._type = IAT_PROCEDURE;
			a._value._procedure = obj->getProcedure();
		}
		else {
			a._type = IAT_NIL;
		}
		delete(obj);
	}
	else {
		a._type = IAT_OVERLOADED;
		a._value._overloaded = obj;
	}
	return a;
}
