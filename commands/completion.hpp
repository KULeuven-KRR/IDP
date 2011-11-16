/************************************
	completion.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef COMPLETION_HPP_
#define COMPLETION_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "utils/TheoryUtils.hpp"

class CompletionInference: public Inference {
public:
	CompletionInference(): Inference("completion") {
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* theory = args[0].theory();
		theory = FormulaUtils::addCompletion(theory);
		return InternalArgument(theory);
	}
};

#endif /* COMPLETION_HPP_ */
