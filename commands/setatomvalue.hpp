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

/**
 * Implements maketrue, makefalse, and makeunknown on a predicate interpretation and a tuple
 */
class SetAtomValueInference: public Inference {
	enum SETVALUE { SET_TRUE, SET_FALSE, SET_UNKNOWN };
private:
	SETVALUE value_;
public:
	static Inference* getMakeAtomTrueInference(){
		return new SetAtomValueInference("maketrue",SET_TRUE);
	}
	static Inference* getMakeAtomFalseInference(){
		return new SetAtomValueInference("makefalse",SET_FALSE);
	}
	static Inference* getMakeAtomUnknownInference(){
		return new SetAtomValueInference("makeunknown",SET_UNKNOWN);
	}

	SetAtomValueInference(const char* command, SETVALUE value): Inference(command,true), value_(value) {
		add(AT_PREDINTER);
		add(AT_TUPLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		PredInter* pri = args[0]._value._predinter;
		ElementTuple* tuple = args[1]._value._tuple;
		switch (value_) {
			case SET_TRUE:
				pri->makeTrue(*tuple);
				break;
			case SET_FALSE:
				pri->makeFalse(*tuple);
				break;
			case SET_UNKNOWN:
				pri->makeUnknown(*tuple);
				break;
			default:
				break;
		}
		return nilarg();
	}
};

#endif /* MAKEFALSE_HPP_ */
