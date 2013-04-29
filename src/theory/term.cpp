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
#include "errorhandling/error.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"
#include "TheoryUtils.hpp"

using namespace std;

IMPLACCEPTBOTH(VarTerm, Term)
IMPLACCEPTBOTH(FuncTerm, Term)
IMPLACCEPTBOTH(AggTerm, Term)
IMPLACCEPTBOTH(DomainTerm, Term)

void Term::setFreeVars() {
	_freevars.clear();
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		_freevars.insert((*it)->freeVars().cbegin(), (*it)->freeVars().cend());
	}
	for (auto it = _subsets.cbegin(); it != _subsets.cend(); ++it) {
		_freevars.insert((*it)->freeVars().cbegin(), (*it)->freeVars().cend());
	}
}

void Term::recursiveDelete() {
	if (not _allwaysDeleteRecursively) {
		deleteChildren(true);
	}
	delete (this);
}
void Term::recursiveDeleteKeepVars() {
	if (not _allwaysDeleteRecursively) {
		deleteChildren(false);
	}
	delete (this);
}

Term::~Term() {
	if (_allwaysDeleteRecursively) {
		deleteChildren(true);
	}
}

void Term::deleteChildren(bool varsalso) {
	if (varsalso) {
		for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
			(*it)->recursiveDelete();
		}
		for (auto it = _subsets.cbegin(); it != _subsets.cend(); ++it) {
			(*it)->recursiveDelete();
		}
	} else {
		for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
			(*it)->recursiveDeleteKeepVars();
		}
		for (auto it = _subsets.cbegin(); it != _subsets.cend(); ++it) {
			(*it)->recursiveDeleteKeepVars();
		}
	}
}

bool Term::contains(const Variable* v) const {
	for (auto it = _freevars.cbegin(); it != _freevars.cend(); ++it) {
		if (*it == v) {
			return true;
		}
	}
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		if ((*it)->contains(v)) {
			return true;
		}
	}
	for (auto it = _subsets.cbegin(); it != _subsets.cend(); ++it) {
		if ((*it)->contains(v)) {
			return true;
		}
	}
	return false;
}

void Term::addSet(EnumSetExpr* s) {
	if(_subsets.size()==0){
		_subsets.push_back(s);
	}else{
		for(auto qs:s->getSets()){
			_subsets.front()->addSet(qs);
		}
	}
	setFreeVars();
}

ostream& operator<<(ostream& output, const Term& t) {
	return t.put(output);
}

/*************
 *  VarTerm
 ************/

void VarTerm::setFreeVars() {
	Term::setFreeVars( { _var });
}

void VarTerm::sort(Sort* s) {
	_var->sort(s);
}

VarTerm::VarTerm(Variable* v, const TermParseInfo& pi)
		: 	Term(TermType::VAR, pi),
			_var(v) {
	Assert(v!=NULL);
	setFreeVars();
}

VarTerm* VarTerm::clone() const {
	return new VarTerm(_var, _pi);
}

VarTerm* VarTerm::cloneKeepVars() const {
	return new VarTerm(_var, _pi);
}

VarTerm* VarTerm::clone(const map<Variable*, Variable*>& mvv) const {
	auto it = mvv.find(_var);
	if (it != mvv.cend()) {
		return new VarTerm(it->second, _pi);
	} else {
		return new VarTerm(_var, _pi);
	}
}

inline Sort* VarTerm::sort() const {
	return _var->sort();
}

ostream& VarTerm::put(std::ostream& output) const {
	var()->put(output);
	return output;
}

/**************
 *  FuncTerm
 *************/

FuncTerm::FuncTerm(Function* function, const vector<Term*>& args, const TermParseInfo& pi)
		: 	Term(TermType::FUNC, pi),
			_function(function) {
	Assert(function!=NULL);
	subterms(args);
}

FuncTerm* FuncTerm::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

FuncTerm* FuncTerm::cloneKeepVars() const {
	vector<Term*> newargs;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		newargs.push_back((*it)->cloneKeepVars());
	}
	return new FuncTerm(_function, newargs, _pi);
}

FuncTerm* FuncTerm::clone(const map<Variable*, Variable*>& mvv) const {
	vector<Term*> newargs;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		newargs.push_back((*it)->clone(mvv));
	}
	return new FuncTerm(_function, newargs, _pi);
}

Sort* FuncTerm::sort() const {
	return _function->outsort();
}

ostream& FuncTerm::put(ostream& output) const {
	function()->put(output);
	if (not subterms().empty()) {
		output << '(';
		subterms()[0]->put(output);
		for (size_t n = 1; n < subterms().size(); ++n) {
			output << ',';
			subterms()[n]->put(output);
		}
		output << ')';
	}
	return output;
}

/****************
 *  DomainTerm
 ***************/

DomainTerm::DomainTerm(Sort* sort, const DomainElement* value, const TermParseInfo& pi)
		: 	Term(TermType::DOM, pi),
			_sort(sort),
			_value(value) {
	Assert(_sort!=NULL);
}

void DomainTerm::sort(Sort* s) {
	Assert(_sort!=NULL);
	_sort = s;
}

DomainTerm* DomainTerm::clone() const {
	return new DomainTerm(_sort, _value, _pi);
}

DomainTerm* DomainTerm::cloneKeepVars() const {
	return clone();
}

DomainTerm* DomainTerm::clone(const map<Variable*, Variable*>&) const {
	return new DomainTerm(_sort, _value, _pi);
}

ostream& DomainTerm::put(ostream& output) const {
	value()->put(output);
	return output;
}

/*************
 *  AggTerm
 ************/

AggTerm::AggTerm(EnumSetExpr* set, AggFunction function, const TermParseInfo& pi)
		: 	Term(TermType::AGG, pi),
			_function(function) {
	addSet(set);
}

AggTerm* AggTerm::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

AggTerm* AggTerm::cloneKeepVars() const {
	auto newset = subsets()[0]->cloneKeepVars();
	return new AggTerm(newset, _function, _pi);
}

AggTerm* AggTerm::clone(const map<Variable*, Variable*>& mvv) const {
	auto newset = subsets()[0]->clone(mvv);
	return new AggTerm(newset, _function, _pi);
}

Sort* AggTerm::sort() const {
	if (_function == AggFunction::CARD) {
		return get(STDSORT::NATSORT);
	} else {
		auto setsort = set()->sort();
		if (setsort == NULL) {
			return setsort;
		}
		if (function() == AggFunction::MAX || function() == AggFunction::MIN) {
			return setsort;
		}
		if (SortUtils::isSubsort(setsort, get(STDSORT::NATSORT))) {
			return get(STDSORT::NATSORT);
		} else if (SortUtils::isSubsort(setsort, get(STDSORT::INTSORT))) {
			return get(STDSORT::INTSORT);
		} else if (SortUtils::isSubsort(setsort, get(STDSORT::FLOATSORT))) {
			return get(STDSORT::FLOATSORT);
		} else {
			Error::notsubsort(setsort->name(), get(STDSORT::FLOATSORT)->name(), pi());
			return NULL;
		}
	}
}

ostream& AggTerm::put(ostream& output) const {
	output << function();
	subsets()[0]->put(output);
	return output;
}

namespace TermUtils {

vector<Term*> makeNewVarTerms(const vector<Variable*>& vars) {
	vector<Term*> terms;
	for (auto v : vars) {
		terms.push_back(new VarTerm(v, TermParseInfo()));
	}
	return terms;
}
vector<Term*> makeNewVarTerms(const vector<Sort*>& sorts) {
	vector<Variable*> vars;
	for (auto s : sorts) {
		vars.push_back(new Variable(s));
	}
	return makeNewVarTerms(vars);
}


Sort* deriveSmallerSort(const Term* term, const Structure* structure) {
	auto sort = term->sort();
	if (term->type()==TermType::VAR or structure == NULL or not SortUtils::isSubsort(term->sort(), get(STDSORT::INTSORT), structure->vocabulary()) or structure->inter(sort)->empty()) {
		return sort;
	}
	auto bounds = TermUtils::deriveTermBounds(term, structure);
	Assert(bounds.size()==2);
	if (bounds[0] != NULL and bounds[1] != NULL and bounds[0]->type() == DET_INT and bounds[1]->type() == DET_INT) {
		auto intmin = bounds[0]->value()._int;
		auto intmax = bounds[1]->value()._int;
		stringstream ss;
		ss << "s" << intmin << ".." << intmax;
		auto existingsort = structure->vocabulary()->sort(ss.str());
		if(existingsort!=NULL){
			return existingsort;
		}
		sort = new Sort(ss.str(), TableUtils::createSortTable(intmin, intmax));
		sort->addParent(get(STDSORT::INTSORT));
		structure->vocabulary()->add(sort);
	}
	return sort;
}

}
