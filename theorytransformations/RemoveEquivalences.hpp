#ifndef REMOVEEQUIVALENCES_HPP_
#define REMOVEEQUIVALENCES_HPP_

#include "TheoryMutatingVisitor.hpp"

#include "theory.hpp"

class RemoveEquivalences: public TheoryMutatingVisitor {
public:
	RemoveEquivalences() :
			TheoryMutatingVisitor() {
	}

	BoolForm* visit(EquivForm*);
};

#endif /* REMOVEEQUIVALENCES_HPP_ */
