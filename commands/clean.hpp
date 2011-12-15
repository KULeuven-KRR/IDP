/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CLEAN_HPP_
#define CLEAN_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

class CleanInference: public Inference {
public:
	CleanInference(): Inference("clean") {
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractStructure* s = args[0].structure();
		s->clean();
		return InternalArgument(s);
	}
};

#endif /* CLEAN_HPP_ */
