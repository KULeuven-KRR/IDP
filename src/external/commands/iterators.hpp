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

#ifndef DOMAINITERATOR_HPP_
#define DOMAINITERATOR_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "IncludeComponents.hpp"

/**
 * Returns an iterator for a given predicate table
 */
class TableIteratorInference: public PredTableBase {
public:
	TableIteratorInference()
			: PredTableBase("iterator", "Create an iterator for the given predtable.") {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto pt = get<0>(args);
		auto tit = new TableIterator(pt->begin());
		InternalArgument ia;
		ia._type = AT_TABLEITERATOR;
		ia._value._tableiterator = tit;
		return ia;
	}
};

class DomainIteratorInference: public SortTableBase {
public:
	DomainIteratorInference()
			: SortTableBase("iterator", "Create an iterator for the given sorttable.") {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto st = get<0>(args);
		auto sit = new SortIterator(st->sortBegin());
		InternalArgument ia;
		ia._type = AT_DOMAINITERATOR;
		ia._value._sortiterator = sit;
		return ia;
	}
};


//Deref_and_increment inferences are used in idp_intern.idp to give to the lua to enable lua_iterators.
typedef TypedInference<LIST(TableIterator*, ElementTuple*)> TableDerefAndIncrementInferenceBase;
class TableDerefAndIncrementInference: public TableDerefAndIncrementInferenceBase {
public:
	TableDerefAndIncrementInference()
			: TableDerefAndIncrementInferenceBase("deref_and_increment", "Returns the current value and increments the tableiterator.") {
		setNameSpace(getInternalNamespaceName());
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

template<typename T>
class DomainDerefAndIncrementInference: public Inference {
public:
	DomainDerefAndIncrementInference()
			: Inference("deref_and_increment", "Returns the current value and increments the sortiterator.") {
		setNameSpace(getInternalNamespaceName());
		addType(Type2Value<SortIterator*>::get());
		addType(Type2Value<T>::get());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto arg = args[0];
		Assert(arg._type == getArgType(0));
		auto it = arg.get<SortIterator*>();
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
