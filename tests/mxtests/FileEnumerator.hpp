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

#ifndef FILEENUMERATOR_HPP_
#define FILEENUMERATOR_HPP_

#include <cmath>
#include <cstdio>

#include "gtest/gtest.h"
#include "external/runidp.hpp"
#include "utils/FileManagement.hpp"
#include "TestUtils.hpp"

#include <exception>

namespace Tests {

std::vector<std::string> generateListOfSimpleMXnbFiles();
std::vector<std::string> generateListOfMXnbFiles();
std::vector<std::string> generateListOfMXsatFiles();
std::vector<std::string> generateListOfMXsatStableFiles();
std::vector<std::string> generateListOfSlowMXsatFiles();

class SimpleMXnbTest: public ::testing::TestWithParam<std::string> {
};
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
