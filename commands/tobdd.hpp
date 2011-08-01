/************************************
	tobdd.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef TOBDD_HPP_
#define TOBDD_HPP_

#include <vector>
#include <sstream>
#include "commandinterface.hpp"
#include "theory.hpp"

/**
 * Class to convert a formula to a bdd and return a string representation of the bdd
 * Probably only useful for debugging purposes
 */
class ToBDDInference: public Inference {
public:
	ToBDDInference(ArgType at): Inference("bddstring") { add(at); }

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Formula* f;
		if(getArgumentTypes()[0] == AT_FORMULA) f = dynamic_cast<Formula*>(args[0]._value._formula);
		else f = args[0]._value._query->query();

		FOBDDManager manager;
		FOBDDFactory m(&manager);
		const FOBDD* bdd = m.run(f);

		std::stringstream sstr;
		manager.put(sstr,bdd);
		InternalArgument ia; ia._type = AT_STRING;
		ia._value._string = StringPointer(sstr.str());
		return ia;
	}
};

#endif /* TOBDD_HPP_ */
