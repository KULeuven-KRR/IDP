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

#include "TermGrounders.hpp"

#include "SetGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"

#include "inferences/grounding/GroundTranslator.hpp"

#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"

#include "utils/CPUtils.hpp"

#include <algorithm> // for min_element and max_element

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
	clog << tabs() << "Grounding term " << print(_origterm) << "\n";
	if (not _origterm->freeVars().empty()) {
		pushtab();
		clog << tabs() << "with instance ";
		for (auto it = _origterm->freeVars().cbegin(); it != _origterm->freeVars().cend(); ++it) {
			clog << print(*it) << " = ";
			const DomainElement* e = _varmap.find(*it)->second->get();
			clog << print(e) << ' ';
		}
		clog << "\n";
		poptab();
	}
}
int TermGrounder::verbosity() const{
	return getOption(IntType::VERBOSE_GROUNDING);
}


GroundTerm DomTermGrounder::run() const {
	Assert(_value != NULL);
	return GroundTerm(_value);
}

// TODO code duplication with AtomGrounder
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
			if (groundterm._domelement == NULL) {
				if (verbosity() > 2) {
					poptab();
					clog << tabs() << "Result = **invalid term**" << "\n";
				}
				return groundterm;
			} else if (not _tables[n]->contains(groundterm._domelement)) { // Checking out-of-bounds
				if (verbosity() > 2) {
					poptab();
					clog << tabs() << "Term value out of argument type" << "\n";
				}
				return groundterm;
			}
			args[n] = groundterm._domelement;
		}
		groundsubterms.push_back(groundterm);
	}
	if (calculable and (_functable != NULL)) { // All ground subterms are domain elements!
		auto result = (*_functable)[args];
		if (verbosity() > 2) {
			if (result) {
				poptab();
				clog << tabs() << "Result = " << print(result) << "\n";
			} else {
				poptab();
				clog << tabs() << "Result = **invalid term**" << "\n";
			}
		}
		return GroundTerm(result);
	}

	Assert(getOption(BoolType::CPSUPPORT));
	Assert(CPSupport::eligibleForCP(_function, _translator->vocabulary()));
	auto varid = _translator->translateTerm(_function, groundsubterms);
	if (verbosity() > 2) {
		poptab();
		clog << tabs() << "Result = var" << _translator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}

CPTerm* createCPSumTerm(const litlist& conditions, const varidlist& ids, const intweightlist& costs) {
	return new CPSetTerm(AggFunction::SUM, conditions, ids, costs);
}

void SumTermGrounder::computeDomain(const GroundTerm& left, const GroundTerm& right) const {
	auto leftdomain = _lefttermgrounder->getDomain();
	auto rightdomain = _righttermgrounder->getDomain();
	if (getDomain() == NULL or not getDomain()->approxFinite()) {
		if (not left.isVariable) {
			leftdomain = TableUtils::createSortTable();
			leftdomain->add(left._domelement);
		}
		if (not right.isVariable) {
			rightdomain = TableUtils::createSortTable();
			rightdomain->add(right._domelement);
		}
		if (leftdomain and rightdomain and leftdomain->isRange() and rightdomain->isRange() and leftdomain->approxFinite() and rightdomain->approxFinite()) {
			Assert(leftdomain->first()->type() == DomainElementType::DET_INT);
			Assert(rightdomain->first()->type() == DomainElementType::DET_INT);
			int leftmin = leftdomain->first()->value()._int;
			int rightmin = rightdomain->first()->value()._int;
			int leftmax = leftdomain->last()->value()._int;
			int rightmax = rightdomain->last()->value()._int;
			int min = 0;
			int max = 0;
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
			if (max < min) { swap(min, max); }
			setDomain(TableUtils::createSortTable(min, max));
		} else if (leftdomain and rightdomain and leftdomain->approxFinite() and rightdomain->approxFinite()) {
			Assert(leftdomain->first()->type() == DomainElementType::DET_INT);
			Assert(rightdomain->first()->type() == DomainElementType::DET_INT);
			auto newdomain = TableUtils::createSortTable();
			for (auto leftit = leftdomain->sortBegin(); not leftit.isAtEnd(); ++leftit) {
				for (auto rightit = rightdomain->sortBegin(); not rightit.isAtEnd(); ++rightit) {
					int leftvalue = (*leftit)->value()._int;
					int rightvalue = (*rightit)->value()._int;
					int newvalue = 0;
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

	auto leftid = left._varid, rightid = right._varid; // NOTE: only correct if it is indeed a variable, otherwise we overwrite it now:
	if(not left.isVariable){
		if(not right.isVariable){ // Both subterms are domain elements, so lookup the result in the function table.
			Assert(not right.isVariable and (_functable != NULL));
			auto domelem = _functable->operator[]( { left._domelement, right._domelement });
			Assert(domelem);
			if (verbosity() > 2) {
				poptab();
				clog << tabs() << "Result = " << print(domelem) << "\n";
			}
			return GroundTerm(domelem);
		}
		leftid = _translator->translateTerm(left._domelement);

	}
	if(not right.isVariable){
		rightid = _translator->translateTerm(right._domelement);
	}
	// Create addition of both terms
	auto sumterm = createCPSumTerm({ _true, _true}, {leftid, rightid}, { 1, (_type == SumType::ST_MINUS?-1:1) });
	auto varid = _translator->translateTerm(sumterm, getDomain());

	// Return result
	if (verbosity() > 2) {
		poptab();
		clog << tabs() << "Result = " << _translator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}


CPTerm* createCPProdTerm(const litlist& conditions, const VarId& left, const VarId& right) {
	return new CPSetTerm(AggFunction::PROD, conditions, { left, right }, {1});
}

void ProdTermGrounder::computeDomain(const GroundTerm& left, const GroundTerm& right) const {
	auto leftdomain = _lefttermgrounder->getDomain();
	auto rightdomain = _righttermgrounder->getDomain();
	if (getDomain() == NULL or not getDomain()->approxFinite()) {
		if (not left.isVariable) {
			leftdomain = TableUtils::createSortTable();
			leftdomain->add(left._domelement);
		}
		if (not right.isVariable) {
			rightdomain = TableUtils::createSortTable();
			rightdomain->add(right._domelement);
		}
		if (leftdomain and rightdomain and leftdomain->isRange() and rightdomain->isRange() and leftdomain->approxFinite() and rightdomain->approxFinite()) {
			Assert(leftdomain->first()->type() == DomainElementType::DET_INT);
			Assert(rightdomain->first()->type() == DomainElementType::DET_INT);
			int leftmin = leftdomain->first()->value()._int;
			int rightmin = rightdomain->first()->value()._int;
			int leftmax = leftdomain->last()->value()._int;
			int rightmax = rightdomain->last()->value()._int;

			auto allposs = { (leftmin * rightmin), (leftmin * rightmax), (leftmax * rightmin), (leftmax * rightmax) };
			// FIXME incorrect (pointer comparison)
			int min = *(std::min_element(allposs.begin(),allposs.end()));
			int max = *(std::max_element(allposs.begin(),allposs.end()));

			setDomain(TableUtils::createSortTable(min, max));
		} else if (leftdomain and rightdomain and leftdomain->approxFinite() and rightdomain->approxFinite()) {
			Assert(leftdomain->first()->type() == DomainElementType::DET_INT);
			Assert(rightdomain->first()->type() == DomainElementType::DET_INT);
			auto newdomain = TableUtils::createSortTable();
			for (auto leftit = leftdomain->sortBegin(); not leftit.isAtEnd(); ++leftit) {
				for (auto rightit = rightdomain->sortBegin(); not rightit.isAtEnd(); ++rightit) {
					int leftvalue = (*leftit)->value()._int;
					int rightvalue = (*rightit)->value()._int;
					int newvalue = leftvalue * rightvalue;
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

GroundTerm ProdTermGrounder::run() const {
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	// Run subtermgrounders
	auto left = _lefttermgrounder->run();
	auto right = _righttermgrounder->run();

	// Compute domain for the sum term
	computeDomain(left, right);

	auto leftid = left._varid, rightid = right._varid; // NOTE: only correct if it is indeed a variable, otherwise we overwrite it now:
	if (not left.isVariable) {
		if (not right.isVariable) { // Both subterms are domain elements, so lookup the result in the function table.
			Assert(not right.isVariable and (_functable != NULL));
			auto domelem = _functable->operator[]( { left._domelement, right._domelement });
			Assert(domelem);
			if (verbosity() > 2) {
				poptab();
				clog << tabs() << "Result = " << print(domelem) << "\n";
			}
			return GroundTerm(domelem);
		}
		leftid = _translator->translateTerm(left._domelement);

	}
	if (not right.isVariable) {
		rightid = _translator->translateTerm(right._domelement);
	}
	// Create addition of both terms
	auto prodterm = createCPProdTerm({ _true, _true }, leftid, rightid);
	auto varid = _translator->translateTerm(prodterm, getDomain());

	// Return result
	if (verbosity() > 2) {
		poptab();
		clog << tabs() << "Result = " << _translator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}


CPTerm* createCPSumTerm(Lit condition, const DomainElement* factor, const VarId& varid) {
	Assert(factor->type() == DomainElementType::DET_INT);
	return createCPSumTerm({condition}, { varid }, { factor->value()._int });
}

void TermWithFactorGrounder::computeDomain(const DomainElement* factor, const GroundTerm& groundsubterm) const {
	Assert(factor->type() == DomainElementType::DET_INT);
	auto subtermdomain = _subtermgrounder->getDomain();

	if (getDomain() == NULL or not getDomain()->approxFinite()) {
		if (not groundsubterm.isVariable) {
			subtermdomain = TableUtils::createSortTable();
			subtermdomain->add(groundsubterm._domelement);
		}
		if (subtermdomain != NULL and subtermdomain->isRange() and subtermdomain->approxFinite()) {
			Assert(subtermdomain->first()->type() == DomainElementType::DET_INT);
			int min = factor->value()._int * subtermdomain->first()->value()._int;
			int max = factor->value()._int * subtermdomain->last()->value()._int;
			if (max < min) { swap(min, max); }
			setDomain(TableUtils::createSortTable(min, max));
		} else if (subtermdomain != NULL and subtermdomain->approxFinite()) {
			Assert(subtermdomain->first()->type() == DomainElementType::DET_INT);
			auto newdomain = TableUtils::createSortTable();
			for (auto it = subtermdomain->sortBegin(); not it.isAtEnd(); ++it) {
				int value = factor->value()._int * (*it)->value()._int;
				newdomain->add(createDomElem(value));
			}
			setDomain(newdomain);
		} else {
			//TODO
			throw notyetimplemented("Domain of the termgrounder is infinite.");
		}
	}
}

GroundTerm TermWithFactorGrounder::run() const {
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	// Run subtermgrounders
	auto factor = _factortermgrounder->run();
	auto groundterm = _subtermgrounder->run();

	Assert(not factor.isVariable);
	Assert(factor._domelement->type() == DomainElementType::DET_INT);

	// Compute domain for the sum term
	computeDomain(factor._domelement, groundterm);

	VarId varid;
	if (groundterm.isVariable) {
		auto sumterm = createCPSumTerm(_true, factor._domelement, groundterm._varid);
		varid = _translator->translateTerm(sumterm, getDomain());
	} else {
		Assert(not groundterm.isVariable and (_functable != NULL));
		auto domelem = _functable->operator[]( { factor._domelement, groundterm._domelement });
		if (verbosity() > 2) {
			poptab();
			if(domelem==NULL){ // For example after overflow of +
				clog << tabs() << "Result = **invalid_term** \n";
			}else{
				clog << tabs() << "Result = " << print(domelem) << "\n";
			}
		}
		return GroundTerm(domelem);
	}

	// Return result
	if (verbosity() > 2) {
		poptab();
		clog << tabs() << "Result = " << _translator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}

CPTerm* createCPAggTerm(const AggFunction& f, const litlist& conditions, const varidlist& varids) {
	Assert(CPSupport::eligibleForCP(f));
	switch (f) {
	case SUM:
		return createCPSumTerm(conditions, varids, intweightlist(varids.size(),1));
	case PROD:
		return new CPSetTerm(AggFunction::PROD, conditions, varids, {1});
	case MIN:
		return new CPSetTerm(AggFunction::MIN, conditions, varids, {});
	case MAX:
		return new CPSetTerm(AggFunction::MAX, conditions, varids, {});
	default:
		throw IdpException("Invalid code path.");
	}
}

// IMPORTANT: should be the same neutrals as returned by commontypes.hpp
Weight getNeutralElement(AggFunction type){
	switch(type){
	case AggFunction::CARD:
		return 0;
	case AggFunction::SUM:
		return 0;
	case AggFunction::PROD:
		return 1;
	case AggFunction::MIN:
		return getMaxElem<int>();
	case AggFunction::MAX:
		return getMinElem<int>();
	}
	throw IdpException("Invalid code path.");
}

AggTermGrounder::AggTermGrounder(AbstractGroundTheory* grounding, GroundTranslator* gt, AggFunction tp, SortTable* dom, SetGrounder* gr)
		: TermGrounder(dom, gt), _type(tp), _setgrounder(gr), grounding(grounding) {
	Assert(CPSupport::eligibleForCP(tp));
}

GroundTerm AggTermGrounder::run() const {
	// Note: This grounder should only be used when the aggregate can be computed now, or when the aggregate is eligible for CPsupport!
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	auto setnr = _setgrounder->runAndRewriteUnknowns();
	auto tsset = _translator->groundset(setnr);

	auto trueweight = applyAgg(_type, tsset.trueweights());

	GroundTerm result(0);
	if(tsset.cpvars().empty()){
		result = GroundTerm(createDomElem(trueweight));
	}else{
		VarId id;
		if(contains(aggterm2cpterm, std::pair<uint,AggFunction>(setnr.id,_type))){
			id = aggterm2cpterm[std::pair<uint,AggFunction>(setnr.id,_type)];
		}else{
			varidlist varids;
			auto conditions = tsset.literals();

			for(auto term: tsset.cpvars()){
				if(term.isVariable){
					varids.push_back(term._varid);
				}else{
					auto domain = TableUtils::createSortTable();
					if(term._domelement==NULL){
						throw notyetimplemented("Undefined term in cp-expression");
					}
					domain->add(term._domelement);
					varids.push_back(_translator->createNewVarIdNumber(domain));
				}
			}
			if (trueweight!=getNeutralElement(_type)) {
				conditions.push_back(_true);
				varids.push_back(_translator->translateTerm(createDomElem(trueweight)));
			}

			auto aggterm = createCPAggTerm(_type, conditions, varids);
			id = _translator->translateTerm(aggterm, getDomain());
			aggterm2cpterm[std::pair<uint,AggFunction>(setnr.id,_type)]=id;
			if(not contains(aggterm2cpterm, std::pair<uint,AggFunction>(setnr.id,_type))){
				throw IdpException("Invalid code path");
			}
		}
		result = GroundTerm(id);
	}

	if (verbosity() > 2) {
		poptab();
		clog << tabs() << "Result = " << _translator->printTerm(result) << "\n";
	}
	return result;
}
