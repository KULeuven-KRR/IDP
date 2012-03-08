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
#include "PropagatorFactory.hpp"

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements the optimal propagator by computing all models of the theory and then taking the intersection
 */
class OptimalPropagation {
public:
	AbstractStructure* propagate(AbstractTheory* theory, AbstractStructure* structure) {
		// TODO: make a clean version of the implementation (should call ModelExpansion)
		// TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented)
		// Compute all models

		//MinisatID solveroptions
		MinisatID::SolverOption modes;
		modes.nbmodels = 0;
		modes.verbosity = 0;
		//modes.remap = false;
		MinisatID::WrappedPCSolver solver(modes);

		//Grounding
		auto symstructure = generateBounds(theory, structure);
		auto grounder = GrounderFactory::create({theory, structure, symstructure}, &solver);
		grounder->toplevelRun();
		auto grounding = grounder->getGrounding();

		//MinisatID modelexpandoptions
		MinisatID::ModelExpandOptions opts;
		opts.nbmodelstofind = 0;
		opts.printmodels = MinisatID::PRINT_NONE;
		opts.savemodels = MinisatID::SAVE_ALL;
		opts.inference = MinisatID::MODELEXPAND;
		MinisatID::Solution* abstractsolutions = new MinisatID::Solution(opts);
		solver.solve(abstractsolutions);

		std::set<int> intersection;
		if (abstractsolutions->getModels().empty()) {
			return new InconsistentStructure(structure->name(), structure->pi());
		}
		// Take the intersection of all models
		auto firstmodel = *(abstractsolutions->getModels().cbegin());
		for (auto it = firstmodel->literalinterpretations.cbegin(); it != firstmodel->literalinterpretations.cend(); ++it) {
			intersection.insert(it->getValue());
		}
		for (auto currmodel = (abstractsolutions->getModels().cbegin()); currmodel != abstractsolutions->getModels().cend(); ++currmodel) {
			for (auto it = (*currmodel)->literalinterpretations.cbegin(); it != (*currmodel)->literalinterpretations.cend(); ++it) {
				if (intersection.find(it->getValue()) == intersection.cend()) {
					intersection.erase((-1) * it->getValue());
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
				if (sametypeid<Predicate>(*symbol)) {
					auto pred = dynamic_cast<Predicate*>(symbol);
					if (*literal < 0) {
						result->inter(pred)->makeFalse(args);
					} else {
						result->inter(pred)->makeTrue(args);
					}
				} else {
					Assert(sametypeid<Function>(*symbol));
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

		return result;
	}
};

#endif /* INFERENCES_OPTIMALPROPAGATE_HPP_ */
