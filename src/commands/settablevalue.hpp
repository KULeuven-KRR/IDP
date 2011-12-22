/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef MAKETABTRUE_HPP_
#define MAKETABTRUE_HPP_

#include "commandinterface.hpp"
#include "monitors/interactiveprintmonitor.hpp"
#include "structure.hpp"

/**
 * Implements makeTrue, makeFalse, and makeUnknown on a predicate interpretation and lua table
 */
typedef TypedInference<LIST(PredInter*, std::vector<InternalArgument>*)> SetTableValueInferenceBase;
class SetTableValueInference: public SetTableValueInferenceBase {
private:
	enum SETVALUE {
		SET_TRUE, SET_FALSE, SET_UNKNOWN
	};
	SETVALUE value_;
public:
	static Inference* getMakeTableTrueInference() {
		return new SetTableValueInference("maketrue", "Sets the given tuple to true", SET_TRUE);
	}
	static Inference* getMakeTableFalseInference() {
		return new SetTableValueInference("makefalse", "Sets the given tuple to false", SET_FALSE);
	}
	static Inference* getMakeTableUnknownInference() {
		return new SetTableValueInference("makeunknown", "Sets the given tuple to unknown", SET_UNKNOWN);
	}

	SetTableValueInference(const char* command, const char* description, SETVALUE value) :
		SetTableValueInferenceBase(command, description, true), value_(value) {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto pri = get<0>(args);
		auto tuple = toTuple(get<1>(args), *printmonitor());
		switch (value_) {
		case SET_TRUE:
			pri->makeTrue(tuple);
			break;
		case SET_FALSE:
			pri->makeFalse(tuple);
			break;
		case SET_UNKNOWN:
			pri->makeUnknown(tuple);
			break;
		default:
			break;
		}
		return nilarg();
	}

	ElementTuple toTuple(std::vector<InternalArgument>* tab, InteractivePrintMonitor& monitor) const {
		ElementTuple tup;
		for (auto it = tab->begin(); it != tab->end(); ++it) {
			switch (it->_type) {
			case AT_INT:
				tup.push_back(createDomElem(it->_value._int));
				break;
			case AT_DOUBLE:
				tup.push_back(createDomElem(it->_value._double));
				break;
			case AT_STRING:
				tup.push_back(createDomElem(it->_value._string));
				break;
			case AT_COMPOUND:
				tup.push_back(createDomElem(it->_value._compound));
				break;
			default:
				monitor.printerror("Wrong value in a tuple. Expected an integer, double, string, or compound");
				break;
			}
		}
		return tup;
	}
};
#endif /* MAKETABTRUE_HPP_ */