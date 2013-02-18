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

#ifndef HASHELEMENTTUPLE_HPP_
#define HASHELEMENTTUPLE_HPP_

#include "DomainElement.hpp"

struct HashTuple {
	inline size_t operator()(const ElementTuple& tuple) const {
		size_t seed = 1;
		int prod = 1;
		for (auto i = tuple.cbegin(); i < tuple.cend(); ++i) {
			Assert((*i)!=NULL);
			switch ((*i)->type()) {
			case DomainElementType::DET_INT:
				seed += (*i)->value()._int * prod;
				break;
			case DomainElementType::DET_DOUBLE:
				seed *= (*i)->value()._double * prod;
				break;
			case DomainElementType::DET_STRING:
				seed += reinterpret_cast<size_t>((*i)->value()._string) * prod;
				break;
			case DomainElementType::DET_COMPOUND:
				seed += reinterpret_cast<size_t>((*i)->value()._compound) * prod;
				break;
			}
			prod *= 10000;
		}
		//std::clog <<seed % 15485863 <<" ";
		return seed % 104729 /*15485863*/;
	}
};

#endif /* HASHELEMENTTUPLE_HPP_ */
