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
#include "utils/UniqueNames.hpp"
#include "utils/ListUtils.hpp"
#include "theory/TheoryUtils.hpp"
#include "utils/BootstrappingUtils.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"

extern void parsefile(const std::string&);

Theory* SplitDefinitions::execute(Theory* t) {
	if (not getOption(SPLIT_DEFS)) {
		return t;
	}
	prepare();

	UniqueNames<PFSymbol*> usn;
	UniqueNames<Rule*> urn;
	UniqueNames<const Definition*> udn;

	auto structure = BootstrappingUtils::getDefinitionInfo(t, usn, urn, udn);
	auto newDefs = split(structure, urn);
	delete(structure);

	deleteList(t->definitions());
	t->definitions(newDefs);

	finish();
	return t;
}

std::vector<Definition*> SplitDefinitions::split(Structure* structure, UniqueNames<Rule*>& uniqueRuleNames){
	auto temptheo = splittheo->clone();
	auto models = ModelExpansion::doModelExpansion(temptheo, structure, NULL, NULL, { })._models;
	if (models.size() != 1) {
		throw IdpException("Invalid code path: no solution to definition splitting problem");
	}
	temptheo->recursiveDelete();

	auto splitmodel = models[0];
	splitmodel->makeTwoValued();

	auto sameDefInter = splitmodel->inter(sameDef);

	std::set<const DomainElement*> rulesDone;
	auto allrules = structure->inter(structure->vocabulary()->sort("rule"));

	std::vector<Definition*> result;
	for(auto ruleIt = allrules->begin(); not ruleIt.isAtEnd(); ++ruleIt){
		auto currentRuleName = (*ruleIt)[0];
		if(contains(rulesDone, currentRuleName)){
			continue;
		}
		Assert(currentRuleName->type() == DET_INT);

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
				rulesDone.insert(currentTuple[1]);
			}
		}
	}

	delete(splitmodel);
	return result;
}

void SplitDefinitions::prepare() {
	savedOptions = BootstrappingUtils::setBootstrappingOptions();

	if (not getGlobal()->instance()->alreadyParsed("definitions")) {
		parsefile("definitions");
	}

	auto defnamespace = getGlobal()->getGlobalNamespace()->subspace("stdspace")->subspace("definitionbootstrapping");
	Assert(defnamespace != NULL);

	auto splitVoc = defnamespace->vocabulary("def_split_voc");
	Assert(splitVoc != NULL);
	sameDef = splitVoc->pred("samedef/2");
	Assert(sameDef != NULL);

	auto deptheo = defnamespace->theory("dependency");
	Assert(deptheo != NULL);
	splittheo = defnamespace->theory("def_split_theory");
	Assert(splittheo != NULL);
	splittheo = FormulaUtils::merge(splittheo, deptheo);
	Assert(splittheo != NULL);
}

void SplitDefinitions::finish() {
	auto newOptions = getGlobal()->getOptions();
	getGlobal()->setOptions(savedOptions);
	splittheo->recursiveDelete();
	delete newOptions;
}

