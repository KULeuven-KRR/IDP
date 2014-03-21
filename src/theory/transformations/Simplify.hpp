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
#include "structure/DomainElement.hpp"
#include "utils/ListUtils.hpp"

class Simplify: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	const Structure* _structure;
	std::set<PFSymbol*> _defsyms;

public:
	template<typename T>
	T execute(T t, const Structure* s) {
		_structure = s;
		_defsyms = {};
		return t->accept(this);
	}

protected:
	bool isTrue(const Formula* f) const {
		auto bf = dynamic_cast<const BoolForm*>(f);
		return bf != NULL && bf->subformulas().size() == 0 && bf->isConjWithSign();
	}
	bool isFalse(const Formula* f) const {
		auto bf = dynamic_cast<const BoolForm*>(f);
		return bf != NULL && bf->subformulas().size() == 0 && not bf->isConjWithSign();
	}

	bool isIdentical(bool eq, Term* left, Term* right) {
		if (left->type() != right->type()) {
			return false;
		}
		switch (left->type()) {
		case TermType::VAR:
			return dynamic_cast<VarTerm*>(left)->var() == dynamic_cast<VarTerm*>(right)->var();
		case TermType::FUNC: {
			auto lft = dynamic_cast<FuncTerm*>(left);
			auto rft = dynamic_cast<FuncTerm*>(right);
			if (lft->function() != rft->function() || (lft->function()->partial() && not eq)) {
				return false;
			}
			auto allidentical = true;
			for (uint i = 0; i < lft->args().size(); ++i) {
				if (not isIdentical(eq, lft->subterms()[i], rft->subterms()[i])) {
					allidentical = false;
					break;
				}
			}
			return allidentical;
		}
		case TermType::DOM:
			return dynamic_cast<DomainTerm*>(left)->value() == dynamic_cast<DomainTerm*>(right)->value();
		case TermType::AGG:
			return false; // TODO smarter checking?
		}
	}

	Formula* getFormula(bool trueform) {
		return new BoolForm(SIGN::POS, trueform, { }, FormulaParseInfo());
	}

	Formula* visit(PredForm* pf) {
		if (VocabularyUtils::isPredicate(pf->symbol(), STDPRED::EQ) && isIdentical(pf->sign() == SIGN::POS, pf->subterms()[0], pf->subterms()[1])) {
			return getFormula(pf->sign() == SIGN::POS);
		}
		return pf;
	}

	Formula* simplify(Formula* f) {
		if (isTrue(f) || isFalse(f)) {
			return f;
		}
		auto bf = dynamic_cast<BoolForm*>(f);
		if (bf != NULL) {
			auto sometrue = false, somefalse = false;
			std::vector<Formula*> subforms;
			for (auto subform : bf->subformulas()) {
				auto sf = simplify(subform);
				if (isTrue(sf)) {
					sometrue = true;
					continue;
				} else if (isFalse(sf)) {
					somefalse = true;
					continue;
				}
				subforms.push_back(sf);
			}
			if (sometrue && not bf->isConjWithSign()) {
				return getFormula(true);
			} else if (somefalse && bf->isConjWithSign()) {
				return getFormula(false);
			} else {
				bf->subformulas(subforms);
			}
			return bf;
		}
		auto qf = dynamic_cast<QuantForm*>(f);
		if (qf != NULL) {
			auto subform = simplify(qf->subformula());
			if (isTrue(subform) && qf->isUnivWithSign()) {
				return subform;
			} else if (isFalse(subform) && not qf->isUnivWithSign()) {
				return subform;
			} else {
				qf->subformula(subform);
				return qf;
			}
		}
		// TODO handle more?
		return f;
	}

	Formula* visit(BoolForm* bf) {
		return simplify(traverse(bf));
	}

	Formula* visit(QuantForm* qf) {
		auto newqf = simplify(qf);
		if (newqf != qf) {
			return newqf->accept(this);
		}
		qf->subformula(qf->subformula()->accept(this));
		varset remainingvars;
		for (auto var : qf->quantVars()) {
			if (contains(qf->subformula()->freeVars(), var) || var->sort() == NULL || _structure==NULL || _structure->inter(var->sort())->empty()) {
				remainingvars.insert(var);
			}
		}

		if (remainingvars.empty()) {
			return qf->subformula()->cloneKeepVars();
		}
		if (remainingvars.size() < qf->quantVars().size()) {
			varset newvars;
			std::map<Variable*, Variable*> var2var;
			for (auto var : remainingvars) {
				auto newvar = new Variable(var->sort());
				newvars.insert(newvar);
				var2var[var] = newvar;
			}
			return new QuantForm(qf->sign(), qf->quant(), newvars, qf->subformula()->clone(var2var), FormulaParseInfo());
		}
		return qf;
	}

	Rule* visit(Rule* rule) {
		rule = TheoryMutatingVisitor::visit(rule);
		if (_structure == NULL) {
			return rule;
		}
		rule->body(rule->body()->accept(this));
		varset remainingvars;
		for (auto var : rule->quantVars()) {
			if (contains(rule->head()->freeVars(), var)
					|| contains(rule->body()->freeVars(), var)
					|| var->sort() == NULL
					|| _structure==NULL
					|| _structure->inter(var->sort())->empty()) {
				remainingvars.insert(var);
			}
		}
		rule->setQuantVars(remainingvars);
		return rule;
	}

	Term* visit(FuncTerm* ft) {
		auto newFuncTerm = dynamic_cast<FuncTerm*>(traverse(ft));
		auto f = newFuncTerm->function();
		if (contains(_defsyms, f)) {
			return newFuncTerm;
		}
		if (_structure->inter(f)->approxTwoValued()) {
			ElementTuple tuple(newFuncTerm->subterms().size());
			int i = 0;
			bool allDomainElements = true;
			for (auto term = newFuncTerm->subterms().cbegin(); term != newFuncTerm->subterms().cend(); term++, i++) {
				auto temp = dynamic_cast<const DomainTerm*>(*term);
				if (temp != NULL) {
					tuple[i] = temp->value();
				} else {
					allDomainElements = false;
					break;
				}
			}
			if (allDomainElements) {
				auto result = f->interpretation(_structure)->funcTable()->operator [](tuple);
				if (result == NULL) {
					//TODO: what should happen here?
					//I think smarter things can be done (like passing on the NULL to the superformula)
					return newFuncTerm;
				}
				auto finalresult = new DomainTerm(ft->sort(), result, ft->pi());
				newFuncTerm->recursiveDelete();
				return finalresult;
			}
		}
		return newFuncTerm;
	}

	Definition* visit(Definition* d) {
		_defsyms = d->defsymbols();
		auto result = TheoryMutatingVisitor::visit(d);
		_defsyms = {};
		return result;
	}
};
