/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#pragma once

#include "commandinterface.hpp"
#include "inferences/entailment/Entails.hpp"
#include "errorhandling/error.hpp"

template<bool Pointer, class T>
class InternalEqual{
public:
	InternalArgument internalExec(const std::vector<InternalArgument>& args);
};

template<class T>
class InternalEqual<true, T>{
public:
	InternalArgument internalExec(const T& one, const T& two){
		return InternalArgument(*one==*two);
	}
};

template<class T>
class InternalEqual<false, T>{
public:
	InternalArgument internalExec(const T& one, const T& two){
		return InternalArgument(one==two);
	}
};

template<class T>
class EqualInference: public TypedInference<LIST(T,T)> {
public:
	EqualInference()
	: TypedInference<LIST(T,T)>("equal", "Checks equality.") {
		TypedInference<LIST(T,T)>::setNameSpace(getInferenceNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto o = InternalEqual<Loki::TypeTraits<T>::isPointer, T>();
		return o.internalExec(this->template get<0>(args), this->template get<1>(args));
	}
};
