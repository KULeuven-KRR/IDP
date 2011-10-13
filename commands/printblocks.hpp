/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINTSTRUCTURE_HPP_
#define PRINTSTRUCTURE_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "printers/print.hpp"
#include "structure.hpp"
#include "theory.hpp"
#include "namespace.hpp"
#include "options.hpp"
#include "vocabulary.hpp"

template<class Object>
std::string print(Object o, Options* opts){
	std::stringstream stream;
	Printer* printer = Printer::create<std::stringstream>(opts, stream);
	printer->startTheory();
	printer->visit(o);
	printer->endTheory();
	return stream.str();
}

class PrintStructureInference: public Inference {
public:
	PrintStructureInference(): Inference("tostring") {
		add(AT_STRUCTURE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractStructure* structure = args[0].structure();
		Options* opts = args[1].options();
		structure->materialize();
		structure->clean();
		return InternalArgument(StringPointer(print(structure, opts)));
	}
};

class PrintNamespaceInference: public Inference {
public:
	PrintNamespaceInference(): Inference("tostring") {
		add(AT_NAMESPACE);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Namespace* space = args[0].space();
		Options* opts = args[1].options();
		return InternalArgument(StringPointer(print(space, opts)));
	}
};

class PrintTheoryInference: public Inference {
public:
	PrintTheoryInference(): Inference("tostring") {
		add(AT_THEORY);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		AbstractTheory* theory = args[0].theory();
		Options* opts = args[1].options();
		return InternalArgument(StringPointer(print(theory, opts)));
	}
};

class PrintFormulaInference: public Inference {
public:
	PrintFormulaInference(): Inference("tostring") {
		add(AT_FORMULA);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Formula* f = args[0]._value._formula;
		Options* opts = args[1].options();
		return InternalArgument(StringPointer(print(f, opts)));
	}
};

class PrintVocabularyInference: public Inference {
public:
	PrintVocabularyInference(): Inference("tostring") {
		add(AT_VOCABULARY);
		add(AT_OPTIONS);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Vocabulary* vocabulary = args[0].vocabulary();
		Options* opts = args[1].options();
		return InternalArgument(StringPointer(print(vocabulary, opts)));
	}
};

#endif /* PRINTSTRUCTURE_HPP_ */
