/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#pragma once

#include "commandinterface.hpp"
#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"
#include "errorhandling/error.hpp"

typedef TypedInference<LIST(AbstractStructure*, AbstractStructure*)> JoinStructuresInferenceBase;
class JoinStructuresInference: public JoinStructuresInferenceBase {
public:
	JoinStructuresInference()
			: JoinStructuresInferenceBase("merge", "Make the first structure more precise using the second one. Only shared relations are combined.") {
		setNameSpace(getStructureNamespaceName());
	}

	void makeMorePrecise(PFSymbol* symbol, PredInter* inter, AbstractStructure const * const newinfo) const {
		if (not newinfo->vocabulary()->contains(symbol)) {
			return;
		}
		auto newinter = newinfo->inter(symbol);
		auto newct = newinter->ct();
		auto newcf = newinter->cf();
		for (auto tupleit = newct->begin(); not tupleit.isAtEnd(); ++tupleit) {
			inter->makeTrue(*tupleit);
		}
		for (auto tupleit = newcf->begin(); not tupleit.isAtEnd(); ++tupleit) {
			inter->makeFalse(*tupleit);
		}
	}

	AbstractStructure* merge(AbstractStructure const * const orig, AbstractStructure const * const newinfo) const {
		auto target = orig->clone();
		// Sorts
		Warning::warning("Sorts are not combined by merging of structures");
		// Predicates
		for (auto targetinter : target->getPredInters()) {
			makeMorePrecise(targetinter.first, targetinter.second, newinfo);
		}
		// Functions
		for (auto targetinter : target->getFuncInters()) {
			makeMorePrecise(targetinter.first, targetinter.second->graphInter(), newinfo);
		}
		return target;
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		return InternalArgument(merge(get<0>(args), get<1>(args)));
	}
};
