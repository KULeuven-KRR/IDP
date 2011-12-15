/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef MATERIALIZE_HPP_
#define MATERIALIZE_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

class MaterializeInference: public Inference {
public:
	MaterializeInference(): Inference("materialize") {
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractStructure* s = args[0].structure();
		s->materialize();
		return InternalArgument(s);
	}
};

#endif /* MATERIALIZE_HPP_ */
