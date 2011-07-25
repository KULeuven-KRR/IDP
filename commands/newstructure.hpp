/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef NEWSTRUCTURE_HPP_
#define NEWSTRUCTURE_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"

class NewStructureInference: public Inference {
public:
	NewStructureInference(): Inference("newStructure") {
		add(AT_VOCABULARY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Vocabulary* v = args[0].vocabulary();
		Structure* s = new Structure("",ParseInfo());
		s->vocabulary(v);
		return InternalArgument(s);
	}
};

#endif /* NEWSTRUCTURE_HPP_ */
