/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINTSTRUCTURE_HPP_
#define PRINTSTRUCTURE_HPP_

#include <vector>
#include "internalargument.hpp"
#include "print.hpp"
#include "structure.hpp"
#include "options.hpp"

class PrintStructureInference: public Inference {
public:
	PrintStructureInference(): Inference("tostring") {
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractStructure* structure = args[0].structure();
		Options* opts = args[1].options();

		Printer* printer = Printer::create(opts);
		std::string str = printer->print(structure);
		delete(printer);

		return InternalArgument(StringPointer(str));
	}
};

#endif /* PRINTSTRUCTURE_HPP_ */
