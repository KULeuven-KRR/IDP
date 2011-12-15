/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef TABLEITERATOR_HPP_
#define TABLEITERATOR_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

/**
 * Returns an iterator for a given predicate table
 */
class TableIteratorInference: public Inference {
public:
	TableIteratorInference(): Inference("tableiterator") {
		add(AT_PREDTABLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		const PredTable* pt = args[0]._value._predtable;
		TableIterator* tit = new TableIterator(pt->begin());
		InternalArgument ia; ia._type = AT_TABLEITERATOR;
		ia._value._tableiterator = tit;
		return ia;
	}
};

#endif /* TABLEITERATOR_HPP_ */
