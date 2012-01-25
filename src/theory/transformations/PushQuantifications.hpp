/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef PUSHQUANTIFICATIONS_HPP_
#define PUSHQUANTIFICATIONS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

class PushQuantifications: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}

protected:
	Formula* visit(QuantForm*);
};

#endif /* PUSHQUANTIFICATIONS_HPP_ */
