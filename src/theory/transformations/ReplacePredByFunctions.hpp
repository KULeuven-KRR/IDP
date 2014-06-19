/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#pragma once

#include "visitors/TheoryMutatingVisitor.hpp"
#include "IncludeComponents.hpp"
#include "creation/cppinterface.hpp"

/**
 * Given a functional dependency <P,domainindices,codomainindicates,partial>
 * 		=> each of the arguments of P at a codomainindex functionally depends only on the arguments of P at the domainindices.
 * 			The dependency is partial if partial is true
 *
 * 		The transformation replaces every occurrence of P with those new functions.
 */
class ReplacePredByFunctions: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	Vocabulary* vocabulary;
	Predicate* _predToReplace;
	std::set<int> _domainindices;
	std::map<int, Function*> _index2function; // Which function was introduced for which argument (codomain)index of _predToReplace

	std::vector<Rule*> _rules; // If the symbol is defined, we might possibly have to introduce multiple rules if there are multiple functions

public:
	template<typename T>
	T execute(T t, Vocabulary* voc, Predicate* pred, bool addinoutputdef, const std::set<int>& domainindices, const std::set<int>& codomainsindices, bool partialfunctions){
		vocabulary = voc;
		_predToReplace = pred;
		_domainindices = domainindices;
		std::vector<Sort*> domainsorts;
		for(auto ind: domainindices){
			domainsorts.push_back(pred->sorts()[ind]);
		}
		for(auto ind: codomainsindices){
			std::stringstream ss;
			ss <<pred->nameNoArity() << "_" << ind;
			auto newfunc = new Function(ss.str(), domainsorts, pred->sorts()[ind], ParseInfo());
			newfunc->partial(partialfunctions);
			vocabulary->add(newfunc);
			_index2function[ind] = newfunc;
		}
		t = t->accept(this);

		if(addinoutputdef){
			std::vector<Variable*> vars;
			for (uint i = 0; i < pred->sorts().size(); ++i) {
				vars.push_back(new Variable(pred->sort(i)));
			}

			auto newrule = new Rule(getVarSet(vars), &Gen::atom(pred, vars), Gen::atom(pred, vars).accept(this), ParseInfo());

			auto outputdef = new Definition();
			outputdef->add(newrule);
			t->add(outputdef);
		}

		return t;
	}

protected:
	Formula* visit(PredForm* pf){
		if(pf->symbol()!=_predToReplace){
			return pf;
		}
		auto bf = new BoolForm(pf->sign(), true, {}, FormulaParseInfo());
		for(auto ind2func: _index2function){
			auto origterm = pf->subterms()[ind2func.first]->cloneKeepVars();
			std::vector<Term*> domainterms;
			for(auto ind: _domainindices){
				domainterms.push_back(pf->subterms()[ind]->cloneKeepVars());
			}
			auto newterm = new FuncTerm(ind2func.second, domainterms, TermParseInfo());
			bf->addSubformula(new PredForm(SIGN::POS, get(STDPRED::EQ)->disambiguate({origterm->sort(), origterm->sort()}), {origterm, newterm}, FormulaParseInfo()));
		}
		pf->recursiveDeleteKeepVars();
		return bf;
	}
	Definition* visit(Definition* def){
		_rules.clear();
		ruleset newset;
		for (auto rule : def->rules()) {
			newset.insert(rule->accept(this));
		}
		def->rules(newset);
		for(auto rule:_rules){
			def->add(rule);
		}
		_rules.clear();
		return def;
	}
	Rule* visit(Rule* rule){
		auto body = rule->body()->accept(this);
		rule->body(body);
		if(rule->head()->symbol()!=_predToReplace){
			return rule;
		}

		varset newvars;
		std::map<Variable*, Variable*> var2var;
		for(auto var: rule->quantVars()){
			auto newvar = new Variable(var->sort());
			newvars.insert(newvar);
			var2var[var] = newvar;
		}

		auto pf = rule->head();
		std::vector<Term*> domainterms;
		for(auto ind: _domainindices){
			domainterms.push_back(pf->subterms()[ind]->clone(var2var));
		}

		std::vector<Rule*> rules;
		for(auto ind2func: _index2function){
			auto origterm = pf->subterms()[ind2func.first]->clone(var2var);
			domainterms.push_back(origterm->clone(var2var));
			auto newhead = new PredForm(SIGN::POS, ind2func.second, domainterms, {});
			rules.push_back(new Rule(newvars, newhead, rule->body()->clone(var2var), ParseInfo()));
		}
		auto lastrule = rules.back();
		rules.pop_back();
		insertAtEnd(_rules, rules);
		return lastrule;
	}
};
