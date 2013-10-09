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
	Assert(hasBeenTransformed(v));
	return _LTCVocData[v];
}

void LTCData::registerTransformation(const Vocabulary* v, const LTCVocInfo* si) {
	Assert(not hasBeenTransformed(v));
	_LTCVocData[v] = si;
}

bool LTCData::hasBeenSplit(const AbstractTheory* t) {
	return contains(_LTCTheoData, t);

}
const SplitLTCTheory* LTCData::getSplitTheory(const AbstractTheory* t) {
	Assert(hasBeenSplit(t));
	return _LTCTheoData[t];
}
void LTCData::registerSplit(const AbstractTheory* t, const SplitLTCTheory* st) {
	Assert(not hasBeenSplit(t));
	_LTCTheoData[t] = st;

}
