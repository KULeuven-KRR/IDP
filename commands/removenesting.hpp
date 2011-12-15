/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CMD_REMOVENESTING_HPP_
#define CMD_REMOVENESTING_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "utils/TheoryUtils.hpp"

class RemoveNestingInference : public Inference {
public:
	RemoveNestingInference (): Inference("removenesting") {
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* t = args[0].theory();
		t = FormulaUtils::unnestTerms(t);
		return InternalArgument(t);
	}
};

#endif /* CMD_REMOVENESTING_HPP_ */
