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
#include "utils/FileManagement.hpp"
#include "TestUtils.hpp"

#include <exception>

using namespace std;

namespace Tests {

vector<string> generateListOfQueryFiles() {
	vector<string> testdirs {"simple/", "aggregates/"/*, "threevalued/"*/};
	return getAllFilesInDirs(getTestDirectory() + "query/", testdirs);
}

class QueryTest: public ::testing::TestWithParam<string> {
};

TEST_P(QueryTest, DoesQuerying) {
	runTests("querytest.idp", GetParam());
}


INSTANTIATE_TEST_CASE_P(Querying, QueryTest, ::testing::ValuesIn(generateListOfQueryFiles()));


}
