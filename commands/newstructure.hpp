/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef NEWSTRUCTURE_HPP_
#define NEWSTRUCTURE_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"

class NewStructureInference: public TypedInference<LIST(Vocabulary*, std::string*)> {
public:
	NewStructureInference(): TypedInference("newstructure", "Create an empty structure with the given name over the given vocabulary.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto s = new Structure(*get<1>(args),ParseInfo());
		s->vocabulary(get<0>(args));
		return InternalArgument(s);
	}
};

#endif /* NEWSTRUCTURE_HPP_ */
