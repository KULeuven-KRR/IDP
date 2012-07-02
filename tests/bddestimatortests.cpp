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
#include "external/rungidl.hpp"
#include "IncludeComponents.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/Estimations.hpp"
#include "testingtools.hpp"

using namespace std;

template<class T, class T2>
ostream& operator<<(ostream& stream, pair<T, T2> p) {
	stream << "(" << p.first << ", " << p.second << ")";
	return stream;
}

template<typename T>
const DomainElement* domelem(T t) {
	return createDomElem(t);
}

namespace Tests {

TEST(BDDEstimators, NbAnswersTrivial) {
	auto bts1 = getBDDTestingSet1(0,0,0,0);
	double result;
	result = BddStatistics::estimateNrAnswers(bts1.truebdd, { }, { }, NULL, bts1.manager);
	ASSERT_EQ(1, result);
	result = BddStatistics::estimateNrAnswers(bts1.falsebdd, { }, { }, NULL, bts1.manager);
	ASSERT_EQ(0, result);
}
TEST(BDDEstimators, NbAnswersDepth2) {
	auto bts1 = getBDDTestingSet1(-1,0,0,0);
	double result;
	result = BddStatistics::estimateNrAnswers(bts1.px,{},{},bts1.ts1.structure,bts1.manager);
	ASSERT_EQ(1, result);

}

}
