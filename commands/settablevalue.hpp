/************************************
 maketabtrue.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef MAKETABTRUE_HPP_
#define MAKETABTRUE_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "monitors/interactiveprintmonitor.hpp"
#include "structure.hpp"

/**
 * Implements makeTrue, makeFalse, and makeUnknown on a predicate interpretation and lua table
 */
class SetTableValueInference: public Inference {
	enum SETVALUE {
		SET_TRUE, SET_FALSE, SET_UNKNOWN
	};
private:
	SETVALUE value_;
public:
	static Inference* getMakeTableTrueInference() {
		return new SetTableValueInference("maketrue", SET_TRUE);
	}
	static Inference* getMakeTableFalseInference() {
		return new SetTableValueInference("makefalse", SET_FALSE);
	}
	static Inference* getMakeTableUnknownInference() {
		return new SetTableValueInference("makeunknown", SET_UNKNOWN);
	}

	SetTableValueInference(const char* command, SETVALUE value) :
			Inference(command, true), value_(value) {
		add(AT_PREDINTER);
		add(AT_TABLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		PredInter* pri = args[0]._value._predinter;
		ElementTuple tuple = toTuple(args[1]._value._table, *printmonitor());
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
