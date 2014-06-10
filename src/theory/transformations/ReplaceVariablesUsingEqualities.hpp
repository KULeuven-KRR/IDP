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

using namespace FormulaUtils;

/**
 * Transformation which looks for variables which will only make a formula true if it is equal to another term.
 * In that case, it replaces all occurrences of the variable with the other term.
 *
 * NOTE: requires flattened, negation pushed and eqchain free input and assumes no graphed functions
 * 		(otherwise, less substitutions might be detected)
 * NOTE: afterwards it is best to apply simplify
 *
 *
 * algo:
 * for exists quants: find conjunctive ~= atoms, remove allowed
 * for forall quants: find disjunctive = atoms, remove allowed
 * for set quants: find conjunctive = atoms, remove?
 * for rule quants: find conjunctive = atoms, do not remove
 *
 * TODO look further down than just one level, e.g. push/pull quantifiers might help for this
 */
class ReplaceVariableUsingEqualities: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::map<PredForm*, Formula*> atomReplace;
	std::map<Variable*, Term*> replacements; // Note: CLONE!
	varset removingvars, allowedvars;

public:
	template<typename T>
	T execute(T t) {
		do{
			atomReplace.clear();
			replacements.clear();
			removingvars.clear();
			allowedvars.clear();
			t = t->accept(this);
		}while(not atomReplace.empty());
		return t;
	}

protected:
	bool isVar(Term* t) {
		return t->type() == TermType::VAR;
	}

	Term* visit(VarTerm* vt) {
		if (contains(replacements, vt->var())) {
			return replacements.at(vt->var())->cloneKeepVars()->accept(this);
		}
		return vt;
	}

	Formula* visit(PredForm* pf) {
		auto it = atomReplace.find(pf);
		if(it!=atomReplace.cend()){
			return it->second;
		}
		return traverse(pf);
	}

	Formula* visit(QuantForm* qf) {
		auto oldrepl = replacements;
		auto oldvars = allowedvars;
		allowedvars.insert(qf->quantVars().cbegin(), qf->quantVars().cend());

		checkAndAddReplacements(qf->subformula(), allowedvars, not qf->isUnivWithSign());
		auto result = traverse(qf);

		replacements = oldrepl;
		allowedvars = oldvars;
		return result;
	}

	QuantSetExpr* visit(QuantSetExpr* set) {
		auto oldrepl = replacements;
		auto oldrem = removingvars;

		checkAndAddReplacements(set->getCondition(), set->quantVars(), true);
		auto result = traverse(set);
		auto quants = result->quantVars();
		for (auto var : removingvars) {
			quants.erase(var);
		}
		set->setQuantVars(quants);

		replacements = oldrepl;
		removingvars = oldrem;
		return result;
	}

	Rule* visit(Rule* rule) {
		auto oldrepl = replacements;

		checkAndAddReplacements(rule->body(), rule->quantVars(), true);
		auto head = rule->head()->accept(this)->clone();
		auto body = rule->body()->accept(this)->clone();
		auto newrule = new Rule(rule->quantVars(), dynamic_cast<PredForm*>(head), body, { });
		rule->recursiveDelete();

		replacements = oldrepl;
		return newrule;
	}

	void checkAndAddReplacements(Formula* f, const varset& quantvars, bool replaceeqs) {
		auto bf = dynamic_cast<BoolForm*>(f);
		if(bf==NULL){
			addReplacements(f, replaceeqs, quantvars);
			return;
		}
		auto conjcontext = bf->isConjWithSign();
		if ((conjcontext || bf->subformulas().size() == 1) && replaceeqs) {
			addReplacements(bf, true, quantvars);
		}
		if (not replaceeqs && (not conjcontext || bf->subformulas().size() == 1)) {
			addReplacements(bf, false, quantvars);
		}
	}

	/**
	 * Goes through all subformulas and check whether an allowedvar can be replaced, through = if needeq is true and through ~= otherwise.
	 * If the replaced term might not have a value, a denotes atom is added (with proper negation depending on needeq).
	 * If the replaced term might not be in the var sort, a type check is added ( " " " ).
	 */
	void addReplacements(Formula* f, bool needeq, const varset& allowedvars) {
		for (auto subform : f->subformulas()) {
			auto pf = dynamic_cast<PredForm*>(subform);
			if (pf == NULL || not VocabularyUtils::isPredicate(pf->symbol(), STDPRED::EQ)) {
				continue;
			}
			auto left = pf->subterms()[0];
			auto right = pf->subterms()[1];
			if(isVar(right)){
				swap(left, right);
			}
			auto vt = dynamic_cast<VarTerm*>(left);
			if(vt==NULL || not contains(allowedvars, vt->var())
					|| contains(replacements, vt->var())
					|| (needeq && pf->sign() == SIGN::NEG)	|| (not needeq && pf->sign() == SIGN::POS)
					|| contains(right->freeVars(), vt->var())){
				continue;
			}
			bool loop = false;
			for (auto var : right->freeVars()) {
				if (contains(replacements, var)) {
					loop = true;
				}
			}
			if (loop) {
				continue;
			}

			auto ft = dynamic_cast<FuncTerm*>(right);
			if(ft!=NULL && ft->function()->partial()){
				if(f->subformulas().size()==1){ // We will add again what we drop, so abort
					continue;
				}
				auto v = Gen::var(left->sort());
				atomReplace[pf] = new QuantForm(needeq?SIGN::POS:SIGN::NEG, QUANT::EXIST, {v}, &Gen::operator ==(*new VarTerm(v, {}), *right->cloneKeepVars()), {});
			}else if(left->sort()!=right->sort()){
				atomReplace[pf] = new PredForm(needeq?SIGN::POS:SIGN::NEG, left->sort()->pred(), {right->cloneKeepVars()}, {});
			} else {
				atomReplace[pf] = needeq?trueFormula():falseFormula();
			}
			removingvars.insert(vt->var());
			replacements[vt->var()] = right->cloneKeepVars();
		}
	}
};
