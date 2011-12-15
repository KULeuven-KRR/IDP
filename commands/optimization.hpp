/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

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
