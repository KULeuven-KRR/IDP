/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINTNAMESPACE_HPP_
#define PRINTNAMESPACE_HPP_

#include <vector>
#include "internalargument.hpp"
#include "options.hpp"
#include "namespace.hpp"
#include "print.hpp"

class PrintNamespaceInference: public Inference {
public:
	PrintNamespaceInference(): Inference("tostring") {
		add(AT_NAMESPACE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Namespace* space = args[0].space();
		Options* opts = args[1].options();

		Printer* printer = Printer::create(opts);
		std::string str = printer->print(space);
		delete(printer);

		return InternalArgument(StringPointer(str));;
	}
};

#endif /* PRINTNAMESPACE_HPP_ */
