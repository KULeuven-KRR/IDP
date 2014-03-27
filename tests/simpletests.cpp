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

#include <cmath>

#include "gtest/gtest.h"
#include "external/runidp.hpp"
#include "commontypes.hpp"
#include "inferences/grounding/GroundUtils.hpp"
#include "structure/TableSize.hpp"
#include "vocabulary/vocabulary.hpp"
#include "structure/Structure.hpp"
#include "theory/theory.hpp"
#include "theory/TheoryUtils.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "testingtools.hpp"

#include <dirent.h>
#include <exception>

using namespace std;
using namespace Tests;

TEST(SimpleTest, TestTableSizeEquality) {
	DataManager m;

	auto exact_ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto exact_ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto exact_max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<long>()); // long unsigned int max value
	auto approx_ts0 = tablesize(TableSizeType::TST_APPROXIMATED, 0);
	auto approx_ts1 = tablesize(TableSizeType::TST_APPROXIMATED, 1);
	auto approx_max_ts = tablesize(TableSizeType::TST_APPROXIMATED, getMaxElem<long>()); // long unsigned int max value
	auto infinite_ts_1 = tablesize(TableSizeType::TST_INFINITE,0);

	ASSERT_TRUE(exact_ts0 == exact_ts0);
	ASSERT_FALSE(exact_ts0 == exact_ts1);
	ASSERT_FALSE(exact_ts0 == exact_max_ts);
	ASSERT_TRUE(exact_ts0 == approx_ts0);
	ASSERT_FALSE(exact_ts0 == approx_ts1);
	ASSERT_FALSE(exact_ts0 == approx_max_ts);
	ASSERT_FALSE(exact_ts0 == infinite_ts_1);

	ASSERT_FALSE(exact_ts1 == exact_ts0);
	ASSERT_TRUE(exact_ts1 == exact_ts1);
	ASSERT_FALSE(exact_ts1 == exact_max_ts);
	ASSERT_FALSE(exact_ts1 == approx_ts0);
	ASSERT_TRUE(exact_ts1 == approx_ts1);
	ASSERT_FALSE(exact_ts1 == approx_max_ts);
	ASSERT_FALSE(exact_ts1 == infinite_ts_1);

	ASSERT_FALSE(exact_max_ts == exact_ts0);
	ASSERT_FALSE(exact_max_ts == exact_ts1);
	ASSERT_TRUE(exact_max_ts == exact_max_ts);
	ASSERT_FALSE(exact_max_ts == approx_ts0);
	ASSERT_FALSE(exact_max_ts == approx_ts1);
	ASSERT_TRUE(exact_max_ts == approx_max_ts);
	ASSERT_FALSE(exact_max_ts == infinite_ts_1);

	ASSERT_TRUE(approx_ts0 == exact_ts0);
	ASSERT_FALSE(approx_ts0 == exact_ts1);
	ASSERT_FALSE(approx_ts0 == exact_max_ts);
	ASSERT_TRUE(approx_ts0 == approx_ts0);
	ASSERT_FALSE(approx_ts0 == approx_ts1);
	ASSERT_FALSE(approx_ts0 == approx_max_ts);
	ASSERT_FALSE(approx_ts0 == infinite_ts_1);

	ASSERT_FALSE(approx_ts1 == exact_ts0);
	ASSERT_TRUE(approx_ts1 == exact_ts1);
	ASSERT_FALSE(approx_ts1 == exact_max_ts);
	ASSERT_FALSE(approx_ts1 == approx_ts0);
	ASSERT_TRUE(approx_ts1 == approx_ts1);
	ASSERT_FALSE(approx_ts1 == approx_max_ts);
	ASSERT_FALSE(approx_ts1 == infinite_ts_1);

	ASSERT_FALSE(approx_max_ts == exact_ts0);
	ASSERT_FALSE(approx_max_ts == exact_ts1);
	ASSERT_TRUE(approx_max_ts == exact_max_ts);
	ASSERT_FALSE(approx_max_ts == approx_ts0);
	ASSERT_FALSE(approx_max_ts == approx_ts1);
	ASSERT_TRUE(approx_max_ts == approx_max_ts);
	ASSERT_FALSE(approx_max_ts == infinite_ts_1);

	ASSERT_FALSE(infinite_ts_1 == exact_ts0);
	ASSERT_FALSE(infinite_ts_1 == exact_ts1);
	ASSERT_FALSE(infinite_ts_1 == exact_max_ts);
	ASSERT_FALSE(infinite_ts_1 == approx_ts0);
	ASSERT_FALSE(infinite_ts_1 == approx_ts1);
	ASSERT_FALSE(infinite_ts_1 == approx_max_ts);
	ASSERT_TRUE(infinite_ts_1 == infinite_ts_1);
}

TEST(SimpleTest, TestTableSizeSmallerThan) {
	DataManager m;

	auto exact_ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto exact_ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto exact_max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<long>()); // long unsigned int max value
	auto approx_ts0 = tablesize(TableSizeType::TST_APPROXIMATED, 0);
	auto approx_ts1 = tablesize(TableSizeType::TST_APPROXIMATED, 1);
	auto approx_max_ts = tablesize(TableSizeType::TST_APPROXIMATED, getMaxElem<long>()); // long unsigned int max value
	auto infinite_ts_1 = tablesize(TableSizeType::TST_INFINITE,0);

	ASSERT_FALSE(exact_ts0 < exact_ts0);
	ASSERT_TRUE(exact_ts0 < exact_ts1);
	ASSERT_TRUE(exact_ts0 < exact_max_ts);
	ASSERT_FALSE(exact_ts0 < approx_ts0);
	ASSERT_TRUE(exact_ts0 < approx_ts1);
	ASSERT_TRUE(exact_ts0 < approx_max_ts);
	ASSERT_TRUE(exact_ts0 < infinite_ts_1);

	ASSERT_FALSE(exact_ts1 < exact_ts0);
	ASSERT_FALSE(exact_ts1 < exact_ts1);
	ASSERT_TRUE(exact_ts1 < exact_max_ts);
	ASSERT_FALSE(exact_ts1 < approx_ts0);
	ASSERT_FALSE(exact_ts1 < approx_ts1);
	ASSERT_TRUE(exact_ts1 < approx_max_ts);
	ASSERT_TRUE(exact_ts1 < infinite_ts_1);

	ASSERT_FALSE(exact_max_ts < exact_ts0);
	ASSERT_FALSE(exact_max_ts < exact_ts1);
	ASSERT_FALSE(exact_max_ts < exact_max_ts);
	ASSERT_FALSE(exact_max_ts < approx_ts0);
	ASSERT_FALSE(exact_max_ts < approx_ts1);
	ASSERT_FALSE(exact_max_ts < approx_max_ts);
	ASSERT_TRUE(exact_max_ts < infinite_ts_1);

	ASSERT_FALSE(approx_ts0 < exact_ts0);
	ASSERT_TRUE(approx_ts0 < exact_ts1);
	ASSERT_TRUE(approx_ts0 < exact_max_ts);
	ASSERT_FALSE(approx_ts0 < approx_ts0);
	ASSERT_TRUE(approx_ts0 < approx_ts1);
	ASSERT_TRUE(approx_ts0 < approx_max_ts);
	ASSERT_TRUE(approx_ts0 < infinite_ts_1);

	ASSERT_FALSE(approx_ts1 < exact_ts0);
	ASSERT_FALSE(approx_ts1 < exact_ts1);
	ASSERT_TRUE(approx_ts1 < exact_max_ts);
	ASSERT_FALSE(approx_ts1 < approx_ts0);
	ASSERT_FALSE(approx_ts1 < approx_ts1);
	ASSERT_TRUE(approx_ts1 < approx_max_ts);
	ASSERT_TRUE(approx_ts1 < infinite_ts_1);

	ASSERT_FALSE(approx_max_ts < exact_ts0);
	ASSERT_FALSE(approx_max_ts < exact_ts1);
	ASSERT_FALSE(approx_max_ts < exact_max_ts);
	ASSERT_FALSE(approx_max_ts < approx_ts0);
	ASSERT_FALSE(approx_max_ts < approx_ts1);
	ASSERT_FALSE(approx_max_ts < approx_max_ts);
	ASSERT_TRUE(approx_max_ts < infinite_ts_1);

	ASSERT_FALSE(infinite_ts_1 < exact_ts0);
	ASSERT_FALSE(infinite_ts_1 < exact_ts1);
	ASSERT_FALSE(infinite_ts_1 < exact_max_ts);
	ASSERT_FALSE(infinite_ts_1 < approx_ts0);
	ASSERT_FALSE(infinite_ts_1 < approx_ts1);
	ASSERT_FALSE(infinite_ts_1 < approx_max_ts);
	ASSERT_FALSE(infinite_ts_1 < infinite_ts_1);
}

TEST(SimpleTest, TestTableSizeGreaterThan) {
	DataManager m;

	auto exact_ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto exact_ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto exact_max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<long>()); // long unsigned int max value
	auto approx_ts0 = tablesize(TableSizeType::TST_APPROXIMATED, 0);
	auto approx_ts1 = tablesize(TableSizeType::TST_APPROXIMATED, 1);
	auto approx_max_ts = tablesize(TableSizeType::TST_APPROXIMATED, getMaxElem<long>()); // long unsigned int max value
	auto infinite_ts_1 = tablesize(TableSizeType::TST_INFINITE,0);

	ASSERT_FALSE(exact_ts0 > exact_ts0);
	ASSERT_FALSE(exact_ts0 > exact_ts1);
	ASSERT_FALSE(exact_ts0 > exact_max_ts);
	ASSERT_FALSE(exact_ts0 > approx_ts0);
	ASSERT_FALSE(exact_ts0 > approx_ts1);
	ASSERT_FALSE(exact_ts0 > approx_max_ts);
	ASSERT_FALSE(exact_ts0 > infinite_ts_1);

	ASSERT_TRUE(exact_ts1 > exact_ts0);
	ASSERT_FALSE(exact_ts1 > exact_ts1);
	ASSERT_FALSE(exact_ts1 > exact_max_ts);
	ASSERT_TRUE(exact_ts1 > approx_ts0);
	ASSERT_FALSE(exact_ts1 > approx_ts1);
	ASSERT_FALSE(exact_ts1 > approx_max_ts);
	ASSERT_FALSE(exact_ts1 > infinite_ts_1);

	ASSERT_TRUE(exact_max_ts > exact_ts0);
	ASSERT_TRUE(exact_max_ts > exact_ts1);
	ASSERT_FALSE(exact_max_ts > exact_max_ts);
	ASSERT_TRUE(exact_max_ts > approx_ts0);
	ASSERT_TRUE(exact_max_ts > approx_ts1);
	ASSERT_FALSE(exact_max_ts > approx_max_ts);
	ASSERT_FALSE(exact_max_ts > infinite_ts_1);

	ASSERT_FALSE(approx_ts0 > exact_ts0);
	ASSERT_FALSE(approx_ts0 > exact_ts1);
	ASSERT_FALSE(approx_ts0 > exact_max_ts);
	ASSERT_FALSE(approx_ts0 > approx_ts0);
	ASSERT_FALSE(approx_ts0 > approx_ts1);
	ASSERT_FALSE(approx_ts0 > approx_max_ts);
	ASSERT_FALSE(approx_ts0 > infinite_ts_1);

	ASSERT_TRUE(approx_ts1 > exact_ts0);
	ASSERT_FALSE(approx_ts1 > exact_ts1);
	ASSERT_FALSE(approx_ts1 > exact_max_ts);
	ASSERT_TRUE(approx_ts1 > approx_ts0);
	ASSERT_FALSE(approx_ts1 > approx_ts1);
	ASSERT_FALSE(approx_ts1 > approx_max_ts);
	ASSERT_FALSE(approx_ts1 > infinite_ts_1);

	ASSERT_TRUE(approx_max_ts > exact_ts0);
	ASSERT_TRUE(approx_max_ts > exact_ts1);
	ASSERT_FALSE(approx_max_ts > exact_max_ts);
	ASSERT_TRUE(approx_max_ts > approx_ts0);
	ASSERT_TRUE(approx_max_ts > approx_ts1);
	ASSERT_FALSE(approx_max_ts > approx_max_ts);
	ASSERT_FALSE(approx_max_ts > infinite_ts_1);

	ASSERT_TRUE(infinite_ts_1 > exact_ts0);
	ASSERT_TRUE(infinite_ts_1 > exact_ts1);
	ASSERT_TRUE(infinite_ts_1 > exact_max_ts);
	ASSERT_TRUE(infinite_ts_1 > approx_ts0);
	ASSERT_TRUE(infinite_ts_1 > approx_ts1);
	ASSERT_TRUE(infinite_ts_1 > approx_max_ts);
	ASSERT_FALSE(infinite_ts_1 > infinite_ts_1);
}

TEST(SimpleTest, TestTableSizeSmallerThanOrEqual) {
	DataManager m;

	auto exact_ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto exact_ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto exact_max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<long>()); // long unsigned int max value
	auto approx_ts0 = tablesize(TableSizeType::TST_APPROXIMATED, 0);
	auto approx_ts1 = tablesize(TableSizeType::TST_APPROXIMATED, 1);
	auto approx_max_ts = tablesize(TableSizeType::TST_APPROXIMATED, getMaxElem<long>()); // long unsigned int max value
	auto infinite_ts_1 = tablesize(TableSizeType::TST_INFINITE,0);

	ASSERT_TRUE(exact_ts0 <= exact_ts0);
	ASSERT_TRUE(exact_ts0 <= exact_ts1);
	ASSERT_TRUE(exact_ts0 <= exact_max_ts);
	ASSERT_TRUE(exact_ts0 <= approx_ts0);
	ASSERT_TRUE(exact_ts0 <= approx_ts1);
	ASSERT_TRUE(exact_ts0 <= approx_max_ts);
	ASSERT_TRUE(exact_ts0 <= infinite_ts_1);

	ASSERT_FALSE(exact_ts1 <= exact_ts0);
	ASSERT_TRUE(exact_ts1 <= exact_ts1);
	ASSERT_TRUE(exact_ts1 <= exact_max_ts);
	ASSERT_FALSE(exact_ts1 <= approx_ts0);
	ASSERT_TRUE(exact_ts1 <= approx_ts1);
	ASSERT_TRUE(exact_ts1 <= approx_max_ts);
	ASSERT_TRUE(exact_ts1 <= infinite_ts_1);

	ASSERT_FALSE(exact_max_ts <= exact_ts0);
	ASSERT_FALSE(exact_max_ts <= exact_ts1);
	ASSERT_TRUE(exact_max_ts <= exact_max_ts);
	ASSERT_FALSE(exact_max_ts <= approx_ts0);
	ASSERT_FALSE(exact_max_ts <= approx_ts1);
	ASSERT_TRUE(exact_max_ts <= approx_max_ts);
	ASSERT_TRUE(exact_max_ts <= infinite_ts_1);

	ASSERT_TRUE(approx_ts0 <= exact_ts0);
	ASSERT_TRUE(approx_ts0 <= exact_ts1);
	ASSERT_TRUE(approx_ts0 <= exact_max_ts);
	ASSERT_TRUE(approx_ts0 <= approx_ts0);
	ASSERT_TRUE(approx_ts0 <= approx_ts1);
	ASSERT_TRUE(approx_ts0 <= approx_max_ts);
	ASSERT_TRUE(approx_ts0 <= infinite_ts_1);

	ASSERT_FALSE(approx_ts1 <= exact_ts0);
	ASSERT_TRUE(approx_ts1 <= exact_ts1);
	ASSERT_TRUE(approx_ts1 <= exact_max_ts);
	ASSERT_FALSE(approx_ts1 <= approx_ts0);
	ASSERT_TRUE(approx_ts1 <= approx_ts1);
	ASSERT_TRUE(approx_ts1 <= approx_max_ts);
	ASSERT_TRUE(approx_ts1 <= infinite_ts_1);

	ASSERT_FALSE(approx_max_ts <= exact_ts0);
	ASSERT_FALSE(approx_max_ts <= exact_ts1);
	ASSERT_TRUE(approx_max_ts <= exact_max_ts);
	ASSERT_FALSE(approx_max_ts <= approx_ts0);
	ASSERT_FALSE(approx_max_ts <= approx_ts1);
	ASSERT_TRUE(approx_max_ts <= approx_max_ts);
	ASSERT_TRUE(approx_max_ts <= infinite_ts_1);

	ASSERT_FALSE(infinite_ts_1 <= exact_ts0);
	ASSERT_FALSE(infinite_ts_1 <= exact_ts1);
	ASSERT_FALSE(infinite_ts_1 <= exact_max_ts);
	ASSERT_FALSE(infinite_ts_1 <= approx_ts0);
	ASSERT_FALSE(infinite_ts_1 <= approx_ts1);
	ASSERT_FALSE(infinite_ts_1 <= approx_max_ts);
	ASSERT_TRUE(infinite_ts_1 <= infinite_ts_1);
}

TEST(SimpleTest, TestTableSizeGreaterThanOrEqual) {
	DataManager m;

	auto exact_ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto exact_ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto exact_max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<long>()); // long unsigned int max value
	auto approx_ts0 = tablesize(TableSizeType::TST_APPROXIMATED, 0);
	auto approx_ts1 = tablesize(TableSizeType::TST_APPROXIMATED, 1);
	auto approx_max_ts = tablesize(TableSizeType::TST_APPROXIMATED, getMaxElem<long>()); // long unsigned int max value
	auto infinite_ts_1 = tablesize(TableSizeType::TST_INFINITE,0);

	ASSERT_TRUE(exact_ts0 >= exact_ts0);
	ASSERT_FALSE(exact_ts0 >= exact_ts1);
	ASSERT_FALSE(exact_ts0 >= exact_max_ts);
	ASSERT_TRUE(exact_ts0 >= approx_ts0);
	ASSERT_FALSE(exact_ts0 >= approx_ts1);
	ASSERT_FALSE(exact_ts0 >= approx_max_ts);
	ASSERT_FALSE(exact_ts0 >= infinite_ts_1);

	ASSERT_TRUE(exact_ts1 >= exact_ts0);
	ASSERT_TRUE(exact_ts1 >= exact_ts1);
	ASSERT_FALSE(exact_ts1 >= exact_max_ts);
	ASSERT_TRUE(exact_ts1 >= approx_ts0);
	ASSERT_TRUE(exact_ts1 >= approx_ts1);
	ASSERT_FALSE(exact_ts1 >= approx_max_ts);
	ASSERT_FALSE(exact_ts1 >= infinite_ts_1);

	ASSERT_TRUE(exact_max_ts >= exact_ts0);
	ASSERT_TRUE(exact_max_ts >= exact_ts1);
	ASSERT_TRUE(exact_max_ts >= exact_max_ts);
	ASSERT_TRUE(exact_max_ts >= approx_ts0);
	ASSERT_TRUE(exact_max_ts >= approx_ts1);
	ASSERT_TRUE(exact_max_ts >= approx_max_ts);
	ASSERT_FALSE(exact_max_ts >= infinite_ts_1);

	ASSERT_TRUE(approx_ts0 >= exact_ts0);
	ASSERT_FALSE(approx_ts0 >= exact_ts1);
	ASSERT_FALSE(approx_ts0 >= exact_max_ts);
	ASSERT_TRUE(approx_ts0 >= approx_ts0);
	ASSERT_FALSE(approx_ts0 >= approx_ts1);
	ASSERT_FALSE(approx_ts0 >= approx_max_ts);
	ASSERT_FALSE(approx_ts0 >= infinite_ts_1);

	ASSERT_TRUE(approx_ts1 >= exact_ts0);
	ASSERT_TRUE(approx_ts1 >= exact_ts1);
	ASSERT_FALSE(approx_ts1 >= exact_max_ts);
	ASSERT_TRUE(approx_ts1 >= approx_ts0);
	ASSERT_TRUE(approx_ts1 >= approx_ts1);
	ASSERT_FALSE(approx_ts1 >= approx_max_ts);
	ASSERT_FALSE(approx_ts1 >= infinite_ts_1);

	ASSERT_TRUE(approx_max_ts >= exact_ts0);
	ASSERT_TRUE(approx_max_ts >= exact_ts1);
	ASSERT_TRUE(approx_max_ts >= exact_max_ts);
	ASSERT_TRUE(approx_max_ts >= approx_ts0);
	ASSERT_TRUE(approx_max_ts >= approx_ts1);
	ASSERT_TRUE(approx_max_ts >= approx_max_ts);
	ASSERT_FALSE(approx_max_ts >= infinite_ts_1);

	ASSERT_TRUE(infinite_ts_1 >= exact_ts0);
	ASSERT_TRUE(infinite_ts_1 >= exact_ts1);
	ASSERT_TRUE(infinite_ts_1 >= exact_max_ts);
	ASSERT_TRUE(infinite_ts_1 >= approx_ts0);
	ASSERT_TRUE(infinite_ts_1 >= approx_ts1);
	ASSERT_TRUE(infinite_ts_1 >= approx_max_ts);
	ASSERT_TRUE(infinite_ts_1 >= infinite_ts_1);
}

TEST(SimpleMX,NoPushNegations){
	DataManager m;

	auto P = new Predicate("P",{});
	auto Q = new Predicate("Q",{});
	auto V = new Vocabulary("V", ParseInfo());
	V->add(P);
	V->add(Q);

	auto S = new Structure("S",V,ParseInfo());

	auto Pp = new PredForm(SIGN::POS, P, {}, FormulaParseInfo());
	auto Qp = new PredForm(SIGN::POS, Q, {}, FormulaParseInfo());
	auto nPp = new PredForm(SIGN::NEG, P, {}, FormulaParseInfo());
	auto nQp = new PredForm(SIGN::NEG, Q, {}, FormulaParseInfo());

	auto d1 = new BoolForm(SIGN::POS, false, {nPp, Qp}, FormulaParseInfo()); // ~P | Q
	auto d2 = new BoolForm(SIGN::POS, false, {nQp, Pp}, FormulaParseInfo()); // ~Q | P

	auto c1 = new BoolForm(SIGN::POS, true, {d1, d2}, FormulaParseInfo()); // (~P | Q) & (~Q | P) // P<=> Q
	auto c2 = new BoolForm(SIGN::NEG, true, {c1}, FormulaParseInfo()); // ~(((~P | Q) & (~Q | P))) // ~((P<=> Q))

	auto T = new Theory("T", V, ParseInfo());
	T->add(c2);

	auto Pp2 = new PredForm(SIGN::POS, P, {}, FormulaParseInfo());
	auto nQp2 = new PredForm(SIGN::NEG, Q, {}, FormulaParseInfo());

	auto r = new Rule({},Pp2,nQp2,ParseInfo());
	auto d = new Definition(); // { P <- ~Q}
	d->add(r);

	T->add(d);
	//T should have models
	auto models = ModelExpansion::doModelExpansion(T->clone(), S->clone());
	ASSERT_FALSE(models.unsat);

	FormulaUtils::pushNegations(T);
	models = ModelExpansion::doModelExpansion(T->clone(), S->clone());
	ASSERT_FALSE(models.unsat);
}
