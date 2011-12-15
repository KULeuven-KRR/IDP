/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef DEREFANDINCREMENT_HPP_
#define DEREFANDINCREMENT_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

class TableDerefAndIncrementInference: public Inference {
public:
	TableDerefAndIncrementInference(): Inference("deref_and_increment") {
		add(AT_TABLEITERATOR);
		add(AT_TUPLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		TableIterator* it = args[0]._value._tableiterator;
		if(it->isAtEnd()){
			return nilarg();
		}

		ElementTuple* tuple = new ElementTuple(*(*it));
		it->operator++();
		InternalArgument ia; ia._type = AT_TUPLE; ia._value._tuple = tuple;
		return ia;
	}
};

template<class Iterator>
InternalArgument derefValueAndIncrementIterator(Iterator it){
	if(it->isAtEnd()){
		return nilarg();
	}

	const DomainElement* element = *(*it);
	it->operator++();
	InternalArgument ia(element);
	return ia;
}

class IntDerefAndIncrementInference: public Inference {
public:
	IntDerefAndIncrementInference(): Inference("deref_and_increment") {
		add(AT_DOMAINITERATOR);
		add(AT_INT);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return derefValueAndIncrementIterator(args[0]._value._sortiterator);
	}
};

class StringDerefAndIncrementInference: public Inference {
public:
	StringDerefAndIncrementInference(): Inference("deref_and_increment") {
		add(AT_DOMAINITERATOR);
		add(AT_STRING);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return derefValueAndIncrementIterator(args[0]._value._sortiterator);
	}
};

class DoubleDerefAndIncrementInference: public Inference {
public:
	DoubleDerefAndIncrementInference(): Inference("deref_and_increment") {
		add(AT_DOMAINITERATOR);
		add(AT_DOUBLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return derefValueAndIncrementIterator(args[0]._value._sortiterator);
	}
};

class CompoundDerefAndIncrementInference: public Inference {
public:
	CompoundDerefAndIncrementInference(): Inference("deref_and_increment") {
		add(AT_DOMAINITERATOR);
		add(AT_COMPOUND);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return derefValueAndIncrementIterator(args[0]._value._sortiterator);
	}
};


#endif /* DEREFANDINCREMENT_HPP_ */
