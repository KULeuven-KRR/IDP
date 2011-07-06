/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CLONESTRUCTURE_HPP_
#define CLONESTRUCTURE_HPP_

#include <vector>
#include "internalargument.hpp"
#include "structure.hpp"

class CloneStructureInference: public Inference {
public:
	CloneStructureInference(): Inference("clone") {
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractStructure* s = args[0].structure();
		return InternalArgument(s->clone());
	}
};

#endif /* CLONESTRUCTURE_HPP_ */
