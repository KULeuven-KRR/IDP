#ifndef PUSHNEGATIONS_HPP_
#define PUSHNEGATIONS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

class PushNegations: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t){
		return t->accept(this);
	}
protected:
	Formula* visit(PredForm*);
	Formula* visit(EqChainForm*);
	Formula* visit(EquivForm*);
	Formula* visit(BoolForm*);
	Formula* visit(QuantForm*);

	Formula* traverse(Formula*);
	Term* traverse(Term*);
};

#endif /* PUSHNEGATIONS_HPP_ */
