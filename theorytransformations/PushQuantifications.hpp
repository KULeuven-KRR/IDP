/************************************
  	PushQuantifications.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PUSHQUANTIFICATIONS_HPP_
#define PUSHQUANTIFICATIONS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

class PushQuantifications: public TheoryMutatingVisitor {
public:
	PushQuantifications() :
			TheoryMutatingVisitor() {
	}

	Formula* visit(QuantForm*);
};

#endif /* PUSHQUANTIFICATIONS_HPP_ */
