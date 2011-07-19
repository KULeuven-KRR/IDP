/************************************
	tableiterator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef TABLEITERATOR_HPP_
#define TABLEITERATOR_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

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
