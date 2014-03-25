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

#include "SplitDefinitions.hpp"
#include "theory/TheoryUtils.hpp"
#include "utils/UniqueNames.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"

#include "IncludeComponents.hpp"

extern void parsefile(const std::string&);

Theory* SplitDefinitions::execute(Theory* t) {
	prepare();

	std::vector<Definition*> newDefs;
	auto oldDefs = t->definitions();

	t->definitions().clear();

	for (auto def : oldDefs) {
		auto add = split(def);
		newDefs.insert(newDefs.end(), add.begin(), add.end());
	}

	t->definitions(newDefs);

	finish();
	return t;
}

Structure* SplitDefinitions::turnIntoStructure(Definition* d, UniqueNames<PFSymbol*>& uniqueSymbNames, UniqueNames<Rule*>& uniqueRuleNames) {
	auto structure = new Structure(createName(), splitVoc, { });
	auto definesInter = structure->inter(definesFunc);
	auto openInter = structure->inter(openPred);

	for (auto r : d->rules()) {
		auto ruleName = mapName(r, uniqueRuleNames);

		auto head = r->head();
		PFSymbol* definedSymbol;
		if (is(head->symbol(), STDPRED::EQ)) {
			auto headTerm = head->subterms()[0];
			auto funcHeadTerm = dynamic_cast<FuncTerm*>(headTerm);
			Assert(funcHeadTerm != NULL);
			definedSymbol = funcHeadTerm->function();
		} else {
			definedSymbol = head->symbol();
		}

		auto symbolName = mapName(definedSymbol, uniqueSymbNames);
		definesInter->add( { ruleName, symbolName });

		//Warning: in current implementation: the defined symbol is always in the set of "opens"
		auto opens = FormulaUtils::collectSymbols(r);
		for (auto open : opens) {
			symbolName = mapName(open, uniqueSymbNames);
			openInter->ct()->add( { ruleName, symbolName });
		}
	}
	structure->checkAndAutocomplete();
	makeUnknownsFalse(definesInter->graphInter());
	makeUnknownsFalse(openInter);
	structure->clean();
	return structure;
}

std::vector<Definition*> SplitDefinitions::split(Definition* d) {

	UniqueNames<PFSymbol*> uniqueSymbNames;
	UniqueNames<Rule*> uniqueRuleNames;

	auto structure = turnIntoStructure(d, uniqueSymbNames, uniqueRuleNames);

	auto temptheo = splitTheo->clone();
	auto models = ModelExpansion::doModelExpansion(temptheo, structure, NULL, NULL, { })._models;
	if (models.size() != 1) {
		throw IdpException("Invalid code path: no solution to splitting problem");
	}
	temptheo->recursiveDelete();
	auto splitmodel = models[0];
	splitmodel->makeTwoValued();

	auto sameDefInter = splitmodel->inter(sameDef);

	ruleset rulesTODO = ruleset(d->rules());
	std::vector<Definition*> result;

	while (not rulesTODO.empty()) {
		auto currentRule = *rulesTODO.begin();
		auto currentRuleName = mapName(currentRule, uniqueRuleNames);

		auto currentDef = new Definition();
		result.push_back(currentDef);

		auto iterator = sameDefInter->ct()->begin();
		for (; not iterator.isAtEnd(); ++iterator) {
			auto currentTuple = *iterator;
			Assert(currentTuple.size() == 2);
			if (currentTuple[0] == currentRuleName) {
				Assert(currentTuple[1]->type() == DET_INT);
				auto otherRule = uniqueRuleNames.getOriginal(currentTuple[1]->value()._int);
				currentDef->add(otherRule);
				rulesTODO.erase(otherRule);
			}
		}

	}

	delete splitmodel;
	delete structure;
	delete d;
	return result;
}

void SplitDefinitions::prepare() {
	savedOptions = getGlobal()->getOptions();
	auto newoptions = new Options(false);
	getGlobal()->setOptions(newoptions);
	setOption(POSTPROCESS_DEFS, false);
	setOption(SPLIT_DEFS, false); //Important to avoid loops!
	setOption(GROUNDWITHBOUNDS, true);
	setOption(LIFTEDUNITPROPAGATION, true);
	setOption(LONGESTBRANCH, 12);
	setOption(NRPROPSTEPS, 12);
	setOption(CPSUPPORT, true);
	setOption(TSEITINDELAY, false);
	setOption(SATISFIABILITYDELAY, false);
	setOption(NBMODELS, 1);
	setOption(AUTOCOMPLETE, true);
	setOption(BoolType::SHOWWARNINGS, false);

	if (not getGlobal()->instance()->alreadyParsed("definition_splitting")) {
		parsefile("definition_splitting");
	}

	auto ns = getGlobal()->getGlobalNamespace()->subspace("stdspace")->subspace("definitionsplitting");
	splitVoc = ns->vocabulary("def_split_voc");
	Assert(splitVoc != NULL);
	splitTheo = ns->theory("def_split_theory");
	Assert(splitTheo != NULL);

	definesFunc = splitVoc->func("defines/1");
	Assert(definesFunc != NULL);
	openPred = splitVoc->pred("open/2");
	Assert(openPred != NULL);
	sameDef = splitVoc->pred("samedef/2");
	Assert(sameDef != NULL);
}

void SplitDefinitions::finish() {
	auto newOptions = getGlobal()->getOptions();
	getGlobal()->setOptions(savedOptions);
	delete newOptions;
}

