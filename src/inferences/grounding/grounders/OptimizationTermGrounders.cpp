/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "OptimizationTermGrounders.hpp"

#include "SetGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"

#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/GroundTermTranslator.hpp"

#include "IncludeComponents.hpp"

using namespace std;

OptimizationGrounder::~OptimizationGrounder() {
	if (_origterm != NULL) {
		_origterm->recursiveDelete();
		_origterm = NULL;
	}
}

void OptimizationGrounder::setOrig(const Term* t) {
	_origterm = t->clone();
}

GroundTranslator* OptimizationGrounder::getTranslator() const {
	return getGrounding()->translator();
}

void OptimizationGrounder::printOrig() const {
	clog << "" << nt() << "Grounding optimization over term " << toString(_origterm);
	clog << "" << nt();
}

void AggregateOptimizationGrounder::run() const {
//TODO Should this grounder return a VarId in some cases?
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	getGrounding()->addOptimization(_type, _setgrounder->run());
}
