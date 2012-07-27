/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "SetGrounders.hpp"

#include "IncludeComponents.hpp"
#include "common.hpp"
#include "TermGrounders.hpp"
#include "FormulaGrounders.hpp"
#include "generators/InstGenerator.hpp"
#include "generators/BasicCheckersAndGenerators.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "utils/ListUtils.hpp"

using namespace std;

template<class LitGrounder, class TermGrounder>
void groundSetLiteral(const LitGrounder& sublitgrounder, const TermGrounder& subtermgrounder,
		litlist& literals, weightlist& weights, weightlist& trueweights, InstChecker& checker) {
	Lit l;
	if (checker.check()) {
		l = _true;
	} else {
		l = sublitgrounder.groundAndReturnLit();
	}
	if (l == _false) {
		return;
	}

	const auto& groundweight = subtermgrounder.run();
	Assert(not groundweight.isVariable);
	const auto& d = groundweight._domelement;
	Assert(d != NULL);
	auto w = (d->type() == DET_INT) ? ((double) d->value()._int) : (d->value()._double);

	if (l == _true) {
		trueweights.push_back(w);
	} else {
		weights.push_back(w);
		literals.push_back(l);
	}
}

template<class LitGrounder, class TermGrounder>
void groundSetLiteral(const LitGrounder& sublitgrounder, const TermGrounder& subtermgrounder,
		weightlist& trueweights, varidlist& varids, InstChecker& checker) {
	Lit l;
	if (checker.check()) {
		l = _true;
	} else {
		l = sublitgrounder.groundAndReturnLit();
	}
	if (l == _false) {
		return;
	}

	const auto& groundweight = subtermgrounder.run();

	if (l == _true) {
		if (groundweight.isVariable) {
			varids.push_back(groundweight._varid);
		} else {
			const auto& d = groundweight._domelement;
			Assert(d != NULL);
			auto w = (d->type() == DET_INT) ? ((double) d->value()._int) : (d->value()._double);
			trueweights.push_back(w);
		}
		return;
	}

	Assert(l != _false and l != _true);
	// Rewrite (l,t) :
	//  - Introduce new constant t'.
	//  - Add two formulas: l => t' = t and -l => t' = 0
	//  - return (true,t')

	auto grounding = sublitgrounder.getGrounding();
	auto translator = grounding->translator();

	// Get CP variable for the groundterm t
	VarId v;
	if (groundweight.isVariable) {
		v = groundweight._varid;
	} else {
		v = translator->translateTerm(groundweight._domelement);
	}

	// Compute domain for term t' = dom(t) U {0}
	SortTable* domain = NULL;
	auto vardom = translator->domain(v);
	Assert(vardom->approxFinite());
	if (vardom->isRange()) {
		Assert(vardom->first()->type() == DomainElementType::DET_INT);
		Assert(vardom->last()->type() == DomainElementType::DET_INT);
		int min = vardom->first()->value()._int;
		int max = vardom->last()->value()._int;
		domain = TableUtils::createSortTable(min, max);
	} else {
		domain = TableUtils::createSortTable();
		for (auto it = vardom->sortBegin(); not it.isAtEnd(); ++it) {
			Assert((*it)->type() == DomainElementType::DET_INT);
			domain->add(*it);
		}
	}
	domain->add(createDomElem(0));

	auto sort = new Sort("_internal_sort_" + convertToString(getGlobal()->getNewID()), domain);
	sort->addParent(get(STDSORT::INTSORT));
	translator->vocabulary()->add(sort);

	// Term t' is a new constant with the computed domain
	auto constant = new Function(vector<Sort*>{}, sort, ParseInfo());
	translator->vocabulary()->add(constant);

	auto varid = translator->translateTerm(constant, vector<GroundTerm>{});

	// Add formulas to the grounding
	auto bl1 = translator->translate(new CPVarTerm(varid), CompType::EQ, CPBound(v), TsType::EQ);
	auto bl2 = translator->translate(new CPVarTerm(varid), CompType::EQ, CPBound(0), TsType::EQ);
	grounding->add({-l, bl1});
	grounding->add({l, bl2});

	varids.push_back(varid);
}

EnumSetGrounder::~EnumSetGrounder() {
	deleteList(_subgrounders);
}

SetId EnumSetGrounder::run() const {
	litlist literals;
	weightlist weights;
	weightlist trueweights;
	for (auto i = _subgrounders.cbegin(); i < _subgrounders.cend(); ++i) {
		(*i)->run(literals, weights, trueweights);
	}
	auto s = _translator->translateSet(literals, weights, trueweights, {});
	return s;
}

SetId EnumSetGrounder::runAndRewriteUnknowns() const {
	weightlist trueweights;
	varidlist varids;
	for (auto i = _subgrounders.cbegin(); i < _subgrounders.cend(); ++i) {
		(*i)->run(trueweights, varids);
	}
	return _translator->translateSet({}, {}, trueweights, varids);
}

QuantSetGrounder::~QuantSetGrounder() {
	delete _subgrounder;
	delete _generator;
	delete _checker;
	delete _weightgrounder;
}

void QuantSetGrounder::run(litlist& literals, weightlist& weights, weightlist& trueweights) const {
	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator++()) {
		groundSetLiteral(*_subgrounder, *_weightgrounder, literals, weights, trueweights, *_checker);
	}
}

void QuantSetGrounder::run(weightlist& trueweights, varidlist& varids) const {
	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator++()) {
		groundSetLiteral(*_subgrounder, *_weightgrounder, trueweights, varids, *_checker);
	}
}

SetId QuantSetGrounder::run() const {
	litlist literals;
	weightlist weights;
	weightlist trueweights;
	run(literals, weights, trueweights);
	return _translator->translateSet(literals, weights, trueweights, {});
}

SetId QuantSetGrounder::runAndRewriteUnknowns() const {
	weightlist trueweights;
	varidlist varids;
	run(trueweights, varids);
	return _translator->translateSet({}, {}, trueweights, varids);
}
