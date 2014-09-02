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

#ifndef NEWVOC_HPP_
#define NEWVOC_HPP_

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"

typedef TypedInference<LIST(std::string*)> NewVocabInferenceBase;
class NewVocabularyInference: public NewVocabInferenceBase {
public:
	NewVocabularyInference()
			: NewVocabInferenceBase("newvocabulary", "Create an empty vocabulary with the given name.") {
		setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto s = new Vocabulary(*get<0>(args), ParseInfo());
		return InternalArgument(s);
	}
};

#endif /* NEWVOC_HPP_ */
