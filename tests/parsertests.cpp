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

#include <dirent.h>
#include <exception>

using namespace std;

namespace Tests {

TEST(ParsingTest, FunctionDefinition){
	string testfile1(string(TESTDIR) + "parser/functiondefinition.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(ParsingTest, Fail){
	string testfile1(string(TESTDIR) + "parser/parseerror.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(ParsingTest, OverloadedType){
	string testfile1(string(TESTDIR) + "parser/parseerror2.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(ParsingTest, IdenticalType){
	string testfile1(string(TESTDIR) + "parser/parseerror3.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(ParsingTest, IdenticalFunction){
	string testfile1(string(TESTDIR) + "parser/parseerror4.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(ParsingTest, IdenticalPredicate){
	string testfile1(string(TESTDIR) + "parser/parseerror5.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(ParsingTest, FailAndContinue){
	string testfile1(string(TESTDIR) + "parser/parseerror.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1 }));
}

TEST(RestartTest, FailAndContinue){
	string testfile1(string(TESTDIR) + "parser/parseerror.idp");
	string testfile2(string(TESTDIR) + "mx/simplemx/atom.idp");
	string testfilemx(string(TESTDIR) + "mxnbofmodelstest.idp");
	ASSERT_EQ(Status::FAIL, test( { testfile1, testfilemx }));
	ASSERT_EQ(Status::SUCCESS, test( { testfile2, testfilemx }));
}

TEST(ParsingTest, ValidScopes){
	string testfile1(string(TESTDIR) + "parser/scoping.idp");
	ASSERT_EQ(Status::SUCCESS, test( { testfile1 }));
}

}
