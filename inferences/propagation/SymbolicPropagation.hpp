#ifndef INFERENCES_SYMBOLICPROPAGATE_HPP_
#define INFERENCES_SYMBOLICPROPAGATE_HPP_

#include <vector>
#include <map>
#include <set>
#include "theory.hpp"
#include "structure.hpp"
#include "PropagatorFactory.hpp"

#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements symbolic propagation, followed by an evaluation of the BDDs to obtain a concrete structure
 */
class SymbolicPropagation {
public:
	// TODO: free allocated memory
	AbstractStructure* propagate(AbstractTheory* theory, AbstractStructure* structure) {
		auto mpi = propagateVocabulary(theory, structure);
		auto propagator = createPropagator(theory, mpi);
		propagator->run();
		return propagator->currstructure(structure);
	}

	/** Collect symbolic propagation vocabulary **/
	std::map<PFSymbol*, InitBoundType> propagateVocabulary(AbstractTheory* theory, AbstractStructure* structure) const {
		std::map<PFSymbol*, InitBoundType> mpi;
		Vocabulary* v = theory->vocabulary();
		for (auto it = v->firstPred(); it != v->lastPred(); ++it) {
			auto spi = it->second->nonbuiltins();
			for (auto jt = spi.cbegin(); jt != spi.cend(); ++jt) {
				if (structure->vocabulary()->contains(*jt)) {
					PredInter* pinter = structure->inter(*jt);
					if (pinter->approxTwoValued()) {
						mpi[*jt] = IBT_TWOVAL;
					} else if (pinter->ct()->approxEmpty()) {
						if (pinter->cf()->approxEmpty()) {
							mpi[*jt] = IBT_NONE;
						} else {
							mpi[*jt] = IBT_CF;
						}
					} else if (pinter->cf()->approxEmpty()) {
						mpi[*jt] = IBT_CT;
					} else {
						mpi[*jt] = IBT_BOTH;
					}
				} else {
					mpi[*jt] = IBT_NONE;
				}
			}
		}
		for (auto it = v->firstFunc(); it != v->lastFunc(); ++it) {
			auto sfi = it->second->nonbuiltins();
			for (auto jt = sfi.cbegin(); jt != sfi.cend(); ++jt) {
				if (structure->vocabulary()->contains(*jt)) {
					FuncInter* finter = structure->inter(*jt);
					if (finter->approxTwoValued()) {
						mpi[*jt] = IBT_TWOVAL;
					} else if (finter->graphInter()->ct()->approxEmpty()) {
						if (finter->graphInter()->cf()->approxEmpty()) {
							mpi[*jt] = IBT_NONE;
						} else {
							mpi[*jt] = IBT_CF;
						}
					} else if (finter->graphInter()->cf()->approxEmpty()) {
						mpi[*jt] = IBT_CT;
					} else {
						mpi[*jt] = IBT_BOTH;
					}
				} else {
					mpi[*jt] = IBT_NONE;
				}
			}
		}
		return mpi;
	}
};

#endif /* INFERENCES_SYMBOLICPROPAGATE_HPP_ */
