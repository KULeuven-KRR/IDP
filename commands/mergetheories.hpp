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

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "utils/TheoryUtils.hpp"

class MergeTheoriesInference: public Inference {
public:
	MergeTheoriesInference(): Inference("merge") {
		add(AT_THEORY);
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* t1 = args[0].theory();
		AbstractTheory* t2 = args[1].theory();
		AbstractTheory* t = FormulaUtils::merge(t1,t2);
		return InternalArgument(t);
	}
};

#endif /* MERGETHEORIES_HPP_ */
