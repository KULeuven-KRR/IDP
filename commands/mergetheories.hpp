/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef MERGETHEORIES_HPP_
#define MERGETHEORIES_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"

class MergeTheoriesInference: public Inference {
public:
	MergeTheoriesInference(): Inference("merge") {
		add(AT_THEORY);
		add(AT_THEORY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* t1 = args[0].theory();
		AbstractTheory* t2 = args[1].theory();
		AbstractTheory* t = TheoryUtils::merge(t1,t2);
		return InternalArgument(t);
	}
};

#endif /* MERGETHEORIES_HPP_ */
