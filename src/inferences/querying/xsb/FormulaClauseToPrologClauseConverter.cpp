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

#include <list>

#include "FormulaClauseToPrologClauseConverter.hpp"
#include "FormulaClause.hpp"
#include "PrologProgram.hpp"
#include "XSBToIDPTranslator.hpp"
#include "theory/TheoryUtils.hpp"


string FormulaClauseToPrologClauseConverter::generateGeneratorClauseName() {
	std::stringstream ss;
	ss <<"generator" << getGlobal()->getNewID();
	return ss.str();
}

void FormulaClauseToPrologClauseConverter::visit(ExistsClause* ec) {
	std::list<PrologTerm*> body;
	body.push_back(ec->child()->asTerm());

	for (auto it = (ec->instantiatedVariables()).begin(); it != (ec->instantiatedVariables()).end(); ++it) {
		body.push_back((*it)->instantiation()->asTerm());
	}
	auto head = ec->asTerm();
	head->addInputvarsToCheck(ec->quantifiedVariables());
	auto clause = new PrologClause(head, body);
	_pp->addClause(clause);

	ec->child()->accept(this);
}

void FormulaClauseToPrologClauseConverter::visit(ForallClause* fc) {
	PrologTerm* forall = new PrologTerm(XSBToIDPTranslator::get_forall_term_name());
	std::list<PrologTerm*> list;
//	Create generator clause
	AndClause* tmp = new AndClause(generateGeneratorClauseName());
	for (auto it = (fc->quantifiedVariables()).begin(); it != (fc->quantifiedVariables()).end(); ++it) {
		tmp->addChild((*it)->instantiation());
		list.push_back(*it);
	}
	tmp->arguments(list);
	this->visit(tmp);
	forall->addArgument(tmp->asTerm());
	auto tmp2 = new PrologTerm("");
	auto term = fc->child()->asTerm();
	std::list<PrologVariable*> emptylist;
	term->variables(emptylist);
	tmp2->addArgument(fc->child()->asTerm());
	forall->addArgument(tmp2);
	forall->addInputvarsToCheck(set<PrologVariable*>(fc->variables().begin(), fc->variables().end()));
	_pp->addClause(new PrologClause(fc->asTerm(), forall));
	fc->child()->accept(this);
}

void FormulaClauseToPrologClauseConverter::visit(AndClause* ac) {
	std::list<PrologTerm*> body;
	for (std::list<FormulaClause*>::iterator it = (ac->children()).begin(); it != (ac->children()).end(); ++it) {
		body.push_back((*it)->asTerm());
		(*it)->accept(this);
	}
	_pp->addClause(new PrologClause(ac->asTerm(), body, false));
}

void FormulaClauseToPrologClauseConverter::visit(OrClause* oc) {
	for (std::list<FormulaClause*>::iterator it = (oc->children()).begin(); it != (oc->children()).end(); ++it) {
		std::list<PrologTerm*> body;
		body.push_back((*it)->asTerm());
		auto head = oc->asTerm();
		auto headvars = list<PrologVariable*>(oc->variables());
		auto childvars = list<PrologVariable*>((*it)->variables());
		for (auto var = childvars.begin(); var != childvars.end(); ++var) {
			headvars.remove(*var);
		}
		head->addInputvarsToCheck(set<PrologVariable*>(headvars.begin(), headvars.end()));
		_pp->addClause(new PrologClause(oc->asTerm(), body));
		(*it)->accept(this);
	}
}

void FormulaClauseToPrologClauseConverter::visit(AggregateClause* ac) {
	list<PrologTerm*> body;
	body.push_back(ac->aggterm()->asTerm());
	auto term = new PrologTerm(ac->comparison_type());
	// Note: it is important to put the term first, because the unnest aggs transformation
	// will rewrite any "AGG COMP TERM" into "TERM COMP* AGG" with COMP* the opposite comparison operator of COMP
	// Thus making the term always the first argument of the comparison
	term->addArgument(ac->term());
	term->addArgument(ac->aggterm()->result());
	if (isa<PrologVariable>(*(ac->term()))) {
		if (ac->comparison_type() == "=") { // if we're doing an assignment, check the OUTPUT
			term->addOutputvarToCheck((PrologVariable*) ac->term());
		} else { // else, instantiate the var before the comparison
			term->addInputvarToCheck((PrologVariable*) ac->term());
		}
	}
	body.push_back(term);
	ac->aggterm()->accept(this);
	
	_pp->addClause(new PrologClause(ac->asTerm(), body, false));
}

void FormulaClauseToPrologClauseConverter::visit(AggregateTerm* at) {
	list<PrologTerm*> body;
	for(auto it = at->instantiatedVariables().cbegin(); it != at->instantiatedVariables().cend(); it++) {
		PrologTerm* boundSort = new PrologTerm((*it)->type());
		auto sortArg = _translator->create((*it)->name());
		boundSort->addArgument(sortArg);
		body.push_back(boundSort);
	}
	PrologTerm* findall;
	if (at->set()->hasTwoValuedBody()) {
		findall = new PrologTerm(XSBToIDPTranslator::get_twovalued_findall_term_name());
	} else {
		// shenanigans 'cause XSB is less than awesome when it comes to findall depending on undefined symbols
		// because of this, a self-made predicate has to be used
		findall = new PrologTerm(XSBToIDPTranslator::get_threevalued_findall_term_name());
	}
	findall->addArgument(at->set()->var());
	findall->addArgument(at->set()->asTerm());
	auto listvar = _translator->create("INTERNAL_LIST");
	findall->addArgument(listvar);
	body.push_back(findall);
	at->set()->accept(this);
	PrologTerm* calculation = new PrologTerm(at->agg_type());
	calculation->addArgument(listvar);
	calculation->addArgument(at->result());
	body.push_back(calculation);
	_pp->addClause(new PrologClause(at->asTerm(), body, false));
}

void FormulaClauseToPrologClauseConverter::visit(QuantSetExpression* set) {
	list<PrologTerm*> body;
	auto f = set->clause();
	auto t = set->term();
	body.push_back(f->asTerm());
	auto unification = new PrologTerm("=");
	unification->addArgument(t);
	unification->addArgument(set->var());
	body.push_back(unification);
	set->clause()->accept(this);
	_pp->addClause(new PrologClause(set->asTerm(), body));
}

void FormulaClauseToPrologClauseConverter::visit(EnumSetExpression* set) {
	for (auto it = set->set().begin(); it != set->set().end(); ++it) {
		list<PrologTerm*> body;
		(*it).first->accept(this);
		(*it).second->accept(this);
		auto f = (*it).first;
		auto t = (*it).second;
		body.push_back(f->asTerm());
		auto unification = new PrologTerm("=");
		unification->addArgument(t);
		unification->addArgument(set->var());
		body.push_back(unification);
		_pp->addClause(new PrologClause(set->asTerm(), body));
	}
}
