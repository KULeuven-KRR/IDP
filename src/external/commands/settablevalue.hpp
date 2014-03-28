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

#ifndef MAKETABTRUE_HPP_
#define MAKETABTRUE_HPP_

#include "commandinterface.hpp"
#include "monitors/interactiveprintmonitor.hpp"
#include "IncludeComponents.hpp"

/**
 * Implements makeTrue, makeFalse, and makeUnknown on a predicate interpretation and lua table
 * An additional implementation because we currently (TODO) have no auto-cast mechanism to automatically cast the second argument to an elementtuple
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
		return new SetTableValueInference("maketrue", "Sets all tuples of the given table to true.\nModifies the table-interpretation.", SET_TRUE);
	}
	static Inference* getMakeTableFalseInference() {
		return new SetTableValueInference("makefalse", "Sets all tuples of the given table to false.\nModifies the table-interpretation.", SET_FALSE);
	}
	static Inference* getMakeTableUnknownInference() {
		return new SetTableValueInference("makeunknown", "Sets all tuples of the given table to unknown.\nModifies the table-interpretation.", SET_UNKNOWN);
	}

	SetTableValueInference(const char* command, const char* description, SETVALUE value)
			: SetTableValueInferenceBase(command, description, true), value_(value) {
		setNameSpace(getStructureNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto pri = get<0>(args);
		auto tuple = toTuple(get<1>(args), *printmonitor());
		switch (value_) {
		case SET_TRUE:
			pri->makeTrueExactly(tuple);
			break;
		case SET_FALSE:
			pri->makeFalseExactly(tuple);
			break;
		case SET_UNKNOWN:
			pri->makeUnknownExactly(tuple);
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
				tup.push_back(createDomElem(*it->_value._string));
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
