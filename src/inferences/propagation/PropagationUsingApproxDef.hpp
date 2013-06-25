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

#include "IncludeComponents.hpp"
#include "inferences/approximatingdefinition/ApproximatingDefinition.hpp"
#include "inferences/approximatingdefinition/GenerateApproximatingDefinition.hpp"

/**
 * Given a theory and a structure, return a new structure which is at least as precise as the structure
 * on the given theory.
 * Implements the optimal propagator by generating the approximating defintion for TRUE downwards and
 * FALSE upwards, calculating this definition and adjust the structure accordingly.
 */
class PropagationUsingApproxDef {

public:
	std::vector<AbstractStructure*>  propagate(AbstractTheory* theory, AbstractStructure* structure) {

		auto derivationtypes = new ApproximatingDefinition::DerivationTypes();
		derivationtypes->addDerivationType(ApproximatingDefinition::TruthPropagation::TRUE, ApproximatingDefinition::Direction::DOWN);
		derivationtypes->addDerivationType(ApproximatingDefinition::TruthPropagation::FALSE, ApproximatingDefinition::Direction::UP);

		auto approxdef = GenerateApproximatingDefinition::doGenerateApproximatingDefinition(theory,derivationtypes);

		if (DefinitionUtils::hasRecursionOverNegation(approxdef->approximatingDefinition())) {
			if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
				//TODO: either go back to normal method or FIX XSB to support recneg!
				clog << "Approximating definition had recursion over negation, not calculating it\n";
			}
			return {structure};
		}

		auto approxdef_structure = approxdef->inputStructure(structure);
		auto def_to_calculate = DefinitionUtils::makeDefinitionCalculable(approxdef->approximatingDefinition(),approxdef_structure);

		auto output_structure = CalculateDefinitions::doCalculateDefinitions(
				def_to_calculate, approxdef_structure, approxdef->getSymbolsToQuery());

		if(not output_structure.empty() && approxdef->isConsistent(output_structure.at(0))) {
			approxdef->updateStructure(structure,output_structure.at(0));
			if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
				clog << "Calculating the approximating definitions with XSB resulted in the following structure:\n" <<
						toString(structure) << "\n";
			}
		}

		return {structure};
	}
};
