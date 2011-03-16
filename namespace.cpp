/************************************
	namespace.cpp	
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
	for(map<string,InfOptions*>::iterator it = _options.begin(); it != _options.end(); ++it) delete(it->second);
	for(map<string,LuaProcedure*>::iterator it = _procedures.begin(); it != _procedures.end(); ++it) delete(it->second);
}

void Namespace::add(LuaProcedure* l) { 
	_procedures[l->name()] = l;
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

vector<string> Namespace::fullnamevector() const {
	if(isGlobal()) {
		return vector<string>(0);
	}
	else {
		vector<string> vs = _superspace->fullnamevector();
		vs.push_back(_name);
		return vs;
	}
}

/** Collect symbols **/

set<Sort*> Namespace::allSorts() const {
	set<Sort*> ss;
	for(unsigned int n = 0; n < _vocs.size(); ++n) {
		for(map<string,set<Sort*> >::iterator it = _vocs[n]->firstsort(); it != _vocs[n]->lastsort(); ++it) {
			ss.insert(it->second.begin(),it->second.end());
		}
	}
	for(unsigned int n = 0; n < _subs.size(); ++n) {
		set<Sort*> temp = _subs[n]->allSorts();
		ss.insert(temp.begin(),temp.end());
	}
	return ss;
}

set<Predicate*> Namespace::allPreds() const {
	set<Predicate*> sp;
	for(unsigned int n = 0; n < _vocs.size(); ++n) {
		for(map<string,Predicate*>::iterator it = _vocs[n]->firstpred(); it != _vocs[n]->lastpred(); ++it) {
			sp.insert(it->second);
			if(it->second->overloaded()) {
				OverloadedPredicate* olp = dynamic_cast<OverloadedPredicate*>(it->second);
				sp.insert(olp->overpreds().begin(),olp->overpreds().end());
			}
		}
	}
	for(unsigned int n = 0; n < _subs.size(); ++n) {
		set<Predicate*> temp = _subs[n]->allPreds();
		sp.insert(temp.begin(),temp.end());
	}
	return sp;
}

set<Function*> Namespace::allFuncs() const {
	set<Function*> sf;
	for(unsigned int n = 0; n < _vocs.size(); ++n) {
		for(map<string,Function*>::iterator it = _vocs[n]->firstfunc(); it != _vocs[n]->lastfunc(); ++it) {
			sf.insert(it->second);
			if(it->second->overloaded()) {
				OverloadedFunction* olf = dynamic_cast<OverloadedFunction*>(it->second);
				sf.insert(olf->overfuncs().begin(),olf->overfuncs().end());
			}
		}
	}
	for(unsigned int n = 0; n < _subs.size(); ++n) {
		set<Function*> temp = _subs[n]->allFuncs();
		sf.insert(temp.begin(),temp.end());
	}
	return sf;
}

/*****************************
	Communication with Lua
*****************************/

/*
 * void Namespace::toLuaGlobal(lua_State* L) const {
 * DESCRIPTION
 *		Loads all objects of the namespaces as global variables in a given lua state.
 * PARAMETERS
 *		L	- the given lua state
 */
void Namespace::toLuaGlobal(lua_State* L) const {

	set<string> doublenames = doubleNames();
	map<string,OverloadedObject*> doubleobjects;
	for(set<string>::const_iterator it = doublenames.begin(); it != doublenames.end(); ++it) 
		doubleobjects[*it] = new OverloadedObject();

	for(unsigned int n = 0; n < nrSubs(); ++n) {
		if(doublenames.find(subspace(n)->name()) == doublenames.end()) {
			Namespace** ptr = (Namespace**) lua_newuserdata(L,sizeof(Namespace*));
			(*ptr) = subspace(n);
			luaL_getmetatable (L,"namespace");
			lua_setmetatable(L,-2);
			lua_setglobal(L,subspace(n)->name().c_str());
		}
		else doubleobjects[subspace(n)->name()]->makenamespace(subspace(n));
	}

	for(unsigned int n = 0; n < nrVocs(); ++n) {
		if(doublenames.find(vocabulary(n)->name()) == doublenames.end()) {
			Vocabulary** ptr = (Vocabulary**) lua_newuserdata(L,sizeof(Vocabulary*));
			(*ptr) = vocabulary(n);
			luaL_getmetatable (L,"vocabulary");
			lua_setmetatable(L,-2);
			lua_setglobal(L,vocabulary(n)->name().c_str());
		}
		else doubleobjects[vocabulary(n)->name()]->makevocabulary(vocabulary(n));
	}

	for(unsigned int n = 0; n < nrStructs(); ++n) {
		if(doublenames.find(structure(n)->name()) == doublenames.end()) {
			AbstractStructure** ptr = (AbstractStructure**) lua_newuserdata(L,sizeof(AbstractStructure*));
			(*ptr) = structure(n);
			luaL_getmetatable (L,"structure");
			lua_setmetatable(L,-2);
			lua_setglobal(L,structure(n)->name().c_str());
		}
		else doubleobjects[structure(n)->name()]->makestructure(structure(n));
	}

	for(unsigned int n = 0; n < nrTheos(); ++n) {
		if(doublenames.find(theory(n)->name()) == doublenames.end()) {
			AbstractTheory** ptr = (AbstractTheory**) lua_newuserdata(L,sizeof(AbstractTheory*));
			(*ptr) = theory(n);
			luaL_getmetatable (L,"theory");
			lua_setmetatable(L,-2);
			lua_setglobal(L,theory(n)->name().c_str());
		}
		else doubleobjects[theory(n)->name()]->maketheory(theory(n));
	}

	for(map<string,InfOptions*>::const_iterator it = _options.begin(); it != _options.end(); ++it) {
		if(doublenames.find(it->first) == doublenames.end()) {
			InfOptions** ptr = (InfOptions**) lua_newuserdata(L,sizeof(InfOptions*));
			(*ptr) = it->second;
			luaL_getmetatable (L,"options");
			lua_setmetatable(L,-2);
			lua_setglobal(L,it->first.c_str());
		}
		else doubleobjects[it->first]->makeoptions(it->second);
	}

	for(map<string,LuaProcedure*>::const_iterator it = _procedures.begin(); it != _procedures.end(); ++it) {
		if(doublenames.find(it->second->name()) == doublenames.end()) {
			it->second->compile(L);
			lua_getfield(L,LUA_REGISTRYINDEX,it->second->registryindex().c_str());
			lua_setglobal(L,it->second->name().c_str());
		}
		else {
			it->second->compile(L);
			doubleobjects[it->second->name()]->makeprocedure(new string(it->second->registryindex()));
		}
	}

	for(map<string,OverloadedObject*>::const_iterator it = doubleobjects.begin(); it != doubleobjects.end(); ++it) {
		OverloadedObject** ptr = (OverloadedObject**) lua_newuserdata(L,sizeof(OverloadedObject*));
		(*ptr) = it->second;
		luaL_getmetatable(L,"overloaded");
		lua_setmetatable(L,-2);
		lua_setglobal(L,it->first.c_str());
	}
}

/*
 * set<string> Namespace::doubleNames() const {
 * DESCRIPTION
 *		Collects the object names that occur more than once in the namespace
 */
set<string> Namespace::doubleNames() const {
	set<string> allnames;
	set<string> doublenames;
	for(unsigned int n = 0; n < nrSubs(); ++n) {
		if(allnames.find(subspace(n)->name()) == allnames.end()) allnames.insert(subspace(n)->name());
		else doublenames.insert(subspace(n)->name());
	}
	for(unsigned int n = 0; n < nrVocs(); ++n) {
		if(allnames.find(vocabulary(n)->name()) == allnames.end()) allnames.insert(vocabulary(n)->name());
		else doublenames.insert(vocabulary(n)->name());
	}
	for(unsigned int n = 0; n < nrStructs(); ++n) {
		if(allnames.find(structure(n)->name()) == allnames.end()) allnames.insert(structure(n)->name());
		else doublenames.insert(structure(n)->name());
	}
	for(unsigned int n = 0; n < nrTheos(); ++n) {
		if(allnames.find(theory(n)->name()) == allnames.end()) allnames.insert(theory(n)->name());
		else doublenames.insert(theory(n)->name());
	}
	for(map<string,InfOptions*>::const_iterator it = _options.begin(); it != _options.end(); ++it) {
		if(allnames.find(it->first) == allnames.end()) allnames.insert(it->first);
		else doublenames.insert(it->first);
	}
	for(map<string,LuaProcedure*>::const_iterator it = _procedures.begin(); it != _procedures.end(); ++it) {
		if(allnames.find(it->second->name()) == allnames.end()) allnames.insert(it->second->name());
		else doublenames.insert(it->second->name());
	}
	return doublenames;
}

void Namespace::toLuaLocal(lua_State* ) const {
	// TODO
}

