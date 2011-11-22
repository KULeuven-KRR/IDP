#ifndef PROPAGATE_HPP_
#define PROPAGATE_HPP_

#include "commandinterface.hpp"

#include "inferences/propagation/GroundingPropagation.hpp"
#include "inferences/propagation/SymbolicPropagation.hpp"
#include "inferences/propagation/OptimalPropagation.hpp"

template<typename Propagator>
InternalArgument executePropagation(Propagator& propagator, const std::vector<InternalArgument>& args){
	AbstractTheory* theory = args[0].theory();
	AbstractStructure* structure = args[1].structure();

	GlobalData::instance()->setOptions(args[2].options());

	AbstractStructure* result = propagator.propagate(theory, structure);

	GlobalData::instance()->resetOptions();

	return InternalArgument(result);
}

/**
 * Implements symbolic propagation, followed by an evaluation of the BDDs to obtain a concrete structure
 */
class PropagateInference: public Inference {
public:
	PropagateInference() :
			Inference("propagate") {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		SymbolicPropagation propagator;
		return executePropagation(propagator, args);
	}
};

/**
 * Implements propagation by grounding and applying unit propagation on the ground theory
 */
class GroundPropagateInference: public Inference {
public:
	GroundPropagateInference() :
			Inference("groundpropagate") {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		GroundingPropagation propagator;
		return executePropagation(propagator, args);
	}
};

/**
 * Implements the optimal propagator by computing all models of the theory and then taking the intersection
 */
class OptimalPropagateInference: public Inference {
public:
	OptimalPropagateInference() :
			Inference("optimalpropagate") {
		add(AT_THEORY);
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		OptimalPropagation propagator;
		return executePropagation(propagator, args);
	}
};

#endif /* PROPAGATE_HPP_ */
