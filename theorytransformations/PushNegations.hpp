#ifndef PUSHNEGATIONS_HPP_
#define PUSHNEGATIONS_HPP_

#include "TheoryMutatingVisitor.hpp"

class PushNegations: public TheoryMutatingVisitor {
public:
	PushNegations() :
			TheoryMutatingVisitor() {
	}

	Formula* visit(PredForm*);
	Formula* visit(EqChainForm*);
	Formula* visit(EquivForm*);
	Formula* visit(BoolForm*);
	Formula* visit(QuantForm*);

	Formula* traverse(Formula*);
	Term* traverse(Term*);
};

#endif /* PUSHNEGATIONS_HPP_ */
