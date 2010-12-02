/************************************
	namespace.cc	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "data.hpp"
#include "namespace.hpp"
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
