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
#include "inferences/grounding/GroundTermTranslator.hpp"
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
	auto w = (d->type() == DET_INT) ? (double) d->value()._int : d->value()._double;

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
	auto grounding = sublitgrounder.getGrounding();
	auto translator = grounding->translator();
	auto termtranslator = grounding->termtranslator();

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
			auto w = (d->type() == DET_INT) ? (double) d->value()._int : d->value()._double;
			trueweights.push_back(w);
		}
		return;
	}

	Assert(l != _false and l != _true);
	// Introduce new constant t'. Add two formulas: l => t' = t and -l => t' = 0

	VarId v;
	if (groundweight.isVariable) {
		v = groundweight._varid;
	} else {
		v = termtranslator->translate(groundweight._domelement);
	}

	SortTable* domain = NULL;
	auto vardom = termtranslator->domain(v);
	Assert(vardom->approxFinite());
	Assert(vardom->first()->type() == DomainElementType::DET_INT);
	if (vardom->isRange()) {
		int min = vardom->first()->value()._int;
		int max = vardom->last()->value()._int;
		domain = TableUtils::createSortTable(min, max);
	} else {
		domain = TableUtils::createSortTable();
		for (auto it = vardom->sortBegin(); not it.isAtEnd(); ++it) {
			domain->add(*it);
		}
	}
	domain->add(createDomElem(0));

	auto sort = new Sort("_internal_sort_" + convertToString(getGlobal()->getNewID()), domain);
	auto constant = new Function(vector<Sort*>{}, sort, ParseInfo());

	auto varid = termtranslator->translate(constant, vector<GroundTerm>{});
	auto vt1 = new CPVarTerm(varid);
	auto vt2 = new CPVarTerm(varid);
	Lit bl1 = translator->translate(vt1, CompType::EQ, CPBound(v), TsType::EQ);
	Lit bl2 = translator->translate(vt2, CompType::EQ, CPBound(0), TsType::EQ);
	Lit l1 = translator->translate( { -l, bl1 }, false, TsType::IMPL);
	Lit l2 = translator->translate( { l, bl2 }, false, TsType::IMPL);
	Lit truelit = translator->translate( { l1, l2 }, true, TsType::EQ);
	grounding->addUnitClause(truelit);

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
	auto s = _translator->translateSet({}, {}, trueweights, varids);
	return s;
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
	auto s = _translator->translateSet(literals, weights, trueweights, {});
	return s;
}

SetId QuantSetGrounder::runAndRewriteUnknowns() const {
	weightlist trueweights;
	varidlist varids;
	run(trueweights, varids);
	auto s = _translator->translateSet({}, {}, trueweights, varids);
	return s;
}
