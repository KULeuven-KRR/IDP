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
					if(pinter->approxTwoValued()) mpi[*jt] = IBT_TWOVAL;
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
					if(finter->approxTwoValued()) mpi[*jt] = IBT_TWOVAL;
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
