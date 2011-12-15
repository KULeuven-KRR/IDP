/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CLONETHEORY_HPP_
#define CLONETHEORY_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"

class CloneTheoryInference: public Inference {
public:
	CloneTheoryInference(): Inference("clone") {
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* t = args[0].theory();
		return InternalArgument(t->clone());
	}
};

#endif /* CLONETHEORY_HPP_ */
