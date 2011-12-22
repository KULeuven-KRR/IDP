/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef NEWOPTIONS_HPP_
#define NEWOPTIONS_HPP_

#include "commandinterface.hpp"
#include "options.hpp"

class NewOptionsInference: public EmptyBase {
public:
	NewOptionsInference(): EmptyBase("newoptions", "Create new options, set to the standard options") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto opts = new Options();
		return InternalArgument(opts);
	}
};

#endif /* NEWOPTIONS_HPP_ */
