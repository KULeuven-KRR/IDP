/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef INFERENCES_GROUNDINGPROPAGATE_HPP_
#define INFERENCES_GROUNDINGPROPAGATE_HPP_

#include "IncludeComponents.hpp"
#include "inferences/modelexpansion/PropagateMonitor.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "PropagatorFactory.hpp"

#include "inferences/SolverConnection.hpp"

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements propagation by grounding and applying unit propagation on the ground theory
 */
class GroundingPropagation {
public:
	 std::vector<AbstractStructure*> propagate(AbstractTheory* theory, AbstractStructure* structure) {
		// TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented)

		//Set MinisatID solver options
		auto data = SolverConnection::createsolver(0);

		//Create and execute grounder
		auto symstructure = generateBounds(theory, structure);
		auto grounder = GrounderFactory::create({theory, structure, symstructure}, data);
		grounder->toplevelRun();
		auto grounding = grounder->getGrounding();

		auto mx = SolverConnection::initpropsolution(data);
		mx->execute();

		auto translator = grounding->translator();
		auto result = structure->clone();
		auto entailed = mx->getEntailedLiterals();
		for (auto literal = entailed.cbegin(); literal < entailed.cend(); ++literal) {
			int atomnr = var(*literal);
			if (translator->isInputAtom(atomnr)) {
				auto symbol = translator->getSymbol(atomnr);
				auto args = translator->getArgs(atomnr);
				if (isa<Predicate>(*symbol)) {
					auto pred = dynamic_cast<Predicate*>(symbol);
					if (sign(*literal)) {
						result->inter(pred)->makeFalse(args);
					} else {
						result->inter(pred)->makeTrue(args);
					}
				} else {
					Assert(isa<Function>(*symbol));
					auto func = dynamic_cast<Function*>(symbol);
					if (sign(*literal)) {
						result->inter(func)->graphInter()->makeFalse(args);
					} else {
						result->inter(func)->graphInter()->makeTrue(args);
					}
				}
			}
		}
		result->clean();
		delete (data);
		delete (mx);

		if (not result->isConsistent()) {
			return std::vector<AbstractStructure*> { };
		}
		return {result};
	}
};

#endif /* INFERENCES_GROUNDINGPROPAGATE_HPP_ */
