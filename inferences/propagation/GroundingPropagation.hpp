#ifndef INFERENCES_GROUNDINGPROPAGATE_HPP_
#define INFERENCES_GROUNDINGPROPAGATE_HPP_

#include <vector>
#include <map>
#include <set>
#include "theory.hpp"
#include "structure.hpp"
#include "monitors/propagatemonitor.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements propagation by grounding and applying unit propagation on the ground theory
 */
class GroundingPropagation {
public:
	AbstractStructure* propagate(AbstractTheory* theory, AbstractStructure* structure) {
		// TODO: make a clean version of this implementation
		// TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented)
		auto monitor = new PropagateMonitor();
		MinisatID::SolverOption modes;
		modes.nbmodels = 0;
		modes.verbosity = 0;
		//modes.remap = false;
		MinisatID::SATSolver* solver = new SATSolver(modes);
		Options options("", ParseInfo());
		options.setValue(IntType::NRMODELS, 0);
		GrounderFactory grounderfactory(structure, &options);
		auto grounder = grounderfactory.create(theory, solver);
		grounder->toplevelRun();
		auto grounding = grounder->grounding();
		MinisatID::ModelExpandOptions opts;
		opts.nbmodelstofind = options.getValue(IntType::NRMODELS);
		opts.printmodels = MinisatID::PRINT_NONE;
		opts.savemodels = MinisatID::SAVE_ALL;
		opts.search = MinisatID::PROPAGATE;
		MinisatID::Solution* abstractsolutions = new MinisatID::Solution(opts);
		monitor->setSolver(solver);
		solver->solve(abstractsolutions);

		GroundTranslator* translator = grounding->translator();
		AbstractStructure* result = structure->clone();
		for (auto literal = monitor->model().cbegin(); literal != monitor->model().cend(); ++literal) {
			int atomnr = literal->getAtom().getValue();

			if (translator->isInputAtom(atomnr)) {
				PFSymbol* symbol = translator->getSymbol(atomnr);
				const ElementTuple& args = translator->getArgs(atomnr);
				if (typeid(*symbol) == typeid(Predicate)) {
					Predicate* pred = dynamic_cast<Predicate*>(symbol);
					if (literal->hasSign()) {
						result->inter(pred)->makeFalse(args);
					} else {
						result->inter(pred)->makeTrue(args);
					}
				} else {
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

		return result;
	}
};

#endif /* INFERENCES_GROUNDINGPROPAGATE_HPP_ */
