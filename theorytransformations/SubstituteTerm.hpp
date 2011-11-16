/************************************
	SubstituteTerm.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SUBSTITUTETERM_HPP_
#define SUBSTITUTETERM_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

class SubstituteTerm: public TheoryMutatingVisitor {
private:
	Term* _term;
	Variable* _variable;

	Term* traverse(Term* t) {
		if (t == _term) {
			return new VarTerm(_variable, TermParseInfo());
		} else {
			return t;
		}
	}
public:
	SubstituteTerm(Term* t, Variable* v) :
			_term(t), _variable(v) {
	}
};


#endif /* SUBSTITUTETERM_HPP_ */
