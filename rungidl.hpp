/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef RUNGIDL_HPP_
#define RUNGIDL_HPP_

#include <string>
#include <vector>
#include <ostream>

enum class Status {SUCCESS, FAIL};

std::ostream& operator<<(std::ostream& stream, Status status);

int run(int argc, char* argv[]);

Status test(const std::vector<std::string>& inputfileurls);

#endif /* RUNGIDL_HPP_ */
