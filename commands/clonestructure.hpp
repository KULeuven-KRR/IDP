/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CLONESTRUCTURE_HPP_
#define CLONESTRUCTURE_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

class CloneStructureInference: public Inference {
public:
	CloneStructureInference(): Inference("clone") {
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractStructure* s = args[0].structure();
		return InternalArgument(s->clone());
	}
};

#endif /* CLONESTRUCTURE_HPP_ */
