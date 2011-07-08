/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef NEWOPTIONS_HPP_
#define NEWOPTIONS_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "options.hpp"

class NewOptionsInference: public Inference {
public:
	NewOptionsInference(): Inference("newoptions") {
	}

	InternalArgument execute(const std::vector<InternalArgument>&) const {
		Options* opts = new Options("",ParseInfo());
		return InternalArgument(opts);
	}
};

#endif /* NEWOPTIONS_HPP_ */
