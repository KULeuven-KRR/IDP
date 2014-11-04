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

#include "JoinDefinitionsForXSB.hpp"
#include "utils/UniqueNames.hpp"
#include "utils/ListUtils.hpp"
#include "utils/BootstrappingUtils.hpp"
#include "theory/TheoryUtils.hpp"
#include "inferences/definitionevaluation/CalculateDefinitions.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"

extern void parsefile(const std::string&);

Theory* JoinDefinitionsForXSB::execute(Theory* t, const Structure* s) {
	if(not getOption(JOIN_DEFS_FOR_XSB)){
		return t;
	}

	UniqueNames<PFSymbol*> usn;
	UniqueNames<Rule*> urn;
	UniqueNames<const Definition*> udn;

	auto structure = BootstrappingUtils::getDefinitionInfo(t, usn, urn, udn);

	prepare(structure,udn,s,usn);

	auto newDefs = joinForXSB(structure, udn);
	delete(structure);

	deleteList(t->definitions());
	t->definitions(newDefs);

	finish();
	return t;
}

std::vector<Definition*> JoinDefinitionsForXSB::joinForXSB(Structure* structure,
		UniqueNames<const Definition*>& uniqueDefinitionNames){
	auto temptheo = jointheo->clone();
	// Set the interpretation for xsbCalculable

	auto models = ModelExpansion::doModelExpansion(temptheo, structure, NULL, NULL, { })._models;
	if (models.size() != 1) {
		throw IdpException("Invalid code path: no solution to definition joining problem");
	}
	temptheo->recursiveDelete();

	auto joinmodel = models[0];
	joinmodel->makeTwoValued();

	auto sameDefInter = joinmodel->inter(sameDef);

	std::set<const DomainElement*> defsDone;
	auto alldefs = structure->inter(structure->vocabulary()->sort("def"));

	std::vector<Definition*> result;
	for(auto defIt = alldefs->begin(); not defIt.isAtEnd(); ++defIt){
		auto currentDefName = (*defIt)[0];
		if(contains(defsDone, currentDefName)){
			continue;
		}
		Assert(currentDefName->type() == DET_INT);

		auto currentDef = new Definition();
		result.push_back(currentDef);

		auto iterator = sameDefInter->ct()->begin();
		for (; not iterator.isAtEnd(); ++iterator) {
			auto currentTuple = *iterator;
			Assert(currentTuple.size() == 2);
			if (currentTuple[0] == currentDefName) {
				Assert(currentTuple[1]->type() == DET_INT);
				if(contains(defsDone, currentTuple[1])){
					continue;
				}
				auto otherDef = uniqueDefinitionNames.getOriginal(currentTuple[1]->value()._int);
				for (auto rule : otherDef->rules()) {
					currentDef->add(rule);
				}
				defsDone.insert(currentTuple[1]);
			}
		}
	}

	delete(joinmodel);
	return result;
}

void JoinDefinitionsForXSB::prepare(Structure* structure,
		UniqueNames<const Definition*>& uniqueDefinitionNames,
		const Structure* structureInput,
		UniqueNames<PFSymbol*>& uniqueSymbolNames) {
	if (not getGlobal()->instance()->alreadyParsed("definitions")) {
		parsefile("definitions");
	}

	auto defnamespace = getGlobal()->getGlobalNamespace()->subspace("stdspace")->subspace("definitionbootstrapping");
	Assert(defnamespace != NULL);

	auto joinVoc = defnamespace->vocabulary("def_join_voc");
	Assert(joinVoc != NULL);
	sameDef = joinVoc->pred("samedef/2");
	Assert(sameDef != NULL);
	xsbCalculable = joinVoc->pred("xsbcalculable/1");
	Assert(xsbCalculable != NULL);
	twoValuedSymbol = joinVoc->pred("twovaluedsymbol/1");
	Assert(twoValuedSymbol != NULL);

	auto higherTheo = defnamespace->theory("findHigher");
	Assert(higherTheo != NULL);
	jointheo = defnamespace->theory("def_join_theory");
	Assert(jointheo != NULL);
	jointheo = FormulaUtils::merge(jointheo, higherTheo);
	Assert(jointheo != NULL);

	structure->changeVocabulary(joinVoc);
	auto alldefs = structure->inter(jointheo->vocabulary()->sort("def"));

	for (auto defIt = alldefs->begin(); not defIt.isAtEnd(); ++defIt) {
		auto defid = (*defIt)[0]->value()._int;
		auto def = uniqueDefinitionNames.getOriginal(defid);
		if (CalculateDefinitions::determineXSBUsage(def)) {
			structure->inter(xsbCalculable)->ct()->add( { mapName(def, uniqueDefinitionNames) });
		}
		auto opens = DefinitionUtils::opens(def);
		for (auto open : opens) {
			if (structureInput->inter(open)->approxTwoValued()) {
				structure->inter(twoValuedSymbol)->ct()->add( { mapName(open, uniqueSymbolNames) });
			}
		}
	}
	structure->checkAndAutocomplete();
	makeUnknownsFalse(structure->inter(xsbCalculable));
	makeUnknownsFalse(structure->inter(twoValuedSymbol));
	structure->clean();

	savedOptions = BootstrappingUtils::setBootstrappingOptions();
}

void JoinDefinitionsForXSB::finish() {
	auto newOptions = getGlobal()->getOptions();
	getGlobal()->setOptions(savedOptions);
	jointheo->recursiveDelete();
	delete newOptions;
}



