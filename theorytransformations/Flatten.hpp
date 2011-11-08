#ifndef FLATTEN_HPP_
#define FLATTEN_HPP_

#include "TheoryMutatingVisitor.hpp"

class Flatten: public TheoryMutatingVisitor {
public:
	Flatten() :
			TheoryMutatingVisitor() {
	}

	Formula* visit(BoolForm*);
	Formula* visit(QuantForm*);
};

#endif /* FLATTEN_HPP_ */
