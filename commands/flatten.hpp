/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FLATTEN_HPP_
#define FLATTEN_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "theorytransformations/Utils.hpp"

class FlattenInference: public Inference {
public:
	FlattenInference(): Inference("flatten") {
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* t = args[0].theory();
		t = FormulaUtils::flatten(t);
		return InternalArgument(t);
	}
};

#endif /* FLATTEN_HPP_ */
