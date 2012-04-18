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
#include "TermGrounders.hpp"
#include "FormulaGrounders.hpp"
#include "generators/InstGenerator.hpp"
#include "generators/BasicCheckers.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

using namespace std;

template<class LitGrounder, class TermGrounder>
void groundSetLiteral(const LitGrounder& sublitgrounder, const TermGrounder& subtermgrounder, litlist& literals, weightlist& weights, weightlist& trueweights, varidlist& varids, InstChecker& checker) {
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
		Assert(l == _true); //FIXME: this is not always the case...
		varids.push_back(groundweight._varid);
		return;
	}

	const auto& d = groundweight._domelement;
	Weight w = (d->type() == DET_INT) ? (double) d->value()._int : d->value()._double;

	if (l == _true) {
		trueweights.push_back(w);
	} else {
		weights.push_back(w);
		literals.push_back(l);
	}
}

SetId EnumSetGrounder::run() const {
	litlist literals;
	weightlist weights;
	weightlist trueweights;
	varidlist varids;
	InstChecker* checker = new FalseInstChecker();
	for (size_t n = 0; n < _subgrounders.size(); ++n) {
		groundSetLiteral(*_subgrounders[n], *_subtermgrounders[n], literals, weights, trueweights, varids, *checker);
	}
	SetId s = _translator->translateSet(literals, weights, trueweights, varids);
	return s;
}

SetId QuantSetGrounder::run() const {
	litlist literals;
	weightlist weights;
	weightlist trueweights;
	varidlist varids;
	for (_generator->begin(); not _generator->isAtEnd(); _generator->operator++()) {
		groundSetLiteral(*_subgrounder, *_weightgrounder, literals, weights, trueweights, varids, *_checker);
	}
	SetId s = _translator->translateSet(literals, weights, trueweights, varids);
	return s;
}
