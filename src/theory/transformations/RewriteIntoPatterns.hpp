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

#include "IncludeComponents.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

class AbstractStructure;
class Vocabulary;
class Formula;
class Variable;
class Sort;
class Term;

/**
 * Algoritme:
 * geef lijst van patronen en predform
 * kies een patroon
 * patroon bepaalt welke acties er op het predform gedaan moeten worden:
 * 		welke termen unnesten (dit mogen dieper geneste termen zijn)
 * 		graphen van het geheel
 * 		negatie van het geheel
 * 		swap van termen
 * voer die acties uit en recursief op elke gegenereerde predform
 */

class Pattern {
public:
	virtual ~Pattern();
	virtual bool isAllowed(PredForm* pf) const = 0;
};

// Default: always allowed to unnest until only variables left (or equality with one var and one domelem).
class DefaultAtom: public Pattern {
public:
	virtual bool isAllowed(PredForm* pf) const {
		int count = 0;
		for (auto t : pf->subterms()) {
			if (t->type() != TermType::VAR) {
				if (t->type() == TermType::DOM) {
					count++;
					if (count > 1) {
						return false;
					}
				} else {
					return false;
				}
			}
		}
		if (count == 1) {
			return is(pf->symbol(), STDPRED::EQ);
		}
		return true;
	}
};

class DVAtom: public Pattern {
public:
	virtual bool isAllowed(PredForm* pf) const {
		for (auto t : pf->subterms()) {
			if (t->type() != TermType::DOM && t->type() != TermType::VAR) {
				return false;
			}
		}
		return true;
	}
};

class RewriteIntoPatterns: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	bool _rewritetootherformula, _done;
	std::vector<Formula*> _addedformula;
	std::vector<Pattern*> _patterns;

	bool _saved;
	std::pair<Variable*, PredForm*> _savedrewriting;

	Vocabulary* _vocabulary;
	AbstractStructure* _structure;

public:
	Formula* execute(Formula* f, const std::vector<Pattern*>& patterns, AbstractStructure* structure) {
		_structure = structure;
		_vocabulary = structure->vocabulary();
		_patterns = patterns;
		_done = false;
		_saved = false;
		while (not _done) {
			_done = true;
			f = f->accept(this);
		}
		return f;
	}

protected:
	// TODO handle other occurrences of atoms (aggform, eqchainform, ...)
	// TODO use the rule approach for sets!

	virtual Formula* visit(PredForm* pf) {
		Term* proposal = NULL;
		for (auto p : _patterns) { // TODO in a first analysis, decide which pattern we want to work towards!
			proposal = p->isAllowed(pf);
			if (proposal==NULL) {
				return pf;
			}
		}
		return rewrite(pf, proposal);
	}

	virtual Rule* visit(Rule* r) {
		_rewritetootherformula = true;
		r->head(dynamic_cast<PredForm*>(r->head()->accept(this)));
		if(_saved){
			r->addvar(_savedrewriting.first);
			r->body(new BoolForm(SIGN::POS, true, _savedrewriting.second, r->body(), FormulaParseInfo()));
			_saved = false;
		}
		_rewritetootherformula = false;
		r->body(r->body()->accept(this));
		return r;
	}

	Formula* rewrite(PredForm* pf, Term* proposal) {
		_done = false;
		int index = -1;
		Sort* sort = NULL;
		if (is(pf->symbol(), STDPRED::EQ)) {
			if(not isVar(pf->subterms()[1]) && isVar(pf->subterms()[0])){
				auto temp = pf->subterms()[1];
				pf->subterm(1, pf->subterms()[0]);
				pf->subterm(0, temp);
			}
			if(not isVar(pf->subterms()[0])){
				index = 0;

				// chosensort is smallest of both sorts if they are subterms
				auto leftsort = pf->subterms()[0]->sort();
				auto rightsort = pf->subterms()[1]->sort();
				if (SortUtils::isSubsort(leftsort, rightsort)) {
					sort = leftsort;
				} else {
					sort = rightsort;
				}
			}else{
				return makeFuncGraph(pf->sign(), pf->subterms()[1], pf->subterms()[0], pf->pi());
			}
		}else{
			for(uint i=0; i<pf->subterms().size(); ++i){
				if(not isVar(pf->subterms()[i]) && not isDom(pf->subterms()[i])){
					index = i;
					break;
				}
			}
		}
		Assert(index>=0);

		auto term = pf->subterms()[index];
		std::pair<Variable*, PredForm*> vp = move(term, sort);
		pf->subterm(index, new VarTerm(vp.first, TermParseInfo()));

		auto formula = vp.second;
		if(_rewritetootherformula){
			_saved = true;
			_savedrewriting = vp;
		}else{
			formula = new BoolForm(SIGN::POS, true, {formula, pf}, pf->pi());
			formula = new QuantForm(SIGN::POS, QUANT::EXIST, {vp.first}, formula, pf->pi()); // TODO sign?
		}
		return formula;
	}

	bool isAgg(Term* t) const {
		return t->type() == TermType::AGG;
	}
	bool isFunc(Term* t) const {
		return t->type() == TermType::FUNC;
	}
	bool isDom(Term* t) const {
		return t->type() == TermType::DOM;
	}
	bool isVar(Term* t) const {
		return t->type() == TermType::VAR;
	}

	/**
	 * Given functerm = dom/varterm, construct graph
	 */
	PredForm* makeFuncGraph(SIGN sign, Term* functerm, Term* varterm, const FormulaParseInfo& pi) const {
		Assert(isVar(varterm));
		if(isAgg(functerm)){
			auto aggterm = dynamic_cast<AggTerm*>(functerm);
			return new AggForm(sign, varterm, CompType::EQ, aggterm, pi);
		}else{
			Assert(isFunc(functerm));
			auto ft = dynamic_cast<FuncTerm*>(functerm);
			auto args = ft->subterms();
			args.push_back(varterm);
			return new PredForm(sign, ft->function(), args, pi);
		}
	}

	/**
	 * Given a term t
	 * 		Add a quantified variable v over sort(t)
	 * 		Add an equality t =_sort(t) v
	 * 		return v
	 */
	std::pair<Variable*, PredForm*> move(Term* origterm, Sort* chosensort) {
		Assert(origterm->sort()!=NULL);

		auto newsort = deriveSort(origterm, chosensort);
		Assert(newsort != NULL);

		auto var = new Variable(newsort);
		if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 1) {
			Warning::introducedvar(var->name(), var->sort()->name(), toString(origterm));
		}

		auto varterm = new VarTerm(var, TermParseInfo(origterm->pi()));
		auto equalatom = new PredForm(SIGN::POS, get(STDPRED::EQ, origterm->sort()), { varterm, origterm }, FormulaParseInfo());

		for(auto sort: equalatom->symbol()->sorts()){
			Assert(sort!=NULL);
		}

		return {var, equalatom};
	}

	/**
	 * Tries to derive a sort for the term given a structure.
	 */
	Sort* deriveSort(Term* term, Sort* chosensort) {
		auto sort = (chosensort != NULL) ? chosensort : term->sort();
		if (_structure != NULL && SortUtils::isSubsort(term->sort(), get(STDSORT::INTSORT), _vocabulary)) {
			sort = TermUtils::deriveSmallerSort(term, _structure);
		}
		return sort;
	}
};
