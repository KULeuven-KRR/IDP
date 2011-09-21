/************************************
	namespace.cpp	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/


#include <iostream>
#include "vocabulary.hpp"
#include "structure.hpp"
#include "theory.hpp"
#include "options.hpp"
#include "namespace.hpp"
#include "internalargument.hpp"
using namespace std;

/** Global namespace **/

Namespace* Namespace::_global = 0; 

Namespace* Namespace::global() {
	if(!_global) {
		ParseInfo pi(1,1,0);
		_global = new Namespace("global_namespace",0,pi);
		_global->add(Vocabulary::std());
		_global->add(new Options("stdoptions",pi));
	}
	return _global;
}

/** Destructor **/

Namespace::~Namespace() {
	for(map<string,Namespace*>::iterator it = _subspaces.begin(); it != _subspaces.end(); ++it) 
		delete(it->second);
	for(map<string,AbstractStructure*>::iterator it = _structures.begin(); it != _structures.end(); ++it) 
		delete(it->second);
	for(map<string,AbstractTheory*>::iterator it = _theories.begin(); it != _theories.end(); ++it) 
		it->second->recursiveDelete();
	for(map<string,Options*>::iterator it = _options.begin(); it != _options.end(); ++it) 
		delete(it->second);
	for(map<string,UserProcedure*>::iterator it = _procedures.begin(); it != _procedures.end(); ++it) 
		delete(it->second);
	for(map<string,Vocabulary*>::iterator it = _vocabularies.begin(); it != _vocabularies.end(); ++it) 
		delete(it->second);
}

/** Mutators **/
void Namespace::add(Vocabulary* v) 			{ _vocabularies[v->name()] = v;	v->setNamespace(this);	}
void Namespace::add(Namespace* n)			{ _subspaces[n->name()] = n; 		}
void Namespace::add(AbstractStructure* s)	{ _structures[s->name()] = s;		}
void Namespace::add(AbstractTheory* t)		{ _theories[t->name()] = t; 		}
void Namespace::add(Options* o)				{ _options[o->name()] = o; 			}
void Namespace::add(UserProcedure* l)		{ _procedures[l->name()] = l;		}
void Namespace::add(const string& name, Query* f)	{ _queries[name] = f;	}
void Namespace::add(const string& name, Term* t)	{ _terms[name] = t;		}

/** Find subparts **/

bool Namespace::isSubspace(const string& sn) const {
	return (_subspaces.find(sn) != _subspaces.end());
}

bool Namespace::isVocab(const string& vn) const {
	return (_vocabularies.find(vn) != _vocabularies.end());
}

bool Namespace::isQuery(const string& fn) const {
	return (_queries.find(fn) != _queries.end());
}

bool Namespace::isTerm(const string& tn) const {
	return (_terms.find(tn) != _terms.end());
}

bool Namespace::isTheory(const string& tn) const {
	return (_theories.find(tn) != _theories.end());
}

bool Namespace::isStructure(const string& sn) const {
	return (_structures.find(sn) != _structures.end());
}

bool Namespace::isOptions(const string& on) const {
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

Query* Namespace::query(const string& fn) const {
	assert(isQuery(fn));
	return ((_queries.find(fn))->second);
}

Term* Namespace::term(const string& tn) const {
	assert(isTerm(tn));
	return ((_terms.find(tn))->second);
}

AbstractStructure* Namespace::structure(const string& sn) const {
	assert(isStructure(sn));
	return ((_structures.find(sn))->second);
}

Options* Namespace::options(const string& on) const {
	assert(isOptions(on));
	return ((_options.find(on))->second);
}

UserProcedure* Namespace::procedure(const string& lp) const { 
	assert(isProc(lp));
	return ((_procedures.find(lp))->second);
}

ostream& Namespace::putName(ostream& output) const {
	if(isGlobal()) { return output; }
	else if(_superspace && not _superspace->isGlobal()) {
		_superspace->putName(output);
		output << "::";
	}
	output << _name;
	return output;
}

ostream& Namespace::putLuaName(ostream& output) const {
	if(isGlobal()) { return output; }
	else if(_superspace && not _superspace->isGlobal()) {
		_superspace->putName(output);
		output << '.';
	}
	output << _name;
	return output;
}
