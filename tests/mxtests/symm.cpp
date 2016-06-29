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

namespace Tests {

TEST_P(MXnbTest, DoesMXWithSymmetryBreaking) {
	runTests("modelexpansion.idp", GetParam(), "mxwithsymm()");
}

TEST_P(MXsatTest, DoesSatWithSymmetryBreaking) {
	runTests("satisfiability.idp", GetParam(), "satwithsymm()");
}

}
