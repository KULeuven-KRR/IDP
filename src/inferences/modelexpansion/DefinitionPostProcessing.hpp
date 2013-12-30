#pragma once

#include "utils/UniqueNames.hpp"
#include "theory/TheoryUtils.hpp"

extern void parsefile(const std::string&);

std::string createName() {
	std::stringstream ss;
	ss << getGlobal()->getNewID();
	return ss.str();
}

template<class T>
const DomainElement* mapName(T elem, UniqueNames<T>& uniquenames) {
	return createDomElem(uniquenames.getUnique(elem));
}

std::vector<Definition*> simplifyTheoryForPostProcessableDefinitions(Theory* theory, Term* term, Structure* inputstructure, Vocabulary* fullvoc, Vocabulary* outputvoc) {
	if (theory->definitions().empty()) {
		return {};
	}

	auto orignbmodels = getOption(NBMODELS);
	auto defverb = getOption(VERBOSE_DEFINITIONS);
	auto savedoptions = getGlobal()->getOptions();
	auto newoptions = new Options(false);
	getGlobal()->setOptions(newoptions);
	setOption(POSTPROCESS_DEFS, false);
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
	auto splitvoc = new Vocabulary(createName());
	splitvoc->add(ns->vocabulary("defs_split_voc"));
	auto structure = new Structure(createName(), splitvoc, { });
	auto insentence = structure->inter(splitvoc->pred("insentence/1"));
	auto defines = structure->inter(splitvoc->pred("defines/2"));
	auto open = structure->inter(splitvoc->pred("open/2"));
	auto outsymbol = structure->inter(splitvoc->pred("outsymbol/1"));
	UniqueNames<PFSymbol*> uniquesymbnames;
	UniqueNames<Definition*> uniquedefnames;
	for (auto sent : theory->sentences()) {
		for (auto symb : FormulaUtils::collectSymbols(sent)) {
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

	auto definitions = theory->definitions();
	theory->definitions().clear();
	for (auto def : definitions) {
		if (not DefinitionUtils::approxTotal(def)) {
			for (auto symb : FormulaUtils::collectSymbols(def)) {
				insentence->ct()->add( { mapName(symb, uniquesymbnames) });
			}
			theory->add(def);
		} else {
			for (auto ds : DefinitionUtils::defined(def)) {
				defines->ct()->add( { mapName(def, uniquedefnames), mapName(ds, uniquesymbnames) });
			}
			for (auto os : DefinitionUtils::opens(def)) {
				open->ct()->add( { mapName(def, uniquedefnames), mapName(os, uniquesymbnames) });
			}
		}
	}
	for (auto s : outputvoc->getNonBuiltinNonOverloadedSymbols()) {
		outsymbol->ct()->add( { mapName<PFSymbol*>(s, uniquesymbnames) });
	}
	structure->checkAndAutocomplete();
	makeUnknownsFalse(outsymbol);
	makeUnknownsFalse(insentence);
	makeUnknownsFalse(defines);
	makeUnknownsFalse(open);
	structure->clean();
	auto processtheory = ns->theory("defs_split_theory")->clone();
	auto processterm = ns->term("defs_split_term")->clone();
	processtheory->vocabulary(splitvoc);
	processterm->vocabulary(splitvoc);
	auto splitsolutions = ModelExpansion::doMinimization(processtheory, structure, processterm, NULL, NULL)._models;
	if (splitsolutions.size() < 1) {
		throw IdpException("Invalid code path: no solution to splitting problem");
	}
	auto splitmodel = splitsolutions[0];
	splitmodel->makeTwoValued();
	if (defverb > 0) {
		std::clog << "Optimal postprocessing split: " << print(splitmodel) << "\n";
	}
	auto postelem = splitmodel->inter(splitvoc->func("post/0"))->value( { });
	auto theoryelem = splitmodel->inter(splitvoc->func("theory/0"))->value( { });
	auto dointer = splitmodel->inter(splitvoc->func("do/1"));

	std::vector<Definition*> postprocessdefs;
	for (auto i = dointer->funcTable()->begin(); not i.isAtEnd(); ++i) {
		auto def = uniquedefnames.getOriginal((*i)[0]->value()._int);
		auto val = (*i)[1];
		auto totheory = false;
		if (val == postelem) {
			if (orignbmodels != 1) { // TODO allow multiple models, by properly postprocessing open symbols
				totheory = true;
			} else {
				if (defverb > 0) {
					std::clog << "Postprocessing: " << print(def) << "\n";
				}
				postprocessdefs.push_back(def);
			}
		}
		if (val == theoryelem || totheory) {
			if (defverb > 0) {
				std::clog << "Considered during search: " << print(def) << "\n";
			}
			theory->add(def);
		}
	}
	deleteList(splitsolutions);
	processtheory->recursiveDelete();
	processterm->recursiveDelete();
	delete (newoptions);
	delete (structure);
	delete (splitvoc);
	getGlobal()->setOptions(savedoptions);
	return postprocessdefs;
}

void computeRemainingDefinitions(const std::vector<Definition*> defs, Structure* structure) {
	// Decide which symbols have to be two-valued to allow evaluation of all definitions
	std::set<PFSymbol*> needtwovalued;
	for (auto def : defs) {
		auto opens = DefinitionUtils::opens(def);
		needtwovalued.insert(opens.cbegin(), opens.cend());
	}
	for (auto i = needtwovalued.begin(); i != needtwovalued.end();) {
		bool found = false;
		for (auto def : defs) {
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
	for (auto def : defs) {
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
	CalculateDefinitions::doCalculateDefinitions(t, structure);
	t->recursiveDelete();
	getGlobal()->setOptions(oldoptions);
}
