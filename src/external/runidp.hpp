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

#ifndef RUNGIDL_HPP_
#define RUNGIDL_HPP_

#include <string>
#include <vector>
#include <ostream>

enum class Status {
	SUCCESS, FAIL
};

std::ostream& operator<<(std::ostream& stream, Status status);

int run(int argc, char* argv[]);
int run(const std::vector<std::string>& inputfiles, bool interact, bool readstdin, const std::string& command);

Status test(const std::vector<std::string>& inputfileurls, const std::string& executioncommand = "");

#endif /* RUNGIDL_HPP_ */
