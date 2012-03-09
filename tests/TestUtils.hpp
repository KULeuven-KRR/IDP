/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*
* Use of this software is governed by the GNU LGPLv3.0 license
*
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef TESTS_UTILS_HPP_
#define TESTS_UTILS_HPP_
#include <string>

std::string getTestDirectory();

namespace Tests {

void runTests(const char* inferencefilename, const std::string& instancefile);
}

void throwexc();

#endif /* TESTS_UTILS_HPP_ */
