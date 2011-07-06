/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CLONETHEORY_HPP_
#define CLONETHEORY_HPP_

#include <vector>
#include "internalargument.hpp"
#include "theory.hpp"

class CloneTheoryInference: public Inference {
public:
	CloneTheoryInference(): Inference("clone") {
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* t = args[0].theory();
		return InternalArgument(t->clone());
	}
};

#endif /* CLONETHEORY_HPP_ */
