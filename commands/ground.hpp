#ifndef GROUND_HPP_
#define GROUND_HPP_

#include <vector>
#include <set>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "structure.hpp"
#include "options.hpp"
#include "inferences/propagation/SymbolicPropagation.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/GrounderFactory.hpp"

class GroundInference: public Inference {
private:
	AbstractTheory* ground(AbstractTheory* theory, AbstractStructure* structure, Options* options) const {
		// Symbolic propagation
		SymbolicPropagation propinference;
		std::map<PFSymbol*,InitBoundType> mpi = propinference.propagateVocabulary(theory,structure);
		auto propagator = createPropagator(theory,mpi);
		propagator->run();
		SymbolicStructure* symstructure = propagator->symbolicstructure();

		// Grounding
		GrounderFactory factory(structure,options,symstructure);
		Grounder* grounder = factory.create(theory);
		grounder->toplevelRun();
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
		Grounder* grounder = factory.create(theory,monitor,options);
		grounder->toplevelRun();
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
