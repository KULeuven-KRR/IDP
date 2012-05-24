/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

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

void SetExpr::deleteChildren(bool andvars) {
	if (andvars) {
		if(getSubFormula()!=NULL){
			getSubFormula()->recursiveDelete();
		}
		if(_subterm!=NULL){
			_subterm->recursiveDelete();
		}
		for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
			delete (*it);
		}
		for(auto i=_subsets.cbegin(); i<_subsets.cend(); ++i) {
			(*i)->recursiveDelete();
		}
	} else {
		if(getSubFormula()!=NULL){
			getSubFormula()->recursiveDeleteKeepVars();
		}
		if(_subterm!=NULL){
			_subterm->recursiveDeleteKeepVars();
		}
		for(auto i=_subsets.cbegin(); i<_subsets.cend(); ++i) {
			(*i)->recursiveDeleteKeepVars();
		}
	}
}

//Sort* SetExpr::sort() const {
//	auto it = _subterms.cbegin();
//	Sort* currsort;
//	if ((*it)->sort()) {
//		currsort = (*it)->sort();
//	} else {
//		return NULL;
//	}
//	++it;
//	for (; it != _subterms.cend(); ++it) {
//		if ((*it)->sort()) {
//			currsort = SortUtils::resolve(currsort, (*it)->sort());
//		} else {
//			return NULL;
//		}
//	}
//	if (currsort == NULL) {
//		throw notyetimplemented("Sets with terms with sorts without common ancestor");
//	} else {
//		return currsort;
//	}
//}
//
//bool SetExpr::contains(const Variable* v) const {
//	for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
//		if (*it == v) {
//			return true;
//		}
//	}
//	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
//		if ((*it)->contains(v)) {
//			return true;
//		}
//	}
//	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
//		if ((*it)->contains(v)) {
//			return true;
//		}
//	}
//	return false;
//}
//
//ostream& operator<<(ostream& output, const SetExpr& set) {
//	return set.put(output);
//}
//
///*****************
// *  EnumSetExpr
// ****************/
//
//EnumSetExpr::EnumSetExpr(const vector<Formula*>& subforms, const vector<Term*>& weights, const SetParseInfo& pi)
//		: SetExpr(pi) {
//	_subformulas = subforms;
//	_subterms = weights;
//	setFreeVars();
//}
//
//EnumSetExpr* EnumSetExpr::clone() const {
//	map<Variable*, Variable*> mvv;
//	return clone(mvv);
//}
//
//EnumSetExpr* EnumSetExpr::cloneKeepVars() const {
//	vector<Formula*> newforms;
//	vector<Term*> newweights;
//	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
//		newforms.push_back((*it)->cloneKeepVars());
//	}
//	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
//		newweights.push_back((*it)->cloneKeepVars());
//	}
//	return new EnumSetExpr(newforms, newweights, _pi);
//}
//
//EnumSetExpr* EnumSetExpr::clone(const map<Variable*, Variable*>& mvv) const {
//	vector<Formula*> newforms;
//	vector<Term*> newweights;
//	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
//		newforms.push_back((*it)->clone(mvv));
//	}
//	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
//		newweights.push_back((*it)->clone(mvv));
//	}
//	return new EnumSetExpr(newforms, newweights, _pi);
//}
//
//tablesize EnumSetExpr::maxSize(const AbstractStructure*) const {
//	return tablesize(TST_EXACT, subformulas().size());
//}
//
//ostream& EnumSetExpr::put(ostream& output) const {
//	output << "[ ";
//	if (not subformulas().empty()) {
//		for (size_t n = 0; n < subformulas().size(); ++n) {
//			output << '(';
//			subformulas()[n]->put(output);
//			output << ',';
//			subterms()[n]->put(output);
//			output << ')';
//			if (n < subformulas().size() - 1) {
//				output << "; ";
//			}
//		}
//	}
//	output << " ]";
//	return output;
//}
//
///******************
// *  QuantSetExpr
// *****************/
//
//QuantSetExpr::QuantSetExpr(const set<Variable*>& qvars, Formula* formula, Term* term, const SetParseInfo& pi)
//		: SetExpr(pi) {
//	_quantvars = qvars;
//	_subterms.push_back(term);
//	_subformulas.push_back(formula);
//	setFreeVars();
//}
//
//QuantSetExpr* QuantSetExpr::clone() const {
//	map<Variable*, Variable*> mvv;
//	return clone(mvv);
//}
//
//QuantSetExpr* QuantSetExpr::cloneKeepVars() const {
//	auto newterm = subterms()[0]->cloneKeepVars();
//	auto newform = subformulas()[0]->cloneKeepVars();
//	return new QuantSetExpr(quantVars(), newform, newterm, _pi);
//}
//
//QuantSetExpr* QuantSetExpr::clone(const map<Variable*, Variable*>& mvv) const {
//	set<Variable*> newvars;
//	map<Variable*, Variable*> nmvv = mvv;
//	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
//		Variable* nv = new Variable((*it)->name(), (*it)->sort(), (*it)->pi());
//		newvars.insert(nv);
//		nmvv[*it] = nv;
//	}
//	auto newterm = subterms()[0]->clone(nmvv);
//	auto newform = subformulas()[0]->clone(nmvv);
//	return new QuantSetExpr(newvars, newform, newterm, _pi);
//}
//
//tablesize QuantSetExpr::maxSize(const AbstractStructure* structure) const {
//	if (structure == NULL) {
//		return tablesize(TST_UNKNOWN, 0);
//	}
//	size_t currentsize = 1;
//	TableSizeType tst = TST_EXACT;
//	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
//		auto qvardom = structure->inter((*it)->sort());
//		Assert(qvardom != NULL);
//		tablesize qvardomsize = qvardom->size();
//		switch (qvardomsize._type) {
//		case TST_UNKNOWN:
//			return tablesize(TST_UNKNOWN, 0);
//		case TST_INFINITE:
//			return tablesize(TST_INFINITE, 0);
//		case TST_APPROXIMATED:
//			currentsize *= qvardomsize._size;
//			tst = TST_APPROXIMATED;
//			break;
//		case TST_EXACT:
//			currentsize *= qvardomsize._size;
//			break;
//		}
//	}
//	return tablesize(tst, currentsize);
//}
//
//ostream& QuantSetExpr::put(ostream& output) const {
//	output << "{";
//	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
//		output << ' ';
//		(*it)->put(output);
//	}
//	output << " : ";
//	subformulas()[0]->put(output);
//	output << " : ";
//	subterms()[0]->put(output);
//	output << " }";
//	return output;
//}
