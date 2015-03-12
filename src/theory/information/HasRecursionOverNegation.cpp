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

#include "HasRecursionOverNegation.hpp"
#include "utils/UniqueNames.hpp"
#include "theory/TheoryUtils.hpp"
#include "utils/BootstrappingUtils.hpp"
#include "IncludeComponents.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"

extern void parsefile(const std::string&);

using namespace std;

bool HasRecursionOverNegation::execute(const Definition* d) {
	return not DefinitionUtils::recurionsOverNegationSymbols(d).empty();
}

bool ApproxHasRecursionOverNegation::execute(const Definition* d) {
	return not DefinitionUtils::approxRecurionsOverNegationSymbols(d).empty();
}

std::set<PFSymbol*> ApproxRecursionOverNegationSymbols::execute(const Definition* d) {
	if (getOption(GUARANTEE_NO_REC_NEG)) {
		return {};
	}
	std::set<PFSymbol*> result;
	auto defsymbols = d->defsymbols();
	for (auto rule : d->rules()) {
		auto occs = FormulaUtils::collectSymbolOccurences(rule->body());
		for (auto occ : occs) {
			if (occ.second != Context::POSITIVE && defsymbols.find(occ.first) != defsymbols.end()) { //Defined symbol negative or both
				result.insert(occ.first);
			}
		}
	}
	return result;
}

std::set<PFSymbol*> RecursionOverNegationSymbols::execute(const Definition* d) {
	if (getOption(GUARANTEE_NO_REC_NEG)) {
		return {};
	}
	prepare();

	UniqueNames<PFSymbol*> usn;
	UniqueNames<Rule*> urn;
	UniqueNames<const Definition*> udn;

	auto structure = BootstrappingUtils::getDefinitionInfo(d, usn, urn, udn);
	auto result = handle(structure, usn);
	finish();
	return result;
}

void RecursionOverNegationSymbols::prepare() {
	savedOptions = BootstrappingUtils::setBootstrappingOptions();

	if (not getGlobal()->instance()->alreadyParsed("definitions")) {
		parsefile("definitions");
	}

	auto defnamespace = getGlobal()->getGlobalNamespace()->subspace("stdspace")->subspace("definitionbootstrapping");
	Assert(defnamespace != NULL);

	auto recnegvoc = defnamespace->vocabulary("recnegvoc");
	Assert(recnegvoc != NULL);
	recursivePredicates = recnegvoc->pred("recneg/2");
	Assert(recursivePredicates != NULL);

	auto deptheo = defnamespace->theory("dependency");
	Assert(deptheo != NULL);
	bootstraptheo = defnamespace->theory("recnegTheo");
	Assert(bootstraptheo != NULL);
	bootstraptheo = FormulaUtils::merge(bootstraptheo, deptheo);
	Assert(bootstraptheo != NULL);

}
void RecursionOverNegationSymbols::finish() {
	auto newOptions = getGlobal()->getOptions();
	getGlobal()->setOptions(savedOptions);
	bootstraptheo->recursiveDelete();
	delete newOptions;
}

std::set<PFSymbol*> RecursionOverNegationSymbols::handle(Structure* structure, UniqueNames<PFSymbol*> uniqueSymbols) {
	auto temptheo = bootstraptheo->clone();
	auto models = ModelExpansion::doModelExpansion(temptheo, structure, NULL, NULL, { })._models;
	if (models.size() != 1) {
		throw IdpException("Invalid code path: no solution to recursion over negation problem");
	}
	temptheo->recursiveDelete();

	auto splitmodel = models[0];
	splitmodel->makeTwoValued();

	auto recneginter = splitmodel->inter(recursivePredicates);
	std::set<PFSymbol*> result;
	for (auto it = recneginter->ct()->begin(); not it.isAtEnd(); ++it) {
		auto tuple = *it;
		Assert(tuple.size() == 2);
		auto el = tuple[1];
		Assert(el->type() == DET_INT);
		auto sym = uniqueSymbols.getOriginal(el->value()._int);
		result.insert(sym);
	}

	delete splitmodel;
	delete structure;
	return result;
}
