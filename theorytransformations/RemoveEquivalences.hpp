#ifndef REMOVEEQUIVALENCES_HPP_
#define REMOVEEQUIVALENCES_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

#include "theory.hpp"

class RemoveEquivalences: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t){
		return t->accept(this);
	}

protected:
	BoolForm* visit(EquivForm*);
};

#endif /* REMOVEEQUIVALENCES_HPP_ */
