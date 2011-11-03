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
#include <iostream>

using namespace std;

template<class T, class T2>
ostream& operator<<(ostream& stream, pair<T, T2> p){
	stream <<"(" <<p.first <<", "<<p.second <<")";
	return stream;
}

template<typename T>
const DomainElement* domelem(T t){
	return DomainElementFactory::instance()->create(t);
}

namespace Tests{
	TEST(ComparisonGenerator, FiniteEquality){
		SortTable *left, *right;
		left = new SortTable(new IntRangeInternalSortTable(-10, 10));
		right = new SortTable(new IntRangeInternalSortTable(0, 20));
		DomElemContainer *leftvar = new DomElemContainer(), *rightvar = new DomElemContainer();
		ComparisonGenerator* gen = new ComparisonGenerator(left, right, leftvar, rightvar, Input::NONE, CompType::EQ);
		set<pair<int, int> > genvalues;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			auto tuple = pair<int, int>(leftvar->get()->value()._int, rightvar->get()->value()._int);
			genvalues.insert(tuple);
			EXPECT_EQ(tuple.first, tuple.second);
		}
		EXPECT_EQ(genvalues.size(), 11);
	}

	TEST(ComparisonGenerator, FiniteLT){
		SortTable *left, *right;
		left = new SortTable(new IntRangeInternalSortTable(-2, 2));
		right = new SortTable(new IntRangeInternalSortTable(0, 4));
		DomElemContainer *leftvar = new DomElemContainer(), *rightvar = new DomElemContainer();
		ComparisonGenerator* gen = new ComparisonGenerator(left, right, leftvar, rightvar, Input::NONE, CompType::LT);
		set<pair<int, int> > genvalues;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			auto tuple = pair<int, int>(leftvar->get()->value()._int, rightvar->get()->value()._int);
			genvalues.insert(tuple);
			EXPECT_LT(tuple.first, tuple.second);
		}
		EXPECT_EQ(genvalues.size(), 19);
	}

	TEST(ComparisonGenerator, FiniteGTLeftInput){
		SortTable *left, *right;
		left = new SortTable(new IntRangeInternalSortTable(-2, 2));
		right = new SortTable(new IntRangeInternalSortTable(-1, 4));
		DomElemContainer *leftvar = new DomElemContainer(), *rightvar = new DomElemContainer();
		ComparisonGenerator* gen = new ComparisonGenerator(left, right, leftvar, rightvar, Input::LEFT, CompType::GT);
		set<pair<int, int> > genvalues;
		leftvar->operator =(DomainElementFactory::instance()->create(1));
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			auto tuple = pair<int, int>(leftvar->get()->value()._int, rightvar->get()->value()._int);
			genvalues.insert(tuple);
			EXPECT_GT(tuple.first, tuple.second);
		}
		EXPECT_EQ(genvalues.size(), 2);
	}

	TEST(SortGenerator, FiniteSort){
		auto sort = new SortTable(new IntRangeInternalSortTable(-2, 2));
		auto var = new DomElemContainer();
		auto gen = new SortInstGenerator(sort->internTable(), var);
		set<int> genvalues;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			auto value = var->get()->value()._int;
			genvalues.insert(value);
			EXPECT_LT(value, 3);
			EXPECT_GT(value, -3);
		}
		EXPECT_EQ(genvalues.size(), 5);
	}

	TEST(LookupGenerator, FiniteSort){
		auto sort = new SortTable(new IntRangeInternalSortTable(-2, 2));
		auto var = new DomElemContainer();
		Universe universe({sort});
		auto gen = new LookupGenerator(new PredTable(new FullInternalPredTable(), universe), {var}, universe);

		var->operator =(DomainElementFactory::instance()->create(-2));
		EXPECT_TRUE(gen->check());
		int count = 0;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			count++;
		}
		EXPECT_EQ(count, 1);

		var->operator =(DomainElementFactory::instance()->create(1));
		EXPECT_TRUE(gen->check());
		count = 0;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			count++;
		}
		EXPECT_EQ(count, 1);

		var->operator =(DomainElementFactory::instance()->create(-10));
		EXPECT_FALSE(gen->check());
		count = 0;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			count++;
		}
		EXPECT_EQ(count, 0);
	}

	TEST(LookupGenerator, Enum){
		auto sort1 = new SortTable(new IntRangeInternalSortTable(-2, 1));
		auto sort2 = new SortTable(new IntRangeInternalSortTable(-2, 2));

		auto var1 = new DomElemContainer();
		auto var2 = new DomElemContainer();
		Universe universe({sort1, sort2});
		SortedElementTable elemTable({
			{domelem(1), domelem(2)},
			{domelem(1), domelem(-1)},
			{domelem(-2), domelem(0)}
		});
		auto predtable = new PredTable(new EnumeratedInternalPredTable(elemTable), universe);

		auto gen = new LookupGenerator(predtable, {var1, var2}, universe);

		var1->operator =(DomainElementFactory::instance()->create(-2));
		var2->operator =(DomainElementFactory::instance()->create(0));
		EXPECT_TRUE(gen->check());
		int count = 0;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			count++;
		}
		EXPECT_EQ(count, 1);

		var1->operator =(DomainElementFactory::instance()->create(1));
		var2->operator =(DomainElementFactory::instance()->create(2));
		EXPECT_TRUE(gen->check());
		count = 0;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			count++;
		}
		EXPECT_EQ(count, 1);

		var1->operator =(DomainElementFactory::instance()->create(2));
		var2->operator =(DomainElementFactory::instance()->create(2));
		EXPECT_FALSE(gen->check());
		count = 0;
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			count++;
		}
		EXPECT_EQ(count, 0);
	}

	TEST(InverseTableGenerator, FiniteSort){
		auto sort1 = new SortTable(new IntRangeInternalSortTable(-2, 1));
		auto sort2 = new SortTable(new IntRangeInternalSortTable(-2, 2));

		auto var1 = new DomElemContainer();
		auto var2 = new DomElemContainer();
		Universe universe({sort1, sort2});
		SortedElementTable elemTable({
			{domelem(1), domelem(2)},
			{domelem(1), domelem(-1)},
			{domelem(-2), domelem(0)}
		});
		auto predtable = new PredTable(new EnumeratedInternalPredTable(elemTable), universe);
		auto gen = new InverseInstGenerator(predtable, {Pattern::INPUT, Pattern::OUTPUT}, {var1, var2});

		set<int> genvalues;
		var1->operator =(DomainElementFactory::instance()->create(1));
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			auto value = var2->get()->value()._int;
			genvalues.insert(value);
			EXPECT_TRUE(value==-1 || value==2);
		}
		var1->operator =(DomainElementFactory::instance()->create(-2));
		for(gen->begin(); not gen->isAtEnd(); gen->operator ++()){
			auto value = var2->get()->value()._int;
			genvalues.insert(value);
			EXPECT_EQ(value, 0);
		}
		EXPECT_EQ(genvalues.size(), 3);
	}
}
