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

#ifndef PUSHNEGATIONS_HPP_
#define PUSHNEGATIONS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

class PushNegations: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}
protected:
	Formula* visit(PredForm*);
	Formula* visit(EqChainForm*);
	Formula* visit(EquivForm*);
	Formula* visit(BoolForm*);
	Formula* visit(QuantForm*);
};

#endif /* PUSHNEGATIONS_HPP_ */
