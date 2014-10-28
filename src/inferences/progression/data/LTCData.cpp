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

#include "LTCData.hpp"
#include "common.hpp"
#include "utils/ListUtils.hpp"
#include "StateVocInfo.hpp"
#include "SplitLTCTheory.hpp"
#include "../LTCTheorySplitter.hpp"
#include "../transformLTCVocabulary.hpp"
#include "vocabulary/vocabulary.hpp"

LTCData* _LTCinstance = NULL;

LTCData* LTCData::instance() {
	if (_LTCinstance == NULL) {
		_LTCinstance = new LTCData();
		GlobalData::instance()->registerForDeletion(_LTCinstance);
	}
	return _LTCinstance;
}

LTCData::~LTCData() {
	Assert(this == _LTCinstance);
	for (auto vocs : _LTCVocData) {
		delete (vocs.second);
	}
	for (auto theo : _LTCTheoData) {
		delete (theo.second);
	}
	_LTCinstance = NULL;
}

bool LTCData::hasBeenTransformed(const Vocabulary* v) {
	return contains(_LTCVocData, v);
}
const LTCVocInfo* LTCData::getStateVocInfo(const Vocabulary* v) {
	if (hasBeenTransformed(v)) {
		return _LTCVocData[v];
	}
	auto symbols = collectLTCSortAndFunctions(v);
	return getStateVocInfo(v, symbols);

}

bool LTCData::hasBeenProjected(PFSymbol* pf) {
	return _projectedSymbols.find(pf) != _projectedSymbols.cend();
}
std::vector<PFSymbol*> LTCData::getProjection(PFSymbol* pf) {
	auto result = _projectedSymbols.find(pf);
	if (result != _projectedSymbols.cend()) {
		return result->second;
	}
	return {};
}
void LTCData::setProjection(PFSymbol* pf, std::vector<PFSymbol*> v) {
	_projectedSymbols[pf] = v;
}

const LTCVocInfo* LTCData::getStateVocInfo(const Vocabulary* v, LTCInputData symbols) {
	if (hasBeenTransformed(v)) {
		return _LTCVocData[v];
	}
	verify(symbols);
	Assert(symbols.start != NULL);
	auto stateVocInfo = LTCVocabularyTransformer::TransformVocabulary(v, symbols);
	Assert(stateVocInfo->start != NULL);
	_LTCVocData[v] = stateVocInfo;
	return stateVocInfo;
}

bool LTCData::hasBeenSplit(const AbstractTheory* t) {
	return contains(_LTCTheoData, t);
}

const SplitLTCTheory* LTCData::getSplitTheory(const AbstractTheory* t) {
	if (hasBeenSplit(t)) {
		return _LTCTheoData[t];
	} else {
		auto splitTheory = LTCTheorySplitter::SplitTheory(t);
		_LTCTheoData[t] = splitTheory;
		return splitTheory;
	}
}

LTCInputData LTCData::collectLTCSortAndFunctions(const Vocabulary* ltcVoc) const {
	LTCInputData result;

//TIME
	if (not ltcVoc->hasSortWithName("Time")) {
		throw IdpException("LTC Vocabularies are required to have a sort named Time. (or, if the name of this sort is not Time, provide it yourself)");
	}
	auto timeSort = ltcVoc->sort("Time");
	Assert(timeSort != NULL);
	result.time = timeSort;

//START
	if (not ltcVoc->hasFuncWithName("Start/0")) {
		throw IdpException(
				"LTC Vocabularies are required to have a constant of type Time named Start.  (or, if the name of this constant is not Start, provide it yourself)");
	}
	auto startFunc = ltcVoc->func("Start/0");

	Assert(startFunc != NULL);
	if (startFunc->overloaded()) {
		throw IdpException("LTC theories can only have one function named Start.");
	}
	if (startFunc->nrSorts() != 1 || startFunc->sorts()[0] != timeSort) {
		throw IdpException("");
	}
	result.start = startFunc;

//NEXT
	if (not ltcVoc->hasFuncWithName("Next/1")) {
		throw IdpException(
				"LTC Vocabularies are required to have a function, typed [Time:Time] named Next.  (or, if the name of this function is not Next, provide it yourself)");
	}
	auto nextFunc = ltcVoc->func("Next/1");
	Assert(nextFunc != NULL);
	if (nextFunc->overloaded()) {
		throw IdpException("LTC Vocabularies can only have one function named Next.");
	}
	if (nextFunc->nrSorts() != 2 || nextFunc->sorts()[0] != timeSort || nextFunc->sorts()[1] != timeSort) {
		throw IdpException("In LTC Vocabularies, the function Next should be typed [Time:Time] .");
	}
	result.next = nextFunc;
	return result;
}

void LTCData::verify(const LTCInputData& data) const {
	if (data.time == NULL) {
		throw IdpException("Did not find a valid Time symbol in LTC vocabulary");
	}
	if (data.start == NULL) {
		throw IdpException("Did not find a valid Start symbol in LTC vocabulary");
	}
	if (data.next == NULL) {
		throw IdpException("Did not find a valid Next symbol in LTC vocabulary");
	}
	if (data.start->arity() != 0 || data.start->outsort() != data.time) {
		throw IdpException("In LTC Vocabularies, the function Start should be typed [:Time].");
	}
	if (data.next->arity() != 1 || data.next->sort(0) != data.time || data.next->outsort() != data.time) {
		throw IdpException("In LTC Vocabularies, Start should be a unary function typed Time->Time");
	}
}

