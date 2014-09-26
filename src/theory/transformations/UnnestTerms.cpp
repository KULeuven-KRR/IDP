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

#include "IncludeComponents.hpp"
#include "structure/StructureComponents.hpp"
#include "UnnestTerms.hpp"

#include "errorhandling/error.hpp"
#include "theory/TheoryUtils.hpp"
#include "utils/ListUtils.hpp"
#include "theory/term.hpp"

#include <numeric> // for accumulate
#include <functional> // for multiplies
#include <algorithm> // for min_element and max_element
using namespace std;

UnnestTerms::UnnestTerms()
		: 	_structure(NULL),
			_vocabulary(NULL),
			_onlyrulehead(false),
			_allowedToUnnest(false),
			_chosenVarSort(NULL) {
}

/**
 * Returns true is the term should be moved
 * (this is the most important method to overwrite in subclasses)
 */
bool UnnestTerms::wouldMove(Term* t) {
	return t->type() != TermType::VAR && (_onlyrulehead || t->type() != TermType::DOM);
}
/**
 * Tries to derive a sort for the term given a structure.
 */
Sort* UnnestTerms::deriveSort(Term* term) {
	auto sort = (_chosenVarSort != NULL) ? _chosenVarSort : term->sort();
	if (_structure != NULL && SortUtils::isSubsort(term->sort(), get(STDSORT::INTSORT), _vocabulary)) {
		sort = TermUtils::deriveSmallerSort(term, _structure);
	}

	if (not isa<DomainTerm>(*term)) {
		return sort;
	}
	//For domainterms, we can optimise: all "built-in" domain terms (int, str, float), instead of
	//unnesting them as quantification over type "string, "int",... we can create a new type with one element in it, and quantify over that type
	//It is important that this type is a subtype of the correct parent since parts of the system rely on
	// well-typedness of all expressions. This also means that we cannot perform this optimsation for compounds.
	auto domterm = dynamic_cast<DomainTerm*>(term);
	auto domelem = domterm->value();
	auto domtype = domelem->type();
	if (domtype == DomainElementType::DET_COMPOUND) {
		return sort;
	}

	Sort* newParent = NULL;
	if (domtype == DomainElementType::DET_INT) {
		newParent = get(STDSORT::INTSORT);
	} else if (domtype == DomainElementType::DET_STRING) {
		newParent = get(STDSORT::STRINGSORT);
	} else if (domtype == DomainElementType::DET_DOUBLE) {
		newParent = get(STDSORT::FLOATSORT);
	}
	Assert(newParent != NULL);
	if (not SortUtils::isSubsort(sort, newParent)) {
		return sort;
	} else {
		auto ist = new EnumeratedInternalSortTable( { domelem });
		auto newsort = new Sort(new SortTable(ist));
		newsort->addParent(newParent);
		return newsort;
	}
}

/**
 * Given a term t
 * 		Add a quantified variable v over sort(t)
 * 		Add an equality t =_sort(t) v
 * 		return v
 */
Term* UnnestTerms::move(Term* origterm, Sort* newsort) {
	Assert(origterm->sort()!=NULL);
	if(newsort==NULL){
		newsort = deriveSort(origterm);
	}
	Assert(newsort != NULL);

	auto var = new Variable(newsort);
	_variables.insert(var);

	if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 2) {
		Warning::introducedvar(var->name(), var->sort()->name(), toString(origterm));
	}

	auto varterm = new VarTerm(var, TermParseInfo(origterm->pi()));
	auto equalatom = new PredForm(SIGN::POS, get(STDPRED::EQ, origterm->sort()), { varterm, origterm }, FormulaParseInfo());
#ifdef DEBUG
	for(auto sort: equalatom->symbol()->sorts()){
		Assert(sort!=NULL);
	}
#endif
	_equalities.push_back(equalatom);
	return varterm->clone();
}

/**
 * Rewrite the given formula with the current equalities
 */
Formula* UnnestTerms::rewrite(Formula* formula) {
	const FormulaParseInfo& origpi = formula->pi();
	if (not _equalities.empty()) {
		_equalities.push_back(formula);
		if (not _variables.empty()) {
			formula = new BoolForm(SIGN::POS, true, _equalities, origpi);
		} else {
			formula = new BoolForm(SIGN::POS, true, _equalities, FormulaParseInfo());
		}
		_equalities.clear();
	}
	if (not _variables.empty()) {
		formula = new QuantForm(SIGN::POS, QUANT::EXIST, _variables, formula, origpi);
		_variables.clear();
	}
	return formula;
}

/**
 *	Visit all parts of the theory, assuming positive context for sentences
 */
Theory* UnnestTerms::visit(Theory* theory) {
	for (auto it = theory->sentences().begin(); it != theory->sentences().end(); ++it) {
		setAllowedToUnnest(false);
		*it = (*it)->accept(this);
	}
	for (auto it = theory->definitions().begin(); it != theory->definitions().end(); ++it) {
		*it = (*it)->accept(this);
	}
	for (auto it = theory->fixpdefs().begin(); it != theory->fixpdefs().end(); ++it) {
		*it = (*it)->accept(this);
	}
	return theory;
}

void UnnestTerms::visitTermRecursive(Term* term){
	for(uint i=0; i<term->subterms().size(); ++i){
		auto subterm = term->subterms()[i];
		if(shouldMove(subterm)){
			auto newsub = move(subterm);
			term->subterm(i, newsub);
		}else{
			visitTermRecursive(subterm);
		}
	}
}

void UnnestTerms::visitRuleHead(Rule* rule) {
	Assert(_equalities.empty());
	for (size_t termposition = 0; termposition < rule->head()->subterms().size(); ++termposition) {
		auto term = rule->head()->subterms()[termposition];
		if (shouldMove(term)) {
			auto new_head_term = move(term, rule->head()->symbol()->sort(termposition));
			rule->head()->subterm(termposition, new_head_term);
		}else{
			visitTermRecursive(term);
		}
	}
	if (not _equalities.empty()) {
		for (auto it = _variables.cbegin(); it != _variables.cend(); ++it) {
			rule->addvar(*it);
		}
		if (not rule->body()->trueFormula()) {
			_equalities.push_back(rule->body());
		}
		rule->body(new BoolForm(SIGN::POS, true, _equalities, FormulaParseInfo()));

		_equalities.clear();
		_variables.clear();
	}
}

/**
 *	Visit a rule, assuming negative context for body.
 *	May move terms from rule head.
 */
Rule* UnnestTerms::visit(Rule* rule) {
// Visit head
	auto saveallowed = isAllowedToUnnest();
	setAllowedToUnnest(true);
	visitRuleHead(rule);

// Visit body
	if(not _onlyrulehead){
		setAllowedToUnnest(false);
		rule->body(rule->body()->accept(this));
	}

	setAllowedToUnnest(saveallowed);
	return rule;
}

Formula* UnnestTerms::traverse(Formula* f) {
	bool savemovecontext = isAllowedToUnnest();
	for (size_t n = 0; n < f->subterms().size(); ++n) {
		f->subterm(n, f->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < f->subformulas().size(); ++n) {
		f->subformula(n, f->subformulas()[n]->accept(this));
	}
	setAllowedToUnnest(savemovecontext);
	return f;
}

Formula* UnnestTerms::visit(EquivForm* ef) {
	auto newef = traverse(ef);
	return doRewrite(newef);
}

Formula* UnnestTerms::visit(AggForm* af) {
	auto savemovecontext = isAllowedToUnnest();
	af->setAggTerm(dynamic_cast<AggTerm*>(af->getAggTerm()->accept(this)));
	setAllowedToUnnest(true);
	af->setBound(af->getBound()->accept(this));
	setAllowedToUnnest(savemovecontext);
	return doRewrite(af);
}

Formula* UnnestTerms::visit(EqChainForm* ecf) {
	if (ecf->comps().size() == 1) { // Rewrite to a normal atom
		SIGN atomsign = ecf->sign();
		Sort* atomsort = SortUtils::resolve(ecf->subterms()[0]->sort(), ecf->subterms()[1]->sort(), _vocabulary);
		Predicate* comppred = NULL;
		switch (ecf->comps()[0]) {
		case CompType::EQ:
			comppred = get(STDPRED::EQ, atomsort);
			break;
		case CompType::LT:
			comppred = get(STDPRED::LT, atomsort);
			break;
		case CompType::GT:
			comppred = get(STDPRED::GT, atomsort);
			break;
		case CompType::NEQ:
			comppred = get(STDPRED::EQ, atomsort);
			atomsign = not atomsign;
			break;
		case CompType::LEQ:
			comppred = get(STDPRED::GT, atomsort);
			atomsign = not atomsign;
			break;
		case CompType::GEQ:
			comppred = get(STDPRED::LT, atomsort);
			atomsign = not atomsign;
			break;
		}
		Assert(comppred != NULL);
		vector<Term*> atomargs = { ecf->subterms()[0], ecf->subterms()[1] };
		PredForm* atom = new PredForm(atomsign, comppred, atomargs, ecf->pi());
		delete ecf;
		return atom->accept(this);
	} else { // Simple recursive call
		bool savemovecontext = isAllowedToUnnest();
		setAllowedToUnnest(true);
		auto newecf = traverse(ecf);
		setAllowedToUnnest(savemovecontext);
		return doRewrite(newecf);
	}
}

Formula* UnnestTerms::unnest(PredForm* predform) {
	// Special treatment for (in)equalities: possibly only one side needs to be moved
	bool savemovecontext = isAllowedToUnnest();
	bool moveonlyleft = false;
	bool moveonlyright = false;
	if (VocabularyUtils::isComparisonPredicate(predform->symbol())) {
		auto leftterm = predform->subterms()[0];
		auto rightterm = predform->subterms()[1];
		if(leftterm->type() == TermType::AGG && rightterm->type() == TermType::AGG){
			if(not wouldMove(rightterm)){
				moveonlyright = true;
			}else{
				moveonlyleft = true;
			}
		}else if (leftterm->type() == TermType::AGG) {
			moveonlyright = true;
		} else if (rightterm->type() == TermType::AGG) {
			moveonlyleft = true;
		} else if (is(predform->symbol(), STDPRED::EQ)) {
			moveonlyright = (leftterm->type() != TermType::VAR) && (rightterm->type() != TermType::VAR);
		} else {
			setAllowedToUnnest(true);
		}

		if (is(predform->symbol(), STDPRED::EQ)) {
			auto leftsort = leftterm->sort();
			auto rightsort = rightterm->sort();
			if (SortUtils::isSubsort(leftsort, rightsort)) {
				_chosenVarSort = leftsort;
			} else {
				_chosenVarSort = rightsort;
			}
		}
	} else {
		setAllowedToUnnest(true);
	}
	// Traverse the atom
	Formula* newf = predform;
	if (moveonlyleft) {
		predform->subterm(1, predform->subterms()[1]->accept(this));
		setAllowedToUnnest(true);
		predform->subterm(0, predform->subterms()[0]->accept(this));
	} else if (moveonlyright) {
		predform->subterm(0, predform->subterms()[0]->accept(this));
		setAllowedToUnnest(true);
		predform->subterm(1, predform->subterms()[1]->accept(this));
	} else {
		newf = traverse(predform);
	}
	_chosenVarSort = NULL;
	setAllowedToUnnest(savemovecontext);
	// Return result
	return newf;
}

Formula* UnnestTerms::visit(PredForm* predform) {
// Special treatment for (in)equalities: possibly only one side needs to be moved
	auto newf = unnest(predform);
	return doRewrite(newf);
}

Term* UnnestTerms::traverse(Term* term) {
	auto saveChosenVarSort = _chosenVarSort;
	_chosenVarSort = NULL;
	bool savemovecontext = isAllowedToUnnest();
	for (size_t n = 0; n < term->subterms().size(); ++n) {
		term->subterm(n, term->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < term->subsets().size(); ++n) {
		term->subset(n, term->subsets()[n]->accept(this));
	}
	_chosenVarSort = saveChosenVarSort;
	setAllowedToUnnest(savemovecontext);
	return term;
}

VarTerm* UnnestTerms::visit(VarTerm* t) {
	return t;
}

Term* UnnestTerms::visit(DomainTerm* t) {
	if (shouldMove(t)) {
		return move(t);
	}
	return t;
}

Term* UnnestTerms::visit(AggTerm* t) {
	auto savemovecontext = isAllowedToUnnest();

	auto result = traverse(t);

	setAllowedToUnnest(savemovecontext);

	if (shouldMove(result)) {
		return move(result);
	}
	return result;
}

Term* UnnestTerms::visit(FuncTerm* t) {
	bool savemovecontext = isAllowedToUnnest();
	setAllowedToUnnest(true);
	auto result = traverse(t);
	setAllowedToUnnest(savemovecontext);
	if (shouldMove(result)) {
		return move(result);
	}
	return result;
}

EnumSetExpr* UnnestTerms::visit(EnumSetExpr* s) {
	auto saveequalities = _equalities;
	_equalities.clear();
	auto savevars = _variables;
	_variables.clear();
	bool savemovecontext = isAllowedToUnnest();
	setAllowedToUnnest(true);

	for (uint i = 0; i < s->getSets().size(); ++i) {
		s->setSet(i, s->getSets()[i]->accept(this));
		if (not _equalities.empty()) {
			//_equalities.push_back(s->subformulas()[n]);
			//s->subformula(n, new BoolForm(SIGN::POS, true, _equalities, FormulaParseInfo()));
			savevars.insert(_variables.cbegin(), _variables.cend());
			insertAtEnd(saveequalities, _equalities);
			_equalities.clear();
			_variables.clear();
		}
	}

	setAllowedToUnnest(savemovecontext);
	_variables = savevars;
	_equalities = saveequalities;
	return s;
}

QuantSetExpr* UnnestTerms::visit(QuantSetExpr* s) {
	vector<Formula*> saveequalities = _equalities;
	_equalities.clear();
	varset savevars = _variables;
	_variables.clear();
	bool savemovecontext = isAllowedToUnnest();
	setAllowedToUnnest(true);

	s->setTerm(s->getTerm()->accept(this));
	if (not _equalities.empty()) {
		_equalities.push_back(s->getCondition());
		BoolForm* bf = new BoolForm(SIGN::POS, true, _equalities, FormulaParseInfo());
		s->setCondition(bf);
		for (auto it = _variables.cbegin(); it != _variables.cend(); ++it) {
			s->addQuantVar(*it);
		}
		_equalities.clear();
		_variables.clear();
	}

	setAllowedToUnnest(false);
	s->setCondition(s->getCondition()->accept(this));
	setAllowedToUnnest(savemovecontext);
	_variables = savevars;
	_equalities = saveequalities;
	return s;
}
