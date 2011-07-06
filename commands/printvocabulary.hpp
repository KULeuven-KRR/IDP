/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINTVOCABULARY_HPP_
#define PRINTVOCABULARY_HPP_

#include <vector>
#include "internalargument.hpp"
#include "print.hpp"
#include "vocabulary.hpp"
#include "options.hpp"

class PrintVocabularyInference: public Inference {
public:
	PrintVocabularyInference(): Inference("tostring") {
		add(AT_VOCABULARY);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Vocabulary* vocabulary = args[0].vocabulary();
		Options* opts = args[1].options();

		Printer* printer = Printer::create(opts);
		std::string str = printer->print(vocabulary);
		delete(printer);

		return InternalArgument(StringPointer(str));
	}
};

#endif /* PRINTVOCABULARY_HPP_ */
