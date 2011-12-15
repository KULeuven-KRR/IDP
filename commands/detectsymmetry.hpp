/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

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
