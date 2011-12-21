/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef PRINTSTRUCTURE_HPP_
#define PRINTSTRUCTURE_HPP_

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
	auto printer = Printer::create<std::stringstream>(stream);
	printer->startTheory();
	printer->print(o);
	printer->endTheory();
	return stream.str();
}

class PrintStructureInference: public TypedInference<LIST(AbstractStructure*)> {
public:
	PrintStructureInference(): TypedInference("tostring", "Prints the given structure.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto structure = get<0>(args);
		structure->materialize();
		structure->clean();
		return InternalArgument(StringPointer(print(structure)));
	}
};

class PrintNamespaceInference: public TypedInference<LIST(Namespace*)> {
public:
	PrintNamespaceInference(): TypedInference("tostring", "Prints the given namespace.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto space = get<0>(args);
		return InternalArgument(StringPointer(print(space)));
	}
};

class PrintTheoryInference: public TypedInference<LIST(AbstractTheory*)> {
public:
	PrintTheoryInference(): TypedInference("tostring", "Prints the given theory.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto theory = get<0>(args);
		return InternalArgument(StringPointer(print(theory)));
	}
};

class PrintFormulaInference: public TypedInference<LIST(Formula*)> {
public:
	PrintFormulaInference(): TypedInference("tostring", "Prints the given formula.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto f = get<0>(args);
		return InternalArgument(StringPointer(print(f)));
	}
};

class PrintVocabularyInference: public TypedInference<LIST(Vocabulary*)> {
public:
	PrintVocabularyInference(): TypedInference("tostring", "Prints the given vocabulary.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto vocabulary = get<0>(args);
		return InternalArgument(StringPointer(print(vocabulary)));
	}
};

#endif /* PRINTSTRUCTURE_HPP_ */
