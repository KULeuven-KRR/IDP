/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINTOPTIONS_HPP_
#define PRINTOPTIONS_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "options.hpp"

class PrintOptionInference: public Inference {
public:
	PrintOptionInference(): Inference("tostring") {
		add(AT_OPTIONS);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Options* opts = args[0].options();
		return InternalArgument(StringPointer(opts->toString()));
	}
};

#endif /* PRINTOPTIONS_HPP_ */
