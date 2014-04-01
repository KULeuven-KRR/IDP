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

#ifdef WITHXSB
#include "inferences/querying/xsb/XSBInterface.hpp"
#endif

#include "options.hpp"
#include <iostream>

using namespace std;
DefinitionRefiningResult refineStructureWithDefinitions::processDefinition(
		Definition* definition, Structure* structure, bool satdelay,
		bool withxsb, std::set<PFSymbol*> symbolsToQuery) const {
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
		clog << "Refining definition: " << toString(definition) << "\n";
	}
	DefinitionRefiningResult result(structure);
	result._hasModel = true;

#ifdef WITHXSB
	if (withxsb) {
		if(satdelay or getOption(SATISFIABILITYDELAY)) { // TODO implement checking threshold by size estimation
			Warning::warning("Lazy threshold is not checked for definitions evaluated with XSB");
		}
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
			clog << "Refining the above definition using XSB\n";
		}
		auto xsb_interface = XSBInterface::instance();
		xsb_interface->load(definition,structure);

		auto symbols = definition->defsymbols();
		if(not symbolsToQuery.empty()) {
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

			if(not (structure->inter(symbol)->ct()->size() == predtable1->size() and
					structure->inter(symbol)->pt()->size() == predtable2->size() )) {
				// The interpretation on this symbol has changed
				result._refined_symbols.insert(symbol);
				structure->inter(symbol)->pt(predtable2);
				structure->inter(symbol)->ct(predtable1);
			}

			if(not structure->inter(symbol)->isConsistent()) {
            	xsb_interface->reset();
            	result._hasModel=false;
            	return result;
			}
            if(isa<Function>(*symbol)) {
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
			result._refined_definitions.insert(definition);
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
	auto opens = DefinitionUtils::opens(theory->definitions()); // Collect the open symbols of all definitions
	DefinitionRefiningResult result(structure);
	result._hasModel = true;

	// Calculate the interpretation of the defined atoms from definitions that do not have
	// three-valued open symbols
	bool fixpoint = false;
	while (not fixpoint) {
		fixpoint = true;
		for (auto it = opens.begin(); it != opens.end();) {
			auto currentdefinition = it++; // REASON: set erasure does only invalidate iterators pointing to the erased elements
			auto definition = currentdefinition->first;

			if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
				clog << "Refining " << toString(currentdefinition->first) << "\n";
			}
			if (getOption(IntType::VERBOSE_DEFINITIONS) >= 4) {
				clog << "Using structure " << toString(structure) << "\n";
			}
			Structure* initialStructure = structure->clone(); // Used at the end to determine consistency
			FormulaUtils::removeInterpretationOfDefinedSymbols(definition,structure);
			DefinitionRefiningResult processDefResult(structure);
#ifdef WITHXSB
			auto useXSB = CalculateDefinitions::determineXSBUsage(definition);
			processDefResult = processDefinition(definition, structure, satdelay,
					useXSB, symbolsToQuery);
#else
			processDefResult = processDefinition(definition, structure, satdelay,
					false, symbolsToQuery);
#endif
			processDefResult._hasModel = postprocess(processDefResult,initialStructure);
			if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
				clog << "Resulting structure:\n" << toString(structure) << "\n";
			}

			if (not processDefResult._hasModel) { // If the definition did not have a model, quit execution (don't set fixpoint to false)
				if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
					clog << "The given structure is not a model of the definition\n" << toString(definition) << "\n";
				}
				delete(structure);
				structure = initialStructure;
				result._hasModel = false;
				return result;
			} else { // If it did have a model, update result and continue
				delete(initialStructure);

				// Find definitions from the calculated definitions list if they have opens for which the
				// interpretation has changed
				for (auto it = result._refined_definitions.begin(); it != result._refined_definitions.end(); ) {
					auto def = *(it++);
					for (auto symbol : processDefResult._refined_symbols) {
						// update the refined symbols (these never have to be removed)
						result._refined_symbols.insert(symbol);
						auto opensOfDefinition = DefinitionUtils::opens(def);
						if (opensOfDefinition.find(symbol) != opensOfDefinition.end()) {
							result._refined_definitions.erase(def);
						}
					}
				}
				// add the current definition as "refined" - it has just been evaluated
				result._refined_definitions.insert(definition);
				if (result._refined_definitions.size() != theory->definitions().size()) {
					// there are definitions still to be calculated - put fixpoint to false
					fixpoint = false;
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
bool refineStructureWithDefinitions::postprocess(DefinitionRefiningResult& result,
		const Structure* s) const {
	if (not result._hasModel) {
		return false;
	}
	// If no inconsistency is detected, "insert" the initial interpretation into every defined symbol
	for (auto def : result._refined_definitions) {
		for (auto symbol : def->defsymbols()) {
			for (auto ct_iterator = s->inter(symbol)->ct()->begin(); not ct_iterator.isAtEnd(); ++ct_iterator) {
				result._calculated_model->inter(symbol)->makeTrueAtLeast(*ct_iterator);
			}
			for (auto cf_iterator = s->inter(symbol)->cf()->begin(); not cf_iterator.isAtEnd(); ++cf_iterator) {
				result._calculated_model->inter(symbol)->makeFalseAtLeast(*cf_iterator);
			}
			if (not isConsistentWith(result._calculated_model->inter(symbol),s->inter(symbol))) {
				return false;
			}
			// If the interpretation did not change, remove it from the list of refined symbols
			if(s->inter(symbol)->ct()->size() == result._calculated_model->inter(symbol)->ct()->size() and
			   s->inter(symbol)->pt()->size() == result._calculated_model->inter(symbol)->pt()->size()) {
				// The interpretation on this symbol has changed
				result._refined_symbols.erase(symbol);
			}
		}
	}
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



