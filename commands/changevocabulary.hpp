/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

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
