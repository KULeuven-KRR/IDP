/************************************
	maketabtrue.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef MAKETABTRUE_HPP_
#define MAKETABTRUE_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

ElementTuple toTuple(std::vector<InternalArgument>* tab) {
	ElementTuple tup;
	for(auto it = tab->begin(); it != tab->end(); ++it) {
		switch(it->_type) {
			case AT_INT:
				tup.push_back(DomainElementFactory::instance()->create(it->_value._int));
				break;
			case AT_DOUBLE:
				tup.push_back(DomainElementFactory::instance()->create(it->_value._double));
				break;
			case AT_STRING:
				tup.push_back(DomainElementFactory::instance()->create(it->_value._string));
				break;
			case AT_COMPOUND:
				tup.push_back(DomainElementFactory::instance()->create(it->_value._compound));
				break;
			//default:
				// FIXME
				//lua_pushstring(L,"Wrong value in a tuple. Expected an integer, double, string, or compound");
				//lua_error(L);
		}
	}
	return tup;
}

class MakeTableTrueInference: public Inference {
public:
	MakeTableTrueInference(): Inference("maketabletrue") {
		add(AT_PREDINTER);
		add(AT_TABLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		PredInter* pri = args[0]._value._predinter;
		pri->makeTrue(toTuple(args[1]._value._table));
		return nilarg();
	}
};

class MakeTableFalseInference: public Inference {
public:
	MakeTableFalseInference(): Inference("maketablefalse") {
		add(AT_PREDINTER);
		add(AT_TABLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		PredInter* pri = args[0]._value._predinter;
		pri->makeFalse(toTuple(args[1]._value._table));
		return nilarg();
	}
};

class MakeTableUnknownInference: public Inference {
public:
	MakeTableUnknownInference(): Inference("maketableunknown") {
		add(AT_PREDINTER);
		add(AT_TABLE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		PredInter* pri = args[0]._value._predinter;
		pri->makeUnknown(toTuple(args[1]._value._table));
		return nilarg();
	}
};


#endif /* MAKETABTRUE_HPP_ */
