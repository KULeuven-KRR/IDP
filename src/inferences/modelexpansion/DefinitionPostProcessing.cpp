/* 
 * File:   DefinitionPostProcessing.cpp
 * Author: rupsbant
 * 
 * Created on November 18, 2014, 2:00 PM
 */

#include "DefinitionPostProcessing.hpp"
#include "ModelExpansion.hpp"
#include "utils/BootstrappingUtils.hpp"
#include "theory/TheoryUtils.hpp"

std::vector<Definition*> simplifyTheoryForPostProcessableDefinitions(Theory* theory, Term* term, Structure* inputstructure, Vocabulary* fullvoc,
		Vocabulary* outputvoc) {
	if (theory->definitions().empty()) {
		return {};
	}

	std::set<Definition*> nonTotalDefs;
	for (auto def : theory->definitions()) {
		//IMPORTANT: this check has to happen before setting bootstrapping options, since bootstrapping option assume all definitoins to be total.
		if (not DefinitionUtils::approxTotal(def)) {
			nonTotalDefs.insert(def);
		}
	}

	auto orignbmodels = getOption(NBMODELS);
	auto defverb = getOption(VERBOSE_DEFINITIONS);

	auto savedoptions = BootstrappingUtils::setBootstrappingOptions();
	UniqueNames<PFSymbol*> uniquesymbnames;
	UniqueNames<Rule*> uniquerulenames;
	UniqueNames<Definition*> uniquedefnames;

	auto structure = BootstrappingUtils::getDefinitionInfo(theory, uniquesymbnames, uniquerulenames, uniquedefnames);

	auto defnamespace = getGlobal()->getGlobalNamespace()->subspace("stdspace")->subspace("definitionbootstrapping");
	Assert(defnamespace != NULL);
	auto voc = defnamespace->vocabulary("prepost");
	Assert(voc != NULL);
	structure->changeVocabulary(voc);
	auto insentence = structure->inter(voc->pred("inFO/1"));
	Assert(insentence != NULL);
	auto outsymbol = structure->inter(voc->pred("output/1"));
	Assert(outsymbol != NULL);

	for (auto sent : theory->sentences()) {
		for (auto symb : FormulaUtils::collectSymbols(sent)) {
			insentence->ct()->add( { mapName(symb, uniquesymbnames) });
		}
	}
	for (auto def : nonTotalDefs) {
		for (auto symb : def->defsymbols()) {
			insentence->ct()->add( { mapName(symb, uniquesymbnames) });
		}
	}
	if (term != NULL) {
		for (auto symb : FormulaUtils::collectSymbols(term)) {
			insentence->ct()->add( { mapName(symb, uniquesymbnames) });
		}
	}
	for (auto s : fullvoc->getNonBuiltinNonOverloadedSymbols()) {
		if (not inputstructure->inter(s)->ct()->approxEmpty() || not inputstructure->inter(s)->cf()->approxEmpty()) {
			insentence->ct()->add( { mapName(s, uniquesymbnames) });
		}
	}

	for (auto s : outputvoc->getNonBuiltinNonOverloadedSymbols()) {
		outsymbol->ct()->add( { mapName<PFSymbol*>(s, uniquesymbnames) });
	}
	structure->checkAndAutocomplete();
	makeUnknownsFalse(outsymbol);
	makeUnknownsFalse(insentence);
	structure->clean();

	auto processtheory = FormulaUtils::merge(defnamespace->theory("prepostTheo"), defnamespace->theory("findHigher"));
	auto splitsolutions = ModelExpansion::doModelExpansion(processtheory, structure, NULL, NULL)._models;
	if (splitsolutions.size() < 1) {
		throw IdpException("Invalid code path: no solution to splitting problem");
	}
	auto splitmodel = splitsolutions[0];
	splitmodel->makeTwoValued();
	if (defverb > 0) {
		std::clog << "Optimal postprocessing split: " << print(splitmodel) << "\n";
	}

	auto postprocess = splitmodel->inter(voc->pred("post/1"));
	auto search = splitmodel->inter(voc->pred("search/1"));

	theory->definitions().clear();
	std::vector<Definition*> postprocessdefs;
	for (auto i = postprocess->ct()->begin(); not i.isAtEnd(); ++i) {
		auto domelem = (*i)[0];
		auto origdef = uniquedefnames.getOriginal(domelem->value()._int);
		if (orignbmodels != 1) {
			theory->add(origdef);
		} else {
			if (defverb > 0) {
				std::clog << "Postprocessing: " << print(origdef) << "\n";
			}
			postprocessdefs.push_back(origdef);
		}
	}
	for (auto i = search->ct()->begin(); not i.isAtEnd(); ++i) {
		auto domelem = (*i)[0];
		auto origdef = uniquedefnames.getOriginal(domelem->value()._int);
		theory->add(origdef);
	}

	deleteList(splitsolutions);
	processtheory->recursiveDelete();
	delete getGlobal()->getOptions();
	delete (structure);
	getGlobal()->setOptions(savedoptions);
	return postprocessdefs;
}

void computeRemainingDefinitions(const std::vector<Definition*> postprocessdefs, Structure* structure, Vocabulary* outputvoc) {
	if (postprocessdefs.empty()) {
		return;
	}

	// First, we recheck which definitions we really need to evaluate (possible including those used as constructions in lazy grounding)
	std::queue<PFSymbol*> sq;
	std::map<PFSymbol*, std::vector<Definition*>> s2defs;
	for (auto d : postprocessdefs) {
		for (auto s : d->defsymbols()) {
			s2defs[s].push_back(d);
			if (not structure->inter(s)->approxTwoValued() && outputvoc->contains(s)) {
				sq.push(s);
			}
		}
	}
	std::set<Definition*> evaluatedefs;
	while (not sq.empty()) {
		auto s = sq.front();
		sq.pop();
		for (auto d : s2defs[s]) {
			if (evaluatedefs.find(d) == evaluatedefs.cend()) {
				evaluatedefs.insert(d);
				for (auto os : DefinitionUtils::opens(d)) {
					sq.push(os);
				}
			}
		}
	}

	// Decide which symbols have to be two-valued to allow evaluation of all definitions
	std::set<PFSymbol*> needtwovalued;
	for (auto def : evaluatedefs) {
		auto opens = DefinitionUtils::opens(def);
		needtwovalued.insert(opens.cbegin(), opens.cend());
	}
	for (auto i = needtwovalued.begin(); i != needtwovalued.end();) {
		bool found = false;
		for (auto def : evaluatedefs) {
			if (contains(DefinitionUtils::defined(def), *i)) {
				found = true;
			}
		}
		if (found) {
			i = needtwovalued.erase(i);
		} else {
			++i;
		}
	}
	// Complete all those false // TODO should be improved to allow it for multiple models
	for (auto s : needtwovalued) {
		Assert(not s->overloaded());
		if (s->builtin()) {
			continue;
		}
		if (s->isPredicate()) {
			auto p = dynamic_cast<Predicate*>(s);
			makeTwoValued(p, structure->inter(p));
		} else {
			auto f = dynamic_cast<Function*>(s);
			makeTwoValued(f, structure->inter(f));
		}
	}

	// Evaluate it (preferably with XSB)
	auto t = new Theory(createName(), structure->vocabulary(), { });
	for (auto def : evaluatedefs) {
		t->add(def->clone()); // TODO currently because calculate deletes definitions (and now we might need them also later on)
	}
	if (getOption(VERBOSE_DEFINITIONS) > 0) {
		std::clog << "Evaluating post-search: " << print(t) << "\n";
	}
	auto newoptions = new Options(false);
	auto oldoptions = getGlobal()->getOptions();
	newoptions->copyValues(*oldoptions);
	getGlobal()->setOptions(newoptions);
	setOption(XSB, true);
	setOption(POSTPROCESS_DEFS, false);
	auto result = CalculateDefinitions::doCalculateDefinitions(t, structure);
	Assert(result._hasModel);
	t->recursiveDelete();
	getGlobal()->setOptions(oldoptions);
}
