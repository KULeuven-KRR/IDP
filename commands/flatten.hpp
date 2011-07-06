/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FLATTEN_HPP_
#define FLATTEN_HPP_

#include <vector>
#include "internalargument.hpp"
#include "theory.hpp"

class FlattenInference: public Inference {
public:
	FlattenInference(): Inference("flatten") {
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* t = args[0].theory();
		TheoryUtils::flatten(t);
		return InternalArgument(t);
	}
};

#endif /* FLATTEN_HPP_ */
