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
		litlist& literals, weightlist& weights, weightlist& trueweights, varidlist& varids, InstChecker& checker) {
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

	if (groundweight.isVariable) {
		Assert(l == _true);
		varids.push_back(groundweight._varid);
//TODO: When l != _true: introduce new term (constant?) t'. Add two formulas: l => t' = t and -l => t' = 0
//		if (l == _true) {
//			varids.push_back(groundweight._varid);
//		} else {
//			auto grounding = sublitgrounder.getGrounding();
//			auto translator = grounding->translator();
//			auto termtranslator = grounding->termtranslator();

//			auto sort = new Sort("_internal_sort_" + convertToString(getGlobal()->getNewID()),subtermgrounder.getDomain());
//			auto func = new Function(vector<Sort*>{},sort,ParseInfo());
//			//TODO: Add to vocabulary somehow?
//
//			auto varid = termtranslator->translate(func,vector<GroundTerm>{});
//			auto vt1 = new CPVarTerm(varid);
//			auto vt2 = new CPVarTerm(varid);
//			Lit bl1 = translator->translate(vt1,CompType::EQ,CPBound(groundweight._varid),TsType::EQ);
//			Lit bl2 = translator->translate(vt2,CompType::EQ,CPBound(0),TsType::EQ);
//			Lit l1 = translator->translate({-l,bl1},false,TsType::EQ);
//			Lit l2 = translator->translate({l,bl2},false,TsType::EQ);
//			Lit truelit = translator->translate({l1,l2},true,TsType::EQ);
//			grounding->addUnitClause(truelit);
//
//			varids.push_back(varid);
//		}
		return;
	}

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

EnumSetGrounder::~EnumSetGrounder() {
	deleteList(_subgrounders);
}

SetId EnumSetGrounder::run() const {
	litlist literals;
	weightlist weights;
	weightlist trueweights;
	varidlist varids;
	for (auto i = _subgrounders.cbegin(); i < _subgrounders.cend(); ++i) {
		(*i)->run(literals, weights, trueweights, varids);
	}
	auto s = _translator->translateSet(literals, weights, trueweights, varids);
	return s;
}

QuantSetGrounder::~QuantSetGrounder() {
	delete _subgrounder;
	delete _generator;
	delete _checker;
	delete _weightgrounder;
}

void QuantSetGrounder::run(litlist& literals, weightlist& weights, weightlist& trueweights, varidlist& varids) const {
	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator++()) {
		groundSetLiteral(*_subgrounder, *_weightgrounder, literals, weights, trueweights, varids, *_checker);
	}
}

SetId QuantSetGrounder::run() const {
	litlist literals;
	weightlist weights;
	weightlist trueweights;
	varidlist varids;
	run(literals, weights, trueweights, varids);
	auto s = _translator->translateSet(literals, weights, trueweights, varids);
	return s;
}
