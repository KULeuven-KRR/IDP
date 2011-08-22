/************************************
	insert.cpp	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <cassert>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include "common.hpp"
#include "insert.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "namespace.hpp"
#include "yyltype.hpp"
#include "parse.h"
#include "error.hpp"
#include "options.hpp"
#include "internalargument.hpp"
#include "luaconnection.hpp"
using namespace std;
using namespace LuaConnection; //TODO add abstraction to remove lua dependence here

class SortDeriver : public TheoryMutatingVisitor {
	private:
		map<Variable*,set<Sort*> >	_untyped;			// The untyped variables, with their possible types
		map<FuncTerm*,Sort*>		_overloadedterms;	// The terms with an overloaded function
		set<PredForm*>				_overloadedatoms;	// The atoms with an overloaded predicate
		set<DomainTerm*>			_domelements;		// The untyped domain elements
		bool						_changed;
		bool						_firstvisit;
		Sort*						_assertsort;
		Vocabulary*					_vocab;

	public:
		// Constructor
		SortDeriver(Formula* f,Vocabulary* v) : _vocab(v) { run(f); }
		SortDeriver(Rule* r,Vocabulary* v)	: _vocab(v) { run(r); }

		// Run sort derivation 
		void run(Formula*);
		void run(Rule*);

		// Visit 
		Formula*	visit(QuantForm*);
		Formula*	visit(PredForm*);
		Formula*	visit(EqChainForm*);
		Rule*		visit(Rule*);
		Term*		visit(VarTerm*);
		Term*		visit(DomainTerm*);
		Term*		visit(FuncTerm*);
		SetExpr*	visit(QuantSetExpr*);

	private:
		// Auxiliary methods
		void derivesorts();		// derive the sorts of the variables, based on the sorts in _untyped
		void derivefuncs();		// disambiguate the overloaded functions
		void derivepreds();		// disambiguate the overloaded predicates

		// Check
		void check();
		
};

Formula* SortDeriver::visit(QuantForm* qf) {
	if(_firstvisit) {
		for(std::set<Variable*>::const_iterator it = qf->quantvars().begin(); it != qf->quantvars().end(); ++it) {
			if(!((*it)->sort())) {
				_untyped[*it] = set<Sort*>();
				_changed = true;
			}
		}
	}
	return traverse(qf);
}

Formula* SortDeriver::visit(PredForm* pf) {
	PFSymbol* p = pf->symbol();

	// At first visit, insert the atoms over overloaded predicates
	if(_firstvisit && p->overloaded()) {
		_overloadedatoms.insert(pf);
		_changed = true;
	}

	// Visit the children while asserting the sorts of the predicate
	vector<Sort*>::const_iterator it = p->sorts().begin();
	vector<Term*>::const_iterator jt = pf->subterms().begin();
	for(; it != p->sorts().end(); ++it, ++jt) {
		_assertsort = *it;
		(*jt)->accept(this);
	}
	return pf;
}

Formula* SortDeriver::visit(EqChainForm* ef) {
	Sort* s = 0;
	if(!_firstvisit) {
		for(vector<Term*>::const_iterator it = ef->subterms().begin(); it != ef->subterms().end(); ++it) {
			Sort* temp = (*it)->sort();
			if(temp && temp->parents().empty() && temp->children().empty()) {
				s = temp;
				break;
			}
		}
	}
	for(vector<Term*>::const_iterator it = ef->subterms().begin(); it != ef->subterms().end(); ++it) {
		_assertsort = s;
		(*it)->accept(this);
	}
	return ef;
}

Rule* SortDeriver::visit(Rule* r) {
	if(_firstvisit) {
		for(set<Variable*>::const_iterator it = r->quantvars().begin(); it != r->quantvars().end(); ++it) {
			if(!((*it)->sort())) _untyped[*it] = set<Sort*>();
			_changed = true;
		}
	}
	r->head()->accept(this);
	r->body()->accept(this);
	return r;
}

Term* SortDeriver::visit(VarTerm* vt) {
	if((!(vt->sort())) && _assertsort) {
		_untyped[vt->var()].insert(_assertsort);
	}
	return vt;
}

Term* SortDeriver::visit(DomainTerm* dt) {
	if(_firstvisit && (!(dt->sort()))) {
		_domelements.insert(dt);
	}

	if((!(dt->sort())) && _assertsort) {
		dt->sort(_assertsort);
		_changed = true;
		_domelements.erase(dt);
	}
	return dt;
}

Term* SortDeriver::visit(FuncTerm* ft) {
	Function* f = ft->function();

	// At first visit, insert the terms over overloaded functions
	if(f->overloaded()) {
		if(_firstvisit || _overloadedterms.find(ft) == _overloadedterms.end() || _assertsort != _overloadedterms[ft]) {
			_changed = true;
			_overloadedterms[ft] = _assertsort;
		}
	}

	// Visit the children while asserting the sorts of the predicate
	vector<Sort*>::const_iterator it = f->insorts().begin();
	vector<Term*>::const_iterator jt = ft->subterms().begin();
	for(; it != f->insorts().end(); ++it, ++jt) {
		_assertsort = *it;
		(*jt)->accept(this);
	}
	return ft;
}

SetExpr* SortDeriver::visit(QuantSetExpr* qs) {
	if(_firstvisit) {
		for(std::set<Variable*>::const_iterator it = qs->quantvars().begin(); it != qs->quantvars().end(); ++it) {
			if(!((*it)->sort())) {
				_untyped[*it] = set<Sort*>();
				_changed = true;
			}
		}
	}
	return traverse(qs);
}

void SortDeriver::derivesorts() {
	for(map<Variable*,std::set<Sort*> >::iterator it = _untyped.begin(); it != _untyped.end(); ) {
		map<Variable*,std::set<Sort*> >::iterator jt = it; ++jt;
		if(!((it->second).empty())) {
			std::set<Sort*>::iterator kt = (it->second).begin(); 
			Sort* s = *kt;
			++kt;
			for(; kt != (it->second).end(); ++kt) {
				s = SortUtils::resolve(s,*kt,_vocab);
				if(!s) { // In case of conflicting sorts, assign the first sort. 
						 // Error message will be given during final check.
					s = *((it->second).begin());
					break;
				}
			}
			assert(s);
			if((it->second).size() > 1 || s->builtin()) {	// Warning when the sort was resolved or builtin
				Warning::derivevarsort(it->first->name(),s->name(),it->first->pi());
			}
			it->first->sort(s);
			_untyped.erase(it);
			_changed = true;
		}
		it = jt;
	}
}

void SortDeriver::derivefuncs() {
	for(map<FuncTerm*,Sort*>::iterator it = _overloadedterms.begin(); it != _overloadedterms.end(); ) {
		map<FuncTerm*,Sort*>::iterator jt = it; ++jt;
		Function* f = it->first->function();
		vector<Sort*> vs;
		for(vector<Term*>::const_iterator kt = it->first->subterms().begin(); kt != it->first->subterms().end(); ++kt) {
			vs.push_back((*kt)->sort());
		}
		vs.push_back(it->second);
		Function* rf = f->disambiguate(vs,_vocab);
		if(rf) {
			it->first->function(rf);
			if(!rf->overloaded()) _overloadedterms.erase(it);
			_changed = true;
		}
		it = jt;
	}
}

void SortDeriver::derivepreds() {
	for(std::set<PredForm*>::iterator it = _overloadedatoms.begin(); it != _overloadedatoms.end(); ) {
		std::set<PredForm*>::iterator jt = it; ++jt;
		PFSymbol* p = (*it)->symbol();
		vector<Sort*> vs;
		for(vector<Term*>::const_iterator kt = (*it)->subterms().begin(); kt != (*it)->subterms().end(); ++kt) {
			vs.push_back((*kt)->sort());
		}
		PFSymbol* rp = p->disambiguate(vs,_vocab);
		if(rp) {
			(*it)->symbol(rp);
			if(!rp->overloaded()) _overloadedatoms.erase(it);
			_changed = true;
		}
		it = jt;
	}
}

void SortDeriver::run(Formula* f) {
	_changed = false;
	_firstvisit = true;
	f->accept(this);	// First visit: collect untyped symbols, set types of variables that occur in typed positions.
	_firstvisit = false;
	while(_changed) {
		_changed = false;
		derivesorts();
		derivefuncs();
		derivepreds();
		f->accept(this);	// Next visit: type derivation over overloaded predicates or functions.
	}
	check();
}

void SortDeriver::run(Rule* r) {
	// Set the sort of the terms in the head
	vector<Sort*>::const_iterator jt = r->head()->symbol()->sorts().begin();
	for(unsigned int n = 0; n < r->head()->subterms().size(); ++n, ++jt) {
		Sort* hs = r->head()->subterms()[n]->sort();
		if(hs) {
			if(!SortUtils::isSubsort(hs,*jt)) {
				Variable* nv = new Variable(*jt);
				VarTerm* nvt1 = new VarTerm(nv,TermParseInfo());
				VarTerm* nvt2 = new VarTerm(nv,TermParseInfo());
				EqChainForm* ecf = new EqChainForm(true,true,nvt1,FormulaParseInfo());
				ecf->add(CT_EQ,r->head()->subterms()[n]);
				BoolForm* bf = new BoolForm(true,true,r->body(),ecf,FormulaParseInfo());
				r->body(bf);
				r->head()->arg(n,nvt2);
				r->addvar(nv);
			}
		}
		else {
			r->head()->subterms()[n]->sort(*jt);
		}
	}
	// Rest of the algorithm
	_changed = false;
	_firstvisit = true;
	r->accept(this);
	_firstvisit = false;
	while(_changed) {
		_changed = false;
		derivesorts();
		derivefuncs();
		derivepreds();
		r->accept(this);
	}
	check();
}

void SortDeriver::check() {
	for(map<Variable*,std::set<Sort*> >::iterator it = _untyped.begin(); it != _untyped.end(); ++it) {
		assert((it->second).empty());
		Error::novarsort(it->first->name(),it->first->pi());
	}
	for(std::set<PredForm*>::iterator it = _overloadedatoms.begin(); it != _overloadedatoms.end(); ++it) {
		if(typeid(*((*it)->symbol())) == typeid(Predicate)) Error::nopredsort((*it)->symbol()->name(),(*it)->pi());
		else Error::nofuncsort((*it)->symbol()->name(),(*it)->pi());
	}
	for(map<FuncTerm*,Sort*>::iterator it = _overloadedterms.begin(); it != _overloadedterms.end(); ++it) {
		Error::nofuncsort(it->first->function()->name(),it->first->pi());
	}
	for(std::set<DomainTerm*>::iterator it = _domelements.begin(); it != _domelements.end(); ++it) {
		Error::nodomsort((*it)->toString(),(*it)->pi());
	}
}

class SortChecker : public TheoryVisitor {

	private:
		Vocabulary* _vocab;

	public:
		SortChecker(Formula* f,Vocabulary* v)		: _vocab(v) { f->accept(this);	}
		SortChecker(Definition* d,Vocabulary* v)	: _vocab(v) { d->accept(this);	}
		SortChecker(FixpDef* d,Vocabulary* v)		: _vocab(v) { d->accept(this);	}

		void visit(const PredForm*);
		void visit(const EqChainForm*);
		void visit(const FuncTerm*);
		void visit(const AggTerm*);

};

void SortChecker::visit(const PredForm* pf) {
	PFSymbol* s = pf->symbol();
	vector<Sort*>::const_iterator it = s->sorts().begin();
	vector<Term*>::const_iterator jt = pf->subterms().begin();
	for(; it != s->sorts().end(); ++it, ++jt) {
		Sort* s1 = *it;
		Sort* s2 = (*jt)->sort();
		if(s1 && s2) {
			if(!SortUtils::resolve(s1,s2,_vocab)) {
				Error::wrongsort((*jt)->toString(),s2->name(),s1->name(),(*jt)->pi());
			}
		}
	}
	traverse(pf);
}

void SortChecker::visit(const FuncTerm* ft) {
	Function* f = ft->function();
	vector<Sort*>::const_iterator it = f->insorts().begin();
	vector<Term*>::const_iterator jt = ft->subterms().begin();
	for(; it != f->insorts().end(); ++it, ++jt) {
		Sort* s1 = *it;
		Sort* s2 = (*jt)->sort();
		if(s1 && s2) {
			if(!SortUtils::resolve(s1,s2,_vocab)) {
				Error::wrongsort((*jt)->toString(),s2->name(),s1->name(),(*jt)->pi());
			}
		}
	}
	traverse(ft);
}

void SortChecker::visit(const EqChainForm* ef) {
	Sort* s = 0;
	vector<Term*>::const_iterator it = ef->subterms().begin();
	while(!s && it != ef->subterms().end()) {
		s = (*it)->sort();
		++it;
	}
	for(; it != ef->subterms().end(); ++it) {
		Sort* t = (*it)->sort();
		if(t) {
			if(!SortUtils::resolve(s,t,_vocab)) {
				Error::wrongsort((*it)->toString(),t->name(),s->name(),(*it)->pi());
			}
		}
	}
	traverse(ef);
}

void SortChecker::visit(const AggTerm* at) {
	if(at->function() != AGG_CARD) {
		SetExpr* s = at->set();
//		if(!s->quantvars().empty() && (*(s->quantvars().begin()))->sort()) {
//			Variable* v = *(s->quantvars().begin());
//			if(!SortUtils::resolve(v->sort(),VocabularyUtils::floatsort(),_vocab)) {
//				Error::wrongsort(v->name(),v->sort()->name(),"int or float",v->pi());
//			}
//		}
		for(vector<Term*>::const_iterator it = s->subterms().begin(); it != s->subterms().end(); ++it) {
			if((*it)->sort() && !SortUtils::resolve((*it)->sort(),VocabularyUtils::floatsort(),_vocab)) {
				Error::wrongsort((*it)->toString(),(*it)->sort()->name(),"int or float",(*it)->pi());
			}
		}
	}
	traverse(at);
}

/**
 * Rewrite a vector of strings s1,s2,...,sn to the single string s1::s2::...::sn
 */
string oneName(const longname& vs) {
	stringstream sstr;
	if(!vs.empty()) {
		sstr << vs[0];
		for(unsigned int n = 1; n < vs.size(); ++n) sstr << "::" << vs[n];
	}
	return sstr.str();
}

string predName(const longname& name, const vector<Sort*>& vs) {
	stringstream sstr;
	sstr << oneName(name);
	if(!vs.empty()) {
		sstr << '[' << vs[0]->name();
		for(unsigned int n = 1; n < vs.size(); ++n) sstr << ',' << vs[n]->name();
		sstr << ']';
	}
	return sstr.str();
}

string funcName(const longname& name, const vector<Sort*>& vs) {
	assert(!vs.empty());
	stringstream sstr;
	sstr << oneName(name) << '[';
	if(vs.size() > 1) {
		sstr << vs[0]->name();
		for(unsigned int n = 1; n < vs.size()-1; ++n) sstr << ',' << vs[n]->name();
	}
	sstr << ':' << vs.back()->name() << ']';
	return sstr.str();
}

/*************
	NSPair
*************/

void NSPair::includePredArity() {
	assert(_sortsincluded && !_arityincluded); 
	_name.back() = _name.back() + '/' + convertToString(_sorts.size());
	_arityincluded = true;
}

void NSPair::includeFuncArity() {
	assert(_sortsincluded && !_arityincluded); 
	_name.back() = _name.back() + '/' + convertToString(_sorts.size() - 1);
	_arityincluded = true;
}

void NSPair::includeArity(unsigned int n) {
	assert(!_arityincluded); 
	_name.back() = _name.back() + '/' + convertToString(n);
	_arityincluded = true;
}

string NSPair::toString() {
	assert(!_name.empty());
	string str = _name[0];
	for(unsigned int n = 1; n < _name.size(); ++n) str = str + "::" + _name[n];
	if(_sortsincluded) {
		if(_arityincluded) str = str.substr(0,str.rfind('/'));
		str = str + '[';
		if(!_sorts.empty()) {
			if(_func && _sorts.size() == 1) str = str + ':';
			if(_sorts[0]) str = str + _sorts[0]->name();
			for(unsigned int n = 1; n < _sorts.size()-1; ++n) {
				if(_sorts[n]) str = str + ',' + _sorts[n]->name();
			}
			if(_sorts.size() > 1) {
				if(_func) str = str + ':';
				else str = str + ',';
				if(_sorts[_sorts.size()-1]) str = str + _sorts[_sorts.size()-1]->name();
			}
		}
		str = str + ']';
	}
	return str;
}

/*************
	Insert
*************/

bool Insert::belongsToVoc(Sort* s) const {
	if(_currvocabulary->contains(s)) return true;
	return false;
}

bool Insert::belongsToVoc(Predicate* p) const {
	if(_currvocabulary->contains(p)) return true;
	return false;
}

bool Insert::belongsToVoc(Function* f) const {
	if(_currvocabulary->contains(f)) return true;
	return false;
}

std::set<Predicate*> Insert::noArPredInScope(const string& name) const {
	std::set<Predicate*> vp;
	for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
		std::set<Predicate*> nvp = _usingvocab[n]->pred_no_arity(name);
		vp.insert(nvp.begin(),nvp.end());
	}
	return vp;
}

std::set<Predicate*> Insert::noArPredInScope(const vector<string>& vs, const ParseInfo& pi) const {
	assert(!vs.empty());
	if(vs.size() == 1) {
		return noArPredInScope(vs[0]);
	}
	else {
		vector<string> vv(vs.size()-1);
		for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
		Vocabulary* v = vocabularyInScope(vv,pi);
		if(v) return v->pred_no_arity(vs.back());
		else return std::set<Predicate*>();
	}
}

std::set<Function*> Insert::noArFuncInScope(const string& name) const {
	std::set<Function*> vf;
	for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
		std::set<Function*> nvf = _usingvocab[n]->func_no_arity(name);
		vf.insert(nvf.begin(),nvf.end());
	}
	return vf;
}

std::set<Function*> Insert::noArFuncInScope(const vector<string>& vs, const ParseInfo& pi) const {
	assert(!vs.empty());
	if(vs.size() == 1) {
		return noArFuncInScope(vs[0]);
	}
	else {
		vector<string> vv(vs.size()-1);
		for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
		Vocabulary* v = vocabularyInScope(vv,pi);
		if(v) return v->func_no_arity(vs.back());
		else return std::set<Function*>();
	}
}

Function* Insert::funcInScope(const string& name) const {
	std::set<Function*> vf;
	for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
		Function* f = _usingvocab[n]->func(name);
		if(f) vf.insert(f);
	}
	if(vf.empty()) return 0;
	else return FuncUtils::overload(vf);
}

Function* Insert::funcInScope(const vector<string>& vs, const ParseInfo& pi) const {
	assert(!vs.empty());
	if(vs.size() == 1) {
		return funcInScope(vs[0]);
	}
	else { 
		vector<string> vv(vs.size()-1);
		for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
		Vocabulary* v = vocabularyInScope(vv,pi);
		if(v) return v->func(vs.back());
		else return 0;
	}
}

Predicate* Insert::predInScope(const string& name) const {
	std::set<Predicate*> vp;
	for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
		Predicate* p = _usingvocab[n]->pred(name);
		if(p) vp.insert(p);
	}
	if(vp.empty()) return 0;
	else return PredUtils::overload(vp);
}

Predicate* Insert::predInScope(const vector<string>& vs, const ParseInfo& pi) const {
	assert(!vs.empty());
	if(vs.size() == 1) {
		return predInScope(vs[0]);
	}
	else { 
		vector<string> vv(vs.size()-1);
		for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
		Vocabulary* v = vocabularyInScope(vv,pi);
		if(v) return v->pred(vs.back());
		else return 0;
	}
}

Sort* Insert::sortInScope(const string& name, const ParseInfo& pi) const {
	Sort* s = 0;
	for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
		const std::set<Sort*>* temp = _usingvocab[n]->sort(name);
		if(temp) {
			if(s) {
				Error::overloadedsort(s->name(),s->pi(),(*(temp->begin()))->pi(),pi);
			}
			else if(temp->size() > 1) {
				std::set<Sort*>::iterator it = temp->begin();
				Sort* s1 = *it; ++it; Sort* s2 = *it;
				Error::overloadedsort(s1->name(),s1->pi(),s2->pi(),pi);
			}
			else s = *(temp->begin());
		}
	}
	return s;
}

Sort* Insert::sortInScope(const vector<string>& vs, const ParseInfo& pi) const {
	assert(!vs.empty());
	if(vs.size() == 1) {
		return sortInScope(vs[0],pi);
	}
	else { 
		vector<string> vv(vs.size()-1);
		for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
		Vocabulary* v = vocabularyInScope(vv,pi);
		if(v) {
			const std::set<Sort*>* ss = v->sort(vs.back());
			if(ss) {
				if(ss->size() > 1) {
					std::set<Sort*>::iterator it = ss->begin();
					Sort* s1 = *it; ++it; Sort* s2 = *it;
					Error::overloadedsort(s1->name(),s1->pi(),s2->pi(),pi);
				}
				return *(ss->begin());
			}
			else return 0;
		}
		else return 0;
	}
}

Namespace* Insert::namespaceInScope(const string& name, const ParseInfo& pi) const {
	Namespace* ns = 0;
	for(unsigned int n = 0; n < _usingspace.size(); ++n) {
		if(_usingspace[n]->isSubspace(name)) {
			if(ns) Error::overloadedspace(name,_usingspace[n]->pi(),ns->pi(),pi);
			else ns = _usingspace[n]->subspace(name);
		}
	}
	return ns;
}

Namespace* Insert::namespaceInScope(const vector<string>& vs, const ParseInfo& pi) const {
	assert(!vs.empty());
	if(vs.size() == 1) {
		return namespaceInScope(vs[0],pi);
	}
	else {
		Namespace* ns = namespaceInScope(vs[0],pi);
		for(unsigned int n = 1; n < vs.size(); ++n) {
			if(ns->isSubspace(vs[n])) {
				ns = ns->subspace(vs[n]);
			}
			else return 0;
		}
		return ns;
	}
}

Vocabulary* Insert::vocabularyInScope(const string& name, const ParseInfo& pi) const {
	Vocabulary* v = 0;
	for(unsigned int n = 0; n < _usingspace.size(); ++n) {
		if(_usingspace[n]->isVocab(name)) {
			if(v) Error::overloadedvocab(name,_usingspace[n]->vocabulary(name)->pi(),v->pi(),pi);
			else v = _usingspace[n]->vocabulary(name);
		}
	}
	return v;
}

Vocabulary* Insert::vocabularyInScope(const vector<string>& vs, const ParseInfo& pi) const {
	assert(!vs.empty());
	if(vs.size() == 1) {
		return vocabularyInScope(vs[0],pi);
	}
	else {
		Namespace* ns = namespaceInScope(vs[0],pi);
		if(ns) {
			for(unsigned int n = 1; n < (vs.size()-1); ++n) {
				if(ns->isSubspace(vs[n])) {
					ns = ns->subspace(vs[n]);
				}
				else return 0;
			}
			if(ns->isVocab(vs.back())) {
				return ns->vocabulary(vs.back());
			}
			else return 0;
		}
		else return 0;
	}
}

AbstractStructure* Insert::structureInScope(const string& name, const ParseInfo& pi) const {
	AbstractStructure* s = 0;
	for(unsigned int n = 0; n < _usingspace.size(); ++n) {
		if(_usingspace[n]->isStructure(name)) {
			if(s) Error::overloadedstructure(name,_usingspace[n]->structure(name)->pi(),s->pi(),pi);
			else s = _usingspace[n]->structure(name);
		}
	}
	return s;
}

AbstractStructure* Insert::structureInScope(const vector<string>& vs, const ParseInfo& pi) const {
	assert(!vs.empty());
	if(vs.size() == 1) {
		return structureInScope(vs[0],pi);
	}
	else {
		Namespace* ns = namespaceInScope(vs[0],pi);
		for(unsigned int n = 1; n < (vs.size()-1); ++n) {
			if(ns->isSubspace(vs[n])) {
				ns = ns->subspace(vs[n]);
			}
			else return 0;
		}
		if(ns->isStructure(vs.back())) {
			return ns->structure(vs.back());
		}
		else return 0;
	}
}

Query* Insert::queryInScope(const string& name, const ParseInfo& pi) const {
	Query* q = 0;
	for(unsigned int n = 0; n < _usingspace.size(); ++n) {
		if(_usingspace[n]->isQuery(name)) {
			if(q) Error::overloadedquery(name,_usingspace[n]->query(name)->pi(),q->pi(),pi);
			else q = _usingspace[n]->query(name);
		}
	}
	return q;
}

AbstractTheory* Insert::theoryInScope(const string& name, const ParseInfo& pi) const {
	AbstractTheory* th = 0;
	for(unsigned int n = 0; n < _usingspace.size(); ++n) {
		if(_usingspace[n]->isTheory(name)) {
			if(th) Error::overloadedtheory(name,_usingspace[n]->theory(name)->pi(),th->pi(),pi);
			else th = _usingspace[n]->theory(name);
		}
	}
	return th;
}

UserProcedure* Insert::procedureInScope(const string& name, const ParseInfo& pi) const {
	UserProcedure* lp = 0;
	for(unsigned int n = 0; n < _usingspace.size(); ++n) {
		if(_usingspace[n]->isProc(name)) {
			if(lp) Error::overloadedproc(name,_usingspace[n]->procedure(name)->pi(),lp->pi(),pi);
			else lp = _usingspace[n]->procedure(name);
		}
	}
	return lp;
}

UserProcedure* Insert::procedureInScope(const vector<string>& vs, const ParseInfo& pi) const {
	assert(!vs.empty());
	if(vs.size() == 1) {
		return procedureInScope(vs[0],pi);
	}
	else {
		Namespace* ns = namespaceInScope(vs[0],pi);
		for(unsigned int n = 1; n < (vs.size()-1); ++n) {
			if(ns->isSubspace(vs[n])) {
				ns = ns->subspace(vs[n]);
			}
			else return 0;
		}
		if(ns->isProc(vs.back())) {
			return ns->procedure(vs.back());
		}
		else return 0;
	}
}

Options* Insert::optionsInScope(const string& name, const ParseInfo& pi) const {
	Options* opt = 0;
	for(unsigned int n = 0; n < _usingspace.size(); ++n) {
		if(_usingspace[n]->isOptions(name)) {
			if(opt) Error::overloadedopt(name,_usingspace[n]->options(name)->pi(),opt->pi(),pi);
			else opt = _usingspace[n]->options(name);
		}
	}
	return opt;
}

Options* Insert::optionsInScope(const vector<string>& vs, const ParseInfo& pi) const {
	assert(!vs.empty());
	if(vs.size() == 1) {
		return optionsInScope(vs[0],pi);
	}
	else {
		Namespace* ns = namespaceInScope(vs[0],pi);
		if(ns) {
			for(unsigned int n = 1; n < (vs.size()-1); ++n) {
				if(ns->isSubspace(vs[n])) {
					ns = ns->subspace(vs[n]);
				}
				else return 0;
			}
			if(ns->isOptions(vs.back())) {
				return ns->options(vs.back());
			}
			else return 0;
		}
		else return 0;
	}
}


UTF getUTF(const string& utf, const ParseInfo& pi) {
	if(utf == "u") return UTF_UNKNOWN;
	else if(utf == "ct") return UTF_CT;
	else if(utf == "cf") return UTF_CF;
	else {
		Error::expectedutf(utf,pi);
		return UTF_ERROR;
	}
}

Insert::Insert() {
	openblock();
	_currfile = 0;
	_currspace = Namespace::global();
	_options = _currspace->options("stdoptions");
	usenamespace(_currspace);
}

string* Insert::currfile() const {
	return _currfile;
}

void Insert::currfile(const string& s) {
	_currfile = StringPointer(s);
}

void Insert::currfile(string* s) {
	_currfile = s;
}

void Insert::partial(Function* f) const {
	f->partial(true);
}

void Insert::makeLFD(FixpDef* d, bool lfp) const {
	if(d) d->lfp(lfp);
}

void Insert::addRule(FixpDef* d, Rule* r) const {
	if(d && r) d->add(r);
}

void Insert::addDef(FixpDef* d, FixpDef* sd) const {
	if(d && sd) d->add(sd);
}

FixpDef* Insert::createFD() const {
	return new FixpDef;
}

ParseInfo Insert::parseinfo(YYLTYPE l) const { 
	return ParseInfo(l.first_line,l.first_column,_currfile);	
}

FormulaParseInfo Insert::formparseinfo(Formula* f, YYLTYPE l) const {
	return FormulaParseInfo(l.first_line,l.first_column,_currfile,f);
}

TermParseInfo Insert::termparseinfo(Term* t, YYLTYPE l) const {
	return TermParseInfo(l.first_line,l.first_column,_currfile,t);
}

TermParseInfo Insert::termparseinfo(Term* t, const ParseInfo& l) const {
	return TermParseInfo(l.line(),l.col(),l.file(),t);
}

SetParseInfo Insert::setparseinfo(SetExpr* s, YYLTYPE l) const {
	return SetParseInfo(l.first_line,l.first_column,_currfile,s);
}

set<Variable*> Insert::freevars(const ParseInfo& pi) {
	std::set<Variable*> vv;
	string vs;
	for(list<VarName>::iterator i = _curr_vars.begin(); i != _curr_vars.end(); ++i) {
		vv.insert(i->_var);
		vs = vs + ' ' + i->_name;
	}
	if(!vv.empty()) Warning::freevars(vs,pi);
	_curr_vars.clear();
	return vv;
}

void Insert::remove_vars(const std::vector<Variable*>& v) {
	for(std::vector<Variable*>::const_iterator it = v.begin(); it != v.end(); ++it) {
		for(list<VarName>::iterator i = _curr_vars.begin(); i != _curr_vars.end(); ++i) {
			if(i->_name == (*it)->name()) {
				_curr_vars.erase(i);
				break;
			}
		}
	}
}

void Insert::remove_vars(const std::set<Variable*>& v) {
	for(std::set<Variable*>::const_iterator it = v.begin(); it != v.end(); ++it) {
		for(list<VarName>::iterator i = _curr_vars.begin(); i != _curr_vars.end(); ++i) {
			if(i->_name == (*it)->name()) {
				_curr_vars.erase(i);
				break;
			}
		}
	}
}

void Insert::usenamespace(Namespace* s) {
	++_nrspaces.back();
	_usingspace.push_back(s);
}

void Insert::usevocabulary(Vocabulary* v) {
	++_nrvocabs.back();
	_usingvocab.push_back(v);
}

void Insert::usingvocab(const longname& vs, YYLTYPE l) {
	ParseInfo pi = parseinfo(l);
	Vocabulary* v = vocabularyInScope(vs,pi);
	if(v) usevocabulary(v);
	else Error::undeclvoc(oneName(vs),pi);
}

void Insert::usingspace(const longname& vs, YYLTYPE l) {
	ParseInfo pi = parseinfo(l);
	Namespace* s = namespaceInScope(vs,pi);
	if(s) usenamespace(s);
	else Error::undeclspace(oneName(vs),pi);
}

void Insert::openblock() {
	_nrvocabs.push_back(0);
	_nrspaces.push_back(0);
}

void Insert::closeblock() {
	for(unsigned int n = 0; n < _nrvocabs.back(); ++n) _usingvocab.pop_back();
	for(unsigned int n = 0; n < _nrspaces.back(); ++n) _usingspace.pop_back();
	_nrvocabs.pop_back();
	_nrspaces.pop_back();
	_currvocabulary = 0;
	_currtheory = 0;
	_currstructure = 0;
	_curroptions = 0;
	_currprocedure = 0;
	_currquery = "";
}

void Insert::openspace(const string& sname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	Namespace* ns = new Namespace(sname,_currspace,pi);
	_currspace = ns;
	usenamespace(ns);
}

void Insert::closespace() {
	if(_currspace->super()->isGlobal()) LuaConnection::addGlobal(_currspace);
	_currspace = _currspace->super(); assert(_currspace);
	closeblock();
}

void Insert::openvocab(const string& vname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	Vocabulary* v = vocabularyInScope(vname,pi);
	if(v) Error::multdeclvoc(vname,pi,v->pi());
	_currvocabulary = new Vocabulary(vname,pi);
	_currspace->add(_currvocabulary);
	usevocabulary(_currvocabulary);
}

void Insert::assignvocab(InternalArgument* arg, YYLTYPE l) {
	Vocabulary* v = LuaConnection::vocabulary(arg);
	if(v) {
		_currvocabulary->addVocabulary(v);
	}
	else {
		ParseInfo pi = parseinfo(l);
		Error::vocabexpected(pi);
	}
}

void Insert::closevocab() {
	assert(_currvocabulary);
	if(_currspace->isGlobal()) LuaConnection::addGlobal(_currvocabulary);
	closeblock();
}

void Insert::setvocab(const longname& vs, YYLTYPE l) {
	ParseInfo pi = parseinfo(l);
	Vocabulary* v = vocabularyInScope(vs,pi);
	if(v) {
		usevocabulary(v);
		_currvocabulary = v;
		if(_currstructure) _currstructure->vocabulary(v);
		else if(_currtheory) _currtheory->vocabulary(v);
	}
	else {
		Error::undeclvoc(oneName(vs),pi);
		_currvocabulary = Vocabulary::std();
		if(_currstructure) _currstructure->vocabulary(Vocabulary::std());
		else if(_currtheory) _currtheory->vocabulary(Vocabulary::std());
	}
}

void Insert::externvocab(const vector<string>& vname, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	Vocabulary* v = vocabularyInScope(vname,pi);
	if(v) _currvocabulary->addVocabulary(v); 
	else Error::undeclvoc(oneName(vname),pi);
}

void Insert::openquery(const string& qname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	Query* q = queryInScope(qname,pi);
	_currquery = qname;
	if(q) Error::multdeclquery(qname,pi,q->pi());
}

void Insert::opentheory(const string& tname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	AbstractTheory* t = theoryInScope(tname,pi);
	if(t) Error::multdecltheo(tname,pi,t->pi());
	_currtheory = new Theory(tname,pi);
	_currspace->add(_currtheory);
}

void Insert::assigntheory(InternalArgument* arg, YYLTYPE l) {
	AbstractTheory* t = LuaConnection::theory(arg);
	if(t) _currtheory->addTheory(t);
	else {
		ParseInfo pi = parseinfo(l);
		Error::theoryexpected(pi);
	}
}

void Insert::closetheory() {
	assert(_currtheory);
	if(_currspace->isGlobal()) LuaConnection::addGlobal(_currtheory);
	closeblock();
}

void Insert::closequery(Query* q) {
	_curr_vars.clear();
	if(q) {
		std::set<Variable*> sv(q->variables().begin(),q->variables().end());
		QuantForm* qf = new QuantForm(true,true,sv,q->query(),FormulaParseInfo());
		SortDeriver sd(qf,_currvocabulary); 
		SortChecker sc(qf,_currvocabulary);
		delete(qf);
		_currspace->add(_currquery,q);
		if(_currspace->isGlobal()) LuaConnection::addGlobal(_currquery,q);
	}
	closeblock();
}

void Insert::openstructure(const string& sname, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	AbstractStructure* s = structureInScope(sname,pi);
	if(s) Error::multdeclstruct(sname,pi,s->pi());
	_currstructure = new Structure(sname,pi);
	_currspace->add(_currstructure);
}

void Insert::assignstructure(InternalArgument* arg, YYLTYPE l) {
	AbstractStructure* s = LuaConnection::structure(arg);
	if(s) _currstructure->addStructure(s);
	else {
		ParseInfo pi = parseinfo(l);
		Error::structureexpected(pi);
	}
}

void Insert::closestructure() {
	assert(_currstructure);
	assignunknowntables();
	if(_options->autocomplete()) _currstructure->autocomplete();
	_currstructure->functioncheck();
	if(_currspace->isGlobal()) LuaConnection::addGlobal(_currstructure);
	closeblock();
}

void Insert::openprocedure(const string& name, YYLTYPE l) {
	// open block
	openblock();
	ParseInfo pi = parseinfo(l);
	UserProcedure* p = procedureInScope(name,pi);
	if(p) Error::multdeclproc(name,pi,p->pi());
	_currprocedure = new UserProcedure(name,pi,l.descr);
	_currspace->add(_currprocedure);

	// include the namespaces and vocabularies in scope
	for(vector<Namespace*>::const_iterator it = _usingspace.begin(); it != _usingspace.end(); ++it) {
		if(!(*it)->isGlobal()) {
			stringstream sstr;
			for(map<string,UserProcedure*>::const_iterator jt = (*it)->procedures().begin(); 
				jt != (*it)->procedures().end(); ++jt) {
				sstr << "local " << jt->second->name() << " = ";
				(*it)->putluaname(sstr);
				sstr << '.' << jt->second->name() << '\n';
			}
			for(map<string,Vocabulary*>::const_iterator jt = (*it)->vocabularies().begin(); 
				jt != (*it)->vocabularies().end(); ++jt) {
				sstr << "local " << jt->second->name() << " = ";
				(*it)->putluaname(sstr);
				sstr << '.' << jt->second->name() << '\n';
			}
			for(map<string,AbstractTheory*>::const_iterator jt = (*it)->theories().begin(); 
				jt != (*it)->theories().end(); ++jt) {
				sstr << "local " << jt->second->name() << " = ";
				(*it)->putluaname(sstr);
				sstr << '.' << jt->second->name() << '\n';
			}
			for(map<string,AbstractStructure*>::const_iterator jt = (*it)->structures().begin(); 
				jt != (*it)->structures().end(); ++jt) {
				sstr << "local " << jt->second->name() << " = ";
				(*it)->putluaname(sstr);
				sstr << '.' << jt->second->name() << '\n';
			}
			_currprocedure->add(sstr.str());
		}
	}
}

void Insert::closeprocedure(stringstream* chunk) {
	_currprocedure->add(chunk->str());
	LuaConnection::compile(_currprocedure);
	if(_currspace->isGlobal()) LuaConnection::addGlobal(_currprocedure);
	closeblock();
}

void Insert::openoptions(const string& name, YYLTYPE l) {
	openblock();
	ParseInfo pi = parseinfo(l);
	Options* o = optionsInScope(name,pi);
	if(o) Error::multdeclproc(name,pi,o->pi());
	_curroptions = new Options(name,pi);
	_currspace->add(_curroptions);
}

void Insert::closeoptions() {
	if(_currspace->isGlobal()) LuaConnection::addGlobal(_curroptions);
	closeblock();
}

Sort* Insert::sort(Sort* s) const {
	if(s) _currvocabulary->addSort(s);
	return s;
}

Predicate* Insert::predicate(Predicate* p) const {
	if(p) _currvocabulary->addPred(p);
	return p;
}

Function* Insert::function(Function* f) const {
	if(f) _currvocabulary->addFunc(f);
	return f;
}

Sort* Insert::sortpointer(const longname& vs, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	Sort* s = sortInScope(vs,pi);
	if(!s) Error::undeclsort(oneName(vs),pi);
	return s;
}

Predicate* Insert::predpointer(longname& vs, int arity, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	vs.back() = vs.back() + '/' + convertToString(arity);
	Predicate* p = predInScope(vs,pi);
	if(!p) Error::undeclpred(oneName(vs),pi);
	return p;
}

Predicate* Insert::predpointer(longname& vs, const vector<Sort*>& va, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	longname copyvs = vs;
	Predicate* p = predpointer(copyvs,va.size(),l);
	if(p) p = p->resolve(va);
	if(!p) Error::undeclpred(predName(vs,va),pi);
	return p;
}

Function* Insert::funcpointer(longname& vs, int arity, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	vs.back() = vs.back() + '/' + convertToString(arity);
	Function* f = funcInScope(vs,pi);
	if(!f) Error::undeclfunc(oneName(vs),pi);
	return f;
}

Function* Insert::funcpointer(longname& vs, const vector<Sort*>& va, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	longname copyvs = vs;
	Function* f = funcpointer(copyvs,va.size()-1,l);
	if(f) f = f->resolve(va);
	if(!f) Error::undeclfunc(funcName(vs,va),pi);
	return f;
}

NSPair* Insert::internpredpointer(const vector<string>& name, const vector<Sort*>& sorts, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	return new NSPair(name,sorts,false,pi);
}

NSPair* Insert::internfuncpointer(const vector<string>& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	NSPair* nsp = new NSPair(name,insorts,false,pi);
	nsp->_sorts.push_back(outsort);
	nsp->_func = true;
	return nsp;
}

NSPair* Insert::internpointer(const vector<string>& name, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	return new NSPair(name,false,pi);
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name		the name of the new sort	
 * \param sups		the supersorts of the new sort
 * \param subs		the subsorts of the new sort
 */
Sort* Insert::sort(const string& name, const vector<Sort*> sups, const vector<Sort*> subs, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);

	// Create the sort
	Sort* s = new Sort(name,pi);

	// Add the sort to the current vocabulary
	_currvocabulary->addSort(s);

	// Collect the ancestors of all super- and subsorts
	vector<std::set<Sort*> > supsa(sups.size());
	vector<std::set<Sort*> > subsa(subs.size());
	for(unsigned int n = 0; n < sups.size(); ++n) {
		if(sups[n]) {
			supsa[n] = sups[n]->ancestors(0);
			supsa[n].insert(sups[n]);
		}
	}
	for(unsigned int n = 0; n < subs.size(); ++n) {
		if(subs[n]) {
			subsa[n] = subs[n]->ancestors(0);
			subsa[n].insert(subs[n]);
		}
	}

	// Add the supersorts
	for(unsigned int n = 0; n < sups.size(); ++n) {
		if(sups[n]) {
			for(unsigned int m = 0; m < subs.size(); ++m) {
				if(subs[m]) {
					if(subsa[m].find(sups[n]) == subsa[m].end()) {
						Error::notsubsort(subs[m]->name(),sups[n]->name(),parseinfo(l));
					}
				}
			}
			s->addParent(sups[n]);
		}
	}

	// Add the subsorts
	for(unsigned int n = 0; n < subs.size(); ++n) {
		if(subs[n]) {
			for(unsigned int m = 0; m < sups.size(); ++m) {
				if(sups[m]) {
					if(supsa[m].find(subs[n]) != supsa[m].end()) {
						Error::cyclichierarchy(subs[n]->name(),sups[m]->name(),parseinfo(l));
					}
				}
			}
			s->addChild(subs[n]);
		}
	}
	return s;
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name	the name of the sort
 */
Sort* Insert::sort(const string& name, YYLTYPE l) const {
	vector<Sort*> vs(0);
	return sort(name,vs,vs,l);
}

/**
 * \brief Create a new sort in the current vocabulary
 *
 * \param name		the name of the sort
 * \param supbs		the super- or subsorts of the sort
 * \param p			true if supbs are the supersorts, false if supbs are the subsorts
 */
Sort* Insert::sort(const string& name, const vector<Sort*> supbs, bool p, YYLTYPE l) const {
	vector<Sort*> vs(0);
	if(p) return sort(name,supbs,vs,l);
	else return sort(name,vs,supbs,l);
}


Predicate* Insert::predicate(const string& name, const vector<Sort*>& sorts, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	string nar = string(name) + '/' + convertToString(sorts.size());
	for(unsigned int n = 0; n < sorts.size(); ++n) {
		if(!sorts[n]) return 0;
	}
	Predicate* p = new Predicate(nar,sorts,pi);
	_currvocabulary->addPred(p);
	return p;
}

Predicate* Insert::predicate(const string& name, YYLTYPE l) const {
	vector<Sort*> vs(0);
	return predicate(name,vs,l);
}

Function* Insert::function(const string& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	string nar = string(name) + '/' + convertToString(insorts.size());
	for(unsigned int n = 0; n < insorts.size(); ++n) {
		if(!insorts[n]) return 0;
	}
	if(!outsort) return 0;
	Function* f = new Function(nar,insorts,outsort,pi);
	_currvocabulary->addFunc(f);
	return f;
}

Function* Insert::function(const string& name, Sort* outsort, YYLTYPE l) const {
	vector<Sort*> vs(0);
	return function(name,vs,outsort,l);
}

Function* Insert::aritfunction(const string& name, const vector<Sort*>& sorts, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	for(unsigned int n = 0; n < sorts.size(); ++n) {
		if(!sorts[n]) return 0;
	}
	Function* orig = _currvocabulary->func(name);
	unsigned int binding = orig ? orig->binding() : 0;
	Function* f = new Function(name,sorts,pi,binding);
	_currvocabulary->addFunc(f);
	return f;
}

InternalArgument* Insert::call(const longname& proc, const vector<longname>& args, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	return LuaConnection::call(proc,args,pi);
}

InternalArgument* Insert::call(const longname& proc, YYLTYPE l) const {
	vector<longname> vl(0);
	return call(proc,vl,l);
}

void Insert::definition(Definition* d) const {
	if(d) _currtheory->add(d);
}

void Insert::sentence(Formula* f) {
	if(f) {
		// 1. Quantify the free variables universally
		std::set<Variable*> vv = freevars(f->pi());
		if(!vv.empty()) f =  new QuantForm(true,true,vv,f,f->pi());
		// 2. Sort derivation & checking
		SortDeriver sd(f,_currvocabulary); 
		SortChecker sc(f,_currvocabulary);
		// 3. Add the formula to the current theory
		_currtheory->add(f);
	}
	else _curr_vars.clear();
}

void Insert::fixpdef(FixpDef* d) const {
	if(d) _currtheory->add(d);
}

Definition* Insert::definition(const vector<Rule*>& rules) const {
	Definition* d = new Definition();
	for(unsigned int n = 0; n < rules.size(); ++n) {
		if(rules[n]) d->add(rules[n]);
	}
	return d;
}

Rule* Insert::rule(const std::set<Variable*>& qv,Formula* head, Formula* body,YYLTYPE l) {
	ParseInfo pi = parseinfo(l);
	remove_vars(qv);
	if(head && body) {
		// Quantify the free variables
		std::set<Variable*> vv = freevars(head->pi());
		remove_vars(vv);
		// Split quantified variables in head and body variables
		std::set<Variable*> hv;
		std::set<Variable*> bv;
		for(std::set<Variable*>::const_iterator it = qv.begin(); it != qv.end(); ++it) {
			if(head->contains(*it)) hv.insert(*it);
			else bv.insert(*it);
		}
		for(std::set<Variable*>::const_iterator it = vv.begin(); it != vv.end(); ++it) {
			if(head->contains(*it)) hv.insert(*it);
			else bv.insert(*it);
		}
		// Create a new rule
		if(!(bv.empty())) body = new QuantForm(true,false,bv,body,FormulaParseInfo((body->pi())));
		assert(typeid(*head) == typeid(PredForm));
		PredForm* pfhead = dynamic_cast<PredForm*>(head);
		Rule* r = new Rule(hv,pfhead,body,pi);
		// Sort derivation
		SortDeriver sd(r,_currvocabulary);
		// Return the rule
		return r;
	}
	else {
		_curr_vars.clear();
		if(head) head->recursiveDelete();
		if(body) body->recursiveDelete();
		for(std::set<Variable*>::const_iterator it = qv.begin(); it != qv.end(); ++it) delete(*it);
		return 0;
	}
}

Rule* Insert::rule(const std::set<Variable*>& qv, Formula* head, YYLTYPE l) {
	Formula* body = FormulaUtils::trueform();
	return rule(qv,head,body,l);
}

Rule* Insert::rule(Formula* head, Formula* body, YYLTYPE l) {
	std::set<Variable*> vv;
	return rule(vv,head,body,l);
}

Rule* Insert::rule(Formula* head,YYLTYPE l) {
	Formula* body = FormulaUtils::trueform();
	return rule(head,body,l);
}

Formula* Insert::trueform(YYLTYPE l) const {
	vector<Formula*> vf(0);
	FormulaParseInfo pi = formparseinfo(new BoolForm(true,true,vf,FormulaParseInfo()),l);
	return new BoolForm(true,true,vf,pi);
}

Formula* Insert::falseform(YYLTYPE l) const {
	vector<Formula*> vf(0);
	FormulaParseInfo pi = formparseinfo(new BoolForm(true,false,vf,FormulaParseInfo()),l);
	return new BoolForm(true,false,vf,pi);
}

Formula* Insert::predform(NSPair* nst, const vector<Term*>& vt, YYLTYPE l) const {
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != vt.size()) Error::incompatiblearity(nst->toString(),nst->_pi);
		if(nst->_func) Error::prednameexpected(nst->_pi);
	}
	nst->includeArity(vt.size());
	Predicate* p = predInScope(nst->_name,nst->_pi);
	if(p && nst->_sortsincluded && (nst->_sorts).size() == vt.size()) p = p->resolve(nst->_sorts);

	PredForm* pf = 0;
	if(p) {
		if(belongsToVoc(p)) {
			unsigned int n = 0;
			for(; n < vt.size(); ++n) { if(!vt[n]) break; }
			if(n == vt.size()) {
				vector<Term*> vtpi;
				for(vector<Term*>::const_iterator it = vt.begin(); it != vt.end(); ++it) {
					if((*it)->pi().original()) vtpi.push_back((*it)->pi().original()->clone());
					else vtpi.push_back((*it)->clone());
				}
				PredForm* pipf = new PredForm(true,p,vtpi,FormulaParseInfo());
				FormulaParseInfo pi = formparseinfo(pipf,l);
				pf = new PredForm(true,p,vt,pi);
			}
		}
		else Error::prednotintheovoc(p->name(),_currtheory->name(),nst->_pi);
	}
	else Error::undeclpred(nst->toString(),nst->_pi);
	
	// Cleanup
	if(!pf) {
		for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) vt[n]->recursiveDelete();	}
	}
	delete(nst);

	return pf;
}

Formula* Insert::predform(NSPair* t, YYLTYPE l) const {
	vector<Term*> vt(0);
	return predform(t,vt,l);
}

Formula* Insert::funcgraphform(NSPair* nst, const vector<Term*>& vt, Term* t, YYLTYPE l) const {
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != vt.size() + 1) Error::incompatiblearity(nst->toString(),nst->_pi);
		if(!nst->_func) Error::funcnameexpected(nst->_pi);
	}
	nst->includeArity(vt.size());
	Function* f = funcInScope(nst->_name,nst->_pi);
	if(f && nst->_sortsincluded && (nst->_sorts).size() == vt.size()+1) f = f->resolve(nst->_sorts);

	PredForm* pf = 0;
	if(f) {
		if(belongsToVoc(f)) {
			unsigned int n = 0;
			for(; n < vt.size(); ++n) { if(!vt[n]) break; }
			if(n == vt.size() && t) {
				vector<Term*> vt2(vt); vt2.push_back(t);
				vector<Term*> vtpi;
				for(vector<Term*>::const_iterator it = vt2.begin(); it != vt2.end(); ++it) {
					if((*it)->pi().original()) vtpi.push_back((*it)->pi().original()->clone());
					else vtpi.push_back((*it)->clone());
				}
				FormulaParseInfo pi = formparseinfo(new PredForm(true,f,vtpi,FormulaParseInfo()),l);
				pf = new PredForm(true,f,vt2,pi);	
			}
		}
		else Error::funcnotintheovoc(f->name(),_currtheory->name(),nst->_pi);
	}
	else Error::undeclfunc(nst->toString(),nst->_pi);

	// Cleanup
	if(!pf) {
		for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]); }
		if(t) delete(t);
	}
	delete(nst);

	return pf;
}

Formula* Insert::funcgraphform(NSPair* nst, Term* t, YYLTYPE l) const {
	vector<Term*> vt;
	return funcgraphform(nst,vt,t,l);
}

Formula* Insert::equivform(Formula* lf, Formula* rf, YYLTYPE l) const {
	if(lf && rf) {
		Formula* lfpi = lf->pi().original() ? lf->pi().original()->clone() : lf->clone();
		Formula* rfpi = rf->pi().original() ? rf->pi().original()->clone() : rf->clone();
		FormulaParseInfo pi = formparseinfo(new EquivForm(true,lfpi,rfpi,FormulaParseInfo()),l);
		return new EquivForm(true,lf,rf,pi);
	}
	else {
		if(lf) delete(lf);
		if(rf) delete(rf);
		return 0;
	}
}

Formula* Insert::boolform(bool conj, Formula* lf, Formula* rf, YYLTYPE l) const {
	if(lf && rf) {
		vector<Formula*> vf(2);
		vector<Formula*> pivf(2);
		vf[0] = lf; vf[1] = rf;
		pivf[0] = lf->pi().original() ? lf->pi().original()->clone() : lf->clone();
		pivf[1] = rf->pi().original() ? rf->pi().original()->clone() : rf->clone();
		FormulaParseInfo pi = formparseinfo(new BoolForm(true,conj,pivf,FormulaParseInfo()),l);
		return new BoolForm(true,conj,vf,pi);
	}
	else {
		if(lf) delete(lf);
		if(rf) delete(rf);
		return 0;
	}
}

Formula* Insert::disjform(Formula* lf, Formula* rf, YYLTYPE l) const {
	return boolform(false,lf,rf,l);
}

Formula* Insert::conjform(Formula* lf, Formula* rf, YYLTYPE l) const {
	return boolform(true,lf,rf,l);
}

Formula* Insert::implform(Formula* lf, Formula* rf, YYLTYPE l) const {
	if(lf) lf->negate();
	return boolform(false,lf,rf,l);
}

Formula* Insert::revimplform(Formula* lf, Formula* rf, YYLTYPE l) const {
	if(rf) rf->negate();
	return boolform(false,rf,lf,l);
}

Formula* Insert::quantform(bool univ, const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	remove_vars(vv);
	if(f) {
		std::set<Variable*> pivv;
		map<Variable*,Variable*> mvv;
		for(std::set<Variable*>::const_iterator it = vv.begin(); it != vv.end(); ++it) {
			Variable* v = new Variable((*it)->name(),(*it)->sort(),(*it)->pi());
			pivv.insert(v);
			mvv[*it] = v;
		}
		Formula* pif = f->pi().original() ? f->pi().original()->clone(mvv) : f->clone(mvv);
		FormulaParseInfo pi = formparseinfo(new QuantForm(true,univ,pivv,pif,FormulaParseInfo()),l);
		return new QuantForm(true,univ,vv,f,pi);
	}
	else {
		for(std::set<Variable*>::const_iterator it = vv.begin(); it != vv.end(); ++it) delete(*it);
		return 0;
	}
}

Formula* Insert::univform(const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	return quantform(true,vv,f,l);
}

Formula* Insert::existform(const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	return quantform(false,vv,f,l);
}

Formula* Insert::bexform(CompType c, int bound, const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	if(f) {
		SetExpr* se = set(vv,f,l);
		AggTerm* a = dynamic_cast<AggTerm*>(aggregate(AGG_CARD,se,l));
		Term* b = domterm(bound,l);
		AggTerm* pia = a->pi().original() ? dynamic_cast<AggTerm*>(a->pi().original()->clone()) : a->clone();
		Term* pib = b->pi().original() ? b->pi().original()->clone() : b->clone();
		FormulaParseInfo pi = formparseinfo(new AggForm(true,pib,invertComp(c),pia,FormulaParseInfo()),l);
		return new AggForm(true,b,invertComp(c),a,pi);
	}
	else return 0;
}

void Insert::negate(Formula* f) const {
	f->negate();
}


Formula* Insert::eqchain(CompType c, Formula* f, Term* t, YYLTYPE) const {
	if(f && t) {
		assert(typeid(*f) == typeid(EqChainForm));
		EqChainForm* ecf = dynamic_cast<EqChainForm*>(f);
		ecf->add(c,t);
		Formula* orig = ecf->pi().original();
		Term* pit = t->pi().original() ? t->pi().original()->clone() : t->clone();
		if(orig) {
			EqChainForm* ecfpi = dynamic_cast<EqChainForm*>(orig);
			ecfpi->add(c,pit);
		}
	}
	return f;
}

Formula* Insert::eqchain(CompType c, Term* left, Term* right, YYLTYPE l) const {
	if(left && right) {
		Term* leftpi = left->pi().original() ? left->pi().original()->clone() : left->clone();
		Term* rightpi = right->pi().original() ? right->pi().original()->clone() : right->clone();
		EqChainForm* ecfpi = new EqChainForm(true,true,leftpi,FormulaParseInfo());
		ecfpi->add(c,rightpi);
		FormulaParseInfo fpi = formparseinfo(ecfpi,l);
		EqChainForm* ecf = new EqChainForm(true,true,left,fpi);
		ecf->add(c,right);
		return ecf;
	}
	else return 0;
}


Variable* Insert::quantifiedvar(const string& name, YYLTYPE l) {
	ParseInfo pi = parseinfo(l);
	Variable* v = new Variable(name,0,pi);
	_curr_vars.push_front(VarName(name,v));
	return v;
}

Variable* Insert::quantifiedvar(const string& name, Sort* sort, YYLTYPE l) {
	Variable* v = quantifiedvar(name,l);
	if(sort) v->sort(sort);
	return v;
}

Sort* Insert::theosortpointer(const vector<string>& vs, YYLTYPE l) const {
	Sort* s = sortpointer(vs,l);
	if(s) {
		if(belongsToVoc(s)) {
			return s;
		}
		else {
			ParseInfo pi = parseinfo(l);
			string uname = oneName(vs);
			if(_currtheory) Error::sortnotintheovoc(uname,_currtheory->name(),pi);
			else if(_currstructure) Error::sortnotinstructvoc(uname,_currstructure->name(),pi);
			return 0;
		}
	}
	else return 0;
}


Term* Insert::functerm(NSPair* nst, const vector<Term*>& vt) {
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != vt.size()+1) Error::incompatiblearity(nst->toString(),nst->_pi);
		if(!nst->_func) Error::funcnameexpected(nst->_pi);
	}
	nst->includeArity(vt.size());
	Function* f = funcInScope(nst->_name,nst->_pi);
	if(f && nst->_sortsincluded && (nst->_sorts).size() == vt.size()+1) f = f->resolve(nst->_sorts);

	FuncTerm* t = 0;
	if(f) {
		if(belongsToVoc(f)) {
			unsigned int n = 0;
			for(; n < vt.size(); ++n) { if(!vt[n]) break; }
			if(n == vt.size()) {
				vector<Term*> vtpi;
				for(vector<Term*>::const_iterator it = vt.begin(); it != vt.end(); ++it) {
					if((*it)->pi().original()) vtpi.push_back((*it)->pi().original()->clone());
					else vtpi.push_back((*it)->clone());
				}
				TermParseInfo pi = termparseinfo(new FuncTerm(f,vtpi,TermParseInfo()),nst->_pi);
				t = new FuncTerm(f,vt,pi);
			}
		}
		else Error::funcnotintheovoc(f->name(),_currtheory->name(),nst->_pi);
	}
	else Error::undeclfunc(nst->toString(),nst->_pi);

	// Cleanup
	if(!t) {
		for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]);	}
	}
	delete(nst);

	return t;
}

Variable* Insert::getVar(const string& name) const {
	for(list<VarName>::const_iterator i = _curr_vars.begin(); i != _curr_vars.end(); ++i) {
		if(name == i->_name) return i->_var;
	}
	return 0;
}


Term* Insert::functerm(NSPair* nst) {
	if(nst->_sortsincluded || (nst->_name).size() != 1) {
		vector<Term*> vt = vector<Term*>(0);
		return functerm(nst,vt);
	}
	else {
		Term* t = 0;
		string name = (nst->_name)[0];
		Variable* v = getVar(name);
		nst->includeArity(0);
		Function* f = funcInScope(nst->_name,nst->_pi);
		if(v) {
			if(f) Warning::varcouldbeconst((nst->_name)[0],nst->_pi);
			t = new VarTerm(v,termparseinfo(new VarTerm(v,TermParseInfo()),nst->_pi));
			delete(nst);
		}
		else if(f) {
			vector<Term*> vt(0);
			nst->_name = vector<string>(1,name); nst->_arityincluded = false;
			t = functerm(nst,vt);
		}
		else {
			YYLTYPE l; 
			l.first_line = (nst->_pi).line();
			l.first_column = (nst->_pi).col();
			v = quantifiedvar(name,l);
			t = new VarTerm(v,termparseinfo(new VarTerm(v,TermParseInfo()),nst->_pi));
			delete(nst);
		}
		return t;
	}
}

Term* Insert::arterm(char c, Term* lt, Term* rt, YYLTYPE l) const {
	if(lt && rt) {
		Function* f = _currvocabulary->func(string(1,c) + "/2");
		assert(f);
		vector<Term*> vt(2); vt[0] = lt; vt[1] = rt;
		vector<Term*> pivt(2); 
		pivt[0] = lt->pi().original() ? lt->pi().original()->clone() : lt->clone();
		pivt[1] = rt->pi().original() ? rt->pi().original()->clone() : rt->clone();
		return new FuncTerm(f,vt,termparseinfo(new FuncTerm(f,pivt,TermParseInfo()),l));
	}
	else {
		if(lt) delete(lt);
		if(rt) delete(rt);
		return 0;
	}
}

Term* Insert::arterm(const string& s, Term* t, YYLTYPE l) const {
	if(t) {
		Function* f = _currvocabulary->func(s + "/1");
		assert(f);
		vector<Term*> vt(1,t);
		vector<Term*> pivt(1,t->pi().original() ? t->pi().original()->clone() : t->clone());
		return new FuncTerm(f,vt,termparseinfo(new FuncTerm(f,pivt,TermParseInfo()),l));
	}
	else {
		delete(t);
		return 0;
	}
}


Term* Insert::domterm(int i,YYLTYPE l) const {
	const DomainElement* d = DomainElementFactory::instance()->create(i);
	Sort* s = (i >= 0 ? VocabularyUtils::natsort() : VocabularyUtils::intsort());
	TermParseInfo pi = termparseinfo(new DomainTerm(s,d,TermParseInfo()),l);
	return new DomainTerm(s,d,pi);
}

Term* Insert::domterm(double f,YYLTYPE l) const	{
	const DomainElement* d = DomainElementFactory::instance()->create(f);
	Sort* s = VocabularyUtils::floatsort();
	TermParseInfo pi = termparseinfo(new DomainTerm(s,d,TermParseInfo()),l);
	return new DomainTerm(s,d,pi);
}

Term* Insert::domterm(std::string* e,YYLTYPE l) const {
	const DomainElement* d = DomainElementFactory::instance()->create(e);
	Sort* s = VocabularyUtils::stringsort();
	TermParseInfo pi = termparseinfo(new DomainTerm(s,d,TermParseInfo()),l);
	return new DomainTerm(s,d,pi);
}

Term* Insert::domterm(char c,YYLTYPE l) const {
	const DomainElement* d = DomainElementFactory::instance()->create(StringPointer(string(1,c)));
	Sort* s = VocabularyUtils::charsort();
	TermParseInfo pi = termparseinfo(new DomainTerm(s,d,TermParseInfo()),l);
	return new DomainTerm(s,d,pi);
}

Term* Insert::domterm(std::string* e,Sort* s,YYLTYPE l) const {
	const DomainElement* d = DomainElementFactory::instance()->create(e);
	TermParseInfo pi = termparseinfo(new DomainTerm(s,d,TermParseInfo()),l);
	return new DomainTerm(s,d,pi);
}

Term* Insert::aggregate(AggFunction f, SetExpr* s, YYLTYPE l) const {
	if(s) {
		SetExpr* pis = s->pi().original() ? s->pi().original()->clone() : s->clone();
		TermParseInfo pi = termparseinfo(new AggTerm(pis,f,TermParseInfo()),l);
		return new AggTerm(s,f,pi);
	}
	else return 0;
}

Query* Insert::query(const std::vector<Variable*>& vv, Formula* f, YYLTYPE l) {
	remove_vars(vv);
	if(f) {
		ParseInfo pi = parseinfo(l);
		return new Query(vv,f,pi);
	}
	else {
		for(std::vector<Variable*>::const_iterator it = vv.begin(); it != vv.end(); ++it) delete(*it);
		return 0;
	}
}

SetExpr* Insert::set(const std::set<Variable*>& vv, Formula* f, Term* counter, YYLTYPE l) {
	remove_vars(vv);
	if(f && counter) {
		std::set<Variable*> pivv;
		map<Variable*,Variable*> mvv;
		for(std::set<Variable*>::const_iterator it = vv.begin(); it != vv.end(); ++it) {
			Variable* v = new Variable((*it)->name(),(*it)->sort(),(*it)->pi());
			pivv.insert(v);
			mvv[*it] = v;
		}
		Term* picounter = counter->pi().original() ? counter->pi().original()->clone() : counter->clone(); 
		Formula* pif = f->pi().original() ? f->pi().original()->clone(mvv) : f->clone(mvv);
		SetParseInfo pi = setparseinfo(new QuantSetExpr(pivv,pif,picounter,SetParseInfo()),l);
		return new QuantSetExpr(vv,f,counter,pi);
	}
	else {
		if(f) { f->recursiveDelete(); }
		if(counter) { counter->recursiveDelete(); }
		for(std::set<Variable*>::const_iterator it = vv.begin(); it != vv.end(); ++it) {
			delete(*it);
		}
		return 0;
	}
}

SetExpr* Insert::set(const std::set<Variable*>& vv, Formula* f, YYLTYPE l) {
	const DomainElement* d = DomainElementFactory::instance()->create(1);
	Term* counter = new DomainTerm(VocabularyUtils::natsort(),d,TermParseInfo());
	return set(vv,f,counter,l);
}

SetExpr* Insert::set(EnumSetExpr* s) const {
	return s;
}

EnumSetExpr* Insert::createEnum(YYLTYPE l) const {
	EnumSetExpr* pis = new EnumSetExpr(SetParseInfo());
	SetParseInfo pi = setparseinfo(pis,l);
	return new EnumSetExpr(pi);
}

void Insert::addFT(EnumSetExpr* s, Formula* f, Term* t) const {
	if(f && s && t) {
		SetExpr* orig = s->pi().original();
		if(orig && typeid(*orig) == typeid(EnumSetExpr)) {
			EnumSetExpr* origset = dynamic_cast<EnumSetExpr*>(orig);
			Formula* pif = f->pi().original() ? f->pi().original()->clone() : f->clone();
			Term* tif = t->pi().original() ? t->pi().original()->clone() : t->clone();
			origset->addTerm(tif);
			origset->addFormula(pif);
		}
		s->addTerm(t);
		s->addFormula(f);
	}
	else {
		if(f) f->recursiveDelete();
		if(s) s->recursiveDelete();
		if(t) t->recursiveDelete();
	}
}

void Insert::addFormula(EnumSetExpr* s, Formula* f) const {
	const DomainElement* d = DomainElementFactory::instance()->create(1);
	Term* t = new DomainTerm(VocabularyUtils::natsort(),d,TermParseInfo());
	addFT(s,f,t);
}

void Insert::emptyinter(NSPair* nst) const {
	if(nst->_sortsincluded) {
		if(nst->_func) {
			EnumeratedInternalFuncTable* ift = new EnumeratedInternalFuncTable();
			FuncTable* ft = new FuncTable(ift,TableUtils::fullUniverse(nst->_sorts.size()));
			funcinter(nst,ft);
		}
		else {
			EnumeratedInternalPredTable* ipt = new EnumeratedInternalPredTable();
			PredTable* pt = new PredTable(ipt,TableUtils::fullUniverse(nst->_sorts.size()));
			predinter(nst,pt);
		}
	}
	else {
		ParseInfo pi = nst->_pi;
		std::set<Predicate*> vp = noArPredInScope(nst->_name,pi);
		if(vp.empty()) Error::undeclpred(nst->toString(),pi);
		else if(vp.size() > 1) {
			std::set<Predicate*>::const_iterator it = vp.begin();
			Predicate* p1 = *it; 
			++it;
			Predicate* p2 = *it;
			Error::overloadedpred(nst->toString(),p1->pi(),p2->pi(),pi);
		}
		else {
			EnumeratedInternalPredTable* ipt = new EnumeratedInternalPredTable();
			PredTable* pt = new PredTable(ipt,TableUtils::fullUniverse((*(vp.begin()))->arity()));
			predinter(nst,pt);
		}
	}
}

void Insert::predinter(NSPair* nst, PredTable* t) const {
	ParseInfo pi = nst->_pi;
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != t->arity()) Error::incompatiblearity(nst->toString(),pi);
		if(nst->_func) Error::prednameexpected(pi);
	}
	nst->includeArity(t->arity());
	Predicate* p = predInScope(nst->_name,pi);
	if(p && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) p = p->resolve(nst->_sorts);
	if(p) {
		if(belongsToVoc(p)) {
			PredTable* nt = new PredTable(t->interntable(),_currstructure->universe(p));
			delete(t);
			PredInter* inter = _currstructure->inter(p);
			inter->ctpt(nt);
		}
		else Error::prednotinstructvoc(nst->toString(),_currstructure->name(),pi);
	}
	else Error::undeclpred(nst->toString(),pi);
	delete(nst);
}


void Insert::funcinter(NSPair* nst, FuncTable* t) const {
	ParseInfo pi = nst->_pi;
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != t->arity()+1) Error::incompatiblearity(nst->toString(),pi);
		if(!(nst->_func)) Error::funcnameexpected(pi);
	}
	nst->includeArity(t->arity());
	Function* f = funcInScope(nst->_name,pi);
	if(f && nst->_sortsincluded && (nst->_sorts).size() == t->arity()+1) f = f->resolve(nst->_sorts);
	if(f) {
		if(belongsToVoc(f)) {
			FuncTable* nt = new FuncTable(t->interntable(),_currstructure->universe(f));
			delete(t);
			FuncInter* inter = _currstructure->inter(f);
			inter->functable(nt);
		}
		else Error::funcnotinstructvoc(nst->toString(),_currstructure->name(),pi);
	}
	else Error::undeclfunc(nst->toString(),pi);
	delete(nst);
}

void Insert::constructor(NSPair* nst) const {
	ParseInfo pi = nst->_pi;
	Function* f = 0;
	if(nst->_sortsincluded) {
		if(!(nst->_func)) Error::funcnameexpected(pi);
		nst->includeFuncArity();
		f = funcInScope(nst->_name,pi);
		if(f) f = f->resolve(nst->_sorts);
		else Error::undeclfunc(nst->toString(),pi);
	}
	else {
		std::set<Function*> vf = noArFuncInScope(nst->_name,pi);
		if(vf.empty()) Error::undeclfunc(nst->toString(),pi);
		else if(vf.size() > 1) {
			std::set<Function*>::const_iterator it = vf.begin();
			Function* f1 = *it; 
			++it;
			Function* f2 = *it;
			Error::overloadedfunc(nst->toString(),f1->pi(),f2->pi(),pi);
		}
		else f = *(vf.begin());
	}
	if(f) {
		if(belongsToVoc(f)) {
			UNAInternalFuncTable* uift = new UNAInternalFuncTable(f);
			FuncTable* ft = new FuncTable(uift,_currstructure->universe(f));
			FuncInter* inter = _currstructure->inter(f);
			inter->functable(ft);
		}
		else Error::funcnotinstructvoc(nst->toString(),_currstructure->name(),pi);
	}
}

void Insert::sortinter(NSPair* nst, SortTable* t) const {
	ParseInfo pi = nst->_pi;
	longname name = nst->_name;
	Sort* s = sortInScope(name,pi);
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != 1) Error::incompatiblearity(nst->toString(),pi);
		if(nst->_func) Error::prednameexpected(pi);
	}
	nst->includeArity(1);
	Predicate* p = predInScope(nst->_name,pi);
	if(p && nst->_sortsincluded && (nst->_sorts).size() == 1) p = p->resolve(nst->_sorts);
	if(s) {
		if(belongsToVoc(s)) {
			SortTable* st = _currstructure->inter(s);
			st->interntable(t->interntable());
			delete(t);
		}
		else Error::sortnotinstructvoc(oneName(name),_currstructure->name(),pi);
	}
	else if(p) {
		if(belongsToVoc(p)) {
			PredTable* pt = new PredTable(t->interntable(),_currstructure->universe(p));
			PredInter* i = _currstructure->inter(p);
			i->ctpt(pt);
			delete(t);
		}
		else Error::prednotinstructvoc(nst->toString(),_currstructure->name(),pi);
	}
	else Error::undeclpred(nst->toString(),pi);
	delete(nst);
}

void Insert::addElement(SortTable* s, int i) const {
	const DomainElement* d = DomainElementFactory::instance()->create(i);
	s->add(d);
}

void Insert::addElement(SortTable* s, double f) const {
	const DomainElement* d = DomainElementFactory::instance()->create(f);
	s->add(d);
}

void Insert::addElement(SortTable* s, std::string* e) const {
	const DomainElement* d = DomainElementFactory::instance()->create(e);
	s->add(d);
}

void Insert::addElement(SortTable* s,const Compound* c)	const {
	const DomainElement* d = DomainElementFactory::instance()->create(c);
	s->add(d);
}

void Insert::addElement(SortTable* s, int i1, int i2)	const {
	s->add(i1,i2);
}

void Insert::addElement(SortTable* s, char c1, char c2) const {
	for(char c = c1; c <= c2; ++c) addElement(s,StringPointer(string(1,c)));
}

SortTable* Insert::createSortTable() const {
	EnumeratedInternalSortTable* eist = new EnumeratedInternalSortTable();
	return new SortTable(eist);
}

void Insert::truepredinter(NSPair* nst) const {
	EnumeratedInternalPredTable* eipt = new EnumeratedInternalPredTable();
	PredTable* pt = new PredTable(eipt,Universe(vector<SortTable*>(0)));
	ElementTuple et;
	pt->add(et);
	predinter(nst,pt);
}

void Insert::falsepredinter(NSPair* nst) const {
	EnumeratedInternalPredTable* eipt = new EnumeratedInternalPredTable();
	PredTable* pt = new PredTable(eipt,Universe(vector<SortTable*>(0)));
	predinter(nst,pt);
}

PredTable* Insert::createPredTable(unsigned int arity) const {
	EnumeratedInternalPredTable* eipt = new EnumeratedInternalPredTable();
	PredTable* pt = new PredTable(eipt,TableUtils::fullUniverse(arity));
	return pt;
}

void Insert::addTuple(PredTable* pt, ElementTuple& tuple, YYLTYPE l) const {
	if(tuple.size() == pt->arity()) {
		pt->add(tuple);
	}
	else if(pt->empty()) {
		pt->add(tuple);
	}
	else {
		ParseInfo pi = parseinfo(l);
		Error::wrongarity(pi);
	}
}

void Insert::addTuple(PredTable* pt, YYLTYPE l) const {
	ElementTuple tuple;
	addTuple(pt,tuple,l);
}

const DomainElement* Insert::element(int i) const {
	return DomainElementFactory::instance()->create(i);
}

const DomainElement* Insert::element(double d) const {
	return DomainElementFactory::instance()->create(d);
}

const DomainElement* Insert::element(char c) const {
	return DomainElementFactory::instance()->create(StringPointer(string(1,c)));
}

const DomainElement* Insert::element(std::string* s) const {
	return DomainElementFactory::instance()->create(s);
}

const DomainElement* Insert::element(const Compound* c) const {
	return DomainElementFactory::instance()->create(c);
}

FuncTable* Insert::createFuncTable(unsigned int arity) const {
	EnumeratedInternalFuncTable* eift = new EnumeratedInternalFuncTable();
	return new FuncTable(eift,TableUtils::fullUniverse(arity));
}

void Insert::addTupleVal(FuncTable* ft, ElementTuple& tuple, YYLTYPE l) const {
	if(ft->arity() == tuple.size()-1) {
		ft->add(tuple);
	}
	else if(ft->empty()) {
		ft->add(tuple);
	}
	else {
		ParseInfo pi = parseinfo(l);
		Error::wrongarity(pi);
	}
}

void Insert::addTupleVal(FuncTable* ft, const DomainElement* d, YYLTYPE l) const {
	ElementTuple et(1,d);
	addTupleVal(ft,et,l);
}

void Insert::inter(NSPair* nsp, const longname& procedure, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	UserProcedure* up = procedureInScope(procedure,pi);
	string* proc = 0;
	if(up) proc = StringPointer(up->registryindex());
	else proc = LuaConnection::getProcedure(procedure,pi);
	if(proc) {
		vector<SortTable*> univ;
		if(nsp->_sortsincluded) {
			for(vector<Sort*>::const_iterator it = nsp->_sorts.begin(); it != nsp->_sorts.end(); ++it) {
				if(*it) {
					univ.push_back(_currstructure->inter(*it));
				}
			}
			if(nsp->_func) {
				ProcInternalFuncTable* pift = new ProcInternalFuncTable(proc);
				FuncTable* ft = new FuncTable(pift,Universe(univ));
				funcinter(nsp,ft);
			}
			else {
				ProcInternalPredTable* pipt = new ProcInternalPredTable(proc);
				PredTable* pt = new PredTable(pipt,Universe(univ));
				predinter(nsp,pt);
			}
		}
		else {
				ParseInfo pi = nsp->_pi;
			std::set<Predicate*> vp = noArPredInScope(nsp->_name,pi);
			if(vp.empty()) Error::undeclpred(nsp->toString(),pi);
			else if(vp.size() > 1) {
				std::set<Predicate*>::const_iterator it = vp.begin();
				Predicate* p1 = *it; 
				++it;
				Predicate* p2 = *it;
				Error::overloadedpred(nsp->toString(),p1->pi(),p2->pi(),pi);
			}
			else {
				for(vector<Sort*>::const_iterator it = (*(vp.begin()))->sorts().begin(); it != (*(vp.begin()))->sorts().end(); ++it) {
					if(*it) {
						univ.push_back(_currstructure->inter(*it));
					}
				}
				ProcInternalPredTable* pipt = new ProcInternalPredTable(proc);
				PredTable* pt = new PredTable(pipt,Universe(univ));
				predinter(nsp,pt);
			}
		}
	}
}

void Insert::emptythreeinter(NSPair* nst, const string& utf) {
	if(nst->_sortsincluded) {
		EnumeratedInternalPredTable* ipt = new EnumeratedInternalPredTable();
		PredTable* pt = new PredTable(ipt,TableUtils::fullUniverse(nst->_sorts.size()));
		if(nst->_func) threefuncinter(nst,utf,pt);
		else threepredinter(nst,utf,pt);
	}
	else {
		ParseInfo pi = nst->_pi;
		std::set<Predicate*> vp = noArPredInScope(nst->_name,pi);
		if(vp.empty()) Error::undeclpred(nst->toString(),pi);
		else if(vp.size() > 1) {
			std::set<Predicate*>::const_iterator it = vp.begin();
			Predicate* p1 = *it; 
			++it;
			Predicate* p2 = *it;
			Error::overloadedpred(nst->toString(),p1->pi(),p2->pi(),pi);
		}
		else {
			EnumeratedInternalPredTable* ipt = new EnumeratedInternalPredTable();
			PredTable* pt = new PredTable(ipt,TableUtils::fullUniverse((*(vp.begin()))->arity()));
			threepredinter(nst,utf,pt);
		}
	}
}

void Insert::threepredinter(NSPair* nst, const string& utf, PredTable* t) {
	ParseInfo pi = nst->_pi;
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != t->arity()) Error::incompatiblearity(nst->toString(),pi);
		if(nst->_func) Error::prednameexpected(pi);
	}
	nst->includeArity(t->arity());
	Predicate* p = predInScope(nst->_name,pi);
	if(p && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) p = p->resolve(nst->_sorts);
	if(p) {
		if(p->arity() == 1 && p->sort(0)->pred() == p) {
			Error::threevalsort(p->name(),pi);
		}
		else {
			if(belongsToVoc(p)) {
				PredTable* nt = new PredTable(t->interntable(),_currstructure->universe(p));
				delete(t);
				switch(getUTF(utf,pi)) {
					case UTF_UNKNOWN:
						_unknownpredtables[p] = nt;
						break;
					case UTF_CT:
					{	
						PredInter* pt = _currstructure->inter(p);
						pt->ct(nt);
						_cpreds[p] = UTF_CT;
						break;
					}
					case UTF_CF:
					{
						PredInter* pt = _currstructure->inter(p);
						pt->cf(nt);
						_cpreds[p] = UTF_CF;
						break;
					}
					case UTF_ERROR:
						break;
					default:
						assert(false);
				}
			}
			else Error::prednotinstructvoc(nst->toString(),_currstructure->name(),pi);
		}
	}
	else Error::undeclpred(nst->toString(),pi);
	delete(nst);
}

void Insert::threefuncinter(NSPair* nst, const string& utf, PredTable* t) {
	ParseInfo pi = nst->_pi;
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != t->arity()) Error::incompatiblearity(nst->toString(),pi);
		if(!(nst->_func)) Error::funcnameexpected(pi);
	}
	nst->includeArity(t->arity()-1);
	Function* f = funcInScope(nst->_name,pi);
	if(f && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) f = f->resolve(nst->_sorts);
	if(f) {
		if(belongsToVoc(f)) {
			PredTable* nt = new PredTable(t->interntable(),_currstructure->universe(f));
			delete(t);
			switch(getUTF(utf,pi)) {
				case UTF_UNKNOWN:
					_unknownfunctables[f] = nt;
					break;
				case UTF_CT:
				{	
					PredInter* ft = _currstructure->inter(f)->graphinter();
					ft->ct(nt);
					_cfuncs[f] = UTF_CT;
					break;
				}
				case UTF_CF:
				{
					PredInter* ft = _currstructure->inter(f)->graphinter();
					ft->cf(nt);
					_cfuncs[f] = UTF_CF;
					break;
				}
				case UTF_ERROR:
					break;
				default:
					assert(false);
			}
		}
		else Error::funcnotinstructvoc(nst->toString(),_currstructure->name(),pi);
	}
	else Error::undeclfunc(nst->toString(),pi);
}

void Insert::threepredinter(NSPair* nst, const string& utf, SortTable* t) {
	PredTable* pt = new PredTable(t->interntable(),TableUtils::fullUniverse(1));
	delete(t);
	threepredinter(nst,utf,pt);
}

void Insert::truethreepredinter(NSPair* nst, const string& utf) {
	EnumeratedInternalPredTable* eipt = new EnumeratedInternalPredTable();
	PredTable* pt = new PredTable(eipt,Universe(vector<SortTable*>(0)));
	ElementTuple et;
	pt->add(et);
	threepredinter(nst,utf,pt);
}

void Insert::falsethreepredinter(NSPair* nst, const string& utf) {
	EnumeratedInternalPredTable* eipt = new EnumeratedInternalPredTable();
	PredTable* pt = new PredTable(eipt,Universe(vector<SortTable*>(0)));
	threepredinter(nst,utf,pt);
}

pair<int,int>* Insert::range(int i1, int i2, YYLTYPE l) const {
	if(i1 > i2) {
		i2 = i1;
		Error::invalidrange(i1,i2,parseinfo(l));
	}
	pair<int,int>* p = new pair<int,int>(i1,i2);
	return p;
}

pair<char,char>* Insert::range(char c1, char c2, YYLTYPE l) const {
	if(c1 > c2) {
		c2 = c1;
		Error::invalidrange(c1,c2,parseinfo(l));
	}
	pair<char,char>* p = new pair<char,char>(c1,c2);
	return p;
}

const Compound* Insert::compound(NSPair* nst, const vector<const DomainElement*>& vte) const {
	ParseInfo pi = nst->_pi;
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != vte.size()+1) Error::incompatiblearity(nst->toString(),pi);
		if(!(nst->_func)) Error::funcnameexpected(pi);
	}
	nst->includeArity(vte.size());
	Function* f = funcInScope(nst->_name,pi);
	const Compound* c = 0;
	if(f && nst->_sortsincluded && (nst->_sorts).size() == vte.size()+1) f = f->resolve(nst->_sorts);
	if(f) {
		if(belongsToVoc(f)) return DomainElementFactory::instance()->compound(f,vte);
		else Error::funcnotinstructvoc(nst->toString(),_currstructure->name(),pi);
	}
	else Error::undeclfunc(nst->toString(),pi);
	return c;
}

const Compound* Insert::compound(NSPair* nst) const {
	ElementTuple t;
	return compound(nst,t);
}

void Insert::predatom(NSPair* nst, const vector<ElRange>& args, bool t) const {
	ParseInfo pi = nst->_pi;
	if(nst->_sortsincluded) {
		if((nst->_sorts).size() != args.size()) Error::incompatiblearity(nst->toString(),pi);
		if(nst->_func) Error::prednameexpected(pi);
	}
	nst->includeArity(args.size());
	Predicate* p = predInScope(nst->_name,pi);
	if(p && nst->_sortsincluded && (nst->_sorts).size() == args.size()) p = p->resolve(nst->_sorts);
	if(p) {
		if(belongsToVoc(p)) {
			if(p->arity() == 1 && p == (*(p->sorts().begin()))->pred()) {
				Sort* s = *(p->sorts().begin());	
				SortTable* st = _currstructure->inter(s);
				switch(args[0]._type) {
					case ERE_EL:
						st->add(args[0]._value._element);
						break;
					case ERE_INT:
						st->add(args[0]._value._intrange->first,args[0]._value._intrange->second);
						break;
					case ERE_CHAR:
						for(char c = args[0]._value._charrange->first; c != args[0]._value._charrange->second; ++c) {
							st->add(DomainElementFactory::instance()->create(StringPointer(string(1,c))));
						}
						break;
					default:
						assert(false);
				}
			}
			else {
				ElementTuple tuple(p->arity());
				for(unsigned int n = 0; n < args.size(); ++n) {
					switch(args[n]._type) {
						case ERE_EL:
							tuple[n] = args[n]._value._element;
							break;
						case ERE_INT:
							tuple[n] = DomainElementFactory::instance()->create(args[n]._value._intrange->first);
							break;
						case ERE_CHAR:
							tuple[n] = DomainElementFactory::instance()->create(StringPointer(string(1,args[n]._value._charrange->first)));
							break;
						default:
							assert(false);
					}
				}
				PredInter* inter = _currstructure->inter(p);
				if(t) inter->makeTrue(tuple);
				else inter->makeFalse(tuple);
				while(true) {
					unsigned int n = 0;
					for(; n < args.size(); ++n) {
						bool end = false;
						switch(args[n]._type) {
							case ERE_EL:
								break;
							case ERE_INT:
							{
								int current = tuple[n]->value()._int;
								if(current == args[n]._value._intrange->second) { current = args[n]._value._intrange->first; }
								else { ++current; end = true; }
								tuple[n] = DomainElementFactory::instance()->create(current);
								break;
							}
							case ERE_CHAR:
							{
								char current = tuple[n]->value()._string->operator[](0);
								if(current == args[n]._value._charrange->second) { 
									current = args[n]._value._charrange->first; }
								else { ++current; end = true; }
								tuple[n] = DomainElementFactory::instance()->create(StringPointer(string(1,current)));
								break;
							}
							default:
								assert(false);
						}
						if(end) break;
					}
					if(n < args.size()) {
						if(t) inter->makeTrue(tuple);
						else inter->makeFalse(tuple);
					}
					else break;
				}
			}
		}
		else Error::prednotinstructvoc(nst->toString(),_currstructure->name(),pi);
	}
	else Error::undeclpred(nst->toString(),pi);
	delete(nst);
}

void Insert::predatom(NSPair* nst, bool t) const {
	vector<ElRange> ver;
	predatom(nst,ver,t);
}
	
void Insert::funcatom(NSPair* , const vector<ElRange>& , const DomainElement* , bool ) const {
	// TODO TODO TODO
}

void Insert::funcatom(NSPair* nst, const DomainElement* d, bool t) const {
	vector<ElRange> ver;
	funcatom(nst,ver,d,t);
}
	
vector<ElRange>* Insert::domaintuple(vector<ElRange>* dt, const DomainElement* d) const {
	dt->push_back(ElRange(d));
	return dt;
}

vector<ElRange>* Insert::domaintuple(vector<ElRange>* dt, pair<int,int>* p) const {
	dt->push_back(ElRange(p));
	return dt;
}

vector<ElRange>* Insert::domaintuple(vector<ElRange>* dt, pair<char,char>* p) const {
	dt->push_back(ElRange(p));
	return dt;
}

vector<ElRange>* Insert::domaintuple(const DomainElement* d) const {
	vector<ElRange>* dt = new vector<ElRange>(0);
	dt->push_back(ElRange(d));
	return dt;
}

vector<ElRange>* Insert::domaintuple(pair<int,int>* p) const {
	vector<ElRange>* dt = new vector<ElRange>(0);
	dt->push_back(ElRange(p));
	return dt;
}

vector<ElRange>* Insert::domaintuple(pair<char,char>* p) const {
	vector<ElRange>* dt = new vector<ElRange>(0);
	dt->push_back(ElRange(p));
	return dt;
}

void Insert::exec(stringstream* chunk) const {
	LuaConnection::execute(chunk);
}

void Insert::procarg(const string& argname) const {
	_currprocedure->addarg(argname);
}

void Insert::externoption(const vector<string>& name, YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	Options* opt = optionsInScope(name,pi);
	if(opt) _curroptions->setvalues(opt);
	else Error::undeclopt(oneName(name),pi);
}

void Insert::option(const string& opt, const string& val,YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	if(_curroptions->isoption(opt)) {
		if(_curroptions->setvalue(opt,val)) { } // do nothing
		else Error::wrongvalue(opt,val,pi);
	}
	else Error::unknoption(opt,pi);
}

void Insert::option(const string& opt, double val,YYLTYPE l) const { 
	ParseInfo pi = parseinfo(l);
	if(_curroptions->isoption(opt)) {
		if(_curroptions->setvalue(opt,val)) { } // do nothing
		else Error::wrongvalue(opt,convertToString(val),pi);
	}
	else Error::unknoption(opt,pi);
}

void Insert::option(const string& opt, int val,YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	if(_curroptions->isoption(opt)) {
		if(_curroptions->setvalue(opt,val)) { } // do nothing
		else Error::wrongvalue(opt,convertToString(val),pi);
	}
	else Error::unknoption(opt,pi);
}

void Insert::option(const string& opt, bool val,YYLTYPE l) const {
	ParseInfo pi = parseinfo(l);
	if(_curroptions->isoption(opt)) {
		if(_curroptions->setvalue(opt,val)) { } // do nothing
		else Error::wrongvalue(opt,val ? "true" : "false",pi);
	}
	else Error::unknoption(opt,pi);
}

void Insert::assignunknowntables() {
	// Assign the unknown predicate interpretations
	for(map<Predicate*,PredTable*>::iterator it = _unknownpredtables.begin(); it != _unknownpredtables.end(); ++it) {
		PredInter* pri = _currstructure->inter(it->first);
		const PredTable* ctable = _cpreds[it->first] == UTF_CT ? pri->ct() : pri->cf();
		PredTable* pt = new PredTable(ctable->interntable(),ctable->universe());
		for(TableIterator tit = it->second->begin() ; tit.hasNext(); ++tit) pt->add(*tit);
		_cpreds[it->first] == UTF_CT ? pri->pt(pt) : pri->pf(pt);
		delete(it->second);
	}
	// Assign the unknown function interpretations
	for(map<Function*,PredTable*>::iterator it = _unknownfunctables.begin(); it != _unknownfunctables.end(); ++it) {
		PredInter* pri = _currstructure->inter(it->first)->graphinter();
		const PredTable* ctable = _cfuncs[it->first] == UTF_CT ? pri->ct() : pri->cf();
		PredTable* pt = new PredTable(ctable->interntable(),ctable->universe());
		for(TableIterator tit = it->second->begin() ; tit.hasNext(); ++tit) pt->add(*tit);
		_cfuncs[it->first] == UTF_CT ? pri->pt(pt) : pri->pf(pt);
		delete(it->second);
	}
	_unknownpredtables.clear();
	_unknownfunctables.clear();
}
