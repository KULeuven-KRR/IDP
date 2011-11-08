#ifndef MOVEQUANTIFICATIONS_HPP_
#define MOVEQUANTIFICATIONS_HPP_

#include "TheoryMutatingVisitor.hpp"

class PushQuantifications: public TheoryMutatingVisitor {
public:
	PushQuantifications() :
			TheoryMutatingVisitor() {
	}

	Formula* visit(QuantForm*);
};

#endif /* MOVEQUANTIFICATIONS_HPP_ */
