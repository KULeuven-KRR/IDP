/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef INFERENCES_OPTIMALPROPAGATE_HPP_
#define INFERENCES_OPTIMALPROPAGATE_HPP_

#include "IncludeComponents.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/SolverConnection.hpp"
#include "PropagatorFactory.hpp"

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements the optimal propagator by computing all models of the theory and then taking the intersection
 */
class OptimalPropagation {
public:
	 std::vector<AbstractStructure*>  propagate(AbstractTheory* theory, AbstractStructure* structure) {
		// TODO: make a clean version of the implementation (should call ModelExpansion)
		// TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented)
		// Compute all models

		auto data = SolverConnection::createsolver(0);

		//Grounding
		auto symstructure = generateBounds(theory, structure);
		auto grounder = GrounderFactory::create({theory, structure, symstructure, false /*TODO CHeck*/}, data);
		grounder->toplevelRun();
		auto grounding = grounder->getGrounding();

		auto mx = SolverConnection::initsolution(data, 0);
		mx->execute();

		auto abstractsolutions = mx->getSolutions();

		std::set<int> intersection;
		if (abstractsolutions.empty()) {
			return std::vector<AbstractStructure*> { };
		}
		// Take the intersection of all models
		auto firstmodel = *(abstractsolutions.cbegin());
		for (auto it = firstmodel->literalinterpretations.cbegin(); it != firstmodel->literalinterpretations.cend(); ++it) {
			intersection.insert(getIntLit(*it));
		}
		for (auto currmodel = (abstractsolutions.cbegin()); currmodel != abstractsolutions.cend(); ++currmodel) {
			for (auto it = (*currmodel)->literalinterpretations.cbegin(); it != (*currmodel)->literalinterpretations.cend(); ++it) {
				if (intersection.find(getIntLit(*it)) == intersection.cend()) {
					intersection.erase(-1*getIntLit(*it));
				}
			}
		}

		//Translate the result
		auto translator = grounding->translator();
		auto result = structure->clone();
		for (auto literal = intersection.cbegin(); literal != intersection.cend(); ++literal) {
			int atomnr = (*literal > 0) ? *literal : (-1) * (*literal);
			if (translator->isInputAtom(atomnr)) {
				auto symbol = translator->getSymbol(atomnr);
				const ElementTuple& args = translator->getArgs(atomnr);
				if (isa<Predicate>(*symbol)) {
					auto pred = dynamic_cast<Predicate*>(symbol);
					if (*literal < 0) {
						result->inter(pred)->makeFalse(args);
					} else {
						result->inter(pred)->makeTrue(args);
					}
				} else {
					Assert(isa<Function>(*symbol));
					Function* func = dynamic_cast<Function*>(symbol);
					if (*literal < 0) {
						result->inter(func)->graphInter()->makeFalse(args);
					} else {
						result->inter(func)->graphInter()->makeTrue(args);
					}
				}
			}
		}
		result->clean();

		delete (grounder);
		grounding->recursiveDelete();
		delete (symstructure);
		delete(mx);
		delete(data);

		if(not result->isConsistent()){
			return std::vector<AbstractStructure*> { };
		}
		return {result};
	}
};

#endif /* INFERENCES_OPTIMALPROPAGATE_HPP_ */
