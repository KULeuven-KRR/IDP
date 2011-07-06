/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINT_HPP_
#define PRINT_HPP_

#include <vector>
#include "internalargument.hpp"
#include "print.hpp"
#include "theory.hpp"
#include "options.hpp"

class PrintFormulaInference: public Inference {
public:
	PrintFormulaInference(): Inference("tostring") {
		add(AT_FORMULA);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Formula* f = args[0]._value._formula;
		Options* opts = args[1].options();
		Printer* printer = Printer::create(opts);
		std::string str = printer->print(f);
		delete(printer);
		return InternalArgument(StringPointer(str));
	}
};

#endif /* PRINT_HPP_ */
