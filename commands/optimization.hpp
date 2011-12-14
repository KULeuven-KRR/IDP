/************************************
	optimization.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef OPTIMIZATION_HPP_
#define OPTIMIZATION_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"

//TODO this as a separate inference seems a bit strange, you might want to ground an optimization problem,
// write out its grounding, do model expansion on it, ...

class OptimizationInference: public Inference {
public:
	OptimizationInference(): Inference("optimize") {
		// TODO
	}

	InternalArgument execute(const std::vector<InternalArgument>& /* args */) const {
		throw notyetimplemented("Optimization inference.");
	}
};

#endif /* OPTIMIZATION_HPP_ */
