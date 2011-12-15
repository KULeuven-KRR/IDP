/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef IDPTYPE_HPP_
#define IDPTYPE_HPP_

#include <vector>
#include "commandinterface.hpp"

class IdpTypeInference: public Inference {
public:
	IdpTypeInference(): Inference("idptype") {
		add(AT_INT);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		ArgType tp = (ArgType)args[0]._value._int;
		return InternalArgument(StringPointer(toCString(tp)));
	}
};

#endif /* IDPTYPE_HPP_ */
