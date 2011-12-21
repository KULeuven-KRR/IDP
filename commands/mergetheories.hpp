/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef MERGETHEORIES_HPP_
#define MERGETHEORIES_HPP_

#include "commandinterface.hpp"
#include "theory.hpp"
#include "utils/TheoryUtils.hpp"

class MergeTheoriesInference: public TypedInference<LIST(AbstractTheory*, AbstractTheory*)> {
public:
	MergeTheoriesInference(): TypedInference("merge", "Create a new theory which is the result of combining both input theories") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return InternalArgument(FormulaUtils::merge(get<0>(args), get<1>(args)));
	}
};

#endif /* MERGETHEORIES_HPP_ */
