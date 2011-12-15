/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef STRUCTPROPERTIES_HPP_
#define STRUCTPROPERTIES_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "structure.hpp"

class IsConsistentInference: public Inference {
public:
	IsConsistentInference() :
			Inference("isconsistent") {
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		bool consistent = args[0].structure()->isConsistent();
		return InternalArgument(consistent?1:0);
	}
};

#endif /* STRUCTPROPERTIES_HPP_ */
