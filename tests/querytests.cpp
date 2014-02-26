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
#include "utils/FileManagement.hpp"
#include "TestUtils.hpp"
#include "testingtools.hpp"
#include "inferences/querying/Query.hpp"
#include "parseinfo.hpp"
#include "IncludeComponents.hpp"
#include "structure/StructureComponents.hpp"
#include "theory/Query.hpp"

#include <exception>

using namespace std;

namespace Tests {
TEST(InternalQueryTest,MultipleOccurencesOfSameVar){
	auto ts1 = getTestingSet1();
	auto q = new Query("query",{ts1.x,ts1.x},ts1.sxx,ParseInfo());
	SortedElementTable els;
	els.insert({createDomElem(0),createDomElem(0)});
	els.insert({createDomElem(0),createDomElem(1)});
	Universe univ({ts1.sorttable,ts1.sorttable});
	auto str = ts1.structure;
	str->changeInter(ts1.s,new PredInter(new PredTable(new EnumeratedInternalPredTable(els),univ),true));
	auto res = Querying::doSolveQuery(q,str);
	auto size = res->size();
	auto sizeIsExact = (size._type==TableSizeType::TST_EXACT);
	ASSERT_TRUE(sizeIsExact);
	long long one = 1;
	ASSERT_EQ(one,size._size);
}

vector<string> generateListOfQueryFiles() {
	vector<string> testdirs {"simple/", "aggregates/", "threevalued/"};
	return getAllFilesInDirs(getTestDirectory() + "query/", testdirs);
}

class QueryTest: public ::testing::TestWithParam<string> {
};

TEST_P(QueryTest, DoesQuerying) {
	runTests("querytest.idp", GetParam());
}


INSTANTIATE_TEST_CASE_P(Querying, QueryTest, ::testing::ValuesIn(generateListOfQueryFiles()));

}
