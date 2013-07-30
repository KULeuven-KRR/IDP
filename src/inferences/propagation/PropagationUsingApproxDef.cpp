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

#include "PropagationUsingApproxDef.hpp"

ApproximatingDefinition::DerivationTypes* PropagationUsingApproxDef::getDerivationTypes() {
	auto derivationtypes = new ApproximatingDefinition::DerivationTypes();
	derivationtypes->addDerivationType(
			ApproximatingDefinition::TruthPropagation::TRUE,
			ApproximatingDefinition::Direction::DOWN);
	derivationtypes->addDerivationType(
			ApproximatingDefinition::TruthPropagation::FALSE,
			ApproximatingDefinition::Direction::UP);
	return derivationtypes;
}

void PropagationUsingApproxDef::processApproxDef(Structure* structure, ApproximatingDefinition* approxdef) {

	if (DefinitionUtils::hasRecursionOverNegation(approxdef->approximatingDefinition())) {
		if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
			//TODO: either go back to normal method or FIX XSB to support recneg!
			clog << "Approximating definition had recursion over negation, not calculating it\n";
		}
		return;
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
	structure->clean();
}

std::vector<Structure*>  PropagationUsingApproxDef::propagateUsingAllRules(AbstractTheory* theory, Structure* structure) {
	auto rule_types = std::set<ApproximatingDefinition::RuleType>();
	rule_types.insert(ApproximatingDefinition::RuleType::CHEAP);
	rule_types.insert(ApproximatingDefinition::RuleType::FORALL);
	auto approxdef = GenerateApproximatingDefinition::doGenerateApproximatingDefinition(theory,getDerivationTypes(),rule_types);
	processApproxDef(structure,approxdef);
	return {structure};
}

std::vector<Structure*>  PropagationUsingApproxDef::propagateUsingCheapRules(AbstractTheory* theory, Structure* structure) {
	auto rule_types = std::set<ApproximatingDefinition::RuleType>();
	rule_types.insert(ApproximatingDefinition::RuleType::CHEAP);
	auto approxdef = GenerateApproximatingDefinition::doGenerateApproximatingDefinition(theory,getDerivationTypes(),rule_types);
	processApproxDef(structure,approxdef);
	return {structure};
}

std::vector<Structure*>  PropagationUsingApproxDef::propagateUsingStratification(AbstractTheory* theory, Structure* structure) {
	auto rule_types_1 = std::set<ApproximatingDefinition::RuleType>();
	rule_types_1.insert(ApproximatingDefinition::RuleType::CHEAP);
	auto rule_types_2 = std::set<ApproximatingDefinition::RuleType>();
	rule_types_2.insert(ApproximatingDefinition::RuleType::FORALL);
	auto approxdef_1 = GenerateApproximatingDefinition::doGenerateApproximatingDefinition(theory,getDerivationTypes(),rule_types_1);
	auto approxdef_2 = GenerateApproximatingDefinition::doGenerateApproximatingDefinition(theory,getDerivationTypes(),rule_types_2);
	processApproxDef(structure,approxdef_1);
	processApproxDef(structure,approxdef_2);
	return {structure};
}

std::vector<Structure*>  PropagationUsingApproxDef::propagate(AbstractTheory* theory, Structure* structure) {
	auto option = getGlobal()->getOptions()->approxDef();
		switch (option) {
		case ApproxDef::NONE:
			return {structure};
		case ApproxDef::ALL_AT_ONCE:
			return propagateUsingAllRules(theory, structure);
		case ApproxDef::CHEAP_RULES_ONLY:
			return propagateUsingCheapRules(theory, structure);
		case ApproxDef::STRATIFIED:
			return propagateUsingStratification(theory, structure);
		default:
			throw IdpException("Invalid code path.");
	}
}
