/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef NEWTHEORY_HPP_
#define NEWTHEORY_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"

class NewTheoryInference: public Inference {
public:
	NewTheoryInference(): Inference("newtheory") {
		add(AT_VOCABULARY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Vocabulary* v = args[0].vocabulary();
		Theory* t = new Theory("",v,ParseInfo());
		return InternalArgument(t);
	}
};

#endif /* NEWTHEORY_HPP_ */
