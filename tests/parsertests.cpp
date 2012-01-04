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
#include "rungidl.hpp"
#include "TestUtils.hpp"

#include "utils/FileManagement.hpp"
#include <exception>

using namespace std;

namespace Tests {

vector<string> generateListOfInvalidParseFiles() {
	vector<string> testdirs {"parser/invalidinput/"};
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}

vector<string> generateListOfValidParseFiles() {
	vector<string> testdirs {"parser/validinput/"};
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}

class ValidParserTest: public ::testing::TestWithParam<string> {
};
class InvalidParserTest: public ::testing::TestWithParam<string> {
};

TEST_P(ValidParserTest, Parse) {
	cerr << "Testing " << GetParam() << "\n";
	ASSERT_EQ(Status::SUCCESS, test({ GetParam()}));
}
TEST_P(InvalidParserTest, Parse) {
	cerr << "Testing " << GetParam() << "\n";
	ASSERT_EQ(Status::FAIL, test({ GetParam()}));
}

INSTANTIATE_TEST_CASE_P(Parsing, ValidParserTest, ::testing::ValuesIn(generateListOfValidParseFiles()));
INSTANTIATE_TEST_CASE_P(Parsing, InvalidParserTest, ::testing::ValuesIn(generateListOfInvalidParseFiles()));

TEST(RestartTest, FailAndContinue){
	string testfile1(getTestDirectory() + "parser/parseerror.idp");
	string testfile2(getTestDirectory() + "mx/simplemx/atom.idp");
	string testfilemx(getTestDirectory() + "mxnbofmodelstest.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1, testfilemx }));
	ASSERT_EQ(Status::SUCCESS, test( { testfile2, testfilemx }));
}

TEST(MakeTrueTest, NoPerson){
	string testfile(getTestDirectory() + "parser/expectException/makeTrueNoPerson");
	ASSERT_EQ(Status::FAIL, test( { testfile}));
}

TEST(MakeTrueTest, NoString){
	string testfile(getTestDirectory() + "parser/expectException/makeTrueNoString");
	ASSERT_EQ(Status::FAIL, test( { testfile}));
}

TEST(MakeTrueTest, NoAge){
	string testfile(getTestDirectory() + "parser/expectException/makeTrueNoAge");
	ASSERT_EQ(Status::FAIL, test( { testfile}));
}

}
