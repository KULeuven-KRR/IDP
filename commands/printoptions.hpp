/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef PRINTOPTIONS_HPP_
#define PRINTOPTIONS_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "options.hpp"

class PrintOptionInference: public Inference {
public:
	PrintOptionInference(): Inference("tostring") {
		add(AT_OPTIONS);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Options* opts = args[0].options();
		return InternalArgument(StringPointer(toString(opts)));
	}
};

#endif /* PRINTOPTIONS_HPP_ */
