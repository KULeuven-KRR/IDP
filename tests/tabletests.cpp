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
#include "IncludeComponents.hpp"
#include "structure/StructureComponents.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "testingtools.hpp"

using namespace std;

namespace Tests {

TEST(TableTest, BDDApproxInverseAndApproxEqualTrivial) {
	auto bts1 = getBDDTestingSet1(0, 0, 0, 0);
	auto truetable = new BDDInternalPredTable(bts1.truebdd, bts1.manager, { }, bts1.ts1.structure);
	auto falsetable = new BDDInternalPredTable(bts1.falsebdd, bts1.manager, { }, bts1.ts1.structure);
	Universe u = Universe(std::vector<SortTable*> { });
	ASSERT_TRUE(truetable->approxInverse(falsetable, u));
	ASSERT_FALSE(truetable->approxEqual(falsetable, u));
}

TEST(TableTest, BDDApproxInverseAndApproxEqual) {
	auto bts1 = getBDDTestingSet1(0, 0, 0, 0);
	auto bdd = bts1.pxandqx;
	auto y = new Variable(bts1.ts1.sort); // variable
	map<const FOBDDVariable*, const FOBDDVariable*> mvv;
	auto negbdd = bts1.manager->negation(bdd);
	mvv[bts1.x] = bts1.manager->getVariable(y);
	negbdd = bts1.manager->substitute(negbdd, mvv);
	auto bddtable = new BDDInternalPredTable(bdd, bts1.manager, { bts1.x->variable() }, bts1.ts1.structure);
	auto negbddtable = new BDDInternalPredTable(negbdd, bts1.manager, { y }, bts1.ts1.structure);
	Universe u = Universe(std::vector<SortTable*> {bts1.ts1.sorttable});
	ASSERT_TRUE(bddtable->approxInverse(negbddtable, u));
	ASSERT_FALSE(bddtable->approxEqual(negbddtable, u));
	auto othernegbddtable = InverseInternalPredTable::getInverseTable(bddtable);
	ASSERT_TRUE(bddtable->approxInverse(othernegbddtable, u));
	ASSERT_FALSE(bddtable->approxEqual(othernegbddtable, u));
	ASSERT_TRUE(negbddtable->approxEqual(othernegbddtable, u));
}

TEST(TableTest, InverseInverseInternalTableTest) {
	auto onetable = new EnumeratedInternalPredTable();
	auto invtable = InverseInternalPredTable::getInverseTable(onetable);
	ASSERT_TRUE(isa<InverseInternalPredTable>(*invtable));
	auto invinvtable = InverseInternalPredTable::getInverseTable(invtable);
	ASSERT_FALSE(isa<InverseInternalPredTable>(*invinvtable));
	auto invinvinvtable = InverseInternalPredTable::getInverseTable(invinvtable);
	ASSERT_TRUE(isa<InverseInternalPredTable>(*invinvinvtable));
}

TEST(TableTest, FunctionInterpretationTest) {
	auto tab = new EnumeratedInternalPredTable();
	ElementTuple x;
	
	x.push_back( getGlobal()->getGlobalDomElemFactory()->create(1) );
	x.push_back( getGlobal()->getGlobalDomElemFactory()->create(2) );
	
	tab->add(x);
	
	Universe u;
	u.addTable(TableUtils::createSortTable(-10, 10));
	u.addTable(TableUtils::createSortTable(-5, 5));
	
	ElementTuple x0;
	x0.push_back(x[0]);
	 //Constructing funcinter via predinter to be sure to test the value of a graphed function
	auto ftab = FuncInter(new PredInter(new PredTable(tab,u), true));

	ASSERT_TRUE(ftab.value(x0)==x[1]);
	
}

}
