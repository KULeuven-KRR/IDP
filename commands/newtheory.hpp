/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef NEWTHEORY_HPP_
#define NEWTHEORY_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"

class NewTheoryInference: public Inference {
public:
	NewTheoryInference(): Inference("newTheory") {
		add(AT_VOCABULARY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Vocabulary* v = args[0].vocabulary();
		Theory* t = new Theory("",v,ParseInfo());
		return InternalArgument(t);
	}
};

#endif /* NEWTHEORY_HPP_ */
