/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef DOMAINITERATOR_HPP_
#define DOMAINITERATOR_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

class DomainIteratorInference: public SortTableBase {
public:
	DomainIteratorInference(): SortTableBase("iterator", "Create an iterator for the given sorttable.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto st = get<0>(args);
		auto it = new SortIterator(st->sortBegin());
		InternalArgument ia; ia._type = AT_DOMAINITERATOR;
		ia._value._sortiterator = it;
		return ia;
	}
};

/**
 * Returns an iterator for a given predicate table
 */
class TableIteratorInference: public PredTableBase {
public:
	TableIteratorInference(): PredTableBase("iterator", "Create an iterator for the given predtable.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto pt = get<0>(args);
		auto tit = new TableIterator(pt->begin());
		InternalArgument ia; ia._type = AT_TABLEITERATOR;
		ia._value._tableiterator = tit;
		return ia;
	}
};

typedef TypedInference<LIST(TableIterator*, ElementTuple*)> TableDerefAndIncrementInferenceBase;
class TableDerefAndIncrementInference: public TableDerefAndIncrementInferenceBase {
public:
	TableDerefAndIncrementInference() :
		TableDerefAndIncrementInferenceBase("deref_and_increment", "Returns the current value and increments the tableiterator.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto it = get<0>(args);
		if (it->isAtEnd()) {
			return nilarg();
		}

		auto tuple = new ElementTuple(**it);
		it->operator++();
		InternalArgument ia;
		ia._type = AT_TUPLE;
		ia._value._tuple = tuple;
		return ia;
	}
};

typedef TypedInference<LIST(SortIterator*, ElementTuple*)> DomainDerefAndIncrementInferenceBase;
class DomainDerefAndIncrementInference: public DomainDerefAndIncrementInferenceBase {
public:
	DomainDerefAndIncrementInference() :
		DomainDerefAndIncrementInferenceBase("deref_and_increment", "Returns the current value and increments the sortiterator") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto it = get<0>(args);
		if (it->isAtEnd()) {
			return nilarg();
		}

		auto element = **it;
		it->operator++();
		InternalArgument ia(element);
		return ia;
	}
};

#endif /* DOMAINITERATOR_HPP_ */
