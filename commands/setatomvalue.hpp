/************************************
	makefalse.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef MAKEFALSE_HPP_
#define MAKEFALSE_HPP_

#include <vector>
#include "internalargument.hpp"
#include "structure.hpp"

class MakeTrueInference: public Inference {
public:
	MakeTrueInference(): Inference("maketrue") {
		add(AT_PREDINTER);
		add(AT_TUPLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		PredInter* pri = args[0]._value._predinter;
		ElementTuple* tup = args[1]._value._tuple;
		pri->makeTrue(*tup);
		return nilarg();
	}
};

class MakeFalseInference: public Inference {
public:
	MakeFalseInference(): Inference("makefalse") {
		add(AT_PREDINTER);
		add(AT_TUPLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		PredInter* pri = args[0]._value._predinter;
		ElementTuple* tup = args[1]._value._tuple;
		pri->makeFalse(*tup);
		return nilarg();
	}
};

class MakeUnknownInference: public Inference {
public:
	MakeUnknownInference(): Inference("makeunknown") {
		add(AT_PREDINTER);
		add(AT_TUPLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		PredInter* pri = args[0]._value._predinter;
		ElementTuple* tup = args[1]._value._tuple;
		pri->makeUnknown(*tup);
		return nilarg();
	}
};


#endif /* MAKEFALSE_HPP_ */
