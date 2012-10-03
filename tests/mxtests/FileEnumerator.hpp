#ifndef FILEENUMERATOR_HPP_
#define FILEENUMERATOR_HPP_

#include <cmath>
#include <cstdio>

#include "gtest/gtest.h"
#include "external/rungidl.hpp"
#include "utils/FileManagement.hpp"
#include "TestUtils.hpp"

#include <exception>

namespace Tests {

std::vector<std::string> generateListOfMXnbFiles();
std::vector<std::string> generateListOfMXsatFiles();
std::vector<std::string> generateListOfMXsatStableFiles();
std::vector<std::string> generateListOfSlowMXsatFiles();

class MXnbTest: public ::testing::TestWithParam<std::string> {
};
class MXsatTest: public ::testing::TestWithParam<std::string> {
};
class MXsatStableTest: public ::testing::TestWithParam<std::string> {
};
class SlowMXsatTest: public ::testing::TestWithParam<std::string> {
};

}


#endif /* FILEENUMERATOR_HPP_ */
