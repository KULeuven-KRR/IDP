/************************************
  	removenesting.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

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
