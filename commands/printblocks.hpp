/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

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
std::string print(Object o){
	std::stringstream stream;
	Printer* printer = Printer::create<std::stringstream>(stream);
	printer->startTheory();
	printer->print(o);
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
		getGlobal()->setOptions(args[1].options());
		structure->materialize();
		structure->clean();
		return InternalArgument(StringPointer(print(structure)));
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
		getGlobal()->setOptions(args[1].options());
		return InternalArgument(StringPointer(print(space)));
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
		getGlobal()->setOptions(args[1].options());
		return InternalArgument(StringPointer(print(theory)));
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
		getGlobal()->setOptions(args[1].options());
		return InternalArgument(StringPointer(print(f)));
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
		getGlobal()->setOptions(args[1].options());
		return InternalArgument(StringPointer(print(vocabulary)));
	}
};

#endif /* PRINTSTRUCTURE_HPP_ */
