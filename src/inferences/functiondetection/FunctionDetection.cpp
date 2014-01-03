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
#include "utils/SubsetIterator.hpp"

using namespace std;
using namespace Gen;

void FunctionDetection::doDetectAndRewriteIntoFunctions(Theory* theory, bool assumeTypesNotEmpty) {
	FunctionDetection c(theory, assumeTypesNotEmpty);
	c.detectAndRewriteIntoFunctions();
}

FunctionDetection::FunctionDetection(Theory* theory, bool assumeTypesNotEmpty)
		: 	origVoc(new Vocabulary("detectionVoc")), theory(theory), assumeTypesNotEmpty(assumeTypesNotEmpty), inoutputvarcount(0), totalfunc(0),partfunc(0),calls(0) {
	origVoc->add(theory->vocabulary());
	stringstream ss;
	ss <<"funcVoc_" <<getGlobal()->getNewID();
	theory->vocabulary(new Vocabulary(ss.str()));
	theory->vocabulary()->add(origVoc);
}

FunctionDetection::~FunctionDetection(){
	delete(origVoc);
}

void FunctionDetection::detectAndRewriteIntoFunctions() {
	if(getOption(FUNCDETECTRESULTS)){
		clog <<"varin&&" <<FormulaUtils::countQuantVars(theory) <<"\n";
	}
	auto somereplaced = false;

	for (auto name2pred : origVoc->getPreds()) { // TODO this does NOT cover everything if we start introducing reduced predicates
		CHECKTERMINATION;
		auto replaced = false;
		auto pred = name2pred.second;
		if (pred->overloaded() || pred->builtin() || pred->arity()==0) {
			continue;
		}

		std::vector<Variable*> predvars;
		for (uint i = 0; i < pred->arity(); i++) {
			auto var = new Variable(pred->sort(i));
			predvars.push_back(var);
		}
		auto subsetgen = SubsetGenerator<Variable*, VarCompare>(predvars, min((int)pred->arity()-1, 3));

		for(auto subset : subsetgen){
			if(replaced){
				break;
			}
			replaced = tryToTransform(theory, pred, predvars, subset, false);
			if(replaced){
				break;
			}
			replaced = tryToTransform(theory, pred, predvars, subset, true);
		}

		if(replaced){
			somereplaced = true;
			continue;
		}
	}

	if(somereplaced){
		FormulaUtils::pushNegations(theory);
		FormulaUtils::flatten(theory);
		for(auto def: theory->definitions()){ // TODO should also go deeper, pull as many variables up as possible!
			for(auto rule:def->rules()){
				auto qf = dynamic_cast<QuantForm*>(rule->body());
				if(qf!=NULL && not qf->isUnivWithSign()){
					for(auto var: qf->quantVars()){
						rule->addvar(var);
					}
					rule->body(qf->subformula());
				}
			}
		}
		theory = FormulaUtils::replaceVariableByDefiningFunctionTerms(theory);
		theory = FormulaUtils::removeValidAtoms(theory);
		theory = FormulaUtils::removeValidQuantifications(theory, assumeTypesNotEmpty);

		// TODO fix transformations so no duplicate code is necessary (which is incomplete anyway)
		FormulaUtils::pushNegations(theory);
		FormulaUtils::flatten(theory);
		theory = FormulaUtils::replaceVariableByDefiningFunctionTerms(theory);
		theory = FormulaUtils::removeValidAtoms(theory);
		theory = FormulaUtils::removeValidQuantifications(theory, assumeTypesNotEmpty);
	}

	theory = FormulaUtils::skolemize(theory);

	for(auto def: theory->definitions()){
		for(auto rule:def->rules()){
			varset bodyonlyvars, rem;
			for(auto var : rule->quantVars()){
				if(not contains(rule->head()->freeVars(), var)){
					bodyonlyvars.insert(var);
				}else{
					rem.insert(var);
				}
			}
			rule->setQuantVars(rem);
			if(not bodyonlyvars.empty()){
				rule->body(new QuantForm(SIGN::POS, QUANT::UNIV, bodyonlyvars, rule->body(), FormulaParseInfo()));
			}
		}
	}
	FormulaUtils::pushNegations(theory);
	FormulaUtils::flatten(theory);

	if(getOption(FUNCDETECTRESULTS)){
		clog <<"varout&&" <<FormulaUtils::countQuantVars(theory)-inoutputvarcount <<"\n";
		clog <<"totalfunc&&" <<totalfunc <<"\n";
		clog <<"partfunc&&" <<partfunc <<"\n";
		clog <<"calls&&" <<calls <<"\n";
	}
}

bool FunctionDetection::tryToTransform(Theory* newTheory, Predicate* pred, const std::vector<Variable*>& predvars, const varset& domainset, bool partial) {
	if(domainset.size()==pred->arity()){
		throw IdpException("Invalid code path");
	}
	auto predvarset = getVarSet(predvars);

	auto functheory = new Theory("Test", newTheory->vocabulary(), ParseInfo());
	auto complement = getComplement(predvarset, domainset);

	if(not partial){
		auto constr = &forall(domainset, exists(complement, atom(pred, predvars)));
		functheory->add(constr->clone());
	}

	auto surjectionconstraintvars = predvarset;
	surjectionconstraintvars.insert(complement.cbegin(), complement.cend());

	std::vector<Variable*> predvars2;
	std::vector<Formula*> equalities;
	for(auto v: predvars){
		auto newvar = v;
		if(contains(complement, v)){
			newvar = new Variable(v->sort());
			auto constr = &atom(::get(STDPRED::EQ)->disambiguate({v->sort(), v->sort()}, newTheory->vocabulary()), std::vector<Variable*>{v,newvar});
			equalities.push_back(constr->clone());
			surjectionconstraintvars.insert(newvar);
		}
		predvars2.push_back(newvar);
	}

	auto constr2 = &forall(surjectionconstraintvars, disj({&not atom(pred, predvars), &not atom(pred, predvars2), &conj(equalities)}));
	functheory->add(constr2->clone());

	auto entails = Entails::doCheckEntailment(newTheory->clone(), functheory);
	calls++;

	if(entails!=State::PROVEN){
		functheory->recursiveDelete();
		return false;
	}

	functheory->recursiveDelete();

	auto computPred = pred;
	if(origVoc->contains(pred)){
		computPred = new Predicate(pred->nameNoArity()+"_c", pred->sorts());
		newTheory->vocabulary()->add(computPred);

		std::vector<Variable*> vars;
		for(uint i=0; i<pred->sorts().size(); ++i){
			vars.push_back(new Variable(pred->sort(i)));
		}

		newTheory = FormulaUtils::replacePredByPred(pred, computPred, newTheory);

		// Add input/output definition, NOTE requires pre- and postprocessing of definitions // TODO overhead in case an symbol is partially interpreted
		auto newdef = new Definition();
		auto newrule = new Rule(getVarSet(vars), &atom(pred, vars), &atom(computPred, vars), ParseInfo());
		newdef->add(newrule->clone());
		newTheory->add(newdef);
		inoutputvarcount+=vars.size();
	}

	std::vector<int> domainindices, codomainsindices;
	for(uint i=0; i<predvars.size(); ++i){
		if(contains(complement, predvars[i])){
			codomainsindices.push_back(i);
		}else{
			domainindices.push_back(i);
		}
	}

	if(partial){
		partfunc+=codomainsindices.size();
	}else{
		totalfunc+=codomainsindices.size();
	}
	newTheory = FormulaUtils::replacePredByFunctions(newTheory, computPred, getSet(domainindices), getSet(codomainsindices), partial);
	return true;
}
