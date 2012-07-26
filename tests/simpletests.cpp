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
#include "commontypes.hpp"
#include "inferences/grounding/GroundUtils.hpp"

#include <dirent.h>
#include <exception>

using namespace std;

namespace Tests {

TEST(SimpleTest, NegationOfTrueIsFalse) {
	ASSERT_EQ(_false, -_true);
}

}
