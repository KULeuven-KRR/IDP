/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef PROPAGATE_HPP_
#define PROPAGATE_HPP_

#include "commandinterface.hpp"

#include "inferences/propagation/GroundingPropagation.hpp"
#include "inferences/propagation/SymbolicPropagation.hpp"
#include "inferences/propagation/OptimalPropagation.hpp"

/**
 * Implements symbolic propagation, followed by an evaluation of the BDDs to obtain a concrete structure
 */
class PropagateInference: public TheoryStructureBase {
public:
	PropagateInference() :
		TheoryStructureBase("propagate", "Return a structure, made more precise than the input by doing symbolic propagation on the theory.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		SymbolicPropagation propagator;
		return InternalArgument(propagator.propagate(get<0>(args), get<1>(args)));
	}
};

/**
 * Implements propagation by grounding and applying unit propagation on the ground theory
 */
class GroundPropagateInference: public TheoryStructureBase {
public:
	GroundPropagateInference() :
		TheoryStructureBase("groundpropagate", "Return a structure, made more precise than the input by grounding and unit propagation on the theory.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		GroundingPropagation propagator;
		return InternalArgument(propagator.propagate(get<0>(args), get<1>(args)));
	}
};

/**
 * Implements the optimal propagator by computing all models of the theory and then taking the intersection
 */

class OptimalPropagateInference: public TheoryStructureBase {
public:
	OptimalPropagateInference() :
		TheoryStructureBase("optimalpropagate", "Return a structure, made more precise than the input by generating all models and checking which literals always have the same truth value.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		OptimalPropagation propagator;
		return InternalArgument(propagator.propagate(get<0>(args), get<1>(args)));
	}
};

#endif /* PROPAGATE_HPP_ */
