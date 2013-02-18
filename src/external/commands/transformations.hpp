/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef CMD_REMOVENESTING_HPP_
#define CMD_REMOVENESTING_HPP_

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"

class RemoveNestingInference: public TheoryBase {
public:
	RemoveNestingInference()
			: TheoryBase("removenesting", "Move nested terms (except equality) out.\nModifies the given theory.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		FormulaUtils::unnestTerms(get<0>(args));
		return nilarg();
	}
};

class PushNegationsInference: public TheoryBase {
public:
	PushNegationsInference()
			: TheoryBase("pushnegations", "Push negations inwards until they are before literals.\nModifies the given theory.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		FormulaUtils::pushNegations(get<0>(args));
		return nilarg();
	}
};

class FlattenInference: public TheoryBase {
public:
	FlattenInference()
			: TheoryBase("flatten", "Rewrites formulas with the same operations in their child formulas by reducing the nesting.\nModifies the given theory.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		FormulaUtils::flatten(get<0>(args));
		return nilarg();
	}
};

class CompletionInference: public TheoryBase {
public:
	CompletionInference()
			: TheoryBase("completion", "Add definitional completion to the given theory.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		FormulaUtils::addCompletion(get<0>(args));
		return nilarg();
	}
};

#endif /* CMD_REMOVENESTING_HPP_ */
