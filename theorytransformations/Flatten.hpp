/************************************
	Flatten.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FLATTEN_HPP_
#define FLATTEN_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

class Flatten: public TheoryMutatingVisitor {
public:
	Flatten() :
			TheoryMutatingVisitor() {
	}

	Formula* visit(BoolForm*);
	Formula* visit(QuantForm*);
};

#endif /* FLATTEN_HPP_ */
