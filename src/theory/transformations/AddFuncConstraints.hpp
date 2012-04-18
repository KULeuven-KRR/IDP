/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef ADDFUNCCON_HPP_
#define ADDFUNCCON_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"
#include <set>
class Function;

class AddFuncConstraints: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::set<Function*> _symbols;

public:

	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}

protected:
	Theory* visit(Theory*);

	Term* visit(FuncTerm* ft);

	Formula* visit(PredForm* pf);
};
#endif /* ADDFUNCCON_HPP_ */
