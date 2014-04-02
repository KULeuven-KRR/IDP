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

#include "CalculateDefinitions.hpp"
#include "inferences/SolverConnection.hpp"
#include "fobdds/FoBddManager.hpp"

#include "theory/TheoryUtils.hpp"
#include "structure/StructureComponents.hpp"

#include "groundtheories/SolverTheory.hpp"

#include "inferences/grounding/grounders/Grounder.hpp"
#include "inferences/grounding/LazyGroundingManager.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"

#ifdef WITHXSB
#include "inferences/querying/xsb/XSBInterface.hpp"
#endif

#include "options.hpp"
#include <iostream>

using namespace std;
DefinitionCalculationResult CalculateDefinitions::calculateDefinition(Definition* definition, Structure* structure,
		bool satdelay, bool& tooExpensive, std::set<PFSymbol*> symbolsToQuery) const {
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
		clog << "Calculating definition: " << toString(definition) << "\n";
	}
	DefinitionCalculationResult result(structure);
#ifdef WITHXSB
	auto withxsb = CalculateDefinitions::determineXSBUsage(definition);
	if (withxsb) {
		if(satdelay or getOption(SATISFIABILITYDELAY)) { // TODO implement checking threshold by size estimation
			Warning::warning("Lazy threshold is not checked for definitions evaluated with XSB");
		}
		if (getOption(IntType::VERBOSE_DEFINITIONS) >= 2) {
			clog << "Calculating the above definition using XSB\n";
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
			auto sorted = xsb_interface->queryDefinition(symbol);
            auto internpredtable1 = new EnumeratedInternalPredTable(sorted);
            auto predtable1 = new PredTable(internpredtable1, structure->universe(symbol));
            if(not isConsistentWith(predtable1, structure->inter(symbol))){
            	xsb_interface->reset();
            	result._hasModel=false;
            	result._calculated_model=structure;
            	return result;
            }
            structure->inter(symbol)->ctpt(predtable1);
            structure->clean();
            if(isa<Function>(*symbol)) {
            	auto fun = dynamic_cast<Function*>(symbol);
            	if(not structure->inter(fun)->approxTwoValued()){ // E.g. for functions
                	xsb_interface->reset();
                	result._hasModel=false;
                	result._calculated_model=structure;
                	return result;
    			}
            }
			if(not structure->inter(symbol)->isConsistent()){
            	xsb_interface->reset();
            	result._hasModel=false;
            	result._calculated_model=structure;
            	return result;
			}
		}

    	xsb_interface->reset();
    	result._calculated_model=structure;
		if (not structure->isConsistent()) {
        	result._hasModel=false;
        	return result;
		} else {
	    	result._hasModel=true;
	    	result._calculated_definitions.push_back(definition);
		}
    	return result;
	}
#endif
	// Default: Evaluation using ground-and-solve
	auto data = SolverConnection::createsolver(1);
	Theory theory("", structure->vocabulary(), ParseInfo());
	theory.add(definition);
	bool LUP = getOption(BoolType::LIFTEDUNITPROPAGATION);
	bool propagate = LUP || getOption(BoolType::GROUNDWITHBOUNDS);
	auto symstructure = generateBounds(&theory, structure, propagate, LUP);
	auto grounder = GrounderFactory::create(GroundInfo(&theory, { structure, symstructure }, NULL, true), data);

	auto size = toDouble(grounder->getMaxGroundSize());
	size = size < 1 ? 1 : size;
	if ((satdelay or getOption(SATISFIABILITYDELAY)) and log(size) / log(2) > 2 * getOption(LAZYSIZETHRESHOLD)) {
		tooExpensive = true;
		delete (data);
		delete (grounder);
    	result._hasModel=true;
		return result;
	}

	bool unsat = grounder->toplevelRun();

	//It's possible that unsat is found (for example when we have a conflict with function constraints)
	if (unsat) {
		// Cleanup
		delete (data);
		delete (grounder);
    	result._hasModel=false;
		return result;
	}

	Assert(not unsat);
	AbstractGroundTheory* grounding = dynamic_cast<SolverTheory*>(grounder->getGrounding());

	// Run solver
	auto mx = SolverConnection::initsolution(data, 1);
	mx->execute();
	if (getGlobal()->terminateRequested()) {
		throw IdpException("Solver was terminated");
	}

	// Collect solutions
	auto abstractsolutions = mx->getSolutions();
	if (not abstractsolutions.empty()) {
		Assert(abstractsolutions.size() == 1);
		auto model = *(abstractsolutions.cbegin());
		SolverConnection::addLiterals(*model, grounding->translator(), structure);
		SolverConnection::addTerms(*model, grounding->translator(), structure);
		structure->clean();
	}
	// Cleanup
	grounding->recursiveDelete();
	delete (data);
	delete (mx);
	delete (grounder);

	result._hasModel=(not abstractsolutions.empty() && structure->isConsistent());
	if(result._hasModel) {
    	result._calculated_definitions.push_back(definition);
	}
	return result;
}

DefinitionCalculationResult CalculateDefinitions::calculateKnownDefinitions(Theory* theory, Structure* structure,
		bool satdelay, std::set<PFSymbol*> symbolsToQuery) const {
	if (theory == NULL || structure == NULL) {
		throw IdpException("Unexpected NULL-pointer.");
	}
	if (structure->vocabulary() != theory->vocabulary()) {
		throw IdpException("Definition Evaluation requires that the theory and structure range over the same vocabulary.");
	}

	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Calculating known definitions\n";
	}

	theory = FormulaUtils::improveTheoryForInference(theory, structure, false, false);

	// Collect the open symbols of all definitions
	auto opens = DefinitionUtils::opens(theory->definitions());

	if (getOption(BoolType::STABLESEMANTICS)) {
		CalculateDefinitions::removeNonTotalDefnitions(opens);
	}

	DefinitionCalculationResult result(structure);
	result._hasModel = true;

	// Calculate the interpretation of the defined atoms from definitions that do not have
	// three-valued open symbols
	bool fixpoint = false;
	while (not fixpoint) {
		fixpoint = true;
		for (auto it = opens.begin(); it != opens.end();) {
			auto currentdefinition = it++; // REASON: set erasure does only invalidate iterators pointing to the erased elements

			// Remove opens that have a two-valued interpretation
			auto toRemove = DefinitionUtils::approxTwoValuedOpens(currentdefinition->first, structure);
			for (auto symbol : toRemove) {
				if (currentdefinition->second.find(symbol) != currentdefinition->second.end()) {
					currentdefinition->second.erase(symbol);
				}
			}

			// If no opens are left, calculate the interpretation of the defined atoms
			if (currentdefinition->second.empty()) {
				auto definition = currentdefinition->first;

				if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
					clog << "Evaluating " << toString(currentdefinition->first) << "\n";
				}
				if (getOption(IntType::VERBOSE_DEFINITIONS) >= 4) {
					clog << "Using structure " << toString(structure) << "\n";
				}
				bool tooexpensive = false;
				auto defCalcResult = calculateDefinition(definition, structure, satdelay, tooexpensive, symbolsToQuery);
				if (tooexpensive) {
					continue;
				}
				result._calculated_model = defCalcResult._calculated_model; // Update current structure

				if (not defCalcResult._hasModel) { // If the definition did not have a model, quit execution (don't set fixpoint to false)
					if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
						clog << "The given structure is not a model of the definition\n" << toString(definition) << "\n";
					}
					result._hasModel = false;
				} else { // If it did have a model, update result and continue
					fixpoint = false;
					opens.erase(currentdefinition);
					theory->remove(definition);
					result._calculated_definitions.push_back(definition);
				}
			}
		}
	}
	if (not result._hasModel or not result._calculated_model->isConsistent()) {
		// When returning a result that has no model, the other arguments are as follows:
		// _calculated_model:  the structure resulting from the last unsuccessful definition calculation
		// _calculated_definitions: all definitions that have been successfully calculated
		result._hasModel = false;
		return result;
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 1) {
		clog << "Done calculating known definitions\n";
	}
	if (getOption(IntType::VERBOSE_DEFINITIONS) >= 4) {
		clog << "Resulting structure:\n" << toString(structure) << "\n";
	}
	result._hasModel = true;
	result._calculated_model = structure;
	return result;
}

void CalculateDefinitions::removeNonTotalDefnitions(std::map<Definition*,
		std::set<PFSymbol*> >& opens) {
	bool foundone = false;
	auto def = opens.begin();
	while (def != opens.end()) {
		auto hasrecursion = DefinitionUtils::hasRecursionOverNegation((*def).first);
		//TODO in the future: put a smarter check here

		auto currentdefinition = def++;
		// REASON: set erasure does only invalidate iterators pointing to the erased elements

		if (hasrecursion) {
			foundone = true;
			opens.erase(currentdefinition);
		}
	}
	if (foundone) {
		Warning::warning("Ignoring definitions for which we cannot detect totality because option stablesemantics is true.");
	}
}

#ifdef WITHXSB
bool CalculateDefinitions::determineXSBUsage(Definition* definition) {
	auto hasrecursion = DefinitionUtils::hasRecursionOverNegation(definition);
	if (getOption(XSB) && hasrecursion) {
		Warning::warning("Currently, no support for definitions that have recursion over negation with XSB");
	}

	auto has_recursive_aggregate = DefinitionUtils::approxContainsRecDefAggTerms(definition);
	if(getOption(XSB) && has_recursive_aggregate) {
		Warning::warning("Currently, no support for definitions that have recursive aggregates");
	}

	return getOption(XSB) && not hasrecursion && not has_recursive_aggregate;
}
#endif

// IMPORTANT: if no longer wrapper in theory, repeat transformations from theory!
DefinitionCalculationResult CalculateDefinitions::calculateKnownDefinition(Definition* definition,
		Structure* structure, bool satdelay, std::set<PFSymbol*> symbolsToQuery) const {
	auto theory = new Theory("wrapper_theory", structure->vocabulary(), ParseInfo());
	theory->add(definition);
	return calculateKnownDefinitions(theory,structure,satdelay, symbolsToQuery);
}
