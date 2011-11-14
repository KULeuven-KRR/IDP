/************************************
  	pushnegations.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CMD_PUSHNEGATIONS_HPP_
#define CMD_PUSHNEGATIONS_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "utils/TheoryUtils.hpp"

class PushNegationsInference: public Inference {
public:
	PushNegationsInference(): Inference("pushnegations") {
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* t = args[0].theory();
		t = FormulaUtils::pushNegations(t);
		return InternalArgument(t);
	}
};

#endif /* CMD_PUSHNEGATIONS_HPP_ */
