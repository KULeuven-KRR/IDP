/************************************
	tableiterator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef TABLESIZE_HPP_
#define TABLESIZE_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

/**
 * Returns the size of a table
 */
class TableSizeInference: public Inference {
public:
	TableSizeInference(): Inference("size") {
		add(AT_PREDTABLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		const PredTable* pt = args[0]._value._predtable;
		const auto& size = pt->size();

		InternalArgument ia; ia._type = AT_INT;
		if(size.first){
			ia._value._int = size.second;
		}else{ // Size cannot be calculated
			ia._value._int = -1; // TODO maybe a bit ugly?
		}
		return ia;
	}
};

#endif /* TABLESIZE_HPP_ */
