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

#include "transformLTCVocabulary.hpp"
#include "common.hpp"
#include "IncludeComponents.hpp"
#include "utils/ListUtils.hpp"
#include "data/LTCData.hpp"

LTCVocabularyTransformer::LTCVocabularyTransformer(const Vocabulary* ltcVoc, LTCInputData input)
		: 	_ltcVoc(ltcVoc),
			_inputData(input),
			_result(NULL) /*Guaranteed to be set in transform()*/{
}
LTCVocabularyTransformer::~LTCVocabularyTransformer() {

}

void LTCVocabularyTransformer::projectAndAddSymbol(PFSymbol* pred) {

	std::vector<PFSymbol*> newSymbols;
	if(LTCData::instance()->hasBeenProjected(pred)){
		projectSorts(pred); //Needed: sets the index of time.
		newSymbols = LTCData::instance()->getProjection(pred);
	} else{
		newSymbols = projectSymbol(pred);
		LTCData::instance()->setProjection(pred,newSymbols);
	}
	if (newSymbols.size() == 0) {
		return;
	} else if (newSymbols.size() == 1) {
		Assert(newSymbols[0] == pred);
		_result->stateVoc->add(pred);
		_result->biStateVoc->add(pred);
	} else {
		Assert(newSymbols.size() == 2);
		auto statesymbol = newSymbols[0];
		auto nextStateSymbol = newSymbols[1];

		_result->LTC2State[pred] = statesymbol;
		_result->State2LTC[statesymbol] = pred;
		_result->LTC2NextState[pred] = nextStateSymbol;
		_result->NextState2LTC[nextStateSymbol] = pred;

		_result->stateVoc->add(statesymbol);
		_result->biStateVoc->add(statesymbol);
		_result->biStateVoc->add(nextStateSymbol);
	}
}

LTCVocInfo* LTCVocabularyTransformer::transform() {
	_result = new LTCVocInfo();
	_result->time = _inputData.time;
	_result->start = _inputData.start;
	_result->next = _inputData.next;
	_result->stateVoc = new Vocabulary(_ltcVoc->name() + "_ss");
	_result->biStateVoc = new Vocabulary(_ltcVoc->name() + "_bs");
	for (auto nameAndSort = _ltcVoc->firstSort(); nameAndSort != _ltcVoc->lastSort(); ++nameAndSort) {
		auto sort = (*nameAndSort).second;
		if (sort != _inputData.time) {
			_result->stateVoc->add(sort);
			_result->biStateVoc->add(sort);
		}
	}

	for (auto nameAndPred = _ltcVoc->firstPred(); nameAndPred != _ltcVoc->lastPred(); ++nameAndPred) {
		auto pred = (*nameAndPred).second;
		for (auto nonOverloadedPred : pred->nonbuiltins()) {
			projectAndAddSymbol(nonOverloadedPred);
		}
	}

	for (auto nameAndFunc = _ltcVoc->firstFunc(); nameAndFunc != _ltcVoc->lastFunc(); ++nameAndFunc) {
		auto func = (*nameAndFunc).second;
		if (func != _inputData.start && func != _inputData.next) {
			for (auto nonOverloadedFunc : func->nonbuiltins()) {
				projectAndAddSymbol(nonOverloadedFunc);
			}
		}
	}
	if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 0) {
		std::clog << "Transforming the LTC vocabulary\n" << toString(_ltcVoc) << "\nResulted in the following two vocabularies:\n"
				<< toString(_result->stateVoc) << "\n" << toString(_result->biStateVoc) << "\n";
	}
	return _result;
}

std::vector<PFSymbol*> LTCVocabularyTransformer::projectSymbol(PFSymbol* pf) {
	Assert(not pf->overloaded());
	auto sorts = pf->sorts();
	if (pf == _inputData.time->pred()) {
		Assert(sorts[0] == _inputData.time);
		return {}; //Should not be added
	}
	auto newsorts = projectSorts(pf);

	if (sorts.size() == newsorts.size()) {
		return {pf};
	}

	PFSymbol* stateSymbol;
	PFSymbol* nextStateSymbol;

	if (isa<Function>(*pf)) {
		auto f = dynamic_cast<Function*>(pf);
		std::stringstream ss;
		ss << f->nameNoArity() << "/" << (f->arity() - 1);
		stateSymbol = new Function(ss.str(), newsorts, NULL, f->binding());
		std::stringstream ss2;
		ss2 << f->nameNoArity() << "_next" << "/" << (f->arity() - 1);
		nextStateSymbol = new Function(ss2.str(), newsorts, NULL, f->binding());

	} else {
		Assert(isa<Predicate>(*pf));
		auto p = dynamic_cast<Predicate*>(pf);
		std::stringstream ss;
		ss << p->nameNoArity() << "/" << (p->arity() - 1);
		stateSymbol = new Predicate(ss.str(), newsorts, ParseInfo(), pf->infix());
		std::stringstream ss2;
		ss2 << p->nameNoArity() << "_next" << "/" << (p->arity() - 1);
		nextStateSymbol = new Predicate(ss2.str(), newsorts,ParseInfo(), pf->infix());
	}
	return {stateSymbol,nextStateSymbol};

}

std::vector<Sort*> LTCVocabularyTransformer::projectSorts(PFSymbol* p) {
	auto sorts = p->sorts();
	if (isa<Function>(*p)) {
		if (sorts[sorts.size() - 1] == _inputData.time) {
			throw IdpException("LTC vocabularies cannot contain functions mapping to the sort Time");
		}
	}
	std::vector<Sort*> newsorts;
	bool timeFound = false;
	size_t indexOfTime = 0;
	for (auto sort : sorts) {
		Assert(sort != NULL);
		if (sort != _inputData.time) {
			newsorts.push_back(sort);
		} else {
			if (timeFound) {
				throw IdpException("In LTC vocabularies, every function/predicate can only have one argument of type Time");
			}
			timeFound = true;
			_result->IndexOfTime[p] = indexOfTime;
		}
		indexOfTime++;
	}

	Assert(newsorts.size() + 1 >= sorts.size());
	Assert(newsorts.size() <= sorts.size());

	return newsorts;

}

