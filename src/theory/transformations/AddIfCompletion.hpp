/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#pragma once

#include <vector>
#include <map>
#include "visitors/TheoryMutatingVisitor.hpp"

class Variable;
class PFSymbol;

class AddIfCompletion: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::map<PFSymbol*, std::vector<Variable*> > _headvars;
	std::map<PFSymbol*, std::vector<Formula*> > _interres;

public:
	std::vector<std::pair<PFSymbol*, Formula*> > getOnlyIfFormulas(const Definition & definition);
	template<typename T>
	T* execute(T* t){
		return t->accept(this);
	}

protected:
	Theory* visit(Theory*);
	void addFor(const Rule& rule );
};
