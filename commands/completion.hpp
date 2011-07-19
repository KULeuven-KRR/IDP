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

class CompletionInference: public Inference {
public:
	CompletionInference(): Inference("completion") {
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* theory = args[0].theory();
		TheoryUtils::completion(theory);
		return nilarg();
	}
};

#endif /* COMPLETION_HPP_ */
