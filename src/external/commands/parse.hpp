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

#ifndef PARSE_HPP_
#define PARSE_HPP_

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"

extern void parsefile(const std::string&);

class ParseInference: public StringBase {
public:
	ParseInference()
			: StringBase("parse", "Parses the given file and adds its information into the datastructures.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		parsefile(*get<0>(args));
		return nilarg();
	}
};

#endif /* PARSE_HPP_ */
