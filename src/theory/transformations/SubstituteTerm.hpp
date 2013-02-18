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

#ifndef SUBSTITUTETERM_HPP_
#define SUBSTITUTETERM_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

class SubstituteTerm: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	Term* _term;
	Variable* _variable;

public:
	template<typename T>
	T execute(T t, Term* term, Variable* v) {
		_term = term;
		_variable = v;
		return t->accept(this);
	}
protected:
	Term* traverse(Term* t) {
		if (t == _term) {
			return new VarTerm(_variable, TermParseInfo());
		} else {
			return t;
		}
	}
};

#endif /* SUBSTITUTETERM_HPP_ */
