/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include <cmath>

#include "gtest/gtest.h"
#include "external/runidp.hpp"
#include "commontypes.hpp"
#include "inferences/grounding/GroundUtils.hpp"
#include "structure/TableSize.hpp"

#include <dirent.h>
#include <exception>

using namespace std;

namespace Tests {

TEST(SimpleTest, NegationOfTrueIsFalse) {
	ASSERT_EQ(_false, -_true);
}

}

TEST(SimpleTest, TestTableSizeEquality) {
	auto exact_ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto exact_ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto exact_max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<unsigned long>()); // long unsigned int max value
	auto approx_ts0 = tablesize(TableSizeType::TST_APPROXIMATED, 0);
	auto approx_ts1 = tablesize(TableSizeType::TST_APPROXIMATED, 1);
	auto approx_max_ts = tablesize(TableSizeType::TST_APPROXIMATED, getMaxElem<unsigned long>()); // long unsigned int max value
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
	auto exact_ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto exact_ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto exact_max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<unsigned long>()); // long unsigned int max value
	auto approx_ts0 = tablesize(TableSizeType::TST_APPROXIMATED, 0);
	auto approx_ts1 = tablesize(TableSizeType::TST_APPROXIMATED, 1);
	auto approx_max_ts = tablesize(TableSizeType::TST_APPROXIMATED, getMaxElem<unsigned long>()); // long unsigned int max value
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
	auto exact_ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto exact_ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto exact_max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<unsigned long>()); // long unsigned int max value
	auto approx_ts0 = tablesize(TableSizeType::TST_APPROXIMATED, 0);
	auto approx_ts1 = tablesize(TableSizeType::TST_APPROXIMATED, 1);
	auto approx_max_ts = tablesize(TableSizeType::TST_APPROXIMATED, getMaxElem<unsigned long>()); // long unsigned int max value
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
	auto exact_ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto exact_ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto exact_max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<unsigned long>()); // long unsigned int max value
	auto approx_ts0 = tablesize(TableSizeType::TST_APPROXIMATED, 0);
	auto approx_ts1 = tablesize(TableSizeType::TST_APPROXIMATED, 1);
	auto approx_max_ts = tablesize(TableSizeType::TST_APPROXIMATED, getMaxElem<unsigned long>()); // long unsigned int max value
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
	auto exact_ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto exact_ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto exact_max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<unsigned long>()); // long unsigned int max value
	auto approx_ts0 = tablesize(TableSizeType::TST_APPROXIMATED, 0);
	auto approx_ts1 = tablesize(TableSizeType::TST_APPROXIMATED, 1);
	auto approx_max_ts = tablesize(TableSizeType::TST_APPROXIMATED, getMaxElem<unsigned long>()); // long unsigned int max value
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
