/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PUSHNEGATIONS_HPP_
#define PUSHNEGATIONS_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"

class PushNegationsInference: public Inference {
public:
	PushNegationsInference(): Inference("push_negations") {
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* t = args[0].theory();
		TheoryUtils::push_negations(t);
		return InternalArgument(t);
	}
};

#endif /* PUSHNEGATIONS_HPP_ */
