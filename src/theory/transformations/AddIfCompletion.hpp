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

#pragma once

#include <vector>
#include <map>
#include "visitors/TheoryVisitor.hpp"

class Variable;
class PFSymbol;

struct OnlyIfFormula{
	PredForm* head;
	Formula* formula;
	int definitionid;
};

class AddIfCompletion: public DefaultTraversingTheoryVisitor{
	VISITORFRIENDS()
private:
	std::vector<OnlyIfFormula> _result;

	std::map<PFSymbol*, std::vector<Variable*> > _headvars;
	std::map<PFSymbol*, std::vector<Formula*> > _interres;

public:
	std::vector<OnlyIfFormula> getOnlyIfFormulas(const Definition & definition);
	std::vector<OnlyIfFormula> getOnlyIfFormulas(const AbstractTheory& theory);

	template<typename T>
	T* execute(T* t){
		_result.clear();
		t->accept(this);
		for(auto r: _result){
			t->add(r.formula);
		}
		return t;
	}

protected:
	void visit(const Theory*);
	void addFor(const Rule& rule );
};
