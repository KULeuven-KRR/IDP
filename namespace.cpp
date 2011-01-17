/************************************
	namespace.cc	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "data.hpp"
#include "namespace.hpp"
#include "execute.hpp"
#include "options.hpp"
#include "builtin.hpp"
#include <iostream>

/** Global namespace **/

Namespace* Namespace::_global = 0; 

Namespace* Namespace::global() {
	if(!_global) {
		ParseInfo pi;
		_global = new Namespace("global_namespace",0,pi);
		_global->add(StdBuiltin::instance());
	}
	return _global;
}

/** Destructor **/

Namespace::~Namespace() {
	if(isGlobal()) {	// delete all symbols
		set<Sort*> ss = allSorts();
		set<Predicate*> sp = allPreds();
		set<Function*> sf = allFuncs();
		for(set<Function*>::iterator it = sf.begin(); it != sf.end(); ++it) {
			delete(*it);
		}
		for(set<Predicate*>::iterator it = sp.begin(); it != sp.end(); ++it) {
			delete(*it);
		}
		for(set<Sort*>::iterator it = ss.begin(); it != ss.end(); ++it) {
			delete(*it);
		}
	}
	for(unsigned int n = 0; n < _subs.size(); ++n) delete(_subs[n]);
	for(unsigned int n = 0; n < _vocs.size(); ++n) delete(_vocs[n]);
	for(unsigned int n = 0; n < _structs.size(); ++n) delete(_structs[n]);
	for(unsigned int n = 0; n < _theos.size(); ++n) _theos[n]->recursiveDelete();
}

/** Find subparts **/

bool Namespace::isSubspace(const string& sn) const {
	return (_subspaces.find(sn) != _subspaces.end());
}

bool Namespace::isVocab(const string& vn) const {
	return (_vocabularies.find(vn) != _vocabularies.end());
}

bool Namespace::isTheory(const string& tn) const {
	return (_theories.find(tn) != _theories.end());
}

bool Namespace::isStructure(const string& sn) const {
	return (_structures.find(sn) != _structures.end());
}

bool Namespace::isOption(const string& on) const {
	return (_options.find(on) != _options.end());
}

bool Namespace::isProc(const string& lp) const {
	return (_procedures.find(lp) != _procedures.end());
}

Namespace* Namespace::subspace(const string& sn) const {
	assert(isSubspace(sn));
	return ((_subspaces.find(sn))->second);
}

Vocabulary* Namespace::vocabulary(const string& vn) const {
	assert(isVocab(vn));
	return ((_vocabularies.find(vn))->second);
}

AbstractTheory* Namespace::theory(const string& tn) const {
	assert(isTheory(tn));
	return ((_theories.find(tn))->second);
}

AbstractStructure* Namespace::structure(const string& sn) const {
	assert(isStructure(sn));
	return ((_structures.find(sn))->second);
}

InfOptions* Namespace::option(const string& on) const {
	assert(isOption(on));
	return ((_options.find(on))->second);
}

LuaProcedure* Namespace::procedure(const string& lp) const { 
	assert(isProc(lp));
	return ((_procedures.find(lp))->second);
}

void Namespace::add(LuaProcedure* lp) {
	_procedures[lp->name()] = lp;
}

/** Insert the namespace in lua **/

void insertlongname(lua_State* L, const vector<string>& longname) {
	lua_newtable(L);
	for(unsigned int n = 0; n < longname.size(); ++n) {
		lua_pushinteger(L,n+1);
		lua_pushstring(L,longname[n].c_str());
		lua_settable(L,-3);
	}
}

int Namespace::tolua(lua_State* L, const vector<string>& longname) const {

	lua_newtable(L);		// table to gather the children of the namespace
	int childcounter = 1;	// maintain the index of the table

	for(map<string,Namespace*>::const_iterator it = _subspaces.begin(); it != _subspaces.end(); ++it) {
		vector<string> sublongname = longname; sublongname.push_back(it->second->name());
		lua_pushinteger(L,childcounter); ++childcounter;
		it->second->tolua(L,sublongname);	
		lua_settable(L,-3);
	}

	for(map<string,Vocabulary*>::const_iterator it = _vocabularies.begin(); it != _vocabularies.end(); ++it) {
		vector<string> sublongname = longname; sublongname.push_back(it->second->name());
		lua_pushinteger(L,childcounter); ++childcounter;
		it->second->tolua(L,sublongname);	
		lua_settable(L,-3);
	}

	for(map<string,LuaProcedure*>::const_iterator it = _procedures.begin(); it != _procedures.end(); ++it) {
		vector<string> sublongname = longname; sublongname.push_back(it->second->name());
		lua_pushinteger(L,childcounter); ++childcounter;
		lua_getglobal(L,"idp_intern");
		lua_getfield(L,-1,"newnode");
		lua_getfield(L,-2,"idpprocedure");
		lua_pushstring(L,(it->second->name()).c_str());
		insertlongname(L,sublongname);
		lua_pushinteger(L,it->second->arity());
		luaL_dostring(L,((it->second)->code()).c_str());
		lua_getglobal(L,"tempfunc");
assert(lua_isfunction(L,-1));
		lua_call(L,4,1);	// call idpprocedure
		lua_call(L,1,1);	// call newnode
		lua_remove(L,-2);	// remove idp_intern
		lua_settable(L,-3);	
	}

	for(map<string,AbstractTheory*>::const_iterator it = _theories.begin(); it != _theories.end(); ++it) {
		vector<string> sublongname = longname; sublongname.push_back(it->second->name());
		lua_pushinteger(L,childcounter); ++childcounter;
		lua_getglobal(L,"idp_intern");
		lua_getfield(L,-1,"newnode");
		lua_getfield(L,-2,"idptheory");
		lua_pushstring(L,(it->second->name()).c_str());
		insertlongname(L,sublongname);
		lua_call(L,2,1);	// call idptheory
		lua_call(L,1,1);	// call newnode
		lua_remove(L,-2);	// remove idp_intern
		lua_settable(L,-3);	
	}

	for(map<string,AbstractStructure*>::const_iterator it = _structures.begin(); it != _structures.end(); ++it) {
		vector<string> sublongname = longname; sublongname.push_back(it->second->name());
		lua_pushinteger(L,childcounter); ++childcounter;
		lua_getglobal(L,"idp_intern");
		lua_getfield(L,-1,"newnode");
		lua_getfield(L,-2,"idpstructure");
		lua_pushstring(L,(it->second->name()).c_str());
		insertlongname(L,sublongname);
		lua_call(L,2,1);	// call idpstructure
		lua_call(L,1,1);	// call newnode
		lua_remove(L,-2);	// remove idp_intern
		lua_settable(L,-3);	
	}

	for(map<string,InfOptions*>::const_iterator it = _options.begin(); it != _options.end(); ++it) {
		vector<string> sublongname = longname; sublongname.push_back(it->second->name());
		lua_pushinteger(L,childcounter); ++childcounter;
		lua_getglobal(L,"idp_intern");
		lua_getfield(L,-1,"newnode");
		lua_getfield(L,-2,"idpoptions");
		lua_pushstring(L,(it->second->name()).c_str());
		insertlongname(L,sublongname);
		lua_call(L,2,1);	// call idpoptions
		lua_call(L,1,1);	// call newnode
		lua_remove(L,-2);	// remove idp_intern
		lua_settable(L,-3);	
	}

	if(longname.empty()) {
		for(int n = 1; n < childcounter; ++n) {
			lua_getglobal(L,"idp_intern");
			lua_pushinteger(L,n);
			lua_gettable(L,-3);
			lua_getfield(L,-2,"getname");
			lua_call(L,1,1);
			const char* nm = lua_tostring(L,-1);
			lua_pop(L,2);
			lua_pushinteger(L,n);
			lua_gettable(L,-2);
			lua_setglobal(L,nm);
		}
		lua_pop(L,1);	// pop the table of the children
	}
	else {
		// Make node for the namespace
		lua_getglobal(L,"idp_intern");
		lua_getfield(L,-1,"newnode");
		lua_getfield(L,-2,"idpnamespace");
		lua_pushstring(L,_name.c_str());
		insertlongname(L,longname);
		lua_call(L,2,1);
		lua_call(L,1,1);
		// Insert children
		for(int n = 1; n < childcounter; ++n) {
			lua_pushvalue(L,-1);
			lua_getfield(L,-3,"doinsert");
			lua_pushinteger(L,n);
			lua_gettable(L,-6);
			lua_call(L,2,0);
		}
		lua_remove(L,-2);
		lua_remove(L,-2);
	}

	return 0;	// TODO: change this
}

int Namespace::tolua(lua_State* L) const {
	vector<string> longname(0);
	return tolua(L,longname);
}

/*
	lua_getglobal(L,"idp_intern");
	lua_getfield(L,-1,"maketable");
	lua_remove(L,-2);
	int counter = 0;
	for(map<string,LuaProcedure*>::const_iterator it = _procedures.begin(); it != _procedures.end(); ++it) {
		lua_pushstring(L,(it->first).c_str());
		lua_newtable(L); // TODO
		lua_pushinteger(L,(it->second)->arity());
		luaL_loadstring(L,((it->second)->code()).c_str());
		lua_getglobal(L,"idp_intern");
		lua_getfield(L,-1, "makeprocedure");
		lua_insert(L,-6);
		lua_pop(L,1);
		lua_call(L,4,1);
		++counter;
	}
	lua_call(L,counter,1);
	lua_getglobal(L,"idp_intern");
	lua_getfield(L,-1,"makenode");
	lua_insert(L,-3);
	lua_pop(L,1);
	lua_call(L,1,1);

	lua_setglobal(L,"t");

	// TODO
	return 0;
	*/

/** Full name of the namespace **/

string Namespace::fullname() const {
	if(isGlobal()) {
		return "";
	}
	else {
		if(_superspace->isGlobal()) {
			return _name;
		}
		else {
			return _superspace->fullname() + "::" + _name;
		}
	}
}

/** Collect symbols **/

set<Sort*> Namespace::allSorts() const {
	set<Sort*> ss;
/*	for(unsigned int n = 0; n < _vocs.size(); ++n) {
		for(unsigned int m = 0; m < _vocs[n]->nrSorts(); ++m) ss.insert(_vocs[n]->sort(m));
	}
	for(unsigned int n = 0; n < _subs.size(); ++n) {
		set<Sort*> temp = _subs[n]->allSorts();
		ss.insert(temp.begin(),temp.end());
	}*/
	return ss;
}

set<Predicate*> Namespace::allPreds() const {
	set<Predicate*> sp;
/*	for(unsigned int n = 0; n < _vocs.size(); ++n) {
		for(unsigned int m = 0; m < _vocs[n]->nrPreds(); ++m) sp.insert(_vocs[n]->pred(m));
	}
	for(unsigned int n = 0; n < _subs.size(); ++n) {
		set<Predicate*> temp = _subs[n]->allPreds();
		sp.insert(temp.begin(),temp.end());
	}*/
	return sp;
}

set<Function*> Namespace::allFuncs() const {
	set<Function*> sf;
/*	for(unsigned int n = 0; n < _vocs.size(); ++n) {
		for(unsigned int m = 0; m < _vocs[n]->nrFuncs(); ++m) sf.insert(_vocs[n]->func(m));
	}
	for(unsigned int n = 0; n < _subs.size(); ++n) {
		set<Function*> temp = _subs[n]->allFuncs();
		sf.insert(temp.begin(),temp.end());
	}*/
	return sf;
}
