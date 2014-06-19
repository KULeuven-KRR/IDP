/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include <iostream>
#include <set>
#include <vector>
#include <iterator>
#include <algorithm>

#include "FunctionDetection.hpp"
#include "IncludeComponents.hpp"
#include "inferences/entailment/Entails.hpp"
#include "errorhandling/error.hpp"
#include "creation/cppinterface.hpp"
#include "theory/TheoryUtils.hpp"
#include "utils/ListUtils.hpp"
#include "utils/SubsetGenerator.hpp"

using namespace std;
using namespace Gen;

void FunctionDetection::doDetectAndRewriteIntoFunctions(Theory* theory) {
	FunctionDetection c(theory);
	c.detectAndRewriteIntoFunctions();
}

FunctionDetection::FunctionDetection(Theory* theory)
		: 	origVoc(new Vocabulary("detectionVoc")),
			theory(theory),
			inoutputvarcount(0),
			totalfunc(0),
			partfunc(0),
			provercalls(0) {
	origVoc->add(theory->vocabulary());
	stringstream ss;
	ss << "funcVoc_" << getGlobal()->getNewID();
	theory->vocabulary(new Vocabulary(ss.str()));
	theory->vocabulary()->add(origVoc);
}

FunctionDetection::~FunctionDetection() {
	delete (origVoc);
}

void FunctionDetection::detectAndRewriteIntoFunctions() {
	if (getOption(VERBOSE_FUNCDETECT) > 0) {
		clog << "varin&&" << FormulaUtils::countQuantVars(theory) << "\n";
	}

	for (auto pn : origVoc->getPreds()) { // TODO this does NOT cover everything if we start introducing reduced predicates
		for (auto pred : pn.second->nonbuiltins()) {
			auto replaced = false;
			if (pred->arity() == 0) {
				continue;
			}

			std::vector<Variable*> predvars;
			for (uint i = 0; i < pred->arity(); i++) {
				auto var = new Variable(pred->sort(i));
				predvars.push_back(var);
			}

			auto subsetgen = SubsetGenerator<Variable*, VarCompare>(predvars, min((int) pred->arity()-1, 3));
			while (subsetgen.hasNextSubset()) {
				CHECKTERMINATION
				if (replaced) {
					break;
				}
				auto subset = subsetgen.getCurrentSubset();
				replaced = tryToTransform(theory, pred, predvars, subset, false);
				if (replaced) {
					break;
				}
				replaced = tryToTransform(theory, pred, predvars, subset, true);

				subsetgen.nextSubset();
			}
		}
	}

	if (getOption(VERBOSE_FUNCDETECT) > 0) {
		clog << "varout&&" << FormulaUtils::countQuantVars(theory) - inoutputvarcount << "\n"; // NOTE: skolelize will further improve this value
		clog << "totalfunc&&" << totalfunc << "\n";
		clog << "partfunc&&" << partfunc << "\n";
		clog << "calls&&" << provercalls << "\n";
	}
}

/**
 * partial: looking for partial functions
 * domainset: set of variables in the domain of the sought-for functions
 */
bool FunctionDetection::tryToTransform(Theory* newTheory, Predicate* pred, const std::vector<Variable*>& predvars, const varset& domainset, bool partial) {
	if (domainset.size() == pred->arity()) {
		throw IdpException("Invalid code path");
	}
	auto predvarset = getVarSet(predvars);

	auto functheory = new Theory("Test", newTheory->vocabulary(), ParseInfo());
	auto complement = getComplement(predvarset, domainset);

	if (not partial) {
		auto constr = &forall(domainset, exists(complement, atom(pred, predvars)));
		functheory->add(constr->clone());
	}

	auto surjectionconstraintvars = predvarset;
	surjectionconstraintvars.insert(complement.cbegin(), complement.cend());

	std::vector<Variable*> predvars2;
	std::vector<Formula*> equalities;
	for (auto v : predvars) {
		auto newvar = v;
		if (contains(complement, v)) {
			newvar = new Variable(v->sort());
			auto constr = &atom(::get(STDPRED::EQ)->disambiguate( { v->sort(), v->sort() }, newTheory->vocabulary()), std::vector<Variable*> { v, newvar });
			equalities.push_back(constr->clone());
			surjectionconstraintvars.insert(newvar);
		}
		predvars2.push_back(newvar);
	}

	auto constr2 = &forall(surjectionconstraintvars, disj( { &not atom(pred, predvars), &not atom(pred, predvars2), &conj(equalities) }));
	functheory->add(constr2->clone());

	auto entails = Entails::doCheckEntailment(newTheory->clone(), functheory);
	provercalls++;

	functheory->recursiveDelete();

	if (entails != State::PROVEN) {
		return false;
	}

	std::vector<int> domainindices, codomainsindices;
	for (uint i = 0; i < predvars.size(); ++i) {
		if (contains(complement, predvars[i])) {
			codomainsindices.push_back(i);
		} else {
			domainindices.push_back(i);
		}
	}

	if (partial) {
		partfunc += codomainsindices.size();
	} else {
		totalfunc += codomainsindices.size();
	}
	newTheory = FormulaUtils::replacePredByFunctions(newTheory, pred, origVoc->contains(pred), getSet(domainindices), getSet(codomainsindices), partial);
	return true;
}
