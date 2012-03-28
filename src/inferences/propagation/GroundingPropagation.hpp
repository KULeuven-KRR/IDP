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
#include "inferences/modelexpansion/propagatemonitor.hpp"

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
		// TODO: make a clean version of this implementation
		// TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented)
		auto monitor = new PropagateMonitor();

		//Set MinisatID solver options
		auto solver = SolverConnection::createsolver(0);

		//Create and execute grounder
		auto symstructure = generateBounds(theory, structure);
		auto grounder = GrounderFactory::create( { theory, structure, symstructure }, solver);
		monitor->setTranslator(grounder->getTranslator());
		monitor->setSolver(solver);
		grounder->toplevelRun();
		auto grounding = grounder->getGrounding();

		//Set MinisatID mx options
		MinisatID::ModelExpandOptions opts;
		opts.nbmodelstofind = 0;
		opts.printmodels = MinisatID::PRINT_NONE;
		opts.savemodels = MinisatID::SAVE_ALL;
		opts.inference = MinisatID::PROPAGATE;
		MinisatID::Solution* abstractsolutions = new MinisatID::Solution(opts);

		//Execute MinisatID
		solver->solve(abstractsolutions);

		GroundTranslator* translator = grounding->translator();
		AbstractStructure* result = structure->clone();
		// Use the propagation monitor to assert everything that was propagated without search
		for (auto literal = monitor->model().cbegin(); literal != monitor->model().cend(); ++literal) {
			int atomnr = literal->getAtom().getValue();
			if (translator->isInputAtom(atomnr)) {
				PFSymbol* symbol = translator->getSymbol(atomnr);
				const ElementTuple& args = translator->getArgs(atomnr);
				if (sametypeid<Predicate>(*symbol)) {
					Predicate* pred = dynamic_cast<Predicate*>(symbol);
					if (literal->hasSign()) {
						result->inter(pred)->makeFalse(args);
					} else {
						result->inter(pred)->makeTrue(args);
					}
				} else {
					Assert(sametypeid<Function>(*symbol));
					Function* func = dynamic_cast<Function*>(symbol);
					if (literal->hasSign()) {
						result->inter(func)->graphInter()->makeFalse(args);
					} else {
						result->inter(func)->graphInter()->makeTrue(args);
					}
				}
			}
		}
		result->clean();
		delete (monitor);
		delete (solver);
		if (not result->isConsistent()) {
			return std::vector<AbstractStructure*> { };
		}
		return {result};
	}
};

#endif /* INFERENCES_GROUNDINGPROPAGATE_HPP_ */
