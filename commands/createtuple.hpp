/************************************
	createtuple.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CREATETUPLE_HPP_
#define CREATETUPLE_HPP_

#include <vector>
#include "commandinterface.hpp"

class CreateTupleInference: public Inference {
public:
	CreateTupleInference(): Inference("dummytuple") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		InternalArgument ia;
		ia._type = AT_TUPLE;
		ia._value._tuple = 0;
		return ia;
	}
};

#endif /* CREATETUPLE_HPP_ */
