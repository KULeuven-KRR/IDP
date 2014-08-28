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

#pragma once

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"
#include "utils/BootstrappingUtils.hpp"

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

typedef TypedInference<LIST(AbstractTheory*, int)> TheoryIntBase;
class RemoveCardinalitiesInference: public TheoryIntBase {
public:
	RemoveCardinalitiesInference()
			: TheoryIntBase("removecardinalities", "Replace all atoms #{xxx: phi}=n with equivalent FO sentences in the given theory, if the given threshold is zero or larger than n*|xxx|.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		FormulaUtils::replaceCardinalitiesWithFOFormulas(get<0>(args), get<1>(args));
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

class RemoveValidQuantificationsInference: public TheoryStructureBase {
public:
	RemoveValidQuantificationsInference()
			: TheoryStructureBase("simplify", "Simplifies the theory, removing valid formulas, computing arithmetic, etc..") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto theory = dynamic_cast<Theory*>(get<0>(args));
		if(theory==NULL){
			return nilarg();
		}
		FormulaUtils::simplify(theory, get<1>(args));
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
			: TheoryBase("completion", "Replaces definitions with their completion in the given theory.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		FormulaUtils::replaceDefinitionsWithCompletion(get<0>(args), NULL); // TODO can be improved with structure
		return nilarg();
	}
};

// IMPORTANT: for debugging purposes, symbols is stored globally for now (cannot be passed to Lua yet).
//	Approach will not work if calling tometa twice before backtranslation!
UniqueStringNames<PFSymbol*> symbols;

class ToMetaInference: public TheoryBase {
public:
	ToMetaInference()
			: TheoryBase("tometa", "Transforms the given theory into a structure over the meta vocabulary.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto result = BootstrappingUtils::toMeta(dynamic_cast<Theory*>(get<0>(args)));
		symbols = result.symbols;
		return InternalArgument(result.metastructure);
	}
};

class FromMetaInference: public TypedInference<LIST(Vocabulary*, Predicate*, Structure*)> {
public:
	FromMetaInference()
			: TypedInference<LIST(Vocabulary*, Predicate*, Structure*)>("frommeta", "Takes as input 1) a vocabulary V 2) the predicate symbol 'sentence' from the meta vocabulary and 3) a structure over the meta vocabulary. This returns the theory represented by the given structure. IMPORTANT: the structure you give as input should be obtained by (transforming the result of) the tometa inference. In between the tometa call and this call, there can currently be no other tometa calls.") {
		setNameSpace(getTheoryNamespaceName());
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return InternalArgument(BootstrappingUtils::fromMeta( {get<0>(args), get<2>(args), symbols}, get<1>(args)));
	}
};
