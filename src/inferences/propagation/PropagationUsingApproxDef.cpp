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

ApproximatingDefinition::DerivationTypes* PropagationUsingApproxDef::getAllDerivationTypes() {
	auto derivationtypes = new ApproximatingDefinition::DerivationTypes();
	derivationtypes->addDerivationType(
			ApproximatingDefinition::TruthPropagation::TRUE,
			ApproximatingDefinition::Direction::DOWN);
	derivationtypes->addDerivationType(
			ApproximatingDefinition::TruthPropagation::TRUE,
			ApproximatingDefinition::Direction::UP);
	derivationtypes->addDerivationType(
			ApproximatingDefinition::TruthPropagation::FALSE,
			ApproximatingDefinition::Direction::DOWN);
	derivationtypes->addDerivationType(
			ApproximatingDefinition::TruthPropagation::FALSE,
			ApproximatingDefinition::Direction::UP);
	return derivationtypes;
}

void PropagationUsingApproxDef::processApproxDef(Structure* structure, ApproximatingDefinition* approxdef) {

	if (DefinitionUtils::approxHasRecursionOverNegation(approxdef->approximatingDefinition())) {
		if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
			//TODO: either go back to normal method or FIX XSB to support recneg!
			clog << "Approximating definition had recursion over negation, not calculating it\n";
		}
		return;
	}

	auto approxdef_structure = approxdef->inputStructure(structure);
	auto def_to_calculate = DefinitionUtils::makeDefinitionCalculable(approxdef->approximatingDefinition(),approxdef_structure);

	auto defCalculatedResult = CalculateDefinitions::doCalculateDefinition(
			def_to_calculate, approxdef_structure, approxdef->getSymbolsToQuery(), false);
        
	Assert(defCalculatedResult._hasModel);
	if( approxdef->isConsistent(defCalculatedResult._calculated_model)) {
		approxdef->updateStructure(structure,defCalculatedResult._calculated_model);
		if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
			clog << "Calculating the approximating definitions with XSB resulted in the following structure:\n" <<
					toString(structure) << "\n";
		}
	}
	structure->clean();
}

// Propagation using the "complete" rules possible in the approximating definition
// These rules are TRUE DOWN and FALSE UP
// It is claimed that executing these rules only is equivalent to executing all
// possible rules
std::vector<Structure*>  PropagationUsingApproxDef::propagateUsingCompleteRules(
		AbstractTheory* theory, Structure* structure) {
	auto rule_types = std::set<ApproximatingDefinition::RuleType>();
	rule_types.insert(ApproximatingDefinition::RuleType::CHEAP);
	rule_types.insert(ApproximatingDefinition::RuleType::FORALL);
	auto approxdef = GenerateApproximatingDefinition::doGenerateApproximatingDefinition(
			theory,getDerivationTypes(),rule_types,structure);
	processApproxDef(structure,approxdef);
	return {structure};
}


// Propagation using the "cheap" rules possible in the approximating definition
// These rules are TRUE DOWN and FALSE UP, but they do not contain rules that
// have a universal quantor in their body
// The rules with a universal quantor in the body can take very long to be
// computed using XSB
std::vector<Structure*>  PropagationUsingApproxDef::propagateUsingCheapRules(
		AbstractTheory* theory, Structure* structure) {
	auto rule_types = std::set<ApproximatingDefinition::RuleType>();
	rule_types.insert(ApproximatingDefinition::RuleType::CHEAP);
	auto approxdef = GenerateApproximatingDefinition::doGenerateApproximatingDefinition(
			theory,getDerivationTypes(),rule_types,structure);
	processApproxDef(structure,approxdef);
	return {structure};
}

// First calculate the "cheap" rules, as explained above, and then
// calculate TRUE DOWN and FALSE UP for only the "expensive" rules
std::vector<Structure*>  PropagationUsingApproxDef::propagateUsingStratification(
		AbstractTheory* theory, Structure* structure) {
	auto rule_types_1 = std::set<ApproximatingDefinition::RuleType>();
	rule_types_1.insert(ApproximatingDefinition::RuleType::CHEAP);
	auto rule_types_2 = std::set<ApproximatingDefinition::RuleType>();
	rule_types_2.insert(ApproximatingDefinition::RuleType::FORALL);
	auto approxdef_1 = GenerateApproximatingDefinition::doGenerateApproximatingDefinition(
			theory,getDerivationTypes(),rule_types_1,structure);
	auto approxdef_2 = GenerateApproximatingDefinition::doGenerateApproximatingDefinition(
			theory,getDerivationTypes(),rule_types_2,structure);
	processApproxDef(structure,approxdef_1);
	processApproxDef(structure,approxdef_2);
	return {structure};
}


// Propagation using all rules possible in the approximating definition
// These rules are TRUE UP, TRUE DOWN, FALSE UP, and FALSE DOWN
// These rules express explicitly every type of unit propagation possible in the theory
// they are created from
std::vector<Structure*>  PropagationUsingApproxDef::propagateUsingFullAD(AbstractTheory* theory, Structure* structure) {
	auto rule_types = std::set<ApproximatingDefinition::RuleType>();
	rule_types.insert(ApproximatingDefinition::RuleType::CHEAP);
	rule_types.insert(ApproximatingDefinition::RuleType::FORALL);
	auto approxdef = GenerateApproximatingDefinition::doGenerateApproximatingDefinition(
			theory,getAllDerivationTypes(),rule_types,structure);
	processApproxDef(structure,approxdef);
	return {structure};
}

std::vector<Structure*>  PropagationUsingApproxDef::propagate(AbstractTheory* theory, Structure* structure) {
	auto option = getGlobal()->getOptions()->approxDef();
		switch (option) {
		case ApproxDef::NONE:
			return {structure};
		case ApproxDef::COMPLETE:
			return propagateUsingCompleteRules(theory, structure);
		case ApproxDef::CHEAP_RULES_ONLY:
			return propagateUsingCheapRules(theory, structure);
		case ApproxDef::STRATIFIED:
			return propagateUsingStratification(theory, structure);
		case ApproxDef::ALL_POSSIBLE_RULES:
			return propagateUsingFullAD(theory, structure);
		default:
			throw IdpException("Invalid code path.");
	}
}
