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

#ifndef CHECKSORTS_HPP_
#define CHECKSORTS_HPP_

#include "visitors/TheoryVisitor.hpp"

class Vocabulary;

class CheckSorts: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	Vocabulary* _vocab;

public:
	template<typename T>
	void execute(T f, Vocabulary* v) {
		_vocab = v;
		f->accept(this);
	}

protected:
	void visit(const PredForm*);
	void visit(const EqChainForm*);
	void visit(const FuncTerm*);
	void visit(const AggTerm*);
};

#endif /* CHECKSORTS_HPP_ */
