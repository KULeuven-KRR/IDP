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
#include "TestUtils.hpp"

#include "generators/ArithmeticOperatorsGenerator.hpp"

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

}
