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

#include "gtest/gtest.h"
#include "external/runidp.hpp"
#include "TestUtils.hpp"
#include "utils/FileManagement.hpp"

#include <cmath>
#include <cstdio>
#include <exception>

using namespace std;

namespace Tests {

class LuaCommands: public ::testing::TestWithParam<std::string> {
};

TEST(LuaCommands, GetAndSetVocabulary) {
	string testfile(getTestDirectory() + "vocabularycommandtests.idp");
	Status result = Status::FAIL;
	ASSERT_NO_THROW( result = test( {testfile}, "main()"););
	ASSERT_EQ(Status::SUCCESS, result);

}

}
