/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef DETECTSYMMETRY_HPP_
#define DETECTSYMMETRY_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "symmetry.hpp"

class DetectSymmetryInference: public Inference {
public:
	DetectSymmetryInference(): Inference("detectsymmetry") {
		add(AT_THEORY);
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const{
		const AbstractTheory* t = args[0].theory();
		const AbstractStructure* s = args[1].structure();
		std::vector<const IVSet*> temp = findIVSets(t, s);
		InternalArgument result = StringPointer("oke");
		return result;
	}
};

#endif /* DETECTSYMMETRY_HPP_ */
