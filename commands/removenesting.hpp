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
	RemoveNestingInference (): Inference("remove_nesting") {
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* t = args[0].theory();
		TheoryUtils::remove_nesting(t);
		return InternalArgument(t);
	}
};

#endif /* REMOVENESTING_HPP_ */
