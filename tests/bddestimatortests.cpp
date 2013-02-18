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

void checkEqual(double expected, double result) {
	EXPECT_GE(expected+10E-5, result);
	EXPECT_LE(expected-10E-5, result);
}

TEST(BDDEstimators, NbAnswersTrivial) {
	auto bts1 = getBDDTestingSet1(0, 0, 0, 0);
	double result;
	result = BddStatistics::estimateNrAnswers(bts1.truebdd, { }, { }, bts1.ts1.structure, bts1.manager);
	checkEqual(1,result);
	result = BddStatistics::estimateNrAnswers(bts1.falsebdd, { }, { }, bts1.ts1.structure, bts1.manager);
	checkEqual(0,result);
}
TEST(BDDEstimators, NbAnswers2DepthNoVars) {
	auto bts1 = getBDDTestingSet1(-1, 0, 0, 0);
	double result;
	result = BddStatistics::estimateNrAnswers(bts1.px, { }, { }, bts1.ts1.structure, bts1.manager);
	checkEqual(0.4,result);
	//For a random input, the chance is 0.4 that the query succeeds
}

TEST(BDDEstimators, NbAnswers2Depth1Var) {
	auto bts1 = getBDDTestingSet1(-1, 0, 0, 0);
	double result;
	result = BddStatistics::estimateNrAnswers(bts1.px, { bts1.x }, { }, bts1.ts1.structure, bts1.manager);
	checkEqual(2,result);
	//Chance that this succeeds is 0.4, univsize is 5, 5*0.4 is 2
}

TEST(BDDEstimators, NbAnswers3DepthNoVars) {
	auto bts1 = getBDDTestingSet1(-1, 0, 0, 0);
	double result;
	result = BddStatistics::estimateNrAnswers(bts1.pxandqx, { }, { }, bts1.ts1.structure, bts1.manager);
	checkEqual(0.4*0.2,result);
}

TEST(BDDEstimators, NbAnswers3Depth1Var) {
	auto bts1 = getBDDTestingSet1(-1, 0, 0, 0);
	double result;
	result = BddStatistics::estimateNrAnswers(bts1.pxandqx, { bts1.x }, { }, bts1.ts1.structure, bts1.manager);
	checkEqual(0.4,result);
	//"naive" chance for an input that the query succeeds is 0.4*0.2. There are 5 inputs, thus expected result is 0.4
}

}
