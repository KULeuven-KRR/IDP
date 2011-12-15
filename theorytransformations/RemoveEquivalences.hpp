/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef REMOVEEQUIVALENCES_HPP_
#define REMOVEEQUIVALENCES_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

#include "theory.hpp"

class RemoveEquivalences: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t){
		return t->accept(this);
	}

protected:
	BoolForm* visit(EquivForm*);
};

#endif /* REMOVEEQUIVALENCES_HPP_ */
