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
#include "TestUtils.hpp"

#include "generators/BinaryArithmeticOperatorsGenerator.hpp"
#include "structure/StructureComponents.hpp"

#include "structure/TableSize.hpp"
#include "utils/ArithmeticUtils.hpp"

#include "structure/DomainElementFactory.hpp"
#include <exception>

using namespace std;

namespace Tests {

TEST(OverflowTest, PlusTable) {
	auto table = PlusInternalFuncTable(true);
	ASSERT_EQ(table.operator []({createDomElem(getMaxElem<int>()), createDomElem(1)}), (void*)NULL);
	ASSERT_EQ(table.operator []({createDomElem(getMinElem<int>()+39), createDomElem(-40)}), (void*)NULL);
}

TEST(OverflowTest, MinusTable) {
	auto table = MinusInternalFuncTable(true);
	ASSERT_EQ(table.operator []({createDomElem(getMaxElem<int>()), createDomElem(-1)}), (void*)NULL);
}

TEST(OverflowTest, ProductTable) {
	auto table = TimesInternalFuncTable(true);
	ASSERT_EQ(table.operator []({createDomElem(getMaxElem<int>()/2), createDomElem(3)}), (void*)NULL);
}

TEST(OverflowTest, DivisionTable) {
	auto table = DivInternalFuncTable(true);
	ASSERT_NE(table.operator []({createDomElem(getMinElem<int>()*2), createDomElem(1)}), (void*)NULL);

	ASSERT_EQ(table.operator []({createDomElem(getMinElem<int>()*2), createDomElem(0)}), (void*)NULL);
}

TEST(OverflowTest, ArithPlusGenerator) {
	auto d1 = new const DomElemContainer(), d2 = new const DomElemContainer(), d3 = new const DomElemContainer();
	auto dom = SortTable(new AllIntegers());
	auto gen = PlusGenerator(d1, d2, d3, NumType::CERTAINLYINT, &dom);
	d1->operator =(createDomElem(getMaxElem<int>()));
	d2->operator =(createDomElem(250));
	gen.begin();
	ASSERT_TRUE(gen.isAtEnd());
	ASSERT_EQ(NULL, d3->get());
}

TEST(OverflowTest, PlusTableSize) {
	auto max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<long long>()); // long unsigned int max value
	auto ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto ts2 = tablesize(TableSizeType::TST_EXACT, 2);
	auto infinite_ts = tablesize(TableSizeType::TST_INFINITE,0);

	auto tmp = max_ts + ts0;
	auto eq = getMaxElem<long long>();
	ASSERT_EQ(tmp._type,TableSizeType::TST_EXACT);
	ASSERT_EQ(tmp._size,eq);
	tmp = ts0 + max_ts;
	ASSERT_EQ(tmp._type,TableSizeType::TST_EXACT);
	ASSERT_EQ(tmp._size,eq);

	tmp = max_ts + ts1;
	ASSERT_EQ(tmp._type,TableSizeType::TST_INFINITE);
	tmp = ts1 + max_ts;
	ASSERT_EQ(tmp._type,TableSizeType::TST_INFINITE);

	tmp = max_ts + infinite_ts;
	ASSERT_EQ(tmp._type,TableSizeType::TST_INFINITE);
	tmp = infinite_ts + max_ts;
	ASSERT_EQ(tmp._type,TableSizeType::TST_INFINITE);

	tmp = ts1 + ts2;
	ASSERT_EQ(tmp._type,TableSizeType::TST_EXACT);
	eq = 3;
	ASSERT_EQ(tmp._size,eq);
}

TEST(OverflowTest, MinusTableSize) {
	auto max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<long long>()); // long unsigned int max value
	auto ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto ts2 = tablesize(TableSizeType::TST_EXACT, 2);
	auto infinite_ts = tablesize(TableSizeType::TST_INFINITE,0);

	ASSERT_THROW(ts1-ts2,IdpException);
	ASSERT_THROW(ts1-infinite_ts,IdpException);

	auto tmp = max_ts - ts1;
	auto eq = getMaxElem<long long>() - 1;
	ASSERT_EQ(tmp._type,TableSizeType::TST_EXACT);
	ASSERT_EQ(tmp._size,eq);

}

TEST(OverflowTest, TimesTableSize) {
	auto max_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<long long>()); // long unsigned int max value
	auto onethirdmax_ts = tablesize(TableSizeType::TST_EXACT, getMaxElem<long long>()/3); // long unsigned int max value
	auto ts0 = tablesize(TableSizeType::TST_EXACT, 0);
	auto ts1 = tablesize(TableSizeType::TST_EXACT, 1);
	auto ts2 = tablesize(TableSizeType::TST_EXACT, 2);
	auto ts3 = tablesize(TableSizeType::TST_EXACT, 3);
	auto ts4 = tablesize(TableSizeType::TST_EXACT, 4);
	auto infinite_ts = tablesize(TableSizeType::TST_INFINITE,0);

	auto tmp = ts2 * ts3;
	ASSERT_EQ(tmp._type,TableSizeType::TST_EXACT);
	long long eq = 6;
	ASSERT_EQ(tmp._size,eq);

	tmp = max_ts * ts0;
	eq = 0;
	ASSERT_EQ(tmp._type,TableSizeType::TST_EXACT);
	ASSERT_EQ(tmp._size,eq);
	tmp = ts0 * max_ts;
	ASSERT_EQ(tmp._type,TableSizeType::TST_EXACT);
	ASSERT_EQ(tmp._size,eq);

	tmp = onethirdmax_ts * ts1;
	eq = getMaxElem<long long>()/3;
	ASSERT_EQ(tmp._type,TableSizeType::TST_EXACT);
	ASSERT_EQ(tmp._size,eq);
	tmp = ts1 * onethirdmax_ts;
	ASSERT_EQ(tmp._type,TableSizeType::TST_EXACT);
	ASSERT_EQ(tmp._size,eq);

	tmp = onethirdmax_ts * ts3;
	eq = 3*(getMaxElem<long long>()/3);
	ASSERT_EQ(tmp._type,TableSizeType::TST_EXACT);
	ASSERT_EQ(tmp._size,eq);
	tmp = ts3 * onethirdmax_ts;
	ASSERT_EQ(tmp._type,TableSizeType::TST_EXACT);
	ASSERT_EQ(tmp._size,eq);

	tmp = onethirdmax_ts * ts4;
	ASSERT_EQ(tmp._type,TableSizeType::TST_INFINITE);
	tmp = ts4 * onethirdmax_ts;
	ASSERT_EQ(tmp._type,TableSizeType::TST_INFINITE);

	tmp = max_ts * infinite_ts;
	ASSERT_EQ(tmp._type,TableSizeType::TST_INFINITE);
	tmp = infinite_ts * max_ts;
	ASSERT_EQ(infinite_ts._type,TableSizeType::TST_INFINITE);
}

}
