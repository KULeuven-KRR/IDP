#ifndef INFERENCES_OPTIMALPROPAGATE_HPP_
#define INFERENCES_OPTIMALPROPAGATE_HPP_

#include <vector>
#include <map>
#include <set>
#include "theory.hpp"
#include "structure.hpp"
#include "vocabulary.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "PropagatorFactory.hpp"

template<typename Structure>
void makeInconsistent(Structure* structure){
	auto voc = structure->vocabulary();
	for(auto i=voc->firstFunc(); i!=voc->lastFunc(); ++i){
		structure->inter((*i).second, InconsistentFuncInterGenerator((*i).second).get(structure));
	}
	for(auto i=voc->firstPred(); i!=voc->lastPred(); ++i){
		structure->inter((*i).second, InconsistentPredInterGenerator((*i).second).get(structure));
	}
}

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements the optimal propagator by computing all models of the theory and then taking the intersection
 */
class OptimalPropagation {
public:
	AbstractStructure* propagate(AbstractTheory* theory, AbstractStructure* structure) {
		// TODO: make a clean version of the implementation
		// TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented)
		// Compute all models
		MinisatID::SolverOption modes;
		modes.nbmodels = 0;
		modes.verbosity = 0;
		//modes.remap = false;
		SATSolver solver(modes);
		Options options("", ParseInfo());
		options.setValue(IntType::NRMODELS, 0);
		auto origoptions = GlobalData::instance()->getOptions();
		GlobalData::instance()->setOptions(&options);
		auto symstructure = generateNaiveApproxBounds(theory, structure);
		GrounderFactory grounderfactory(structure, symstructure);
		Grounder* grounder = grounderfactory.create(theory, &solver);
		grounder->toplevelRun();
		AbstractGroundTheory* grounding = grounder->getGrounding();
		MinisatID::ModelExpandOptions opts;
		opts.nbmodelstofind = options.getValue(IntType::NRMODELS);
		opts.printmodels = MinisatID::PRINT_NONE;
		opts.savemodels = MinisatID::SAVE_ALL;
		opts.search = MinisatID::MODELEXPAND;
		MinisatID::Solution* abstractsolutions = new MinisatID::Solution(opts);
		solver.solve(abstractsolutions);

		std::set<int> intersection;
		if (abstractsolutions->getModels().empty()) {
			AbstractStructure* result = structure->clone();
			makeInconsistent(result);
			return result;
		}
		// Take the intersection of all models
		MinisatID::Model* firstmodel = *(abstractsolutions->getModels().cbegin());
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

		GroundTranslator* translator = grounding->translator();
		AbstractStructure* result = structure->clone();
		for (auto literal = intersection.cbegin(); literal != intersection.cend(); ++literal) {
			int atomnr = (*literal > 0) ? *literal : (-1) * (*literal);
			if (translator->isInputAtom(atomnr)) {
				PFSymbol* symbol = translator->getSymbol(atomnr);
				const ElementTuple& args = translator->getArgs(atomnr);
				if (typeid(*symbol) == typeid(Predicate)) {
					Predicate* pred = dynamic_cast<Predicate*>(symbol);
					if (*literal < 0) {
						result->inter(pred)->makeFalse(args);
					} else {
						result->inter(pred)->makeTrue(args);
					}
				} else {
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
		GlobalData::instance()->setOptions(origoptions);
		return result;
	}
};

#endif /* INFERENCES_OPTIMALPROPAGATE_HPP_ */
