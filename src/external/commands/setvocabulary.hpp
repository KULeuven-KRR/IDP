/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef CHANGEVOCABULARY_HPP_
#define CHANGEVOCABULARY_HPP_

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"

typedef TypedInference<LIST(Structure*, Vocabulary*)> ChangeVocabularyInferenceBase;
class ChangeVocabularyInference: public ChangeVocabularyInferenceBase {
public:
	ChangeVocabularyInference()
			: ChangeVocabularyInferenceBase("setvocabulary", "Changes the vocabulary of a structure to the given one.") {
		setNameSpace(getVocabularyNamespaceName());
		//If some symbol occurs both in V and in the previous vocabulary of S, its interpretation in S is kept.
		//For all symbols that belong to V but not to the previous vocabulary of S,
		//the interpretation in S is initialized to the least precise interpretation.
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		get<0>(args)->changeVocabulary(get<1>(args));
		return nilarg();
	}
};

#endif /* CHANGEVOCABULARY_HPP_ */
