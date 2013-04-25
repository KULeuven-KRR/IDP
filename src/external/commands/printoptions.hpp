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

#ifndef PRINTOPTIONS_HPP_
#define PRINTOPTIONS_HPP_

#include "commandinterface.hpp"
#include "options.hpp"
#include <iostream>

class PrintOptionInference: public OptionsBase {
public:
	PrintOptionInference()
			: OptionsBase("tostring", "Prints the given optionsblock.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto opts = get<0>(args);
		return InternalArgument(new std::string(toString(opts)));
	}
};

#endif /* PRINTOPTIONS_HPP_ */
