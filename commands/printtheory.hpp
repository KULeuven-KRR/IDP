/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINTTHEORY_HPP_
#define PRINTTHEORY_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "print.hpp"
#include "theory.hpp"
#include "options.hpp"

class PrintTheoryInference: public Inference {
public:
	PrintTheoryInference(): Inference("tostring") {
		add(AT_THEORY);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* theory = args[0].theory();
		Options* opts = args[1].options();

		Printer* printer = Printer::create(opts);
		std::string str = printer->print(theory);
		delete(printer);

		return InternalArgument(StringPointer(str));
	}
};

#endif /* PRINTTHEORY_HPP_ */
