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
#include "common.hpp"
#include "TestUtils.hpp"

using namespace std;

int main(int argc, char **argv) {
	setInstallDirectoryPath(getTestDirectory() + "../data/"); // Guarantees that make check etc. work with the non-installed datafiles
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
