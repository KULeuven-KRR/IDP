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

#include "LTCTheorySplitter.hpp"
#include "data/LTCData.hpp"
#include "IncludeComponents.hpp"
#include "data/SplitLTCTheory.hpp"
#include "ReplaceLTCSymbols.hpp"
#include "data/StateVocInfo.hpp"
#include "theory/TheoryUtils.hpp"
#include "errorhandling/error.hpp"
#include "utils/ListUtils.hpp"
#include "creation/cppinterface.hpp"

SplitLTCTheory* LTCTheorySplitter::SplitTheory(const AbstractTheory* ltcTheo) {
	auto g = LTCTheorySplitter();
	if (not isa<const Theory>(*ltcTheo)) {
		throw IdpException("Can only perform progression on theories");
	}
	auto theo = dynamic_cast<const Theory*>(ltcTheo);
	return g.split(theo);
}

SplitLTCInvariant* LTCTheorySplitter::SplitInvariant(const AbstractTheory* invar) {
	auto g = LTCTheorySplitter();
	if (not isa<const Theory>(*invar)) {
		throw IdpException("Can only perform invariant checking on theories");
	}
	auto inv = dynamic_cast<const Theory*>(invar);
	return g.splitInvar(inv);
}

LTCTheorySplitter::LTCTheorySplitter()
		: 	_initialTheory(NULL),
			_bistateTheory(NULL),
			_ltcVoc(NULL),
			_time(NULL),
			_start(NULL),
			_next(NULL),
			_vocInfo(NULL) {
}
LTCTheorySplitter::~LTCTheorySplitter() {

}
template<class T>
LTCFormulaInfo LTCTheorySplitter::info(T* t) {
	LTCFormulaInfo result;
	auto symbols = FormulaUtils::collectSymbols(t);
	result.containsStart = contains(symbols, _start);
	result.containsNext = contains(symbols, _next);

	auto vars = FormulaUtils::collectVariables(t);
	result.hasTimeVariable = false;
	for (auto var : vars) {
		if (var->sort() == _time) {
			result.hasTimeVariable = true;
		}
	}

	return result;
}

void LTCTheorySplitter::createTheories(const Theory* theo, bool single_state_invar) {
	_initialTheory = new Theory(theo->name() + "_init", _vocInfo->stateVoc, theo->pi());
	_bistateTheory = new Theory(theo->name() + "_bistate", _vocInfo->biStateVoc, theo->pi());
	auto workingTheo = theo->clone();
	FormulaUtils::removeEquivalences(workingTheo);
	FormulaUtils::pushNegations(workingTheo);
	FormulaUtils::flatten(workingTheo);
	checkQuantificationsForTheory(workingTheo);

	/*
	 * For the initial theory:
	 * * we remove all bistate formulas.
	 * * We keep static formulas
	 * * We keep initial formulas
	 * * We instantiate one-state formulas with the state-predicate
	 *
	 * For the bistate theory:
	 * * we keep all bistate formulas.
	 * * We keep static formulas
	 * * We remove initial formulas
	 * * We instantiate one-state formulas with the next-state-predicate
	 */
	for (auto sentence : workingTheo->sentences()) {
		handleAndAddToConstruct(sentence, _initialTheory, _bistateTheory, single_state_invar);
	}
	for (auto def : workingTheo->definitions()) {
		if(single_state_invar){
			Error::LTC::invarContainsDefinitions(workingTheo->pi());
		}
		auto initDef = new Definition();
		auto biStateDef = new Definition();
		for (auto rule : def->rules()) {
			auto head = rule->head();
			auto body = rule->body();
			auto headinfo = info(head);
			auto bodyinfo = info(body);
			auto headstatic = not (headinfo.hasTimeVariable || headinfo.containsStart);
			auto bodystatic = not (bodyinfo.hasTimeVariable || bodyinfo.containsStart);
			if (headstatic && not bodystatic) {
				Error::LTC::defineStaticInTermsOfDynamic(rule->pi());
			}
			if (bodyinfo.containsNext && not headinfo.containsNext) {
				Error::LTC::timeStratificationViolated(rule->pi());
			}
			if(headinfo.hasTimeVariable && not headinfo.containsNext){
				//Special case: rule of the form !t: P(t) <- \phi(t)
				//This is dangerous in case that SAME defintion also contains rules of the form !t: P(next(t)) <- psi(t).
				//since only part of  the rules defining P will end up in the same definition!!!!
				//Solution: explicitely split this rules in two, one with t=Start and one with t next t

				//NOTE: in case we are not in the special case, we do not wish to do this simplifications, since this might reduce the
				//number of provable invariants
				bool specialcase = false;
				for(auto otherRule: def->rules()){
					if(otherRule == rule){
						continue;
					}
					auto otherHead = otherRule->head();
					auto symbol = otherHead->symbol();
					if(symbol != head->symbol() ){
						continue;
					}
					auto otherInfo = info(otherHead);
					if(otherInfo.containsNext){
						specialcase=true;
						break;
					}
				}
				handleAndAddToConstruct(rule, initDef, biStateDef, single_state_invar, specialcase);

			} else{
				handleAndAddToConstruct(rule, initDef, biStateDef, single_state_invar);
			}

		}
		auto defsymbols = def->defsymbols();
		auto initDefsymbols = initDef->defsymbols();
		auto biDefsymbols = biStateDef->defsymbols();

		//Add false defined symbols: for defined symbols that no longer have a rule now, we add a rule defining it false.
		for (auto sym : defsymbols) {
			if (contains(_vocInfo->LTC2State, sym)) {
				auto statesym = _vocInfo->LTC2State.at(sym);
				auto nextsym = _vocInfo->LTC2NextState.at(sym);
				if (not contains(initDefsymbols, statesym)) {
					initDef->add(DefinitionUtils::falseRule(statesym));
				}
				if (not contains(biDefsymbols, nextsym)) {
					biStateDef->add(DefinitionUtils::falseRule(nextsym));
				}
			}
		}

		if (initDef->rules().size() != 0) {
			_initialTheory->add(initDef);
		} else {
			delete (initDef);
		}
		if (biStateDef->rules().size() != 0) {
			_bistateTheory->add(biStateDef);
		} else {
			delete (biStateDef);
		}
	}

	workingTheo->recursiveDelete();
}

template<class Form, class Construct>
void LTCTheorySplitter::handleAndAddToConstruct(Form* sentence, Construct* initConstruct, Construct* biStateConstruct, bool invar, bool onlyStartNext) {
	auto sentenceInfo = info(sentence);
	auto newSentence = sentence->clone();
	//We already checked quantifications. They are okay now. First, we remove all quantifications over time,
	//later, we will check which type of formula we are dealing with and handle it appropriately.
	newSentence = FormulaUtils::removeQuantificationsOverSort(newSentence, _time);
	auto pi=sentence->pi();



	if (sentenceInfo.containsStart) {
		if(invar){
			//Extra checks for invariants: they cannot contain START
			Error::LTC::invarContainsStart(pi);
		}
		if (sentenceInfo.containsNext) {
			Error::LTC::containsStartAndNext(pi);
		}
		if (sentenceInfo.hasTimeVariable) {
			Error::LTC::containsStartAndOther(pi);
		}
		newSentence = ReplaceLTCSymbols::replaceSymbols(newSentence, _ltcVoc, false);
		initConstruct->add(newSentence);
	} else if (sentenceInfo.containsNext) {
		if (invar) {
			//Extra checks for invariants: they cannot contain START
			Error::LTC::invarContainsNext(pi);
		}
		Assert(not sentenceInfo.containsStart);
		//Should be guaranteed by previous case
		if(not sentenceInfo.hasTimeVariable){
			Error::LTC::invalidTimeTerm(pi);
		}
		Assert(sentenceInfo.hasTimeVariable);
		//Don't know what else could be filled in here.
		newSentence = ReplaceLTCSymbols::replaceSymbols(newSentence, _ltcVoc, false);
		biStateConstruct->add(newSentence);
	} else if (sentenceInfo.hasTimeVariable) {
		auto newNextSentence = newSentence->clone();
		//This kind of sentences needs to be added to both theories. For the initial theory, this sentence constraints the initial state
		//For bistate theory: this sentence constraints the next state.
		newSentence = ReplaceLTCSymbols::replaceSymbols(newSentence, _ltcVoc, false);
		newNextSentence = ReplaceLTCSymbols::replaceSymbols(newNextSentence, _ltcVoc, true);
		initConstruct->add(newSentence);
		biStateConstruct->add(newNextSentence);
		if((not invar) && not onlyStartNext){
			auto newTSentence = newSentence->clone();
			biStateConstruct->add(newTSentence);
		}
	} else {
		if (invar) {
			//Extra checks for invariants: they cannot contain START
			Error::LTC::invarIsStatic(pi);
		}
		//Static formulas are added both to Init and next state. TODO we might also leave it out of the bistate formula.
		auto newNextSentence = newSentence->clone();
		initConstruct->add(newSentence);
		biStateConstruct->add(newNextSentence);
	}
}

void LTCTheorySplitter::initializeVariables(const Theory* theo) {
	_ltcVoc = theo->vocabulary();
	auto data = LTCData::instance();
	Assert(data->hasBeenTransformed(_ltcVoc));
	_vocInfo = data->getStateVocInfo(_ltcVoc);
	_time = _vocInfo->time;
	_start = _vocInfo->start;
	_next = _vocInfo->next;

	Assert(_time != NULL);
	Assert(_start != NULL);
	Assert(_next != NULL);

}

SplitLTCTheory* LTCTheorySplitter::split(const Theory* theo) {
	Assert(theo != NULL);

	initializeVariables(theo);
	createTheories(theo, false);

	auto result = new SplitLTCTheory();
	result->initialTheory = _initialTheory;
	result->bistateTheory = _bistateTheory;
	if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 0) {
		std::clog << "Splitting the LTC theory\n" << toString(theo) << "\nresulted in the following two theories: \n" << toString(_initialTheory) << "\n"
				<< toString(_bistateTheory) << "\n";
	}
	return result;
}

SplitLTCInvariant* LTCTheorySplitter::splitInvar(const Theory* theo) {
	Assert(theo != NULL);

	initializeVariables(theo);
	auto symbols = FormulaUtils::collectSymbols(theo);
	if (contains(symbols, _next)) {
		//This is a bistate invariant -> bistate formula -> Create theories the usual way
		createTheories(theo, false);
		if (not _initialTheory->definitions().empty() || not _bistateTheory->definitions().empty()) {
			Error::LTC::invarContainsDefinitions(theo->pi());
		}
		if (not _initialTheory->sentences().empty()) {
			Error::LTC::invarContainsStart(theo->pi());
		}
		auto result = new SplitLTCInvariant();
		result->invartype = InvarType::BistateInvar;
		result->bistateInvar = new BoolForm(SIGN::POS, true, _bistateTheory->sentences(), FormulaParseInfo());
		if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 0) {
			std::clog << "Splitting the LTC bistate invariant\n" << toString(theo) << "\nresulted in the following formula: \n"
					<< toString(result->bistateInvar) << "\n";
		}
		_initialTheory->recursiveDelete();
		delete (_bistateTheory); //only delete top of the bistate theory
		return result;
	}

	//This is the case where we have a single-state invariant.
	createTheories(theo, true);

	auto result = new SplitLTCInvariant();
	result->invartype = InvarType::SingleStateInvar;
	//For transforming ss-invariants, we still need to post-process a bit.
	//_initTheo contains all invar constraints on time Start, we still need to conjoin this
	// _bistateTheo, contains invar on time t+1. Should be transformed to: allinvars(t) => allinvars(t+1)
	auto allinvars = new BoolForm(SIGN::POS, true, _initialTheory->sentences(), FormulaParseInfo());
	auto allinvarsNext = new BoolForm(SIGN::POS, true, _bistateTheory->sentences(), FormulaParseInfo());
	auto notallinvars = allinvars->clone();
	notallinvars->negate();

	//Formula(Start)
	result->baseStep = allinvars;
	//Formula(now) => Formula(next)
	result->inductionStep = new BoolForm(SIGN::POS, false, notallinvars, allinvarsNext, FormulaParseInfo());
	if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 0) {
		std::clog << "Splitting the LTC invariant\n" << toString(theo) << "\nresulted in the following two formulas: \n" << toString(result->baseStep) << "\n"
				<< toString(result->inductionStep) << "\n";
	}
	delete (_initialTheory);
	delete (_bistateTheory);

	return result;
}

template<class T>
void LTCTheorySplitter::checkQuantifications(T* t) {
	auto vars = FormulaUtils::collectQuantifiedVariables(t, true);
	auto topLevelVars = FormulaUtils::collectQuantifiedVariables(t, false);

	bool timefound = false;
	Variable* timevar = NULL;
	for (auto tuple : vars) {
		auto var = tuple.first;
		if (var->sort() == _time) {
			if (timefound) {
				Assert(timevar != NULL);
				Error::LTC::multipleTimeVars(toString(timevar), toString(var), t->pi());
			}
			timefound = true;
			timevar = var;
			if (tuple.second != QuantType::UNIV) {
				Error::LTC::wrongTimeQuantification(toString(var), t->pi());
			}
			if (not contains(topLevelVars, var)) {
				Error::LTC::nonTopLevelTimeVar(toString(var), t-> pi());
			}
		}
	}
}
void LTCTheorySplitter::checkQuantificationsForTheory(const Theory* theo) {
	for (auto sentence : theo->sentences()) {
		checkQuantifications(sentence);
	}
	for (auto def : theo->definitions()) {
		for (auto rule : def->rules()) {
			checkQuantifications(rule);
		}
	}
}

