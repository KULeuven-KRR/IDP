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

#include "projectLTCStructure.hpp"
#include "common.hpp"
#include "IncludeComponents.hpp"
#include "utils/ListUtils.hpp"
#include "data/LTCData.hpp"
#include "data/StateVocInfo.hpp"

void LTCStructureProjector::init(const Structure* input, bool ignoreStart) {
	_inputStruc = input;
	_ltcVoc = input->vocabulary();
	_vocInfo = LTCData::instance()->getStateVocInfo(_ltcVoc);
	_stateVoc = _vocInfo->stateVoc;
	_time = _vocInfo->time;
	_start = _vocInfo->start;
	_next = _vocInfo->next;

	//In case start is interpreted, we should incorporate this when projecting dynamic predicates and functions.
	_shouldUseStart = (_inputStruc->inter(_start)->approxTwoValued() ) && not ignoreStart;
	_forceIgnoreStart = ignoreStart;
	if (_shouldUseStart) {
		_startDomElem = _inputStruc->inter(_start)->funcTable()->operator []( { });
	}

	_result = new Structure(_inputStruc->name() + "_projected", _stateVoc, ParseInfo());

}
Structure* LTCStructureProjector::run() {
	setSorts();
	projectPredicates();
	projectFunctions();

	_result->clean();
	if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 0) {
		std::clog << "Projecting the LTC structure\n" << toString(_inputStruc) << "\nto start, resulted in: \n" << toString(_result) << "\n";
	}
	return _result;
}

void LTCStructureProjector::setSorts() {
	for (auto it = _stateVoc->firstSort(); it != _stateVoc->lastSort(); it++) {
		auto sort = it->second;
		if (sort->builtin()) {
			continue;
		}
		cloneAndSetInter(sort);
	}
	//Update sort tables in predicates etcetera.
	_result->reset();
}

void LTCStructureProjector::projectPredicates() {
	for (auto it = _ltcVoc->firstPred(); it != _ltcVoc->lastPred(); it++) {
		auto overloadedPred = it->second;
		projectSymbol(overloadedPred);
	}
}

void LTCStructureProjector::projectFunctions() {
	for (auto it = _ltcVoc->firstFunc(); it != _ltcVoc->lastFunc(); it++) {
		auto overloadedFunc = it->second;
		projectSymbol(overloadedFunc);
	}
}

template<class T>
void LTCStructureProjector::projectSymbol(T* s) {
	for (auto pred : s->nonbuiltins()) {
		if (pred->sorts().size() == 1 and pred->sorts()[0]->pred() == (PFSymbol*) pred) {
			continue;
		}
		if ((PFSymbol*) pred == _start || (PFSymbol*) pred == _next) {
			continue;
		}
		auto image = _vocInfo->LTC2State.find(pred);
		bool hasImage = (image != _vocInfo->LTC2State.cend());
		if (not hasImage) {
			cloneAndSetInter(pred);
		} else {
			projectAndSetInter(pred);
		}
	}
}

void LTCStructureProjector::projectAndSetInter(PFSymbol* symbol) {
	Assert(not symbol->overloaded());
	Assert(not symbol->builtin());
	Assert(_ltcVoc->contains(symbol));
	Assert(contains(_vocInfo->LTC2State, symbol));
	Assert(contains(_vocInfo->IndexOfTime, symbol));

	auto stateSymbol = _vocInfo->LTC2State.at(symbol);
	auto timeindex = _vocInfo->IndexOfTime.at(symbol);

	auto oldinter = _inputStruc->inter(symbol);
	auto newinter = _result->inter(stateSymbol);
	Assert(newinter->ct()->empty());
	Assert(newinter->cf()->empty());

	bool warned = false;


	if (not _shouldUseStart) {
		//If start info is ignored, we do not have to do anything
		if (not _forceIgnoreStart && (not oldinter->ct()->empty() || not oldinter->cf()->empty())) {
			//In case it was not explicitely asked to ignore this, and we COULD have done something, warn the user about this.
			std::stringstream ss;
			ss << "In structure " << _inputStruc->name() << ", Start is uninterpreted. Therefore, we ignore the interpretation of "
					<< toString(symbol) << " even though it is not completely unknown.\n";
			Warning::warning(ss.str());
		}
		return;
	}

	for (auto it = oldinter->ct()->begin(); not it.isAtEnd(); it.operator ++()) {
		auto tuple = *it;
		if (tuple[timeindex] == _startDomElem) {
			auto newtuple = projectTuple(tuple, timeindex);
			newinter->makeTrueExactly(newtuple);
		} else {
			if (not warned) {
				warned = true;
				std::stringstream ss;
				ss << "In structure " << _inputStruc->name() << ", for symbol " << toString(symbol)
						<< " information about other time-points than start is given." << " All this information is ignored.\n";
				Warning::warning(ss.str());
			}
		}
	}

	for (auto it = oldinter->cf()->begin(); not it.isAtEnd(); it.operator ++()) {
		auto tuple = *it;
		if (tuple[timeindex] == _startDomElem) {
			auto newtuple = projectTuple(tuple, timeindex);
			newinter->makeFalseExactly(newtuple);
		} else {
			if (not warned) {
				warned = true;
				std::stringstream ss;
				ss << "In structure " << _inputStruc->name() << ", for symbol " << toString(symbol)
						<< " information about other time-points than start is given." << " All this information is ignored.\n";
				Warning::warning(ss.str());

			}
		}
	}
}

const ElementTuple LTCStructureProjector::projectTuple(const ElementTuple& tuple, size_t ignore) {
	ElementTuple result(tuple.size() - 1);
	for (size_t i = 0; i < tuple.size() - 1; i++) {
		if (i < ignore) {
			result[i] = tuple[i];
		} else {
			result[i] = tuple[i + 1];
		}
	}
	return result;
}

template<class T>
void LTCStructureProjector::cloneAndSetInter(T* object) {
	auto objectinter = _inputStruc->inter(object);
	auto newinter = objectinter->clone();
	_result->changeInter(object, newinter);
}
