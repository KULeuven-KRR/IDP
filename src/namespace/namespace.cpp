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

#include "IncludeComponents.hpp"
#include "options.hpp"
#include "internalargument.hpp"
using namespace std;

Namespace* Namespace::createGlobal() {
	ParseInfo pi(1, 1, 0);
	auto global = new Namespace(getGlobalNamespaceName(), 0, pi);
	global->add(Vocabulary::std());
	return global;
}

Namespace::~Namespace() {
	for (auto it = _subspaces.cbegin(); it != _subspaces.cend(); ++it){
		delete (it->second);
	}
	for (auto it = _structures.cbegin(); it != _structures.cend(); ++it){
		delete (it->second);
	}
	for (auto it = _theories.cbegin(); it != _theories.cend(); ++it){
		it->second->recursiveDelete();
	}
	for (auto it = _procedures.cbegin(); it != _procedures.cend(); ++it){
		delete (it->second);
	}
	for (auto it = _vocabularies.cbegin(); it != _vocabularies.cend(); ++it){
		delete (it->second);
	}
}

void Namespace::add(Vocabulary* v) {
	_vocabularies[v->name()] = v;
	v->setNamespace(this);
}
void Namespace::add(Namespace* n) {
	_subspaces[n->name()] = n;
}
void Namespace::add(Structure* s) {
	_structures[s->name()] = s;
}
void Namespace::add(AbstractTheory* t) {
	_theories[t->name()] = t;
}
void Namespace::add(UserProcedure* l) {
	_procedures[l->name()] = l;
}
void Namespace::add(const string& name, Query* q) {
	_queries[name] = q;
}
void Namespace::add(const string& name, const FOBDD* b) {
	_fobdds[name] = b;
}
void Namespace::add(const string& name, Term* t) {
	_terms[name] = t;
}

bool Namespace::isSubspace(const string& sn) const {
	return (_subspaces.find(sn) != _subspaces.cend());
}

bool Namespace::isVocab(const string& vn) const {
	return (_vocabularies.find(vn) != _vocabularies.cend());
}

bool Namespace::isQuery(const string& fn) const {
	return (_queries.find(fn) != _queries.cend());
}
bool Namespace::isFOBDD(const string& bn) const {
	return (_fobdds.find(bn) != _fobdds.cend());
}

bool Namespace::isTerm(const string& tn) const {
	return (_terms.find(tn) != _terms.cend());
}

bool Namespace::isTheory(const string& tn) const {
	return (_theories.find(tn) != _theories.cend());
}

bool Namespace::isStructure(const string& sn) const {
	return (_structures.find(sn) != _structures.cend());
}

bool Namespace::isProc(const string& lp) const {
	return (_procedures.find(lp) != _procedures.cend());
}

Namespace* Namespace::subspace(const string& sn) const {
	Assert(isSubspace(sn));
	return ((_subspaces.find(sn))->second);
}

Vocabulary* Namespace::vocabulary(const string& vn) const {
	if(not isVocab(vn)){
		stringstream ss;
		ss <<"The vocabulary " <<vn <<" does not exist in namespace " <<name() <<"\n";
		throw IdpException(ss.str());
	}
	return ((_vocabularies.find(vn))->second);
}

AbstractTheory* Namespace::theory(const string& tn) const {
	if(not isTheory(tn)){
		stringstream ss;
		ss <<"The theory " <<tn <<" does not exist in namespace " <<name() <<"\n";
		throw IdpException(ss.str());
	}
	return ((_theories.find(tn))->second);
}

Query* Namespace::query(const string& fn) const {
	if(not isQuery(fn)){
		stringstream ss;
		ss <<"The query " <<fn <<" does not exist in namespace " <<name() <<"\n";
		throw IdpException(ss.str());
	}
	return ((_queries.find(fn))->second);
}
const FOBDD* Namespace::fobdd(const string& fn) const {
	if(not isQuery(fn)){
		stringstream ss;
		ss <<"The fobdd " <<fn <<" does not exist in namespace " <<name() <<"\n";
		throw IdpException(ss.str());
	}
	return ((_fobdds.find(fn))->second);
}

Term* Namespace::term(const string& tn) const {
	if(not isTerm(tn)){
		stringstream ss;
		ss <<"The term " <<tn <<" does not exist in namespace " <<name() <<"\n";
		throw IdpException(ss.str());
	}
	return ((_terms.find(tn))->second);
}

Structure* Namespace::structure(const string& sn) const {
	if(not isStructure(sn)){
		stringstream ss;
		ss <<"The structure " <<sn <<" does not exist in namespace " <<name() <<"\n";
		throw IdpException(ss.str());
	}
	return ((_structures.find(sn))->second);
}

UserProcedure* Namespace::procedure(const string& lp) const {
	Assert(isProc(lp));
	return ((_procedures.find(lp))->second);
}

std::vector<std::string> Namespace::getComponentNamesExceptSpaces() const {
	std::vector<std::string> names;
	for(auto i=procedures().cbegin(); i!=procedures().cend(); ++i){
		names.push_back(i->first);
	}
/*		for(auto i=subspaces().cbegin(); i!=subspaces().cend(); ++i){
		names.push_back(i->first);
	}*/
	for(auto i=vocabularies().cbegin(); i!=vocabularies().cend(); ++i){
		names.push_back(i->first);
	}
	for(auto i=structures().cbegin(); i!=structures().cend(); ++i){
		names.push_back(i->first);
	}
	for(auto i=theories().cbegin(); i!=theories().cend(); ++i){
		names.push_back(i->first);
	}
	for(auto i=queries().cbegin(); i!=queries().cend(); ++i){
		names.push_back(i->first);
	}
	for(auto i=terms().cbegin(); i!=terms().cend(); ++i){
		names.push_back(i->first);
	}
	return names;
}

ostream& Namespace::putName(ostream& output) const {
	if (isGlobal()) {
		return output;
	} else if (_superspace && not _superspace->isGlobal()) {
		_superspace->putName(output);
		output << "::";
	}
	output << _name;
	return output;
}

ostream& Namespace::putLuaName(ostream& output) const {
	if (isGlobal()) {
		return output;
	} else if (_superspace && not _superspace->isGlobal()) {
		_superspace->putName(output);
		output << '.';
	}
	output << _name;
	return output;
}
