#ifndef FLATTEN_HPP_
#define FLATTEN_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

class Flatten: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t){
		return t->accept(this);
	}
protected:
	Formula* visit(BoolForm*);
	Formula* visit(QuantForm*);
};

#endif /* FLATTEN_HPP_ */
