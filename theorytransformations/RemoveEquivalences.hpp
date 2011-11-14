/************************************
  	RemoveEquivalences.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef REMOVEEQUIVALENCES_HPP_
#define REMOVEEQUIVALENCES_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

#include "theory.hpp"

class RemoveEquivalences: public TheoryMutatingVisitor {
public:
	RemoveEquivalences() :
			TheoryMutatingVisitor() {
	}

	BoolForm* visit(EquivForm*);
};

#endif /* REMOVEEQUIVALENCES_HPP_ */
