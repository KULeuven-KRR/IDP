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

#include "refineStructureWithDefinitions.hpp"
#include "CalculateDefinitions.hpp"
#include "creation/cppinterface.hpp"
#include "theory/TheoryUtils.hpp"
#include "structure/StructureComponents.hpp"
#include "structure/Structure.hpp"

#ifdef WITHXSB
#include "inferences/querying/xsb/XSBInterface.hpp"
#endif

#include "options.hpp"
#include <iostream>

using namespace std;
DefinitionRefiningResult refineStructureWithDefinitions::processDefinition(
		const Definition* d, Structure* structure, bool satdelay,
		std::set<PFSymbol*> symbolsToQuery) const {
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
		clog << "Refining definition: " << toString(d) << "\n";
	}
	DefinitionRefiningResult result(structure);
	result._hasModel = true;

#ifdef WITHXSB
	auto withxsb = CalculateDefinitions::determineXSBUsage(d);
	if (withxsb) {
		if(satdelay or getOption(SATISFIABILITYDELAY)) { // TODO implement checking threshold by size estimation
			Warning::warning("Lazy threshold is not checked for definitions evaluated with XSB");
		}
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
			clog << "Refining the above definition using XSB\n";
		}
		auto xsb_interface = XSBInterface::instance();
		xsb_interface->load(d,structure);

		auto symbols = d->defsymbols();
		if (not symbolsToQuery.empty()) {
			for(auto it = symbols.begin(); it != symbols.end();) {
				auto symbol = *(it++);
				if(symbolsToQuery.find(symbol) == symbolsToQuery.end()) {
					symbols.erase(symbol);
				}
			}
		}

		for (auto symbol : symbols) {

			// TODO: refactor this code, make it neat!
			auto sortedTableTRUE = xsb_interface->queryDefinition(symbol);
            auto predtable1 = Gen::predtable(sortedTableTRUE, structure->universe(symbol));
			auto sortedTableUNKN = xsb_interface->queryDefinition(symbol, TruthValue::Unknown);
			sortedTableUNKN.insert(sortedTableTRUE.begin(),sortedTableTRUE.end());
            auto predtable2 = Gen::predtable(sortedTableUNKN, structure->universe(symbol));

			if (not (structure->inter(symbol)->ct()->size() == predtable1->size() and
					structure->inter(symbol)->pt()->size() == predtable2->size() )) {
				// The interpretation on this symbol has changed
				result._refined_symbols.insert(symbol);
				structure->inter(symbol)->pt(predtable2);
				structure->inter(symbol)->ct(predtable1);
			}

			if (not structure->inter(symbol)->isConsistent()) {
            	xsb_interface->reset();
            	result._hasModel=false;
            	return result;
			}
            if (isa<Function>(*symbol)) {
            	// for functions, check whether the interpretation satisfies function constraints
            	auto fun = dynamic_cast<Function*>(symbol);
            	if(not structure->satisfiesFunctionConstraints(fun)) {
                	xsb_interface->reset();
                	result._hasModel=false;
                	result._calculated_model=structure;
                	return result;
    			}
            }
		}

		xsb_interface->reset();
		if (not structure->isConsistent()) {
			result._hasModel=false;
			return result;
		} else {
			result._hasModel=true;
		}
		return result;
	}
#endif
	// Not possible without XSB
	Warning::warning("Tried to evaluate definitions for three-valued opens without XSB,\n"
			"this is not possible and nothing was done instead.");
	return result;
}

DefinitionRefiningResult refineStructureWithDefinitions::refineDefinedSymbols(Theory* theory, Structure* structure,
		bool satdelay, std::set<PFSymbol*> symbolsToQuery) const {
	if (theory == NULL || structure == NULL) {
		throw IdpException("Unexpected NULL-pointer.");
	}
	if (structure->vocabulary() != theory->vocabulary()) {
		throw IdpException("Definition refining requires that the theory and structure range over the same vocabulary.");
	}

	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Refining definitions\n";
	}
	theory = FormulaUtils::improveTheoryForInference(theory, structure, false, false);
	auto definitions_to_process = std::set<Definition*>(theory->definitions().begin(), theory->definitions().end());
	std::map<PFSymbol*, PredInter*> initial_interpretations;
	DefinitionRefiningResult result(structure);
	result._hasModel = true;

	// Calculate the interpretation of the defined atoms from definitions that do not have
	// three-valued open symbols
	while (not definitions_to_process.empty()) {
		auto definition = *(definitions_to_process.begin());
		definitions_to_process.erase(definition);
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
			clog << "Refining " << toString(definition) << "\n";
		}
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 4) {
			clog << "Using structure " << toString(structure) << "\n";
		}
		for (auto defsymbol : definition->defsymbols()) {
			initial_interpretations.insert(std::pair<PFSymbol*, PredInter*>(defsymbol, structure->inter(defsymbol)->clone()));
		}
		FormulaUtils::removeInterpretationOfDefinedSymbols(definition,structure);
		DefinitionRefiningResult processDefResult(structure);
		processDefResult = processDefinition(definition, structure, satdelay, symbolsToQuery);
		processDefResult._hasModel = postprocess(processDefResult, initial_interpretations);
		for (auto i : initial_interpretations) {
			delete(i.second);
		}
		initial_interpretations.clear(); // These are not needed anymore
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
			clog << "Resulting structure:\n" << toString(structure) << "\n";
		}
		if (not processDefResult._hasModel) { // If the definition did not have a model, quit execution
			if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
				clog << "The given structure is not a model of the definition\n" << toString(definition) << "\n";
			}
			result._hasModel = false;
			return result;
		}
		// update the refined symbols
		result._refined_symbols.insert(processDefResult._refined_symbols.begin(),
				processDefResult._refined_symbols.end());

		// Find definitions with opens for which the interpretation has changed
		for (auto def : theory->definitions()) {
			// Don't do anything for the definition if it is still in the queue
			if (definitions_to_process.find(def) == definitions_to_process.end()) {
				for (auto symbol : processDefResult._refined_symbols) {
					auto opensOfDefinition = DefinitionUtils::opens(def);
					if (opensOfDefinition.find(symbol) != opensOfDefinition.end()) {
						definitions_to_process.insert(def);
					}
				}
			}
		}
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Done refining definitions\n";
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 4) {
		clog << "Resulting structure:\n" << toString(result._calculated_model) << "\n";
	}
	return result;
}

// Separate procedure to decide whether the definition refinement is acceptable
// Also contains some verbosity code
bool refineStructureWithDefinitions::postprocess(DefinitionRefiningResult& result, std::map<PFSymbol*, PredInter*>& initial_inters) const {
	if (not result._hasModel) {
		return false;
	}
	result._refined_symbols.clear();
	for (auto it : initial_inters) {
		auto symbol = it.first;
		auto inter = it.second;
		// Insert the "old" interpretation into the refined one.
		for (auto ct_iterator = inter->ct()->begin(); not ct_iterator.isAtEnd(); ++ct_iterator) {
			result._calculated_model->inter(symbol)->makeTrueAtLeast(*ct_iterator);
		}
		for (auto cf_iterator = inter->cf()->begin(); not cf_iterator.isAtEnd(); ++cf_iterator) {
			result._calculated_model->inter(symbol)->makeFalseAtLeast(*cf_iterator);
		}
		// If inconsistency is detected, return false
		if (not result._calculated_model->inter(symbol)->isConsistent()) {
			result._hasModel = false;
			return false;
		}
		// Update the refined symbols
		if(not (inter->ct()->size() == result._calculated_model->inter(symbol)->ct()->size() and
				inter->pt()->size() == result._calculated_model->inter(symbol)->pt()->size()) ) {
			// The interpretation on this symbol has changed
			result._refined_symbols.insert(symbol);
		}
	}
	result._calculated_model->clean();
	return true;
}

// IMPORTANT: if you change the below interpretation, remember that this way (wrapping it into a theory)
//            ensures that all necessary theory transformation have occurred and that the new
//            implementation should ensure the same!
DefinitionRefiningResult refineStructureWithDefinitions::refineDefinedSymbols(Definition* definition,
		Structure* structure, bool satdelay, std::set<PFSymbol*> symbolsToQuery) const {
	auto theory = Theory("wrapper_theory", structure->vocabulary(), ParseInfo());
	theory.add(definition);
	return refineDefinedSymbols(&theory,structure,satdelay,symbolsToQuery);
}



