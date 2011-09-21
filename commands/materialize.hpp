/************************************
	materialize.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef MATERIALIZE_HPP_
#define MATERIALIZE_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

class MaterializeInference: public Inference {
public:
	MaterializeInference(): Inference("materialize") {
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractStructure* s = args[0].structure();
		s->materialize();
		return InternalArgument(s);
	}
};

#endif /* MATERIALIZE_HPP_ */
