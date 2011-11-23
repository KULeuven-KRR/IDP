/*
 * Copyright 2007-2011 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat and Maarten Mariën, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <cmath>

#include "gtest/gtest.h"
#include "rungidl.hpp"
#include "generators/ComparisonGenerator.hpp"
#include "generators/SortInstGenerator.hpp"
#include "generators/InverseInstGenerator.hpp"
#include "generators/LookupGenerator.hpp"
#include "structure.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "term.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include <iostream>
#include "IdpException.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"

using namespace std;

template<typename T>
const DomainElement* domelem(T t){
	return createDomElem(t);
}

namespace Tests{
	TEST(PropagationTest, PredFormBound){
		auto sorttable = new SortTable(new IntRangeInternalSortTable(-2, 2));
		auto sort = new Sort("x", sorttable);
		auto variable = new Variable(sort);
		auto x = new VarTerm(variable, TermParseInfo());
		auto p = new Predicate("p", {sort}, false);
		auto q = new Predicate("q", {sort}, false);
		auto notpx = new PredForm(SIGN::NEG, p, {x}, FormulaParseInfo());
		auto qx = new PredForm(SIGN::POS, p, {x}, FormulaParseInfo());
		auto pximplqx = new BoolForm(SIGN::POS, true, notpx, qx, FormulaParseInfo());
		auto forallpximplqx = new QuantForm(SIGN::POS, QUANT::UNIV, set<Variable*>{x}, pximplqx, FormulaParseInfo());
		auto vocabulary = new Vocabulary("V");
		auto theory = new Theory("T", vocabulary, ParseInfo());
		// TODO
		//auto propagator = createPropagator(theory, InitBoundType::IBT_TWOVAL, {});
	}
}
