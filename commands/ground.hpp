/************************************
	ground.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GROUND_HPP_
#define GROUND_HPP_

#include <vector>
#include <set>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "structure.hpp"
#include "options.hpp"
#include "commands/propagate.hpp"

class GroundInference: public Inference {
	private:
		AbstractTheory* ground(AbstractTheory* theory, AbstractStructure* structure, Options* options) const {
			
			// Symbolic propagation
			PropagateInference propinference;
			std::map<PFSymbol*,InitBoundType> mpi = propinference.propagateVocabulary(theory,structure);
//			FOPropagator* propagator = propinference.createPropagator(theory,mpi,options);
//			propagator->run();
//			SymbolicStructure* symstructure = propagator->symbolicstructure();
			SymbolicStructure* symstructure = NULL; // FIXME add bdd code again
	
//std::cerr << "Computed the following symbolic structure:" << std::endl;
//symstructure->put(std::cerr);
	
			// Grounding
			GrounderFactory factory(structure,options,symstructure);
			TopLevelGrounder* grounder = factory.create(theory);
			grounder->run();
			AbstractGroundTheory* grounding = grounder->grounding();
			delete(grounder);
			return grounding;
		}
	
	public:
		GroundInference(): Inference("ground") {
			add(AT_THEORY);
			add(AT_STRUCTURE);
			add(AT_OPTIONS);
		}
	
		InternalArgument execute(const std::vector<InternalArgument>& args) const {
			AbstractTheory* grounding = ground(args[0].theory(),args[1].structure(),args[2].options());
			return InternalArgument(grounding);
		}
};

// TODO add an option to write to a file instead to stdout.

class GroundAndPrintInference: public Inference {
	private:
		AbstractTheory* ground(AbstractTheory* theory, AbstractStructure* structure, Options* options, InteractivePrintMonitor* monitor) const {
			GrounderFactory factory(structure,options);
			TopLevelGrounder* grounder = factory.create(theory,monitor,options);
			grounder->run();
			AbstractGroundTheory* grounding = grounder->grounding();
			delete(grounder);
			monitor->flush();
			return grounding;
		}
	
	public:
		GroundAndPrintInference(): Inference("printgrounding", true) {
			add(AT_THEORY);
			add(AT_STRUCTURE);
			add(AT_OPTIONS);
		}
	
		InternalArgument execute(const std::vector<InternalArgument>& args) const {
			AbstractTheory* grounding = ground(args[0].theory(),args[1].structure(),args[2].options(), printmonitor());
			return InternalArgument(grounding);
		}
};

#endif /* GROUND_HPP_ */
