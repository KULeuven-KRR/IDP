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
#include "common.hpp"
#include "IncludeComponents.hpp"
#include "data/LTCData.hpp"
#include "data/StateVocInfo.hpp"
#include "data/SplitLTCTheory.hpp"
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

	auto vocinfo = data->getStateVocInfo(_ltcTheo->vocabulary());
	auto voc = vocinfo->stateVoc;
	auto bistatevoc = vocinfo->biStateVoc;
	auto bistatetheo = data->getSplitTheory(_ltcTheo)->bistateTheory;
	if (_stateBefore->vocabulary() != voc) {
		Error::LTC::progressOverWrongVocabulary(voc->name(), _stateBefore->vocabulary()->name());
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
			_nextInput(Next),
			_vocInfo(NULL){
			prepareVocabulary();

}
InitialiseInference::~InitialiseInference() {

}

initData InitialiseInference::init() {
	if (_ltcTheo->vocabulary() != _inputStruc->vocabulary()) {
		Error::LTC::strucVocIsNotTheoVoc();
	}
	auto data = LTCData::instance();
	initData output;
	auto theos = data->getSplitTheory(_ltcTheo);
	output._bistateTheo = theos->bistateTheory;
	output._initTheo = theos->initialTheory;
	output._bistateVoc = _vocInfo->biStateVoc;
	output._onestateVoc = _vocInfo->stateVoc;

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


void InitialiseInference::prepareVocabulary() {
	auto ltcdata = LTCData::instance();
	auto ltcVoc = _ltcTheo->vocabulary();
	if (_timeInput != NULL) {
		LTCInputData symbols;
		Assert(_startInput!=NULL && _nextInput!=NULL);
		symbols.time = _timeInput;
		symbols.start = _startInput;
		symbols.next = _nextInput;
		_vocInfo = ltcdata->getStateVocInfo(ltcVoc, symbols);

	} else {
		_vocInfo = ltcdata->getStateVocInfo(ltcVoc);
	}
}


