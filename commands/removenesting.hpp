/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef REMOVENESTING_HPP_
#define REMOVENESTING_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"

class RemoveNestingInference : public Inference {
public:
	RemoveNestingInference (): Inference("removeNesting") {
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* t = args[0].theory();
		TheoryUtils::removeNesting(t);
		return InternalArgument(t);
	}
};

#endif /* REMOVENESTING_HPP_ */
