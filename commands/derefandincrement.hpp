/************************************
	derefandincrement.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

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
		if(it->hasNext()) {
			ElementTuple* tuple = new ElementTuple(*(*it));
			it->operator++();
			InternalArgument ia; ia._type = AT_TUPLE; ia._value._tuple = tuple;
			return ia;
		}
		else{
			return nilarg();
		}
	}
};

template<class Iterator>
InternalArgument derefValueAndIncrementIterator(Iterator it){
	if(it->hasNext()) {
		const DomainElement* element = *(*it);
		it->operator++();
		InternalArgument ia(element);
		return ia;
	}
	else{
		return nilarg();
	}
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
