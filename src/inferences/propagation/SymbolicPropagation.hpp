/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once

#include "IncludeComponents.hpp"
#include "PropagatorFactory.hpp"
#include "Propagator.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements symbolic propagation, followed by an evaluation of the BDDs to obtain a concrete structure
 */
class SymbolicPropagation {
public:
	// TODO: free allocated memory
	std::vector<Structure*> propagate(AbstractTheory* theory, Structure* structure) {
		auto result = structure->clone();
		auto mpi = propagateVocabulary(theory, result);
		auto propagator = createPropagator(theory, structure, mpi);
		propagator->doPropagation();
		propagator->applyPropagationToStructure(result, *result->vocabulary());
		return {result};
	}
};
