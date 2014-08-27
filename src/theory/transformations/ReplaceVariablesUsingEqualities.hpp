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
 * NOTE: requires flattened, negation and quant pushed and eqchain free input and assumes no graphed functions
 * 		(otherwise, less substitutions might be detected)
 * NOTE: variables are not removed, as we do not have a structure here to check for empty domains.
 * 		For this, run simplify or improvetheoryforinference with a structure afterwards *
 *
 * algo:
 * 		for exists quants: remove conjunctive = atoms
 * 		for forall quants: remove disjunctive ~= atoms
 * 		for set quants: remove conjunctive = atoms, IMPORTANT: remove variable from quantset, otherwise set size might change
 * 		for rule quants: remove conjunctive = atoms
 */
class ReplaceVariableUsingEqualities: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::map<PredForm*, Formula*> atomReplace;
	std::map<Variable*, Term*> replacements;
	varset removingvars;
	uint replaced;

public:
	template<typename T>
	T execute(T t) {
		do {
			replaced = 0;
			atomReplace.clear();
			replacements.clear();
			removingvars.clear();
			t = t->accept(this);
		} while (replaced > 0);
		return t;
	}

protected:
	Term* visit(VarTerm* vt) {
		if (contains(replacements, vt->var())) {
			replaced++;
			return replacements.at(vt->var())->cloneKeepVars()->accept(this);
		}
		return vt;
	}

	Formula* visit(PredForm* pf) {
		auto it = atomReplace.find(pf);
		if (it != atomReplace.cend()) {
			return it->second;
		}
		return traverse(pf);
	}

	Formula* visit(QuantForm* qf) {
		auto oldrepl = replacements;
		checkAndAddReplacements(qf->subformula(), qf->quantVars(), not qf->isUniv());
		auto result = traverse(qf);
		replacements = oldrepl;
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
		if (bf == NULL) {
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

	bool canReplaceFirstWithSecond(Term* left, Term* right, PredForm* pf, bool hasToBeEquality, const varset& allowedvars){
		auto vt = dynamic_cast<VarTerm*>(left);
		if (vt == NULL
				|| not contains(allowedvars, vt->var())
				|| contains(replacements, vt->var())
				|| (hasToBeEquality && pf->sign() == SIGN::NEG)
				|| (not hasToBeEquality && pf->sign() == SIGN::POS)
				|| contains(right->freeVars(), vt->var())) {
			return false;
		}
		bool loop = false;
		for (auto var : right->freeVars()) {
			if (contains(replacements, var)) {
				loop = true;
			}
		}
		return not loop;
	}

	/**
	 * Goes through all subformulas and check whether an allowedvar can be replaced, through = if needeq is true and through ~= otherwise.
	 * If the replaced term might not have a value, a denotes atom is added (with proper negation depending on needeq).
	 * If the replaced term might not be in the var sort, a type check is added ( " " " ).
	 */
	void addReplacements(BoolForm* f, bool needeq, const varset& allowedvars) {
		for (auto subform : f->subformulas()) {
			auto pf = dynamic_cast<PredForm*>(subform);
			if (pf == NULL || not VocabularyUtils::isPredicate(pf->symbol(), STDPRED::EQ)) {
				continue;
			}
			auto left = pf->subterms()[0];
			auto right = pf->subterms()[1];
			if(canReplaceFirstWithSecond(right, left, pf, needeq, allowedvars)){
				swap(left,right);
			}
			if(not canReplaceFirstWithSecond(left, right, pf, needeq, allowedvars)){
				continue;
			}

			auto ft = dynamic_cast<FuncTerm*>(right);
			if (ft != NULL && ft->function()->partial()) {
				auto v = Gen::var(left->sort());
				atomReplace[pf] = new QuantForm(needeq ? SIGN::POS : SIGN::NEG, QUANT::EXIST, { v },
						&Gen::operator ==(*new VarTerm(v, { }), *right->cloneKeepVars()), { });
			} else if (left->sort() != right->sort()) {
				atomReplace[pf] = new PredForm(needeq ? SIGN::POS : SIGN::NEG, left->sort()->pred(), { right->cloneKeepVars() }, { });
			} else {
				atomReplace[pf] = needeq ? trueFormula() : falseFormula();
			}
			auto var = dynamic_cast<VarTerm*>(left)->var();
			removingvars.insert(var);
			replacements[var] = right->cloneKeepVars();
		}
	}
};
