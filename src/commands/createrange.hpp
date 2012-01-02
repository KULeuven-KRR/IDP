/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CREATERANGE_HPP_
#define CREATERANGE_HPP_

#include "commandinterface.hpp"
#include "error.hpp"

typedef TypedInference<LIST(int, int)> CreateRangeInferenceBase;
class CreateRangeInference: public CreateRangeInferenceBase {
public:
	CreateRangeInference(): CreateRangeInferenceBase("range", "Create a domain containing all integers between First and Last") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		int n1 = get<0>(args);
		int n2 = get<1>(args);
		InternalArgument ia;
		ia._type = AT_DOMAIN;
		if(n2<n1){
			Error::error("Begin should be lower or equal than end");
			return nilarg();
		}
		ia._value._domain = new SortTable(new IntRangeInternalSortTable(n1,n2));
		addToGarbageCollection(ia._value._domain);
		return ia;
	}
};

#endif /* CREATERANGE_HPP_ */
