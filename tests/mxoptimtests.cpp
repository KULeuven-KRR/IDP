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

vector<string> generateListOfMXOptimFiles() {
	vector<string> testdirs {"minimization/"};
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}
class MXOptimTest: public ::testing::TestWithParam<string> {
};

TEST_P(MXOptimTest, DoesMX) {
	runTests("optimization.idp", GetParam());
}

INSTANTIATE_TEST_CASE_P(ModelOptimization, MXOptimTest, ::testing::ValuesIn(generateListOfMXOptimFiles()));

}
