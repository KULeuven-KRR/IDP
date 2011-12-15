/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef TOBDD_HPP_
#define TOBDD_HPP_

#include <vector>
#include <sstream>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"

/**
 * Class to convert a formula to a bdd and return a string representation of the bdd
 * Probably only useful for debugging purposes
 */
class ToBDDInference: public Inference {
public:
	ToBDDInference(ArgType at): Inference("bddstring") { add(at); }

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Formula* f;
		if(getArgumentTypes()[0] == AT_FORMULA) {
			f = dynamic_cast<Formula*>(args[0]._value._formula);
		} else {
			f = args[0]._value._query->query();
		}

		FOBDDManager manager;
		FOBDDFactory m(&manager);
		const FOBDD* bdd = m.turnIntoBdd(f);

		std::stringstream sstr;
		manager.put(sstr,bdd);
		InternalArgument ia; ia._type = AT_STRING;
		ia._value._string = StringPointer(sstr.str());
		return ia;
	}
};

#endif /* TOBDD_HPP_ */
