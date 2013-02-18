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

#ifndef NEWSTRUCTURE_HPP_
#define NEWSTRUCTURE_HPP_

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"

typedef TypedInference<LIST(Vocabulary*, std::string*)> NewStructureInferenceBase;
class NewStructureInference: public NewStructureInferenceBase {
public:
	NewStructureInference()
			: NewStructureInferenceBase("newstructure", "Create an empty structure with the given name over the given vocabulary.") {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto s = new Structure(*get<1>(args), ParseInfo());
		s->changeVocabulary(get<0>(args));
		return InternalArgument(s);
	}
};

#endif /* NEWSTRUCTURE_HPP_ */
