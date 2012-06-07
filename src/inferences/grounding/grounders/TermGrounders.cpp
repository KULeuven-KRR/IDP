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

#include "utils/CPUtils.hpp"

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

void TermGrounder::setOrig(const Term* t, const var2dommap& mvd) {
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
	Assert(_value != NULL);
	return GroundTerm(_value);
}

GroundTerm FuncTermGrounder::run() const {
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	bool calculable = true;
	vector<GroundTerm> groundsubterms;
	ElementTuple args(_subtermgrounders.size());
	for (size_t n = 0; n < _subtermgrounders.size(); ++n) {
		CHECKTERMINATION
		auto groundterm = _subtermgrounders[n]->run();
		if (groundterm.isVariable) {
			calculable = false;
		} else {
			if(groundterm._domelement == NULL){
				if (verbosity() > 2) {
					poptab();
					clog << tabs() << "Result = **invalid term**" << "\n";
				}
				return groundterm;
			}
			args[n] = groundterm._domelement;
		}
		groundsubterms.push_back(groundterm);
	}
	if (calculable && _functable) { // All ground subterms are domain elements!
		auto result = (*_functable)[args];
		if (verbosity() > 2) {
			if (result) {
				poptab();
				clog << tabs() << "Result = " << toString(result) << "\n";
			} else {
				if (verbosity() > 2) {
					poptab();
					clog << tabs() << "Result = **invalid term**" << "\n";
				}
			}
		}
		return result;
	}

	Assert(getOption(BoolType::CPSUPPORT));
	auto varid = _termtranslator->translate(_function, groundsubterms);
	if (verbosity() > 2) {
		poptab();
		clog << tabs() << "Result = var" << _termtranslator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}

CPTerm* createCPSumTerm(const SumType& type, const VarId& left, const VarId& right) {
	if (type == SumType::ST_MINUS) {
		return new CPWSumTerm({ left, right }, { 1, -1 });
	} else {
		return new CPSumTerm(left, right);
	}
}

void SumTermGrounder::computeDomain(GroundTerm& left, GroundTerm& right) const {
	auto leftdomain = _lefttermgrounder->getDomain();
	auto rightdomain = _righttermgrounder->getDomain();
	if (getDomain() == NULL || not getDomain()->approxFinite()) {
		if (not left.isVariable) {
			leftdomain = TableUtils::createSortTable();
			leftdomain->add(left._domelement);
		}
		if (not right.isVariable) {
			rightdomain = TableUtils::createSortTable();
			rightdomain->add(right._domelement);
		}
		if (leftdomain && rightdomain && leftdomain->isRange() && rightdomain->isRange() && leftdomain->approxFinite() && rightdomain->approxFinite()) {
			Assert(leftdomain->first()->type() == DomainElementType::DET_INT);
			Assert(rightdomain->first()->type() == DomainElementType::DET_INT);
			int leftmin = leftdomain->first()->value()._int;
			int rightmin = rightdomain->first()->value()._int;
			int leftmax = leftdomain->last()->value()._int;
			int rightmax = rightdomain->last()->value()._int;
			int min, max;
			switch(_type){
			case SumType::ST_PLUS:
				min = leftmin + rightmin;
				max = leftmax + rightmax;
				break;
			case SumType::ST_MINUS:
				min = leftmin - rightmax;
				max = leftmax - rightmin;
				break;
			}
			if (max < min) {
				swap(min, max);
			}
			setDomain(TableUtils::createSortTable(min, max));
		} else if (leftdomain->approxFinite() && rightdomain->approxFinite()) {
			Assert(leftdomain->first()->type() == DomainElementType::DET_INT);
			Assert(rightdomain->first()->type() == DomainElementType::DET_INT);
			auto newdomain = TableUtils::createSortTable();
			for (auto leftit = leftdomain->sortBegin(); not leftit.isAtEnd(); ++leftit) {
				for (auto rightit = rightdomain->sortBegin(); not rightit.isAtEnd(); ++rightit) {
					int leftvalue = (*leftit)->value()._int;
					int rightvalue = (*rightit)->value()._int;
					int newvalue;
					switch(_type){
					case SumType::ST_PLUS:
						newvalue = leftvalue + rightvalue;
						break;
					case SumType::ST_MINUS:
						newvalue = leftvalue - rightvalue;
						break;
					}
					newdomain->add(createDomElem(newvalue));
				}
			}
			setDomain(newdomain);
		} else {
			if (leftdomain && not leftdomain->approxFinite()) {
				Warning::warning("Left domain is infinite...");
			}
			if (rightdomain && not rightdomain->approxFinite()) {
				Warning::warning("Right domain is infinite...");
			}
			//TODO one of the domains is unknown or infinite...
			//setDomain(new SortTable(new AllIntegers()));
			throw notyetimplemented("One of the domains in a sumtermgrounder is infinite.");
		}
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

	// Compute domain for the sum term
	computeDomain(left, right);

	VarId varid;
	if (left.isVariable) {
		if (right.isVariable) {
			auto sumterm = createCPSumTerm(_type, left._varid, right._varid);
			varid = _termtranslator->translate(sumterm, getDomain());
		} else {
			Assert(not right.isVariable);
			auto rightvarid = _termtranslator->translate(right._domelement);
			// Create cp sum term
			auto sumterm = createCPSumTerm(_type, left._varid, rightvarid);
			varid = _termtranslator->translate(sumterm, getDomain());
		}
	} else {
		Assert(not left.isVariable);
		if (right.isVariable) {
			auto leftvarid = _termtranslator->translate(left._domelement);
			// Create cp sum term
			auto sumterm = createCPSumTerm(_type, leftvarid, right._varid);
			varid = _termtranslator->translate(sumterm, getDomain());
		} else { // Both subterms are domain elements, so lookup the result in the function table.
			Assert(not right.isVariable && _functable!=NULL);
			auto domelem = _functable->operator[]({left._domelement, right._domelement});
			Assert(domelem);
			if (verbosity() > 2) {
				poptab();
				clog << tabs() << "Result = " << toString(domelem) << "\n";
			}
			return GroundTerm(domelem);
		}
	}

	// Return result
	if (verbosity() > 2) {
		poptab();
		clog << tabs() << "Result = " << _termtranslator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}

CPTerm* createCPAggTerm(const AggFunction& f, const varidlist& varids) {
	Assert(CPSupport::eligibleForCP(f));
	switch (f) {
	case SUM :
		return new CPSumTerm(varids);
	default:
		notyetimplemented("No CP support for aggregate functions other that sum.");
		return NULL;
	}
}

GroundTerm AggTermGrounder::run() const {
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	auto setnr = _setgrounder->run();
	auto tsset = _translator->groundset(setnr);
	Assert(tsset.literals().empty());

	if (not tsset.varids().empty()) {
		//Note: When the aggregate is not computable (its set is three-valued), it should've been rewritten into an AggForm!
		// Only when grounding with cpsupport it is possible that we end up here.
		Assert(getOption(BoolType::CPSUPPORT) && tsset.trueweights().empty());

		auto sumterm = createCPAggTerm(_type, tsset.varids());
		auto varid = _termtranslator->translate(sumterm, getDomain());

		if (verbosity() > 2) {
			poptab();
			clog << tabs() << "Result = " << _termtranslator->printTerm(varid) << "\n";
		}
		return GroundTerm(varid);
	}

	//Note: This only happens when the set is two-valued, and the aggregate is computable (otherwise this term would've been unnested and graphed to an AggForm).
	auto value = applyAgg(_type, tsset.trueweights());
	auto domelem = createDomElem(value);
	if (verbosity() > 2) {
		poptab();
		clog << tabs() << "Result = " << toString(domelem) << "\n";
	}
	return GroundTerm(domelem);
}
