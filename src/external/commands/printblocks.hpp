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
#include "IncludeComponents.hpp"
#include "theory/Query.hpp"

#include "options.hpp"

template<class Object>
std::string print(Object o) {
	std::stringstream stream;
	auto printer = Printer::create<std::stringstream>(stream);
	printer->startTheory();
	printer->print(o);
	printer->endTheory();
	delete(printer);
	return stream.str();
}

typedef TypedInference<LIST(AbstractStructure*)> PrintStructureInferenceBase;
class PrintStructureInference: public PrintStructureInferenceBase {
public:
	PrintStructureInference()
			: PrintStructureInferenceBase("tostring", "Prints the given structure.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto structure = get<0>(args);
		return InternalArgument(StringPointer(print(structure)));
	}
};

typedef TypedInference<LIST(Namespace*)> PrintNamespaceInferenceBase;
class PrintNamespaceInference: public PrintNamespaceInferenceBase {
public:
	PrintNamespaceInference()
			: PrintNamespaceInferenceBase("tostring", "Prints the given namespace.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto space = get<0>(args);
		return InternalArgument(StringPointer(print(space)));
	}
};

typedef TypedInference<LIST(AbstractTheory*)> PrintTheoryInferenceBase;
class PrintTheoryInference: public PrintTheoryInferenceBase {
public:
	PrintTheoryInference()
			: PrintTheoryInferenceBase("tostring", "Prints the given theory.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto theory = get<0>(args);
		return InternalArgument(StringPointer(print(theory)));
	}
};

typedef TypedInference<LIST(Query*)> PrintQueryInferenceBase;
class PrintQueryInference: public PrintQueryInferenceBase {
public:
	PrintQueryInference()
			: PrintQueryInferenceBase("tostring", "Prints the given query.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto query = get<0>(args);
		return InternalArgument(StringPointer(print(query)));
	}
};

typedef TypedInference<LIST(Term*)> PrintTermInferenceBase;
class PrintTermInference: public PrintTermInferenceBase {
public:
	PrintTermInference()
			: PrintTermInferenceBase("tostring", "Prints the given term.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto term = get<0>(args);
		return InternalArgument(StringPointer(print(term)));
	}
};

typedef TypedInference<LIST(Formula*)> PrintFormulaInferenceBase;
class PrintFormulaInference: public PrintFormulaInferenceBase {
public:
	PrintFormulaInference()
			: PrintFormulaInferenceBase("tostring", "Prints the given formula.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto f = get<0>(args);
		return InternalArgument(StringPointer(print(f)));
	}
};

typedef TypedInference<LIST(Vocabulary*)> PrintVocabularyInferenceBase;
class PrintVocabularyInference: public PrintVocabularyInferenceBase {
public:
	PrintVocabularyInference()
			: PrintVocabularyInferenceBase("tostring", "Prints the given vocabulary.") {
		setNameSpace(getInternalNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto vocabulary = get<0>(args);
		return InternalArgument(StringPointer(print(vocabulary)));
	}
};

#endif /* PRINTSTRUCTURE_HPP_ */
