/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef CONTAINMENTCHECKER_HPP_
#define CONTAINMENTCHECKER_HPP_

#include "visitors/TheoryVisitor.hpp"

class PFSymbol;

class CheckContainment: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	const PFSymbol* _symbol;
	bool _result;

public:
	bool execute(const PFSymbol* s, const Formula* f);

protected:
	void visit(const PredForm* pf);
	void visit(const FuncTerm* ft);
};

#endif /* CONTAINMENTCHECKER_HPP_ */
