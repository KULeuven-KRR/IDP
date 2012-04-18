/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef IDPTYPE_HPP_
#define IDPTYPE_HPP_

#include "commandinterface.hpp"

typedef TypedInference<LIST(int)> IdpTypeInferenceBase;
class IdpTypeInference: public IdpTypeInferenceBase {
public:
	IdpTypeInference()
			: IdpTypeInferenceBase("idptype", "Returns custom typeids for first-class idp citizens.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto tp = (ArgType) get<0>(args);
		return InternalArgument(StringPointer(toCString(tp)));
	}
};

#endif /* IDPTYPE_HPP_ */
