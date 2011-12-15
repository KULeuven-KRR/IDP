/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef DOMAINITERATOR_HPP_
#define DOMAINITERATOR_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

class DomainIteratorInference: public Inference {
public:
	DomainIteratorInference(): Inference("domainiterator") {
		add(AT_DOMAIN);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		const SortTable* st = args[0]._value._domain;
		SortIterator* it = new SortIterator(st->sortBegin());
		InternalArgument ia; ia._type = AT_DOMAINITERATOR;
		ia._value._sortiterator = it;
		return ia;
	}
};

#endif /* DOMAINITERATOR_HPP_ */
