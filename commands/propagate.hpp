/************************************
	propagate.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PROPAGATE_HPP_
#define PROPAGATE_HPP_

#include <vector>
#include <map>
#include <set>
#include "internalargument.hpp"
#include "theory.hpp"
#include "structure.hpp"
#include "commandinterface.hpp"
#include "monitors/propagatemonitor.hpp"

/**
 * Implements propagation by grounding and applying unit propagation on the ground theory
 */
class GroundPropagateInference : public Inference {
	public:
		GroundPropagateInference() : Inference("groundpropagate") {
			add(AT_THEORY);
			add(AT_STRUCTURE);

		}

		InternalArgument execute(const std::vector<InternalArgument>& args) const {
			AbstractTheory*	theory = args[0].theory();
			AbstractStructure* structure = args[1].structure();

			PropagateMonitor monitor;
			MinisatID::SolverOption modes;
			modes.nbmodels = 0;
			modes.verbosity = 0;
			modes.remap = false;
			SATSolver solver(modes);
			Options options("",ParseInfo());
			options.setValue("nrmodels",0);
			GrounderFactory grounderfactory(structure,&options);
			TopLevelGrounder* grounder = grounderfactory.create(theory,&solver);
			grounder->run();
			AbstractGroundTheory* grounding = grounder->grounding();
			MinisatID::ModelExpandOptions opts;
			opts.nbmodelstofind = options.nrmodels();
			opts.printmodels = MinisatID::PRINT_NONE;
			opts.savemodels = MinisatID::SAVE_ALL;
			opts.search = MinisatID::PROPAGATE;
			MinisatID::Solution* abstractsolutions = new MinisatID::Solution(opts);
			monitor.setSolver(&solver);
			solver.solve(abstractsolutions);

			GroundTranslator* translator = grounding->translator();
			AbstractStructure* result = structure->clone();
			for(auto literal = monitor.model().begin(); literal != monitor.model().end(); ++literal) {
				int atomnr = literal->getAtom().getValue();
				PFSymbol* symbol = translator->atom2symbol(atomnr);
				if(symbol) {
					const ElementTuple& args = translator->args(atomnr);
					if(typeid(*symbol) == typeid(Predicate)) {
						Predicate* pred = dynamic_cast<Predicate*>(symbol);
						if(literal->hasSign()) result->inter(pred)->makeFalse(args);
						else result->inter(pred)->makeTrue(args);
					}
					else {
						Function* func = dynamic_cast<Function*>(symbol);
						if(literal->hasSign()) result->inter(func)->graphinter()->makeFalse(args);
						else result->inter(func)->graphinter()->makeTrue(args);
					}
				}
			}
			result->clean();
			return InternalArgument(result);
		}

};

/**
 * Implements the optimal propagator by computing all models of the theory and then taking the intersection
 */
class OptimalPropagateInference : public Inference {
	public:
		OptimalPropagateInference() : Inference("optimalpropagate") {
			add(AT_THEORY);
			add(AT_STRUCTURE);
		}

		InternalArgument execute(const std::vector<InternalArgument>& args) const {
			AbstractTheory*	theory = args[0].theory();
			AbstractStructure* structure = args[1].structure();

			// TODO: make a clean version of the implementation
			// TODO: doens't work with cp support (because a.o.(?) backtranslation is not implemented)
			// Compute all models
			MinisatID::SolverOption modes;
			modes.nbmodels = 0;
			modes.verbosity = 0;
			modes.remap = false;
			SATSolver solver(modes);
			Options options("",ParseInfo());
			options.setValue("nrmodels",0);
			GrounderFactory grounderfactory(structure,&options);
			TopLevelGrounder* grounder = grounderfactory.create(theory,&solver);
			grounder->run();
			AbstractGroundTheory* grounding = grounder->grounding();
			MinisatID::ModelExpandOptions opts;
			opts.nbmodelstofind = options.nrmodels();
			opts.printmodels = MinisatID::PRINT_NONE;
			opts.savemodels = MinisatID::SAVE_ALL;
			opts.search = MinisatID::MODELEXPAND;
			MinisatID::Solution* abstractsolutions = new MinisatID::Solution(opts);
			solver.solve(abstractsolutions);

			std::set<int> intersection;
			if(abstractsolutions->getModels().empty()) return nilarg();
			else { // Take the intersection of all models
				MinisatID::Model* firstmodel = *(abstractsolutions->getModels().begin());
				for(auto it = firstmodel->literalinterpretations.begin(); 
					it != firstmodel->literalinterpretations.end(); ++it) {
					intersection.insert(it->getValue());
				}
				for(auto currmodel = (abstractsolutions->getModels().begin()); 
					currmodel != abstractsolutions->getModels().end(); ++currmodel) {
					for(auto it = (*currmodel)->literalinterpretations.begin(); 
						it != (*currmodel)->literalinterpretations.end(); ++it) {
						if(intersection.find(it->getValue()) == intersection.end()) {
							intersection.erase((-1) * it->getValue());
						}
					}
				}
			}

			GroundTranslator* translator = grounding->translator();
			AbstractStructure* result = structure->clone();
			for(auto literal = intersection.begin(); literal != intersection.end(); ++literal) {
				int atomnr = *literal > 0 ? *literal : (-1) * (*literal);
				PFSymbol* symbol = translator->atom2symbol(atomnr);
				if(symbol) {
					const ElementTuple& args = translator->args(atomnr);
					if(typeid(*symbol) == typeid(Predicate)) {
						Predicate* pred = dynamic_cast<Predicate*>(symbol);
						if(*literal < 0) result->inter(pred)->makeFalse(args);
						else result->inter(pred)->makeTrue(args);
					}
					else {
						Function* func = dynamic_cast<Function*>(symbol);
						if(*literal < 0) result->inter(func)->graphinter()->makeFalse(args);
						else result->inter(func)->graphinter()->makeTrue(args);
					}
				}
			}
			result->clean();
			return InternalArgument(result);
		}
};

class PropagateInference: public Inference {
public:
	PropagateInference(): Inference("propagate") {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory*	theory = args[0].theory();
		AbstractStructure* structure = args[1].structure();

		std::map<PFSymbol*,InitBoundType> mpi;
		Vocabulary* v = theory->vocabulary();
		for(auto it = v->firstpred(); it != v->lastpred(); ++it) {
			auto spi = it->second->nonbuiltins();
			for(auto jt = spi.begin(); jt != spi.end(); ++jt) {
				if(structure->vocabulary()->contains(*jt)) {
					PredInter* pinter = structure->inter(*jt);
					if(pinter->approxtwovalued()) mpi[*jt] = IBT_TWOVAL;
					else {
						// TODO
						mpi[*jt] = IBT_NONE;
					}
				}
				else mpi[*jt] = IBT_NONE;
			}
		}
		for(auto it = v->firstfunc(); it != v->lastfunc(); ++it) {
			auto sfi = it->second->nonbuiltins();
			for(auto jt = sfi.begin(); jt != sfi.end(); ++jt) {
				if(structure->vocabulary()->contains(*jt)) {
					FuncInter* finter = structure->inter(*jt);
					if(finter->approxtwovalued()) mpi[*jt] = IBT_TWOVAL;
					else {
						// TODO
						mpi[*jt] = IBT_NONE;
					}
				}
				else mpi[*jt] = IBT_NONE;
			}
		}

		FOPropBDDDomainFactory* domainfactory = new FOPropBDDDomainFactory();
		FOPropScheduler* scheduler = new FOPropScheduler();
		FOPropagatorFactory propfactory(domainfactory,scheduler,true,mpi,args[2].options());
		FOPropagator* propagator = propfactory.create(theory);
		propagator->run();

		// TODO: free allocated memory
		// TODO: return a structure (instead of nil)
		return nilarg();
	}
};


#endif /* PROPAGATE_HPP_ */
