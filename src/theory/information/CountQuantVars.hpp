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

#include "visitors/TheoryVisitor.hpp"
#include "vocabulary/vocabulary.hpp"

class PFSymbol;

class CountQuantVars: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	int count;

public:
	template<class T>
	int execute(const T* f) {
		count = 0;
		f->accept(this);
		return count;
	}

protected:
	void visit(const QuantForm* f) {
		count+=f->quantVars().size();
		f->subformula()->accept(this);
	}

	void visit(const QuantSetExpr* f) {
		count+=f->quantVars().size();
		f->getCondition()->accept(this);
	}

	void visit(const Rule* f){
		count+=f->quantVars().size();
		f->body()->accept(this);
	}
};
