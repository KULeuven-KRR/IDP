/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef MAKEFALSE_HPP_
#define MAKEFALSE_HPP_

#include "commandinterface.hpp"
#include "structure.hpp"

/**
 * Implements maketrue, makefalse, and makeunknown on a predicate interpretation and a tuple
 */
typedef TypedInference<LIST(PredInter*, ElementTuple*)> SetAtomValueInferenceBase;
class SetAtomValueInference: public SetAtomValueInferenceBase {
private:
	enum SETVALUE {
		SET_TRUE, SET_FALSE, SET_UNKNOWN
	};
	SETVALUE value_;

public:
	static Inference* getMakeAtomTrueInference() {
		return new SetAtomValueInference("maketrue", "Sets the given tuple to true", SET_TRUE);
	}
	static Inference* getMakeAtomFalseInference() {
		return new SetAtomValueInference("makefalse", "Sets the given tuple to false", SET_FALSE);
	}
	static Inference* getMakeAtomUnknownInference() {
		return new SetAtomValueInference("makeunknown", "Sets the given tuple to unknown", SET_UNKNOWN);
	}

	SetAtomValueInference(const char* command, const char* description, SETVALUE value) :
		SetAtomValueInferenceBase(command, description, true), value_(value) {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto pri = get<0>(args);
		auto tuple = get<1>(args);
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
