/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef ADDCOMPLETION_HPP_
#define ADDCOMPLETION_HPP_

#include <vector>
#include <map>
#include "visitors/TheoryMutatingVisitor.hpp"

class Variable;
class PFSymbol;

class AddCompletion: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::vector<Formula*> _result;
	std::map<PFSymbol*, std::vector<Variable*> > _headvars;
	std::map<PFSymbol*, std::vector<Formula*> > _interres;

public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}

protected:
	Theory* visit(Theory*);
	Definition* visit(Definition*);
	Rule* visit(Rule*);
};

#endif /* ADDCOMPLETION_HPP_ */
