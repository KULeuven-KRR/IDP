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
#include "BootstrappingUtils.hpp"
#include "common.hpp"
#include "commontypes.hpp"
#include "utils/UniqueNames.hpp"
#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"

extern void parsefile(const std::string&);

namespace BootstrappingUtils {
template<class Def>
Structure* getDefinitionInfo(const std::vector<Def>& defs, UniqueNames<PFSymbol*>& uniqueSymbNames, UniqueNames<Rule*>& uniqueRuleNames,
		UniqueNames<Def>& uniqueDefNames) {
	if (not getGlobal()->instance()->alreadyParsed("definitions")) {
		parsefile("definitions");
	}

	//SET BASIC DATA:REFERENCE TO FILES ETCETERA
	auto defnamespace = getGlobal()->getGlobalNamespace()->subspace("stdspace")->subspace("definitionbootstrapping");
	Assert(defnamespace != NULL);

	auto defVoc = defnamespace->vocabulary("basicdata");
	auto structure = new Structure(createName(), defVoc, { });

	auto definesFunc = defVoc->func("defines/1");
	Assert(definesFunc != NULL);
	auto definesInter = structure->inter(definesFunc);

	auto openPred = defVoc->pred("inbody/3");
	Assert(openPred != NULL);
	auto openInter = structure->inter(openPred);

	auto infunc = defVoc->func("in/1");
	Assert(infunc != NULL);
	auto inInter = structure->inter(infunc);

	auto posfunc = defVoc->func("pos/0");
	auto negfunc = defVoc->func("neg/0");
	Assert(posfunc != NULL);
	Assert(negfunc != NULL);
	auto pos = structure->inter(posfunc)->value( { });
	auto neg = structure->inter(negfunc)->value( { });
	Assert(pos != NULL);
	Assert(neg != NULL);

	for (auto d : defs) {
		auto defname = mapName(d, uniqueDefNames);

		for (auto r : d->rules()) {
			auto ruleName = mapName(r, uniqueRuleNames);
			inInter->add( { ruleName, defname });

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

			auto ruleclone = r->clone();
			ruleclone = DefinitionUtils::unnestHeadTermsNotVarsOrDomElems(ruleclone, structure);
			auto opens = FormulaUtils::collectSymbolOccurences(ruleclone->body());
			for (auto open : opens) {
				symbolName = mapName(open.first, uniqueSymbNames);
				auto symbolOcc = open.second;
				if (symbolOcc != Context::NEGATIVE) { //Positive or both -> certainly positive
					openInter->ct()->add( { ruleName, symbolName, pos });
				}
				if (symbolOcc != Context::POSITIVE) { //Negative or both -> certainly negative
					openInter->ct()->add( { ruleName, symbolName, neg });
				}
			}
		}

	}
	structure->checkAndAutocomplete();
	makeUnknownsFalse(definesInter->graphInter());
	makeUnknownsFalse(inInter->graphInter());
	makeUnknownsFalse(openInter);
	structure->clean();
	return structure;

}

Structure* getDefinitionInfo(const Definition* d, UniqueNames<PFSymbol*>& usn, UniqueNames<Rule*>& urn, UniqueNames<const Definition*>& udn) {
	return getDefinitionInfo(std::vector<const Definition*>({ d }), usn, urn, udn);
}
Structure* getDefinitionInfo(Definition* d, UniqueNames<PFSymbol*>& usn, UniqueNames<Rule*>& urn, UniqueNames<Definition*>& udn) {
	return getDefinitionInfo(std::vector<Definition*>({ d }), usn, urn, udn);
}
Structure* getDefinitionInfo(const Theory* t, UniqueNames<PFSymbol*>& usn, UniqueNames<Rule*>& urn, UniqueNames<const Definition*>& udn) {
	std::vector<const Definition*> defs;
	for(auto d: t->definitions()){
		defs.push_back(d);
	}
	return getDefinitionInfo(defs, usn, urn, udn);
}
Structure* getDefinitionInfo(const Theory* t, UniqueNames<PFSymbol*>& usn, UniqueNames<Rule*>& urn, UniqueNames<Definition*>& udn) {
	std::vector<Definition*> defs;
	for(auto d: t->definitions()){
		defs.push_back(d);
	}
	return getDefinitionInfo(defs, usn, urn, udn);
}

Options* setBootstrappingOptions() {
	//All bootstrapping methods that use this procedure should have a dedicated option that is set to false here and that guarantees that
	//that specific bootstrapping procedure will not be executed again.
	auto old = getGlobal()->getOptions();
	auto newoptions = new Options(false);
	getGlobal()->setOptions(newoptions);
	setOption(POSTPROCESS_DEFS, false); //Important since postprocessing is implemented with bootstrapping
	setOption(SPLIT_DEFS, false); //Important since splitting is implemented with bootstrapping
	setOption(JOIN_DEFS_FOR_XSB, false); //Important since joining is implemented with bootstrapping
	setOption(GUARANTEE_NO_REC_NEG, true); //Important since checking recursion over negation is implemented with bootstrapping

	setOption(GROUNDWITHBOUNDS, true);
	setOption(LIFTEDUNITPROPAGATION, true);
	setOption(LONGESTBRANCH, 12);
	setOption(NRPROPSTEPS, 12);
	setOption(CPSUPPORT, true);
	setOption(TSEITINDELAY, false);
	setOption(XSB, true);
	setOption(SATISFIABILITYDELAY, false);
	setOption(NBMODELS, 1);
	setOption(AUTOCOMPLETE, true);
	setOption(BoolType::SHOWWARNINGS, false);
	return old;
}

}
