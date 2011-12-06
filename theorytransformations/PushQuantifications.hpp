#ifndef PUSHQUANTIFICATIONS_HPP_
#define PUSHQUANTIFICATIONS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

class PushQuantifications: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t){
		return t->accept(this);
	}

protected:
	Formula* visit(QuantForm*);
};

#endif /* PUSHQUANTIFICATIONS_HPP_ */
