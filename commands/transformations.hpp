/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CMD_REMOVENESTING_HPP_
#define CMD_REMOVENESTING_HPP_

#include "commandinterface.hpp"
#include "theory.hpp"
#include "utils/TheoryUtils.hpp"

class RemoveNestingInference : public TypedInference<LIST(AbstractTheory*)> {
public:
	RemoveNestingInference (): TypedInference("removenesting", "Move nested terms (except equality) out.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		FormulaUtils::unnestTerms(get<0>(args));
		return nilarg();
	}
};

class PushNegationsInference: public TypedInference<LIST(AbstractTheory*)>{
public:
	PushNegationsInference(): TypedInference("pushnegations", "Push negations inwards until they are before literals.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		FormulaUtils::pushNegations(get<0>(args));
		return nilarg();
	}
};

class FlattenInference: public TypedInference<LIST(AbstractTheory*)> {
public:
	FlattenInference(): TypedInference("flatten", "Rewrites formulas with the same operations in their child formulas by reducing the nesting") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		FormulaUtils::flatten(get<0>(args));
		return nilarg();
	}
};

class CompletionInference: public TypedInference<LIST(AbstractTheory*)> {
public:
	CompletionInference(): TypedInference("completion", "Add definitional completion to the given theory.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		FormulaUtils::addCompletion(get<0>(args));
		return nilarg();
	}
};

#endif /* CMD_REMOVENESTING_HPP_ */
