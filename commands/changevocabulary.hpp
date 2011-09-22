/************************************
	changevocabulary.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CHANGEVOCABULARY_HPP_
#define CHANGEVOCABULARY_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"

class ChangeVocabularyInference: public Inference {
public:
	ChangeVocabularyInference(): Inference("changevocabulary") {
		add(AT_STRUCTURE);
		add(AT_VOCABULARY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractStructure* s = args[0].structure();
		Vocabulary* v = args[1].vocabulary();
		s->vocabulary(v);
		return nilarg();
	}
};

#endif /* CHANGEVOCABULARY_HPP_ */
