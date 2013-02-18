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

#pragma once

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"

template<class Object>
Vocabulary* getvoc(Object o) {
	return o->vocabulary();
}

template<typename Base>
class GetVocabularyInference: public TypedInference<Base> {
public:
	GetVocabularyInference()
			: TypedInference<Base>("getvocabulary", "Returns the vocabulary of the given object.") {
		TypedInference<Base>::setNameSpace(getVocabularyNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto a = this-> template get<0>(args);
		return InternalArgument(getvoc(a));
	}
};
