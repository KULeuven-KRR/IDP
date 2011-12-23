/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "common.hpp"
#include "theorytransformations/UnnestTerms.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "structure.hpp"
#include "error.hpp"
#include "utils/TheoryUtils.hpp"

#include <numeric> // for accumulate
#include <functional> // for multiplies
#include <algorithm> // for min_element and max_element

using namespace std;

UnnestTerms::UnnestTerms() 
	: _structure(NULL), _vocabulary(NULL), _context(Context::POSITIVE), 
		_allowedToUnnest(false), _chosenVarSort(NULL) {
}

void UnnestTerms::contextProblem(Term* t) {
	if (t->pi().original()) {
		if (TermUtils::isPartial(t)) {
			Warning::ambigpartialterm(toString(t->pi().original()), t->pi());
		}
	}
}

/**
 * Returns true is the term should be moved
 * (this is the most important method to overwrite in subclasses)
 */
bool UnnestTerms::shouldMove(Term* t) {
	Assert(t->type() != TT_VAR && t->type() != TT_DOM);
	return getAllowedToUnnest();
}
/**
 * Tries to derive a sort for the term given a structure.
 */
Sort* UnnestTerms::deriveSort(Term* term) {
	auto sort = (_chosenVarSort != NULL) ? _chosenVarSort : term->sort();
	if (_structure != NULL && SortUtils::isSubsort(term->sort(),VocabularyUtils::intsort(),_vocabulary)) {
		auto bounds = TermUtils::deriveTermBounds(term,_structure);
		if (bounds[0] != NULL && bounds[1] != NULL && bounds[0]->type() == DET_INT && bounds[1]->type() == DET_INT) {
			auto intmin = bounds[0]->value()._int;
			auto intmax = bounds[1]->value()._int;
			stringstream ss; ss << "_sort_" << intmin << '_' << intmax;
			sort = new Sort(ss.str(), new SortTable(new IntRangeInternalSortTable(intmin,intmax)));
			sort->addParent(VocabularyUtils::intsort());
		}
	}
	return sort;
}

/**
 * Create a variable and an equation for the given term
 */
VarTerm* UnnestTerms::move(Term* term) {
	if (getContext() == Context::BOTH) {
		contextProblem(term);
	}

	auto newsort = deriveSort(term);
	Assert(newsort != NULL);

	auto introduced_var = new Variable(newsort);
	_variables.insert(introduced_var);

	if (getOption(IntType::GROUNDVERBOSITY) > 1) {
		Warning::introducedvar(introduced_var->name(), introduced_var->sort()->name(), toString(term));
	}

	auto introduced_eq_term = new VarTerm(introduced_var, TermParseInfo(term->pi()));
	auto equalpred = VocabularyUtils::equal(term->sort());
	auto equalatom = new PredForm(SIGN::POS, equalpred, { introduced_eq_term, term }, FormulaParseInfo());
	_equalities.push_back(equalatom);

	auto introduced_subst_term = new VarTerm(introduced_var, TermParseInfo(term->pi()));
	return introduced_subst_term;
}

/**
 * Rewrite the given formula with the current equalities
 */
Formula* UnnestTerms::rewrite(Formula* formula) {
	const FormulaParseInfo& origpi = formula->pi();
	bool univ_and_disj = false;
	if (getContext() == Context::POSITIVE) {
		univ_and_disj = true;
		for (auto it = _equalities.cbegin(); it != _equalities.cend(); ++it) {
			(*it)->negate();
		}
	}
	if (not _equalities.empty()) {
		_equalities.push_back(formula);
		if (not _variables.empty()) {
			formula = new BoolForm(SIGN::POS, !univ_and_disj, _equalities, origpi);
		} else {
			formula = new BoolForm(SIGN::POS, !univ_and_disj, _equalities, FormulaParseInfo());
		}
		_equalities.clear();
	}
	if (not _variables.empty()) {
		formula = new QuantForm(SIGN::POS, (univ_and_disj ? QUANT::UNIV : QUANT::EXIST), _variables, formula, origpi);
		_variables.clear();
	}
	return formula;
}

template<typename T>
Formula* UnnestTerms::doRewrite(T origformula){
	auto rewrittenformula = rewrite(origformula);
	if (rewrittenformula == origformula) {
		return origformula;
	} else {
		return rewrittenformula->accept(this);
	}
}

/**
 *	Visit all parts of the theory, assuming positive context for sentences
 */
Theory* UnnestTerms::visit(Theory* theory) {
	for (auto it = theory->sentences().begin(); it != theory->sentences().end(); ++it) {
		setContext(Context::POSITIVE);
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

/**
 *	Visit a rule, assuming negative context for body.
 *	May move terms from rule head.
 */
Rule* UnnestTerms::visit(Rule* rule) {
// Visit head
	auto saveallowed = getAllowedToUnnest();
	setAllowedToUnnest(true);
	for (size_t termposition = 0; termposition < rule->head()->subterms().size(); ++termposition) {
		Term* term = rule->head()->subterms()[termposition];
		if (shouldMove(term)) {
			VarTerm* new_head_term = move(term);
			rule->head()->subterm(termposition, new_head_term);
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

// Visit body
	_context = Context::NEGATIVE;
	setAllowedToUnnest(false);
	rule->body(rule->body()->accept(this));
	setAllowedToUnnest(saveallowed);
	return rule;
}

Formula* UnnestTerms::traverse(Formula* f) {
	Context savecontext = _context;
	bool savemovecontext = getAllowedToUnnest();
	if (isNeg(f->sign())) {
		setContext(not _context);
	}
	for (size_t n = 0; n < f->subterms().size(); ++n) {
		f->subterm(n, f->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < f->subformulas().size(); ++n) {
		f->subformula(n, f->subformulas()[n]->accept(this));
	}
	setContext(savecontext);
	setAllowedToUnnest(savemovecontext);
	return f;
}

Formula* UnnestTerms::traverse(PredForm* f) {
//TODO Very ugly static cast!! XXX This needs to be done differently!! FIXME
	return traverse(static_cast<Formula*>(f));
}

Formula* UnnestTerms::visit(EquivForm* ef) {
	Context savecontext = getContext();
	setContext(Context::BOTH);
	auto newef = traverse(ef);
	setContext(savecontext);
	return doRewrite(newef);
}

Formula* UnnestTerms::visit(AggForm* af) {
	auto newaf = traverse(af);
	return doRewrite(newaf);
}

Formula* UnnestTerms::visit(EqChainForm* ecf) {
	if (ecf->comps().size() == 1) { // Rewrite to a normal atom
		SIGN atomsign = ecf->sign();
		Sort* atomsort = SortUtils::resolve(ecf->subterms()[0]->sort(), ecf->subterms()[1]->sort(), _vocabulary);
		Predicate* comppred;
		switch (ecf->comps()[0]) {
		case CompType::EQ:
			comppred = VocabularyUtils::equal(atomsort);
			break;
		case CompType::LT:
			comppred = VocabularyUtils::lessThan(atomsort);
			break;
		case CompType::GT:
			comppred = VocabularyUtils::greaterThan(atomsort);
			break;
		case CompType::NEQ:
			comppred = VocabularyUtils::equal(atomsort);
			atomsign = not atomsign;
			break;
		case CompType::LEQ:
			comppred = VocabularyUtils::greaterThan(atomsort);
			atomsign = not atomsign;
			break;
		case CompType::GEQ:
			comppred = VocabularyUtils::lessThan(atomsort);
			atomsign = not atomsign;
			break;
		}
		vector<Term*> atomargs = { ecf->subterms()[0], ecf->subterms()[1] };
		PredForm* atom = new PredForm(atomsign, comppred, atomargs, ecf->pi());
		return atom->accept(this);
	} else { // Simple recursive call
		bool savemovecontext = getAllowedToUnnest();
		setAllowedToUnnest(true);
		auto newecf = traverse(ecf);
		setAllowedToUnnest(savemovecontext);
		return doRewrite(newecf);
	}
}

Formula* UnnestTerms::visit(PredForm* predform) {
	bool savemovecontext = getAllowedToUnnest();
// Special treatment for (in)equalities: possibly only one side needs to be moved
	bool moveonlyleft = false;
	bool moveonlyright = false;
	if (VocabularyUtils::isComparisonPredicate(predform->symbol())) {
		auto leftterm = predform->subterms()[0];
		auto rightterm = predform->subterms()[1];
		if (leftterm->type() == TT_AGG) {
			moveonlyright = true;
		} else if (rightterm->type() == TT_AGG) {
			moveonlyleft = true;
		} else if (predform->symbol()->name() == "=/2") {
			moveonlyright = (leftterm->type() != TT_VAR) && (rightterm->type() != TT_VAR);
		} else {
			setAllowedToUnnest(true);
		}

		if (predform->symbol()->name() == "=/2") {
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
	return doRewrite(newf);
}

Term* UnnestTerms::traverse(Term* term) {
	Context savecontext = getContext();
	bool savemovecontext = getAllowedToUnnest();
	for (size_t n = 0; n < term->subterms().size(); ++n) {
		term->subterm(n, term->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < term->subsets().size(); ++n) {
		term->subset(n, term->subsets()[n]->accept(this));
	}
	setContext(savecontext);
	setAllowedToUnnest(savemovecontext);
	return term;
}

VarTerm* UnnestTerms::visit(VarTerm* t) {
	return t;
}

Term* UnnestTerms::visit(DomainTerm* t) {
	return t;
}

Term* UnnestTerms::visit(AggTerm* t) {
	//FIXME shouldn't this term be traversed before it is (possibly) moved?
	if (getAllowedToUnnest() && shouldMove(t)) {
		return traverse(move(t)); //TODO Check whether this is correct: traverse after move...
	} else {
		return traverse(t);
	}
}

Term* UnnestTerms::visit(FuncTerm* ft) {
	bool savemovecontext = getAllowedToUnnest();
	setAllowedToUnnest(true);
	auto result = traverse(ft);
	setAllowedToUnnest(savemovecontext);
	if (getAllowedToUnnest() && shouldMove(result)) {
		return move(result);
	} else {
		return result;
	}
}

SetExpr* UnnestTerms::visit(EnumSetExpr* s) {
	vector<Formula*> saveequalities = _equalities;
	_equalities.clear();
	set<Variable*> savevars = _variables;
	_variables.clear();
	bool savemovecontext = getAllowedToUnnest();
	setAllowedToUnnest(true);
	Context savecontext = getContext();

	for (size_t n = 0; n < s->subterms().size(); ++n) {
		s->subterm(n, s->subterms()[n]->accept(this));
		if (not _equalities.empty()) {
			//_equalities.push_back(s->subformulas()[n]);
			//s->subformula(n, new BoolForm(SIGN::POS, true, _equalities, FormulaParseInfo()));
			savevars.insert(_variables.cbegin(), _variables.cend());
			saveequalities.insert(saveequalities.end(),_equalities.cbegin(), _equalities.cend());
			_equalities.clear();
			_variables.clear();
		}
	}

	setContext(Context::POSITIVE);
	setAllowedToUnnest(false);
	for (size_t n = 0; n < s->subformulas().size(); ++n) {
		s->subformula(n, s->subformulas()[n]->accept(this));
	}
	setContext(savecontext);
	setAllowedToUnnest(savemovecontext);
	_variables = savevars;
	_equalities = saveequalities;
	return s;
}

SetExpr* UnnestTerms::visit(QuantSetExpr* s) {
	vector<Formula*> saveequalities = _equalities;
	_equalities.clear();
	set<Variable*> savevars = _variables;
	_variables.clear();
	bool savemovecontext = getAllowedToUnnest();
	setAllowedToUnnest(true);
	Context savecontext = getContext();
	setContext(Context::POSITIVE);

	s->subterm(0, s->subterms()[0]->accept(this));
	if (not _equalities.empty()) {
		_equalities.push_back(s->subformulas()[0]);
		BoolForm* bf = new BoolForm(SIGN::POS, true, _equalities, FormulaParseInfo());
		s->subformula(0, bf);
		for (auto it = _variables.cbegin(); it != _variables.cend(); ++it) {
			s->addQuantVar(*it);
		}
		_equalities.clear();
		_variables.clear();
	}

	setAllowedToUnnest(false);
	setContext(Context::POSITIVE);
	s->subformula(0, s->subformulas()[0]->accept(this));
	setContext(savecontext);
	setAllowedToUnnest(savemovecontext);
	_variables = savevars;
	_equalities = saveequalities;
	return s;
}
