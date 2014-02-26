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

#include "OptimizationTermGrounders.hpp"

#include "SetGrounders.hpp"
#include "TermGrounders.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"

#include "inferences/grounding/GroundTranslator.hpp"

#include "IncludeComponents.hpp"

using namespace std;

OptimizationGrounder::OptimizationGrounder(AbstractGroundTheory* g, const GroundingContext& context, Term const*const origterm)
		: Grounder(g, context), _origterm(origterm==NULL?NULL:origterm->clone()) {
}

OptimizationGrounder::~OptimizationGrounder() {
	if (_origterm != NULL) {
		_origterm->recursiveDelete();
		_origterm = NULL;
	}
}

GroundTranslator* OptimizationGrounder::getTranslator() const {
	return getGrounding()->translator();
}

void OptimizationGrounder::printOrig() const {
	clog << tabs() << "Grounding optimization over term " << print(_origterm) << "\n";
}


AggregateOptimizationGrounder::~AggregateOptimizationGrounder() {
	delete (_setgrounder);
}

void AggregateOptimizationGrounder::internalRun(ConjOrDisj& formula, LazyGroundingRequest& ){
	formula.setType(Conn::CONJ);
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}
	auto setid = _setgrounder->run();
	auto tsset = getTranslator()->groundset(setid);

	Assert(tsset.cpvars().empty());
	getGrounding()->addOptimization(_type, setid);

	if (verbosity() > 2) {
		poptab();
	}
}

void AggregateOptimizationGrounder::put(std::ostream&) const {
	notyetimplemented("Printing aggregate optimization grounder"); // No reason to throw, this is only debug output
}


VariableOptimizationGrounder::~VariableOptimizationGrounder() {
	delete (_termgrounder);
}

void VariableOptimizationGrounder::internalRun(ConjOrDisj& formula, LazyGroundingRequest& ){
	formula.setType(Conn::CONJ);
	if (verbosity() > 2) {
		printOrig();
		pushtab();
	}

	auto groundterm = _termgrounder->run();
	VarId varid;
	if (groundterm.isVariable) {
		varid = groundterm._varid;
	} else {
		varid = getTranslator()->translateTerm(groundterm._domelement);
	}
	getGrounding()->addOptimization(varid);

	if (verbosity() > 2) {
		poptab();
	}
}

void VariableOptimizationGrounder::put(std::ostream&) const {
	notyetimplemented("Printing variable optimization grounder"); // No reason to throw, this is only debug output
}
