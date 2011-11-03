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
#include "structure.hpp"
#include <iostream>

using namespace std;

template<class T, class T2>
ostream& operator<<(ostream& stream, pair<T, T2> p){
	stream <<"(" <<p.first <<", "<<p.second <<")";
	return stream;
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
		EXPECT_TRUE(genvalues.size()==11);
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
		EXPECT_TRUE(genvalues.size()==19);
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
		EXPECT_TRUE(genvalues.size()==2);
	}
}
