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

#pragma once

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"

/**
 * Implements maketrue, makefalse, and makeunknown on a predicate interpretation and a tuple
 * The base interpretation
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
		return new SetAtomValueInference("maketrue", "Sets the interpretation of the given tuple to true.\nModifies the table-interpretation.", SET_TRUE);
	}
	static Inference* getMakeAtomFalseInference() {
		return new SetAtomValueInference("makefalse", "Sets the interpretation of the given tuple to false.\nModifies the table-interpretation.", SET_FALSE);
	}
	static Inference* getMakeAtomUnknownInference() {
		return new SetAtomValueInference("makeunknown", "Sets the interpretation of the given tuple to unknown.\nModifies the table-interpretation.", SET_UNKNOWN);
	}

	SetAtomValueInference(const char* command, const char* description, SETVALUE value)
			: SetAtomValueInferenceBase(command, description, true), value_(value) {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto pri = get<0>(args);
		auto tuple = get<1>(args);
		switch (value_) {
		case SET_TRUE:
			pri->makeTrueExactly(*tuple);
			break;
		case SET_FALSE:
			pri->makeFalseExactly(*tuple);
			break;
		case SET_UNKNOWN:
			pri->makeUnknownExactly(*tuple);
			break;
		default:
			break;
		}
		return nilarg();
	}
};
