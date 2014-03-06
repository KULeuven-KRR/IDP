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
	if (_term != NULL) {
		_term->recursiveDelete();
		_term = NULL;
	}
	if (not _varmap.empty()) {
		for (auto i = _varmap.begin(); i != _varmap.end(); ++i) {
			//FIXME delete (i->first); // Delete the Variable
			// Don't delete the Domain element!
		}
		_varmap.clear();
	}
}

void TermGrounder::printOrig() const {
	clog << tabs() << "Grounding term " << print(getTerm()) << "\n";
	if (not getTerm()->freeVars().empty()) {
		pushtab();
		clog << tabs() << "with instance ";
		for (auto var : getTerm()->freeVars()) {
			clog << print(var) << " = ";
			auto e = _varmap.find(var)->second->get();
			clog << print(e) << ' ';
		}
		clog << "\n";
		poptab();
	}
}
int TermGrounder::verbosity() const {
	return getOption(IntType::VERBOSE_GROUNDING);
}

GroundTerm DomTermGrounder::run() const {
	Assert(_value != NULL);
	return GroundTerm(_value);
}

FuncTermGrounder::FuncTermGrounder(GroundTranslator* tt, Function* func, FuncTable* ftable, SortTable* dom, const std::vector<SortTable*>& tables,
		const std::vector<TermGrounder*>& sub)
		: TermGrounder(dom, tt), _function(func), _functable(ftable), _tables(tables), _subtermgrounders(sub) {
	std::vector<Term*> subterms;
	for(auto st:sub){
		addAll(_varmap, st->getVarmapping());
		subterms.push_back(st->getTerm()->cloneKeepVars());
	}
	setTerm(new FuncTerm(func, subterms, {}));
}

TwinTermGrounder::TwinTermGrounder(GroundTranslator* tt, Function* func, TwinTT type, FuncTable* ftable, SortTable* dom, TermGrounder* ltg, TermGrounder* rtg)
		: TermGrounder(dom, tt), _type(type), _functable(ftable), _lefttermgrounder(ltg), _righttermgrounder(rtg), _latestdomain(NULL) {
	addAll(_varmap, _lefttermgrounder->getVarmapping());
	addAll(_varmap, _righttermgrounder->getVarmapping());
	setTerm(new FuncTerm(func, {_lefttermgrounder->getTerm()->cloneKeepVars(), _righttermgrounder->getTerm()->cloneKeepVars()}, {}));
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
		CHECKTERMINATION;
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
				groundterm._domelement =NULL;
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
	if (not CPSupport::eligibleForCP(_function, _translator->vocabulary())) {
		throw IdpException("Invalid code path");
	}

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

SortTable* TwinTermGrounder::computeDomain(const GroundTerm& left, const GroundTerm& right) const {
	if ((not left.isVariable && left._domelement==NULL) || (not right.isVariable && right._domelement==NULL) ){
		throw IdpException("Invalid code path");
	}

	auto leftdomain = _lefttermgrounder->getLatestDomain();
	auto rightdomain = _righttermgrounder->getLatestDomain();

	if (getDomain() != NULL && getDomain()->approxFinite()) { // TODO In fact should be: if the basic interpretation is small enough
									// TODO why finite?
		_latestdomain = getDomain();
		return getDomain();
	}

	bool cansave = true;
	if (not left.isVariable) {
		leftdomain = TableUtils::createSortTable();
		leftdomain->add(left._domelement);
		cansave = false;
	}
	if (not right.isVariable) {
		rightdomain = TableUtils::createSortTable();
		rightdomain->add(right._domelement);
		cansave = false;
	}

	SortTable* newdomain = NULL;
	auto exists = leftdomain and rightdomain;
	auto finite = exists and leftdomain->approxFinite() and rightdomain->approxFinite();
	if (exists and leftdomain->isRange() and rightdomain->isRange() and finite) {
		// FIXME overflow possible
		Assert(leftdomain->first()->type() == DomainElementType::DET_INT);
		Assert(rightdomain->first()->type() == DomainElementType::DET_INT);
		int leftmin = leftdomain->first()->value()._int;
		int rightmin = rightdomain->first()->value()._int;
		int leftmax = leftdomain->last()->value()._int;
		int rightmax = rightdomain->last()->value()._int;
		int min = 0;
		int max = 0;
		switch(_type){
		case TwinTT::PLUS:
			min = leftmin + rightmin;
			max = leftmax + rightmax;
			break;
		case TwinTT::MIN:
			min = leftmin - rightmax;
			max = leftmax - rightmin;
			break;
		case TwinTT::PROD:
			auto allposs = { (leftmin * rightmin), (leftmin * rightmax), (leftmax * rightmin), (leftmax * rightmax) };
			min = *(std::min_element(allposs.begin(),allposs.end()));
			max = *(std::max_element(allposs.begin(),allposs.end()));
			break;
		}
		if (max < min) { swap(min, max); }
		newdomain = TableUtils::createSortTable(min, max);
	} else if (exists and finite) {
		Assert(leftdomain->first()->type() == DomainElementType::DET_INT);
		Assert(rightdomain->first()->type() == DomainElementType::DET_INT);
		newdomain = TableUtils::createSortTable();
		for (auto leftit = leftdomain->sortBegin(); not leftit.isAtEnd(); ++leftit) {
			for (auto rightit = rightdomain->sortBegin(); not rightit.isAtEnd(); ++rightit) {
				int leftvalue = (*leftit)->value()._int;
				int rightvalue = (*rightit)->value()._int;
				int newvalue = 0;
				switch(_type){
				case TwinTT::PLUS:
					newvalue = leftvalue + rightvalue;
					break;
				case TwinTT::MIN:
					newvalue = leftvalue - rightvalue;
					break;
				case TwinTT::PROD:
					newvalue = leftvalue * rightvalue;
					break;
				}
				newdomain->add(createDomElem(newvalue));
			}
		}
	} else if(exists and leftdomain->isRange() and rightdomain->isRange()) {
		Warning::warning("Approximating int as all integers in -2^32..2^32, as the solver does not support true infinity at the moment. Models might be lost.");
		Assert(leftdomain->first()->type() == DomainElementType::DET_INT);
		Assert(rightdomain->first()->type() == DomainElementType::DET_INT);
		Assert(leftdomain->last()->type() == DomainElementType::DET_INT);
		Assert(rightdomain->last()->type() == DomainElementType::DET_INT);
		newdomain = TableUtils::createSortTable(min(leftdomain->first(), rightdomain->first())->value()._int, max(leftdomain->last(), rightdomain->last())->value()._int);
	} else{
		throw notyetimplemented("One of the domains in a twintermgrounder is infinite.");
	}

	if(cansave){
		setDomain(newdomain);
	}
	_latestdomain = newdomain;
	return newdomain;
}

CPTerm* createCPProdTerm(const litlist& conditions, const VarId& left, const VarId& right) {
	return new CPSetTerm(AggFunction::PROD, conditions, { left, right }, {1});
}

CPTerm* createCPSumTerm(Lit condition, const DomainElement* factor, const VarId& varid) {
	Assert(factor->type() == DomainElementType::DET_INT);
	return createCPSumTerm({condition}, { varid }, { factor->value()._int });
}

GroundTerm TwinTermGrounder::run() const {
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	// Run subtermgrounders
	auto left = _lefttermgrounder->run();
	auto right = _righttermgrounder->run();

	if((not left.isVariable && left._domelement==NULL) || (not right.isVariable && right._domelement==NULL)){
		return GroundTerm(NULL);
	}

	// Compute domain for the sum term
	auto domain = computeDomain(left, right);

	GroundTerm result(NULL);
	bool done = false;
	auto leftid = left._varid, rightid = right._varid;
	if (not left.isVariable) {
		if (not right.isVariable) { // Both subterms are domain elements, so lookup the result in the function table.
			Assert(not right.isVariable and (_functable != NULL));
			if(left._domelement!=NULL && right._domelement!=NULL){
				auto domelem = _functable->operator[]( { left._domelement, right._domelement });
				result = GroundTerm(domelem);
			}
			done = true;
		}else{
			leftid = _translator->translateTerm(left._domelement);
		}
	}
	if (not right.isVariable) {
		rightid = _translator->translateTerm(right._domelement);
	}
	if(not done){
		CPTerm* computedterm = NULL;
		switch(_type){
		case TwinTT::PLUS:
			computedterm = createCPSumTerm({ _true, _true}, {leftid, rightid}, { 1, 1 });
			break;
		case TwinTT::MIN:
			computedterm = createCPSumTerm({ _true, _true}, {leftid, rightid}, { 1, -1 });
			break;
		case TwinTT::PROD:
			computedterm = createCPProdTerm({ _true, _true }, leftid, rightid);
			break;
		}
		auto varid = _translator->translateTerm(computedterm, domain);
		result = GroundTerm(varid);
	}

	if (verbosity() > 2) {
		poptab();
		clog << tabs() << "Result = " << _translator->printTerm(result) << "\n";
	}
	return result;
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
		return getMaxElem<int>(); // TODO should become infinity
	case AggFunction::MAX:
		return getMinElem<int>(); // TODO should become infinity
	}
	throw IdpException("Invalid code path.");
}
AggTermGrounder::AggTermGrounder(GroundTranslator* gt, AggFunction tp, SortTable* dom, EnumSetGrounder* gr)
		: 	TermGrounder(dom, gt),
			_type(tp),
			_setgrounder(gr) {
	Assert(CPSupport::eligibleForCP(tp));
	addAll(_varmap, gr->getVarmapping());
	setTerm(new AggTerm(gr->getSet()->cloneKeepVars(), tp, { }));
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
	if (tsset.cpvars().empty()) {
		result = GroundTerm(createDomElem(trueweight));
	} else {
		VarId id;
		if (contains(aggterm2cpterm, std::pair<uint, AggFunction>(setnr.id, _type))) {
			id = aggterm2cpterm[std::pair<uint, AggFunction>(setnr.id, _type)];
		} else {
			varidlist varids;
			auto conditions = tsset.literals();

			for (auto term : tsset.cpvars()) {
				if (term.isVariable) {
					varids.push_back(term._varid);
				} else {
					if (term._domelement == NULL) {
						throw notyetimplemented("Undefined term in cp-expression");
					}
					varids.push_back(_translator->translateTerm(term._domelement));
				}
			}

			auto neutral = getNeutralElement(_type);
			auto dom = getDomain();
			bool emptysetpossible = true;
			for(auto c: conditions){
				if(c==_true){
					emptysetpossible = false;
				}
			}
			if (trueweight != neutral) {
				conditions.push_back(_true);
				varids.push_back(_translator->translateTerm(createDomElem(trueweight)));
			}else if(emptysetpossible) {
				//#warning Hack because DeriveTermBounds (so ALSO in other parts of the system) does not derive complete bounds for MIN and MAX aggregates (it ignores infinity), we add it here later on
						// Try not to do this if not necessary, it messes with cp as domains are no longer ranges
				dom = getDomain()->clone();
				auto neutraldom = createDomElem(neutral);
				if(not dom->contains(neutraldom)){
					dom->add(neutraldom);
					conditions.push_back(_true);
					varids.push_back(_translator->translateTerm(neutraldom));
				}
			}

			auto aggterm = createCPAggTerm(_type, conditions, varids);
			id = _translator->translateTerm(aggterm, dom);
			aggterm2cpterm[std::pair<uint, AggFunction>(setnr.id, _type)] = id;
			if (not contains(aggterm2cpterm, std::pair<uint, AggFunction>(setnr.id, _type))) {
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
