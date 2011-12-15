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

#include <vector>
#include "commandinterface.hpp"

class CreateRangeInference: public Inference {
public:
	CreateRangeInference(): Inference("range") {
		add(AT_INT);
		add(AT_INT);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		int n1 = args[0]._value._int;
		int n2 = args[1]._value._int;
		InternalArgument ia;
		ia._type = AT_DOMAIN;
		if(n1 <= n2) {
			ia._value._domain = new SortTable(new IntRangeInternalSortTable(n1,n2));
		}else{
			ia._value._domain = new SortTable(new EnumeratedInternalSortTable());
		}
		addToGarbageCollection(ia._value._domain);
		return ia;
	}
};

#endif /* CREATERANGE_HPP_ */
