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

#include "commandinterface.hpp"
#include "structure.hpp"
#include "theory.hpp"

class CloneStructureInference: public StructureBase {
public:
	CloneStructureInference(): StructureBase("clone", "Clones the given structure.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return InternalArgument(get<0>(args)->clone());
	}
};

class CloneTheoryInference: public TheoryBase {
public:
	CloneTheoryInference(): TheoryBase("clone", "Clones the given theory.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return InternalArgument(get<0>(args)->clone());
	}
};

#endif /* CLONESTRUCTURE_HPP_ */
