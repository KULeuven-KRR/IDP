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

#include "Sets.hpp"
#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"
#include "TheoryUtils.hpp"

using namespace std;

IMPLACCEPTBOTH(QuantSetExpr, QuantSetExpr)
IMPLACCEPTBOTH(EnumSetExpr, EnumSetExpr)

/*************
 *  SetExpr
 ************/

void SetExpr::recursiveDelete() {
	if (not _allwaysDeleteRecursively) {
		deleteChildren(true);
	}
	delete (this);
}

void SetExpr::recursiveDeleteKeepVars() {
	if (not _allwaysDeleteRecursively) {
		deleteChildren(false);
	}
	delete (this);
}

SetExpr::~SetExpr() {
	if (_allwaysDeleteRecursively) {
		deleteChildren(true);
	}
}

void SetExpr::setFreeVars() {
	_freevars.clear();
	if (getSubTerm() != NULL) {
		_freevars.insert(getSubTerm()->freeVars().cbegin(), getSubTerm()->freeVars().cend());
	}
	if (getSubFormula() != NULL) {
		_freevars.insert(getSubFormula()->freeVars().cbegin(), getSubFormula()->freeVars().cend());
	}
	for (auto i = getSubSets().cbegin(); i < getSubSets().cend(); ++i) {
		_freevars.insert((*i)->freeVars().cbegin(), (*i)->freeVars().cend());
	}
	for (auto i = _quantvars.cbegin(); i != _quantvars.cend(); ++i) {
		_freevars.erase(*i);
	}
}

void SetExpr::addSubSet(QuantSetExpr* set) {
	_subsets.push_back(set);
	setFreeVars();
}
void SetExpr::setSubSets(std::vector<QuantSetExpr*> sets) {
	_subsets = sets;
	setFreeVars();
}

void SetExpr::deleteChildren(bool andvars) {
	if (andvars) {
		if (getSubFormula() != NULL) {
			getSubFormula()->recursiveDelete();
		}
		if (_subterm != NULL) {
			_subterm->recursiveDelete();
		}
		for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
			//FIXME delete (*it);
		}
		for (auto i = _subsets.cbegin(); i < _subsets.cend(); ++i) {
			(*i)->recursiveDelete();
		}
	} else {
		if (getSubFormula() != NULL) {
			getSubFormula()->recursiveDeleteKeepVars();
		}
		if (_subterm != NULL) {
			_subterm->recursiveDeleteKeepVars();
		}
		for (auto i = _subsets.cbegin(); i < _subsets.cend(); ++i) {
			(*i)->recursiveDeleteKeepVars();
		}
	}
}

Sort* SetExpr::sort() const {
	Sort* currsort = NULL;
	if (getSubTerm() != NULL) {
		currsort = getSubTerm()->sort();
	}
	for (auto i = getSubSets().cbegin(); i < getSubSets().cend(); ++i) {
		auto sort = (*i)->sort();
		if (sort == NULL) {
			return NULL;
		}
		if (currsort == NULL) {
			currsort = sort;
		} else {
			currsort = SortUtils::resolve(currsort, sort);
			if (currsort == NULL) {
				throw notyetimplemented("Sets with terms with sorts without common ancestor");
			}
		}
	}
	return currsort;
}

bool SetExpr::contains(const Variable* v) const {
	for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		if (*it == v) {
			return true;
		}
	}
	for (auto it = _freevars.cbegin(); it != _freevars.cend(); ++it) {
		if (*it == v) {
			return true;
		}
	}
	if (getSubFormula() != NULL && getSubFormula()->contains(v)) {
		return true;
	}
	if (getSubTerm() != NULL && getSubTerm()->contains(v)) {
		return true;
	}
	for (auto it = getSubSets().cbegin(); it != getSubSets().cend(); ++it) {
		if ((*it)->contains(v)) {
			return true;
		}
	}
	return false;
}

ostream& operator<<(ostream& output, const SetExpr& set) {
	return set.put(output);
}

/*****************
 *  EnumSetExpr
 ****************/

EnumSetExpr::EnumSetExpr(const vector<QuantSetExpr*>& subsets, const SetParseInfo& pi)
		: SetExpr(pi) {
	setSubSets(subsets);
}

EnumSetExpr* EnumSetExpr::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

EnumSetExpr* EnumSetExpr::cloneKeepVars() const {
	vector<QuantSetExpr*> newsets;
	for (auto it = getSets().cbegin(); it != getSets().cend(); ++it) {
		newsets.push_back((*it)->cloneKeepVars());
	}
	return new EnumSetExpr(newsets, pi());
}

EnumSetExpr* EnumSetExpr::clone(const map<Variable*, Variable*>& mvv) const {
	vector<QuantSetExpr*> newsets;
	for (auto it = getSets().cbegin(); it != getSets().cend(); ++it) {
		newsets.push_back((*it)->clone(mvv));
	}
	return new EnumSetExpr(newsets, pi());
}

tablesize EnumSetExpr::maxSize(const Structure*) const {
	tablesize t(TST_EXACT, 0);
	for (auto i = getSets().cbegin(); i < getSets().cend(); ++i) {
		t = t + (*i)->maxSize();
	}
	return t;
}

ostream& EnumSetExpr::put(ostream& output) const {
	output << "[ ";
	for (auto i = getSets().cbegin(); i < getSets().cend(); ++i) {
		output << print(*i) << " ";
	}
	output << "]";
	return output;
}

/******************
 *  QuantSetExpr
 *****************/

QuantSetExpr::QuantSetExpr(const varset& qvars, Formula* formula, Term* term, const SetParseInfo& pi)
		: SetExpr(pi) {
	setTerm(term);
	setQuantVars(qvars);
	setCondition(formula);
}

QuantSetExpr* QuantSetExpr::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

QuantSetExpr* QuantSetExpr::cloneKeepVars() const {
	auto newterm = getTerm()->cloneKeepVars();
	auto newform = getCondition()->cloneKeepVars();
	return new QuantSetExpr(quantVars(), newform, newterm, pi());
}

QuantSetExpr* QuantSetExpr::clone(const map<Variable*, Variable*>& mvv) const {
	varset newvars;
	map<Variable*, Variable*> nmvv = mvv;
	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
		auto nv = new Variable((*it)->name(), (*it)->sort(), (*it)->pi());
		newvars.insert(nv);
		nmvv[*it] = nv;
	}
	auto newterm = getTerm()->clone(nmvv);
	auto newform = getCondition()->clone(nmvv);
	return new QuantSetExpr(newvars, newform, newterm, pi());
}

tablesize QuantSetExpr::maxSize(const Structure* structure) const {
	Assert(not (structure == NULL));
	auto t = tablesize(TST_EXACT, 1);
	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
		auto qvardom = structure->inter((*it)->sort());
		Assert(qvardom != NULL);
		t = t * qvardom->size();
	}
	return t;
}

ostream& QuantSetExpr::put(ostream& output) const {
	output << "{";
//	if (not quantVars().empty()) { // TODO when syntax changes
	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
		output << ' ' << print(*it);
	}
	output << " :";
//	}
	output << " " << print(getCondition()) << " : " << print(getTerm()) << " }";
	return output;
}
