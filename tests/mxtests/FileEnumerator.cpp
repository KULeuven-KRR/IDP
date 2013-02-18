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

#include "FileEnumerator.hpp"

using namespace std;

namespace Tests{

vector<string> generateListOfSimpleMXnbFiles() {
	vector<string> testdirs { "simplemx/" };
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}
vector<string> generateListOfMXnbFiles() {
	vector<string> testdirs { "simplemx/", "numberknown/", "nontotal/" };
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}
vector<string> generateListOfMXsatFiles() {
	vector<string> testdirs { "satmx/" };
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}
vector<string> generateListOfMXsatStableFiles() {
	vector<string> testdirs { "stable/" };
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}
vector<string> generateListOfSlowMXsatFiles() {
	vector<string> testdirs {"satmxlongrunning/"};
	return getAllFilesInDirs(getTestDirectory() + "mx/", testdirs);
}

INSTANTIATE_TEST_CASE_P(ModelExpansion, SimpleMXnbTest, ::testing::ValuesIn(generateListOfSimpleMXnbFiles()));
INSTANTIATE_TEST_CASE_P(ModelExpansion, MXnbTest, ::testing::ValuesIn(generateListOfMXnbFiles()));
INSTANTIATE_TEST_CASE_P(ModelExpansion, MXsatTest, ::testing::ValuesIn(generateListOfMXsatFiles()));
INSTANTIATE_TEST_CASE_P(ModelExpansion, MXsatStableTest, ::testing::ValuesIn(generateListOfMXsatStableFiles()));
INSTANTIATE_TEST_CASE_P(ModelExpansionLong, SlowMXsatTest, ::testing::ValuesIn(generateListOfSlowMXsatFiles()));

}
