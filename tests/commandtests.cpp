/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "gtest/gtest.h"
#include "external/commands/commandinterface.hpp"
#include "external/commands/iterators.hpp"
#include "IncludeComponents.hpp"

using namespace std;

namespace Tests {

TEST(IteratorInferenceTest, IntDomain) {
	auto dom = new SortTable(new IntRangeInternalSortTable(-2,2));
	auto itInf = DomainIteratorInference();
	auto itRes = itInf.execute({ InternalArgument(dom) });

	ASSERT_TRUE(itRes._type == AT_DOMAINITERATOR);
	ASSERT_TRUE(isa<SortIterator>(*(itRes._value._sortiterator)));

	auto daiInf = DomainDerefAndIncrementInference<int>();
	auto daiRes1 = daiInf.execute({ itRes, InternalArgument(0) });

	ASSERT_TRUE(daiRes1._type == AT_INT);
	ASSERT_EQ(-2,daiRes1._value._int);

	auto daiRes2 = daiInf.execute({ itRes, InternalArgument(0) });

	ASSERT_TRUE(daiRes2._type == AT_INT);
	ASSERT_EQ(-1,daiRes2._value._int);
}

TEST(IteratorInferenceTest, DoubleDomain) {
	auto eist = new EnumeratedInternalSortTable();
	eist->add(createDomElem(1.1));
	eist->add(createDomElem(1.2));
	eist->add(createDomElem(1.3));
	auto dom = new SortTable(eist);
	auto itInf = DomainIteratorInference();
	auto itRes = itInf.execute({ InternalArgument(dom) });

	ASSERT_TRUE(itRes._type == AT_DOMAINITERATOR);
	ASSERT_TRUE(isa<SortIterator>(*(itRes._value._sortiterator)));

	auto daiInf = DomainDerefAndIncrementInference<double>();
	auto daiRes1 = daiInf.execute({ itRes, InternalArgument(0) });

	ASSERT_TRUE(daiRes1._type == AT_DOUBLE);
	ASSERT_EQ(1.1,daiRes1._value._double);

	auto daiRes2 = daiInf.execute({ itRes, InternalArgument(0) });

	ASSERT_TRUE(daiRes2._type == AT_DOUBLE);
	ASSERT_EQ(1.2,daiRes2._value._double);
}

TEST(IteratorInferenceTest, StringDomain) {
	auto eist = new EnumeratedInternalSortTable();
	eist->add(createDomElem(StringPointer("A")));
	eist->add(createDomElem(StringPointer("B")));
	eist->add(createDomElem(StringPointer("C")));
	auto dom = new SortTable(eist);
	auto itInf = DomainIteratorInference();
	auto itRes = itInf.execute({ InternalArgument(dom) });

	ASSERT_TRUE(itRes._type == AT_DOMAINITERATOR);
	ASSERT_TRUE(isa<SortIterator>(*(itRes._value._sortiterator)));

	auto daiInf = DomainDerefAndIncrementInference<std::string*>();
	auto daiRes1 = daiInf.execute({ itRes, InternalArgument(0) });

	ASSERT_TRUE(daiRes1._type == AT_STRING);
	ASSERT_EQ(StringPointer("A"),daiRes1._value._string);

	auto daiRes2 = daiInf.execute({ itRes, InternalArgument(0) });

	ASSERT_TRUE(daiRes2._type == AT_STRING);
	ASSERT_EQ(StringPointer("B"),daiRes2._value._string);
}

//TEST(IteratorInferenceTest, CompoundDomain) {
	//TODO
//}

} /* namespace Tests */
