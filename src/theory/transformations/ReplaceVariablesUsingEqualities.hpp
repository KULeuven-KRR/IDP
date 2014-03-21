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

/**
 * Transformation which looks for variables which will only make a formula true if it is equal to a function term.
 * In that case, it replaces all occurrences of the variable with the function term.
 *
 * NOTE: prior needs FLATTENED, NEG PUSHED and EQCHAIN removal!
 * NOTE: afterwards it is best to run an unused-variable elimination transformation.
 *
 * FIXME Should folding of partial terms introduce EXISTS?
 */
class ReplaceVariableByFuncTerm: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::set<PredForm*> removals;
	std::map<Variable*, FuncTerm*> replacements; // Note: CLONE!
	varset removingvars, allowedvars;

public:
	template<typename T>
	T execute(T t) {
		replacements.clear();
		t = dynamic_cast<T>(FormulaUtils::splitComparisonChains(t));
		t = t->accept(this);
		return t;
	}

protected:
	bool hasVarInEq(PredForm* pf) {
		return getVarOfEq(pf) != NULL;
	}
	bool hasFuncInEq(PredForm* pf) {
		return getFuncOfEq(pf) != NULL;
	}
	bool isVar(Term* t) {
		return t->type() == TermType::VAR;
	}
	Variable* getVarOfEq(PredForm* pf) {
		if (isa<VarTerm>(*pf->subterms()[0])) {
			return dynamic_cast<VarTerm*>(pf->subterms()[0])->var();
		} else if (isa<VarTerm>(*pf->subterms()[1])) {
			return dynamic_cast<VarTerm*>(pf->subterms()[1])->var();
		} else {
			return NULL;
		}
	}
	FuncTerm* getFuncOfEq(PredForm* pf) {
		if (isa<FuncTerm>(*pf->subterms()[0])) {
			return dynamic_cast<FuncTerm*>(pf->subterms()[0]);
		} else if (isa<FuncTerm>(*pf->subterms()[1])) {
			return dynamic_cast<FuncTerm*>(pf->subterms()[1]);
		} else {
			return NULL;
		}
	}

	Term* visit(VarTerm* vt) {
		if (contains(replacements, vt->var())) {
			return replacements.at(vt->var())->cloneKeepVars()->accept(this);
		}
		return vt;
	}

	Formula* visit(PredForm* pf) {
		if (contains(removals, pf)) {
			auto maketrue = pf->sign() == SIGN::POS;
			if (maketrue) {
				return new BoolForm(SIGN::POS, true, { }, pf->pi());
			} else {
				return new BoolForm(SIGN::POS, false, { }, pf->pi());
			}
		}
		return traverse(pf);
	}

	/**
	 * algo:
	 * for exists quants: find conjunctive ~= atoms, remove allowed
	 * for forall quants: find disjunctive = atoms, remove allowed
	 * for set quants: find conjunctive = atoms, remove?
	 * for rule quants: find conjunctive = atoms, do not remove
	 */

	Formula* visit(QuantForm* qf) {
		auto oldrepl = replacements;
		auto oldvars = allowedvars;

		allowedvars.insert(qf->quantVars().cbegin(), qf->quantVars().cend());
		checkAndAddReplacements(qf->subformula(), allowedvars, not qf->isUnivWithSign(), true);
		auto result = traverse(qf);

		replacements = oldrepl;
		allowedvars = oldvars;

		return result;
	}

	QuantSetExpr* visit(QuantSetExpr* set) {
		auto oldrepl = replacements;
		removingvars.clear();
		checkAndAddReplacements(set->getCondition(), set->quantVars(), true, false);
		auto result = traverse(set);
		auto quants = result->quantVars();
		for(auto var:removingvars){
			quants.erase(var);
		}
		set->setQuantVars(quants);
		replacements = oldrepl;
		return result;
	}

	void checkAndAddReplacements(Formula* f, const varset& quantvars, bool whenconj, bool alsonegation){
		auto bf = dynamic_cast<BoolForm*>(f);
		if (bf != NULL) {
			auto conjcontext = bf->isConjWithSign();
			if((conjcontext || bf->subformulas().size()==1) && whenconj){
				addReplacements(bf, conjcontext, quantvars);
			}
			if(alsonegation && not whenconj && (not conjcontext || bf->subformulas().size()==1)){
				addReplacements(bf, conjcontext, quantvars);
			}
		}else{
			addReplacements(new BoolForm(f->sign(), whenconj, {f}, FormulaParseInfo()), whenconj, quantvars);
		}
	}

	Rule* visit(Rule* rule) {
		auto oldrepl = replacements;
		checkAndAddReplacements(rule->body(), rule->quantVars(), true, false);
		auto head = rule->head()->accept(this);
		auto body = rule->body()->accept(this);
		replacements = oldrepl;
		auto newrule = new Rule(rule->quantVars(), dynamic_cast<PredForm*>(head), body, ParseInfo());
		return newrule->clone();
	}

	void addReplacements(BoolForm* bf, bool conjcontext, const varset& allowedvars) {
		for (auto subform : bf->subformulas()) {
			auto pf = dynamic_cast<PredForm*>(subform);
			if (pf == NULL || pf->args().size() != 2) {
				continue;
			}
			if (((conjcontext && pf->sign() == SIGN::POS) || (not conjcontext && pf->sign() == SIGN::NEG))
					&& VocabularyUtils::isPredicate(pf->symbol(), STDPRED::EQ) && hasVarInEq(pf) && hasFuncInEq(pf)
					&& not contains(replacements, getVarOfEq(pf)) && contains(allowedvars, getVarOfEq(pf))) {
				if(contains(getFuncOfEq(pf)->freeVars(), getVarOfEq(pf))){ // TODO can be improved
					continue;
				}
				bool loop = false;
				for(auto var:getFuncOfEq(pf)->freeVars()){
					if(contains(replacements, var)){
						loop = true;
						continue;
					}
				}
				if(loop){
					continue;
				}
				replacements[getVarOfEq(pf)] = getFuncOfEq(pf)->cloneKeepVars();
				if(not getFuncOfEq(pf)->function()->partial() || pf->sign()==SIGN::POS){
					removals.insert(pf);
				}
				removingvars.insert(getVarOfEq(pf));
			} else if (((conjcontext && pf->sign() == SIGN::POS) || (not conjcontext && pf->sign() == SIGN::NEG)) && pf->symbol()->isFunction()
					&& pf->subterms().back()->type() == TermType::VAR && not contains(replacements, dynamic_cast<VarTerm*>(pf->subterms().back())->var())
			 	 	 && contains(allowedvars, dynamic_cast<VarTerm*>(pf->subterms().back())->var())) {
				auto var = dynamic_cast<VarTerm*>(pf->subterms().back())->var();
				if(contains(getFuncOfEq(pf)->freeVars(), var)){ // TODO can be improved
					continue;
				}
				bool loop = false;
				for(auto var:getFuncOfEq(pf)->freeVars()){
					if(contains(replacements, var)){
						loop = true;
						continue;
					}
				}
				if(loop){
					continue;
				}
				std::vector<Term*> terms;
				for (auto term : pf->subterms()) {
					terms.push_back(term->cloneKeepVars());
				}
				terms.pop_back();
				replacements[dynamic_cast<VarTerm*>(pf->subterms().back())->var()] = new FuncTerm(dynamic_cast<Function*>(pf->symbol()), terms,	TermParseInfo());
				if(not getFuncOfEq(pf)->function()->partial() || pf->sign()==SIGN::POS){
					removals.insert(pf);
				}
				removingvars.insert(dynamic_cast<VarTerm*>(pf->subterms().back())->var());
			}
		}
	}
};
