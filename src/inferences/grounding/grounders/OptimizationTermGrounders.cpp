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
#include "TermGrounders.hpp"
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

GroundTermTranslator* OptimizationGrounder::getTermTranslator() const {
	return getGrounding()->termtranslator();
}

void OptimizationGrounder::printOrig() const {
	clog << tabs() << "Grounding optimization over term " << toString(_origterm) << "\n";
}

void AggregateOptimizationGrounder::run() const {
//TODO Should this grounder return a VarId in some cases?
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	auto setid = _setgrounder->run();
	auto tsset = getTranslator()->groundset(setid);

	if (not tsset.varids().empty()) {
		Assert(getOption(BoolType::CPSUPPORT) && tsset.trueweights().empty());
		auto sumterm = createCPAggTerm(_type, tsset.varids());
		auto varid = getTermTranslator()->translate(sumterm, NULL); //FIXME domain of the term!!!
		getGrounding()->addOptimization(varid);
	} else {
		getGrounding()->addOptimization(_type, setid);
	}

	if (verbosity() > 2) {
		poptab();
	}
}
