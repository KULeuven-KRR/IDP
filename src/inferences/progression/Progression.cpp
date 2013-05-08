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

#include "Progression.hpp"
#include "transformLTCVocabulary.hpp"
#include "common.hpp"
#include "IncludeComponents.hpp"
#include "data/LTCData.hpp"
#include "data/SplitLTCTheory.hpp"
#include "LTCTheorySplitter.hpp"
#include "projectLTCStructure.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"

initData InitialiseInference::doInitialisation(const AbstractTheory* ltcTheo, const Structure* str, const Sort* Time, Function* Start,
		Function* Next) {
	auto g = InitialiseInference(ltcTheo, str, Time, Start, Next);
	auto sols = g.init();
	return sols;
}

std::vector<Structure*> ProgressionInference::doProgression(const AbstractTheory* ltcTheo, const Structure* stateBefore) {
	auto g = ProgressionInference(ltcTheo, stateBefore);
	return g.progress();
}

ProgressionInference::ProgressionInference(const AbstractTheory* ltcTheo, const Structure* stateBefore)
		: 	_ltcTheo(ltcTheo),
			_stateBefore(stateBefore) {
}
ProgressionInference::~ProgressionInference() {

}

std::vector<Structure*> ProgressionInference::progress() {
	auto data = LTCData::instance();

	if (not data->hasBeenTransformed(_ltcTheo->vocabulary()) or not data->hasBeenSplit(_ltcTheo)) {
		throw IdpException(
				"The theory you are using the progression inference on, has not yet been initialised. Please first apply the setupprogression inference.");
	}
	auto vocinfo = data->getStateVocInfo(_ltcTheo->vocabulary());
	auto voc = vocinfo->stateVoc;
	auto bistatevoc = vocinfo->biStateVoc;
	auto bistatetheo = data->getSplitTheory(_ltcTheo)->bistateTheory;
	if (_stateBefore->vocabulary() != voc) {
		throw IdpException(
				"The structure given to the progression inference should range over vocabulary " + voc->name() + " but ranges over "
						+ _stateBefore->vocabulary()->name());
	}
	auto newstruc = _stateBefore->clone();
	newstruc->changeVocabulary(bistatevoc);
	auto models = ModelExpansion::doModelExpansion(bistatetheo, newstruc, NULL)._models;
	delete newstruc;
	postprocess(models);
	return generateEnoughTwoValuedExtensions(models);
}
void ProgressionInference::postprocess(std::vector<Structure*>& v) {
	auto data = LTCData::instance();
	auto vocinfo = data->getStateVocInfo(_ltcTheo->vocabulary());

	auto nextToOrig = vocinfo->NextState2LTC;
	auto origToState = vocinfo->LTC2State;

	for (auto model : v) {
		for (auto tuple : nextToOrig) {
			auto nextPred = tuple.first;
			auto statePred = origToState[tuple.second];
			if (isa<Predicate>(*nextPred)) {
				auto nextPredicate = dynamic_cast<Predicate*>(nextPred);
				auto statePredicate = dynamic_cast<Predicate*>(statePred);
				auto inter = model->inter(nextPredicate);
				auto newinter = inter->clone();
				model->changeInter(statePredicate, newinter);
			} else {
				Assert(isa<Function>(*nextPred));
				auto nextFunc = dynamic_cast<Function*>(nextPred);
				auto stateFunc = dynamic_cast<Function*>(statePred);
				auto inter = model->inter(nextFunc);
				auto newinter = inter->clone();
				model->changeInter(stateFunc, newinter);
			}
		}
		model->changeVocabulary(vocinfo->stateVoc);
	}

}

InitialiseInference::InitialiseInference(const AbstractTheory* ltcTheo, const Structure* str, const Sort* Time, Function* Start, Function* Next)
		: 	_ltcTheo(ltcTheo),
			_inputStruc(str),
			_projectedStructure(NULL),
			_timeInput(Time),
			_startInput(Start),
			_nextInput(Next) {

}
InitialiseInference::~InitialiseInference() {

}

initData InitialiseInference::init() {
	auto data = LTCData::instance();
	if (not data->hasBeenTransformed(_ltcTheo->vocabulary())) {
		prepareVocabulary();
		Assert(data->hasBeenTransformed(_ltcTheo->vocabulary()));
	}
	if (not data->hasBeenSplit(_ltcTheo)) {
		prepareTheory();
		Assert(data->hasBeenSplit(_ltcTheo));
	}
	initData output;
	auto theos = data->getSplitTheory(_ltcTheo);
	output._bistateTheo = theos->bistateTheory;
	output._initTheo = theos->initialTheory;
	auto vocs = data->getStateVocInfo(_ltcTheo->vocabulary());
	output._bistateVoc = vocs->biStateVoc;
	output._onestateVoc = vocs->stateVoc;

	prepareStructure();
	Assert(_projectedStructure != NULL);

	auto models = ModelExpansion::doModelExpansion(output._initTheo, _projectedStructure, NULL)._models;
	output._models = generateEnoughTwoValuedExtensions(models);
	return output;
}

void InitialiseInference::prepareStructure() {
	if (_ltcTheo->vocabulary() != _inputStruc->vocabulary()) {
		throw IdpException("The initialise inference expects a theory and a structure ranging over the same vocabulary");
	}
	_projectedStructure = LTCStructureProjector::projectStructure(_inputStruc);
}

void InitialiseInference::prepareTheory() {
	auto ltcdata = LTCData::instance();
	if (ltcdata->hasBeenSplit(_ltcTheo)) {
		//TODO: check if nothing has changed since then
	} else {
		auto splitTheory = LTCTheorySplitter::SplitTheory(_ltcTheo);
		ltcdata->registerSplit(_ltcTheo, splitTheory);
	}
}

void InitialiseInference::prepareVocabulary() {
	auto ltcdata = LTCData::instance();
	auto ltcVoc = _ltcTheo->vocabulary();
	if (ltcdata->hasBeenTransformed(ltcVoc)) {
		//TODO: check if nothing has changed since then
	} else {
		LTCInputData symbols;
		if (_timeInput != NULL) {
			Assert(_startInput!=NULL && _nextInput!=NULL);
			symbols.time = _timeInput;
			symbols.start = _startInput;
			symbols.next = _nextInput;

		} else {
			symbols = collectLTCSortAndFunctions(ltcVoc);
		}

		verify(symbols);

		Assert(symbols.start != NULL);
		auto stateVocInfo = LTCVocabularyTransformer::TransformVocabulary(ltcVoc, symbols);
		Assert(stateVocInfo->start != NULL);

		ltcdata->registerTransformation(ltcVoc, stateVocInfo);
	}
}

void InitialiseInference::verify(const LTCInputData& data) const {
	if (data.time == NULL) {
		throw IdpException("Not find a valid Time symbol for progression");
	}
	if (data.start == NULL) {
		throw IdpException("Not find a valid Start symbol for progression");
	}
	if (data.next == NULL) {
		throw IdpException("Not find a valid Next symbol for progression");
	}
	if (data.start->arity() != 0 || data.start->outsort() != data.time) {
		throw IdpException("In LTC theories, the function Start should be typed [:Time].");
	}
	if (data.next->arity() != 1 || data.next->sort(0) != data.time || data.next->outsort() != data.time) {
		throw IdpException("Start should be a unary function typed Time->Time");
	}
}

LTCInputData InitialiseInference::collectLTCSortAndFunctions(Vocabulary* ltcVoc) {
	LTCInputData result;

//TIME
	if (not ltcVoc->hasSortWithName("Time")) {
		throw IdpException("LTC theories are required to have a sort named Time. (or, if the name of this sort is not Time, provide it yourself)");
	}
	auto timeSort = ltcVoc->sort("Time");
	Assert(timeSort != NULL);
	result.time = timeSort;

//START
	if (not ltcVoc->hasFuncWithName("Start/0")) {
		throw IdpException("LTC theories are required to have a constant of type Time named Start.  (or, if the name of this constant is not Start, provide it yourself)");
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
		throw IdpException("LTC theories are required to have a function, typed [Time:Time] named Next.  (or, if the name of this function is not Next, provide it yourself)");
	}
	auto nextFunc = ltcVoc->func("Next/1");
	Assert(nextFunc != NULL);
	if (nextFunc->overloaded()) {
		throw IdpException("LTC theories can only have one function named Next.");
	}
	if (nextFunc->nrSorts() != 2 || nextFunc->sorts()[0] != timeSort || nextFunc->sorts()[1] != timeSort) {
		throw IdpException("In LTC theories, the function Next should be typed [Time:Time] .");
	}
	result.next = nextFunc;
	return result;
}
