#include "FileEnumerator.hpp"

namespace Tests {

std::vector<std::string> generateListOfMXTotalnbFiles(){
	std::vector<std::string> testdirs { "simplemx/", "numberknown/", "outputvoc_large/" };
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}

class MXTotalnbTest: public ::testing::TestWithParam<std::string> {
};

TEST_P(MXTotalnbTest, DoesMXWithTseitinAndSatDelaying) {
	runTests("modelexpansion.idp", GetParam(), "mxverylazy()");
}

TEST_P(MXsatTest, DoesSatMXWithLazyTseitinAndSatDelaying) {
	runTests("satisfiability.idp", GetParam(), "satverylazy()");
}

INSTANTIATE_TEST_CASE_P(ModelExpansion, MXTotalnbTest, ::testing::ValuesIn(generateListOfMXTotalnbFiles()));

}
