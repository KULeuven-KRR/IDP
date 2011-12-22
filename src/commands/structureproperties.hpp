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

#include "commandinterface.hpp"
#include "theory.hpp"
#include "structure.hpp"

class IsConsistentInference: public StructureBase {
public:
	IsConsistentInference() :
		StructureBase("isconsistent", "Check whether the structure is consistent") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		InternalArgument ia;
		ia._type = AT_BOOLEAN;
		ia._value._boolean = get<0>(args)->isConsistent();
		return ia;
	}
};

#endif /* STRUCTPROPERTIES_HPP_ */
