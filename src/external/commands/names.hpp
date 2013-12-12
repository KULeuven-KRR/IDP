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

class Structure;
#include "commandinterface.hpp"
#include "IncludeComponents.hpp"
#include <string>


template<class Base>
class setNameInference: public TypedInference<LIST(Base, std::string*)> {
public:
	setNameInference()
			: TypedInference<LIST(Base, std::string*)>("setname", "sets the name of the given object.") {
		TypedInference<LIST(Base, std::string*)>::setNameSpace(getNamespaceName<Base>());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {

		auto object = this-> template get<0>(args);
		auto string = this-> template get<1>(args);

		object->name(*string);
		return InternalArgument();
	}
};

template<class Base>
class getNameInference: public TypedInference<LIST(Base)> {
public:
	getNameInference()
			:  TypedInference<LIST(Base)>("getname", "returns the name of the given object.") {
		 TypedInference<LIST(Base)>::setNameSpace(getNamespaceName<Base>());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto object = this-> template get<0>(args);
		return InternalArgument(new std::string(object->name()));
	}
};
