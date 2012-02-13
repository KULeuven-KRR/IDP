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
#include "errorhandling/error.hpp"

using namespace std;

TermGrounder::~TermGrounder() {
	if (_origterm != NULL) {
		_origterm->recursiveDelete();
		_origterm = NULL;
	}
	if (not _varmap.empty()) {
		for (auto i = _varmap.begin(); i != _varmap.end(); ++i) {
			delete (i->first); // Delete the Variable
			// Don't delete the Domain element!
		}
		_varmap.clear();
	}
}

void TermGrounder::setOrig(const Term* t, const var2dommap& mvd, int verb) {
	_verbosity = verb;
	map<Variable*, Variable*> mvv;
	for (auto it = t->freeVars().cbegin(); it != t->freeVars().cend(); ++it) {
		Variable* v = new Variable((*it)->name(), (*it)->sort(), ParseInfo());
		mvv[*it] = v;
		_varmap[v] = mvd.find(*it)->second;
	}
	_origterm = t->clone(mvv);
}

void TermGrounder::printOrig() const {
	clog << tabs() << "Grounding term " << toString(_origterm) << "\n";
	if (not _origterm->freeVars().empty()) {
		pushtab();
		clog << tabs() << "with instance ";
		for (auto it = _origterm->freeVars().cbegin(); it != _origterm->freeVars().cend(); ++it) {
			clog << toString(*it) << " = ";
			const DomainElement* e = _varmap.find(*it)->second->get();
			clog << toString(e) << ' ';
		}
		clog << "\n";
		poptab();
	}
}

GroundTerm DomTermGrounder::run() const {
	return GroundTerm(_value);
}

GroundTerm FuncTermGrounder::run() const {
	if (_verbosity > 2) {
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
			Assert(groundsubterms[n]._domelement != NULL);
			args[n] = groundsubterms[n]._domelement;
		}
	}
	if (calculable && _functable != NULL) { // All ground subterms are domain elements!
		const DomainElement* domelem = (*_functable)[args];
		if (domelem) {
			if (_verbosity > 2) {
				poptab();
				clog << tabs() << "Result = " << toString(domelem) << "\n";
			}
			return GroundTerm(domelem);
		}
	}
	// Assert(isCPSymbol(_function->symbol())) && some of the ground subterms are CP terms.
	VarId varid = _termtranslator->translate(_function, groundsubterms);
	if (_verbosity > 2) {
		poptab();
		clog << tabs() << "Result = " << _termtranslator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}

CPTerm* createSumTerm(SumType type, const VarId& left, const VarId& right) {
	if (type == ST_MINUS) {
		varidlist varids({ left, right });
		intweightlist weights({ 1, -1 });
		return new CPWSumTerm(varids, weights);
	} else {
		return new CPSumTerm(left, right);
	}
}

GroundTerm SumTermGrounder::run() const {
	if (_verbosity > 2) {
		printOrig();
		pushtab();
	}
	// Run subtermgrounders
	const GroundTerm& left = _lefttermgrounder->run();
	const GroundTerm& right = _righttermgrounder->run();
	SortTable* leftdomain = _lefttermgrounder->getDomain();
	SortTable* rightdomain = _righttermgrounder->getDomain();

	// Compute domain for the sum term
	if (not _domain || not _domain->approxFinite()) {
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
			_domain = new SortTable(new IntRangeInternalSortTable(min, max));
		} else {
			if (leftdomain && not leftdomain->approxFinite()) {
				Warning::warning("Left domain is infinite...");
			}
			if (rightdomain && not rightdomain->approxFinite()) {
				Warning::warning("Right domain is infinite...");
			}
			//TODO one of the domains is unknown or infinite...
			//TODO one case when left or right is a domain element!
			throw notyetimplemented("One of the domains in a sumtermgrounder is infinite.");
		}
	}

	VarId varid;
	if (left.isVariable) {
		if (right.isVariable) {
			CPTerm* sumterm = createSumTerm(_type, left._varid, right._varid);
			varid = _termtranslator->translate(sumterm, _domain);
		} else {
			Assert(not right.isVariable);
			VarId rightvarid = _termtranslator->translate(right._domelement);
			// Create tseitin
			CPTsBody* cpelement = _termtranslator->cprelation(rightvarid);
			Lit tseitin = _grounding->translator()->translate(cpelement->left(), cpelement->comp(), cpelement->right(), TsType::EQ);
			_grounding->addUnitClause(tseitin);
			// Create cp sum term
			CPTerm* sumterm = createSumTerm(_type, left._varid, rightvarid);
			varid = _termtranslator->translate(sumterm, _domain);
		}
	} else {
		Assert(not left.isVariable);
		if (right.isVariable) {
			VarId leftvarid = _termtranslator->translate(left._domelement);
			// Create tseitin
			CPTsBody* cpelement = _termtranslator->cprelation(leftvarid);
			Lit tseitin = _grounding->translator()->translate(cpelement->left(), cpelement->comp(), cpelement->right(), TsType::EQ);
			_grounding->addUnitClause(tseitin);
			// Create cp sum term
			CPTerm* sumterm = createSumTerm(_type, leftvarid, right._varid);
			varid = _termtranslator->translate(sumterm, _domain);
		} else { // Both subterms are domain elements, so lookup the result in the function table.
			Assert(not right.isVariable);
			Assert(_functable != NULL);
			ElementTuple args({ left._domelement, right._domelement });
			const DomainElement* domelem = (*_functable)[args];
			Assert(domelem != NULL);
			if (_verbosity > 2) {
				poptab();
				clog << tabs() << "Result = " << toString(domelem) << "\n";
			}
			return GroundTerm(domelem);
		}
	}

	// Ask for a new tseitin for this cp constraint and add it to the grounding.
	CPTsBody* cprelation = _termtranslator->cprelation(varid);
	Lit sumtseitin = _grounding->translator()->translate(cprelation->left(),cprelation->comp(),cprelation->right(),TsType::EQ);
	_grounding->addUnitClause(sumtseitin);

	// Return result
	if (_verbosity > 2) {
		poptab();
		clog << tabs() << "Result = " << _termtranslator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}

GroundTerm AggTermGrounder::run() const {
	if (_verbosity > 2) {
		printOrig();
		pushtab();
	}
	SetId setnr = _setgrounder->run();
	auto tsset = _translator->groundset(setnr);
	// FIXME if grounding aggregates, with an upper and lower bound, should not return a domelem from the subgrounder, but a vardomain or something?
	if (tsset.empty()) {
		Weight value = applyAgg(_type, tsset.trueweights());
		const DomainElement* domelem = createDomElem(value);
		if (_verbosity > 2) {
			poptab();
			clog << tabs() << "Result = " << toString(domelem) << "\n";
		}
		return GroundTerm(domelem);
	} else {
		//FIXME in this case, there should have been a dedicated cpgrounder for this aggterm...
		Assert(false);
		return GroundTerm();
	}
}
