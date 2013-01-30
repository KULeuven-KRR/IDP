/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef CLONESTRUCTURE_HPP_
#define CLONESTRUCTURE_HPP_

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"

class CloneStructureInference: public StructureBase {
public:
	CloneStructureInference()
			: StructureBase("clone", "Clones the given structure.") {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return InternalArgument(get<0>(args)->clone());
	}
};

class CloneTheoryInference: public TheoryBase {
public:
	CloneTheoryInference()
			: TheoryBase("clone", "Clones the given theory.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return InternalArgument(get<0>(args)->clone());
	}
};

#endif /* CLONESTRUCTURE_HPP_ */
