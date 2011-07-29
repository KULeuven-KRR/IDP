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
	ToBDDInference(): Inference("bddstring") {
		add(AT_FORMULA);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Formula* f = dynamic_cast<Formula*>(args[0]._value._formula);
		FOBDDManager manager;
		FOBDDFactory m(&manager);
		f->accept(&m);
		const FOBDD* bdd = m.bdd();
		std::stringstream sstr;
		manager.put(sstr,bdd);
		InternalArgument ia; ia._type = AT_STRING;
		ia._value._string = StringPointer(sstr.str());
		return ia;
	}
};

#endif /* TOBDD_HPP_ */
