/************************************
	clean.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CLEAN_HPP_
#define CLEAN_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

class CleanInference: public Inference {
public:
	CleanInference(): Inference("clean") {
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractStructure* s = args[0].structure();
		s->clean();
		return InternalArgument(s);
	}
};

#endif /* CLEAN_HPP_ */
