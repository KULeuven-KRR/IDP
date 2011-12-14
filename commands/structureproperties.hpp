/************************************
 calculatedefinitions.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef STRUCTPROPERTIES_HPP_
#define STRUCTPROPERTIES_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "theory.hpp"
#include "structure.hpp"

class IsConsistentInference: public Inference {
public:
	IsConsistentInference() :
			Inference("isconsistent") {
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		bool consistent = args[0].structure()->isConsistent();
		return InternalArgument(consistent?1:0);
	}
};

#endif /* STRUCTPROPERTIES_HPP_ */
