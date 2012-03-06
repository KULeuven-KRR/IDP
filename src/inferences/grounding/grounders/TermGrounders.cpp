/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "TermGrounders.hpp"

#include "SetGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"

#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/GroundTermTranslator.hpp"

#include "IncludeComponents.hpp"

using namespace std;

TermGrounder::~TermGrounder() {
	if (_origterm != NULL) {
		_origterm->recursiveDelete();
		_origterm = NULL;
	}
	if (not _varmap.empty()) {
		for (auto i = _varmap.begin(); i != _varmap.end(); ++i) {
			delete (i->first);
			//delete (i->second);
		}
		_varmap.clear();
	}
}

void TermGrounder::setOrig(const Term* t, const map<Variable*, const DomElemContainer*>& mvd) {
	map<Variable*, Variable*> mvv;
	for (auto it = t->freeVars().cbegin(); it != t->freeVars().cend(); ++it) {
		Variable* v = new Variable((*it)->name(), (*it)->sort(), ParseInfo());
		mvv[*it] = v;
		_varmap[v] = mvd.find(*it)->second;
	}
	_origterm = t->clone(mvv);
}

void TermGrounder::printOrig() const {
	clog << "" <<nt() << "Grounding term " << toString(_origterm);
	if (not _origterm->freeVars().empty()) {
		clog << "" <<nt() << " with instance ";
		for (auto it = _origterm->freeVars().cbegin(); it != _origterm->freeVars().cend(); ++it) {
			clog << toString(*it) << " = ";
			const DomainElement* e = _varmap.find(*it)->second->get();
			clog << toString(e) << ' ';
		}
	}
	clog << "" <<nt();
}

GroundTerm DomTermGrounder::run() const {
	//SortTable* _domain = new SortTable(new EnumeratedInternalSortTable());
	//_domain->add(_value);
	return GroundTerm(_value);
}

/*GroundTerm VarTermGrounder::run() const {
	if (_verbosity > 2) {
		clog <<"value="<<toString(_value->get());
	}
	return GroundTerm(_value->get());
}*/

GroundTerm FuncTermGrounder::run() const {
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	bool calculable = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for (size_t n = 0; n < _subtermgrounders.size(); ++n) {
		CHECKTERMINATION
		groundsubterms[n] = _subtermgrounders[n]->run();
		if (groundsubterms[n].isVariable) {
			calculable = false;
		} else {
			Assert(groundsubterms[n]._domelement!=NULL);
			args[n] = groundsubterms[n]._domelement;
		}
	}
	if (calculable && _functable) { // All ground subterms are domain elements!
		auto result = (*_functable)[args];
		Assert(result!=NULL);
		if (result!=NULL) {
			if (verbosity() > 2) {
				clog << "Result = " << *result;
				poptab();
				clog << "" <<nt();
			}
			return GroundTerm(result);
		}
	}
	// Assert(isCPSymbol(_function->symbol())) && some of the ground subterms are CP terms.
	auto varid = _termtranslator->translate(_function, groundsubterms);
	if (verbosity() > 2) {
		clog << "Result = " << _termtranslator->printTerm(varid);
		poptab();
		clog << "" <<nt();
	}
	return GroundTerm(varid);
}

CPTerm* createSumTerm(SumType type, const VarId& left, const VarId& right) {
	if (type == ST_MINUS) {
		return new CPWSumTerm({ left, right }, { 1, -1 });
	} else {
		return new CPSumTerm(left, right);
	}
}

GroundTerm SumTermGrounder::run() const {
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	// Run subtermgrounders
	auto left = _lefttermgrounder->run();
	auto right = _righttermgrounder->run();
	auto leftdomain = _lefttermgrounder->getDomain();
	auto rightdomain = _righttermgrounder->getDomain();

	// Compute domain for the sum term
	if (getDomain()==NULL || not getDomain()->approxFinite()) {
		if (not left.isVariable) {
			leftdomain = new SortTable(new EnumeratedInternalSortTable());
			leftdomain->add(left._domelement);
		}
		if (not right.isVariable) {
			rightdomain = new SortTable(new EnumeratedInternalSortTable());
			rightdomain->add(right._domelement);
		}
		if (leftdomain && rightdomain && leftdomain->approxFinite() && rightdomain->approxFinite()) {
			int leftmin = leftdomain->first()->value()._int;
			int rightmin = rightdomain->first()->value()._int;
			int leftmax = leftdomain->last()->value()._int;
			int rightmax = rightdomain->last()->value()._int;
			int min, max;
			if (_type == ST_PLUS) {
				min = leftmin + rightmin;
				max = leftmax + rightmax;
			} else if (_type == ST_MINUS) {
				min = leftmin - rightmax;
				max = leftmax - rightmin;
			}
			if (max < min) {
				swap(min, max);
			}
			setDomain(new SortTable(new IntRangeInternalSortTable(min, max)));
		} else {
			if (leftdomain && not leftdomain->approxFinite()) {
				clog << "Left domain is infinite..." << endl;
			}
			if (rightdomain && not rightdomain->approxFinite()) {
				clog << "Right domain is infinite..." << endl;
			}
			//TODO one of the domains is unknown or infinite...
			//TODO one case when left or right is a domain element!
			throw notyetimplemented("One of the domains in a sumtermgrounder is infinite.");
		}
	}

	VarId varid;
	if (left.isVariable) {
		if (right.isVariable) {
			auto sumterm = createSumTerm(_type, left._varid, right._varid);
			varid = _termtranslator->translate(sumterm, getDomain());
		} else {
			Assert(not right.isVariable);
			auto rightvarid = _termtranslator->translate(right._domelement);
			// Create tseitin
			auto cpelement = _termtranslator->cprelation(rightvarid);
			auto tseitin = _grounding->translator()->translate(cpelement->left(), cpelement->comp(), cpelement->right(), TsType::EQ);
			_grounding->addUnitClause(tseitin);
			// Create cp sum term
			auto sumterm = createSumTerm(_type, left._varid, rightvarid);
			varid = _termtranslator->translate(sumterm, getDomain());
		}
	} else {
		Assert(not left.isVariable);
		if (right.isVariable) {
			auto leftvarid = _termtranslator->translate(left._domelement);
			// Create tseitin
			auto cpelement = _termtranslator->cprelation(leftvarid);
			auto tseitin = _grounding->translator()->translate(cpelement->left(), cpelement->comp(), cpelement->right(), TsType::EQ);
			_grounding->addUnitClause(tseitin);
			// Create cp sum term
			auto sumterm = createSumTerm(_type, leftvarid, right._varid);
			varid = _termtranslator->translate(sumterm, getDomain());
		} else { // Both subterms are domain elements, so lookup the result in the function table.
			Assert(not right.isVariable && _functable!=NULL);
			auto result = _functable->operator []({left._domelement, right._domelement});
			Assert(result);
			if (verbosity() > 2) {
				clog << "Result = " << *result;
				poptab();
				clog << "" <<nt();
			}
			return GroundTerm(result);
		}
	}

	// Ask for a new tseitin for this cp constraint and add it to the grounding.
	//FIXME Following should be done more efficiently... overhead because of lookup in translator...
	//CPTsBody* cprelation = _termtranslator->cprelation(varid);
	//int sumtseitin = _grounding->translator()->translate(cprelation->left(),cprelation->comp(),cprelation->right(),TS_EQ);
	//_grounding->addUnitClause(sumtseitin);

	// Return result
	if (verbosity() > 2) {
		clog << "Result = " << _termtranslator->printTerm(varid);
		poptab();
		clog << "" <<nt();
	}
	return GroundTerm(varid);
}

GroundTerm AggTermGrounder::run() const {
//TODO Should this grounder return a VarId in some cases?
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	auto setnr = _setgrounder->run();
	auto tsset = _translator->groundset(setnr);
	Assert(not tsset.empty());
	auto value = applyAgg(_type, tsset.trueweights());
	auto result = createDomElem(value);
	if (verbosity() > 2) {
		clog << "Result = " << *result;
		poptab();
		clog << "" <<nt();
	}
	// FIXME if grounding aggregates, with an upper and lower bound, should not return a domelem from the subgrounder, but a vardomain or something?
	return GroundTerm(result);
}
