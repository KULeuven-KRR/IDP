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
#include "GlobalData.hpp"
#include "structure/MainStructureComponents.hpp"
#include "commontypes.hpp"
#include "inferences/grounding/Utils.hpp"

#include <dirent.h>
#include <exception>

using namespace std;

namespace Tests {

TEST(Insertion, EmptyTable) {
	auto table = TableUtils::createSortTable();
	ASSERT_FALSE(table->isRange()); // Empty tables cannot be ranges
	table->add(createDomElem(1));
	ASSERT_TRUE(table->isRange());
	table->add(createDomElem(2));
	ASSERT_TRUE(table->isRange());
	table->add(createDomElem(0));
	table->add(createDomElem(-1));
	ASSERT_TRUE(table->isRange());
	table->add(5,2);
	ASSERT_TRUE(table->isRange());
	table->add(1,-3);
	ASSERT_TRUE(table->isRange());
	table->add(createDomElem(-4));
	ASSERT_FALSE(table->isRange());
}

TEST(Insertion, InRangeTable) {
	auto table = TableUtils::createSortTable();
	ASSERT_FALSE(table->isRange()); // Empty tables cannot be ranges
	table->add(1,10);
	table->add(5,2);
	ASSERT_TRUE(table->isRange());
}

TEST(Insertion, OutTable) {
	auto table = TableUtils::createSortTable();
	ASSERT_FALSE(table->isRange()); // Empty tables cannot be ranges
	table->add(-5,5);
	table->add(-100,-7);
	ASSERT_FALSE(table->isRange());
}

TEST(Insertion, WithFloatTable) {
	auto table = TableUtils::createSortTable();
	ASSERT_FALSE(table->isRange()); // Empty tables cannot be ranges
	table->add(-5,5);
	ASSERT_TRUE(table->isRange());
	table->add(createDomElem(0.3));
	ASSERT_FALSE(table->isRange());
}

}
