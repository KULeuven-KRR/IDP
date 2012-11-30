
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
