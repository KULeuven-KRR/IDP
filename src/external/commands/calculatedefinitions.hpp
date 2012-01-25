/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef CALCULATEDEFINITIONSCOMMAND_HPP_
#define CALCULATEDEFINITIONSCOMMAND_HPP_

#include "commandinterface.hpp"
#include "inferences/definitionevaluation/CalculateDefinitions.hpp"
#include "errorhandling/error.hpp"

typedef TypedInference<LIST(AbstractTheory*, AbstractStructure*)> CalculateDefinitionInferenceBase;
class CalculateDefinitionInference: public CalculateDefinitionInferenceBase {
public:
	CalculateDefinitionInference()
			: CalculateDefinitionInferenceBase("calculatedefinitions",
					"Make the structure more precise than the given one by evaluating all definitions with known open symbols.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto t = get<0>(args);
		Theory* theory = NULL;
		if (sametypeid<Theory>(*t)) {
			theory = (dynamic_cast<Theory*>(t))->clone(); //Because the doCalculateDefinitions Inferences changes the theory.
		} else {
			Error::error("Can only calculate definitions with a non-ground theory.");
			return nilarg();
		}
		// FIXME this should not return a new structure! (solve creating inconsistentstructure then)
		return InternalArgument(CalculateDefinitions::doCalculateDefinitions(theory, get<1>(args)));
	}
};

#endif /* CALCULATEDEFINITIONSCOMMAND_HPP_ */
