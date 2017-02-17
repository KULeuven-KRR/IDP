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

#include <vector>
#include <stack>

#include "DefinitionsToNormalForm.hpp"
#include "creation/cppinterface.hpp"
#include "theory/TheoryUtils.hpp"
#include "IncludeComponents.hpp"
#include "common.hpp"

using namespace std;

Formula * DefinitionsToNormalForm::processFormula(Formula* f) {
	if(_isNested) {
		std::map<Variable*, Variable*> var2var;
		std::map<Variable*, Variable*> var2varInverse;
		varset newvars;
		std::vector<Sort*> sorts;
		for (auto i = f->freeVars().cbegin(); i != f->freeVars().cend(); ++i) {
			auto newvar = new Variable((*i)->sort());
			var2var.insert({*i,newvar});
			var2varInverse.insert({newvar,*i});
			newvars.insert(newvar);
			sorts.push_back(newvar->sort());
		}
		stringstream ss;
		ss << "tseitin" << getGlobal()->getNewID();
		auto newRuleBody = FormulaUtils::substituteVarWithVar(f,var2var);
		auto varvector = std::vector<Variable*>(newvars.begin(),newvars.end());
		auto newHead = &Gen::atom(new Predicate(sorts),varvector);
		Rule* newrule = new Rule(newvars,newHead,newRuleBody,ParseInfo());
		_rulesToProcess.push(newrule);
		return newHead->clone(var2varInverse);
	} else {
		traverse(f);
		return f;
	}
}

Formula* DefinitionsToNormalForm::traverse(Formula* f) {
	bool oldNested = _isNested;
	_isNested = true;
	auto ret = TheoryMutatingVisitor::traverse(f);
	_isNested = oldNested;
	return ret;
}

Theory* DefinitionsToNormalForm::visit(Theory* t) {
	for (auto it = t->definitions().begin(); it != t->definitions().end(); ++it) {
		*it = (*it)->accept(this);
	}
	return t;
}

Definition* DefinitionsToNormalForm::visit(Definition* d) {
	ruleset newset;
	for (auto rit = d->rules().rbegin(); rit != d->rules().rend(); rit++) {
		_rulesToProcess.push(*rit);
	}
	while (not _rulesToProcess.empty()) {
		auto nextel = _rulesToProcess.top();
		_rulesToProcess.pop();
		newset.insert(nextel->accept(this));
	}
	d->rules(newset);
	return d;
}

Rule* DefinitionsToNormalForm::visit(Rule* r) {
	_isNested = false;
	auto newhead = r->head()->accept(this);
	Assert(isa<PredForm>(*newhead));
	r->head(dynamic_cast<PredForm*>(newhead));
	r->body(r->body()->accept(this));
	return r;
}

Formula* DefinitionsToNormalForm::visit(AggForm* f) {
	return processFormula(f);
}

Formula* DefinitionsToNormalForm::visit(BoolForm* f) {
	return processFormula(f);
}

Formula* DefinitionsToNormalForm::visit(EqChainForm* f) {
	return processFormula(f);
}
Formula* DefinitionsToNormalForm::visit(QuantForm* f) {
	return processFormula(f);
}
