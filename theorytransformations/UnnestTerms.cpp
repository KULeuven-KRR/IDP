#include "common.hpp"
#include "theorytransformations/UnnestTerms.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "error.hpp"

using namespace std;

void UnnestTerms::contextProblem(Term* t) {
	if (t->pi().original()) {
		if (TermUtils::isPartial(t)) {
			Warning::ambigpartialterm(t->pi().original()->toString(), t->pi());
		}
	}
}

/**
 * Returns true is the term should be moved
 * (this is the most important method to overwrite in subclasses)
 */
bool UnnestTerms::shouldMove(Term* t) {
	Assert(t->type() != TT_VAR);
	return getAllowedToUnnest();
}

/**
 * Create a variable and an equation for the given term
 */
VarTerm* UnnestTerms::move(Term* term) {
	if (_context == Context::BOTH) contextProblem(term);

	Variable* introduced_var = new Variable(term->sort());

	VarTerm* introduced_subst_term = new VarTerm(introduced_var, TermParseInfo(term->pi()));
	VarTerm* introduced_eq_term = new VarTerm(introduced_var, TermParseInfo(term->pi()));
	vector<Term*> equality_args(2);
	equality_args[0] = introduced_eq_term;
	equality_args[1] = term;
	Predicate* equalpred = VocabularyUtils::equal(term->sort());
	PredForm* equalatom = new PredForm(SIGN::POS, equalpred, equality_args, FormulaParseInfo());

	_equalities.push_back(equalatom);
	_variables.insert(introduced_var);
	return introduced_subst_term;
}

/**
 * Rewrite the given formula with the current equalities
 */
Formula* UnnestTerms::rewrite(Formula* formula) {
	const FormulaParseInfo& origpi = formula->pi();
	bool univ_and_disj = false;
	if (_context == Context::POSITIVE) {
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
		formula = new QuantForm(SIGN::POS, univ_and_disj ? QUANT::UNIV : QUANT::EXIST, _variables, formula, origpi);
		_variables.clear();
	}
	return formula;
}

/**
 *	Visit all parts of the theory, assuming positive context for sentences
 */
Theory* UnnestTerms::visit(Theory* theory) {
	for (size_t n = 0; n < theory->sentences().size(); ++n) {
		_context = Context::POSITIVE;
		setAllowedToUnnest(false);
		theory->sentence(n, theory->sentences()[n]->accept(this));
	}
	for (auto it = theory->definitions().cbegin(); it != theory->definitions().cend(); ++it) {
		(*it)->accept(this);
	}
	for (auto it = theory->fixpdefs().cbegin(); it != theory->fixpdefs().cend(); ++it) {
		(*it)->accept(this);
	}
	return theory;
}

/**
 *	Visit a rule, assuming negative context for body.
 *	May move terms from rule head.
 */
Rule* UnnestTerms::visit(Rule* rule) {
// Visit head
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

		_equalities.push_back(rule->body());
		rule->body(new BoolForm(SIGN::POS, true, _equalities, FormulaParseInfo()));

		_equalities.clear();
		_variables.clear();
	}

// Visit body
	_context = Context::NEGATIVE;
	setAllowedToUnnest(false);
	rule->body(rule->body()->accept(this));
	return rule;
}

Formula* UnnestTerms::traverse(Formula* f) {
	Context savecontext = _context;
	bool savemovecontext = getAllowedToUnnest();
	if (isNeg(f->sign())) {
		_context = not _context;
	}
	for (size_t n = 0; n < f->subterms().size(); ++n) {
		f->subterm(n, f->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < f->subformulas().size(); ++n) {
		f->subformula(n, f->subformulas()[n]->accept(this));
	}
	_context = savecontext;
	setAllowedToUnnest(savemovecontext);
	return f;
}

Formula* UnnestTerms::traverse(PredForm* f) {
//TODO Very ugly static cast!! XXX This needs to be done differently!! FIXME
	return traverse(static_cast<Formula*>(f));
}

Formula* UnnestTerms::visit(EquivForm* ef) {
	Context savecontext = _context;
	_context = Context::BOTH;
	Formula* f = traverse(ef);
	_context = savecontext;
	return f;
}

Formula* UnnestTerms::visit(AggForm* af) {
	traverse(af);
	Formula* rewrittenformula = rewrite(af);
	if (rewrittenformula == af) {
		return af;
	} else {
		return rewrittenformula->accept(this);
	}
}

Formula* UnnestTerms::visit(EqChainForm* ef) {
	if (ef->comps().size() == 1) { // Rewrite to an normal atom
		SIGN atomsign = ef->sign();
		Sort* atomsort = SortUtils::resolve(ef->subterms()[0]->sort(), ef->subterms()[1]->sort(), _vocabulary);
		Predicate* comppred;
		switch (ef->comps()[0]) {
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
		vector<Term*> atomargs(2);
		atomargs[0] = ef->subterms()[0];
		atomargs[1] = ef->subterms()[1];
		PredForm* atom = new PredForm(atomsign, comppred, atomargs, ef->pi());
		return atom->accept(this);
	} else { // Simple recursive call
		bool savemovecontext = getAllowedToUnnest();
		setAllowedToUnnest(true);
		Formula* rewrittenformula = TheoryMutatingVisitor::traverse(ef);
		setAllowedToUnnest(savemovecontext);
		if (rewrittenformula == ef) {
			return ef;
		} else {
			return rewrittenformula->accept(this);
		}
	}
}

Formula* UnnestTerms::visit(PredForm* predform) {
	bool savemovecontext = getAllowedToUnnest();
// Special treatment for (in)equalities: possibly only one side needs to be moved
	bool moveonlyleft = false;
	bool moveonlyright = false;
	string symbolname = predform->symbol()->name();
	if (symbolname == "=/2" || symbolname == "</2" || symbolname == ">/2") {
		Term* leftterm = predform->subterms()[0];
		Term* rightterm = predform->subterms()[1];
		if (leftterm->type() == TT_AGG) {
			moveonlyright = true;
		} else if (rightterm->type() == TT_AGG) {
			moveonlyleft = true;
		} else if (symbolname == "=/2") {
			moveonlyright = (leftterm->type() != TT_VAR) && (rightterm->type() != TT_VAR);
		} else {
			setAllowedToUnnest(true);
		}
	} else {
		setAllowedToUnnest(true);
	}
// Traverse the atom
	if (moveonlyleft) {
		predform->subterm(1, predform->subterms()[1]->accept(this));
		setAllowedToUnnest(true);
		predform->subterm(0, predform->subterms()[0]->accept(this));
	} else if (moveonlyright) {
		predform->subterm(0, predform->subterms()[0]->accept(this));
		setAllowedToUnnest(true);
		predform->subterm(1, predform->subterms()[1]->accept(this));
	} else {
		traverse(predform);
	}
	setAllowedToUnnest(savemovecontext);

// Change the atom
	Formula* rewrittenformula = rewrite(predform);
	if (rewrittenformula == predform) {
		return predform;
	} else {
		return rewrittenformula->accept(this);
	}
}

Term* UnnestTerms::traverse(Term* term) {
	Context savecontext = _context;
	bool savemovecontext = getAllowedToUnnest();
	for (size_t n = 0; n < term->subterms().size(); ++n) {
		term->subterm(n, term->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < term->subsets().size(); ++n) {
		term->subset(n, term->subsets()[n]->accept(this));
	}
	_context = savecontext;
	setAllowedToUnnest(savemovecontext);
	return term;
}

VarTerm* UnnestTerms::visit(VarTerm* t) {
	return t;
}

Term* UnnestTerms::visit(DomainTerm* t) {
	if (getAllowedToUnnest() && shouldMove(t)) {
		return move(t);
	} else {
		return t;
	}
}

Term* UnnestTerms::visit(AggTerm* t) {
	if (getAllowedToUnnest() && shouldMove(t)) {
		return move(t);
	} else {
		return traverse(t);
	}
}

Term* UnnestTerms::visit(FuncTerm* ft) {
	bool savemovecontext = getAllowedToUnnest();
	setAllowedToUnnest(true);
	Term* result = traverse(ft);
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
	Context savecontext = _context;

	for (size_t n = 0; n < s->subterms().size(); ++n) {
		s->subterm(n, s->subterms()[n]->accept(this));
		if (not _equalities.empty()) {
			_equalities.push_back(s->subformulas()[n]);
			s->subformula(n, new BoolForm(SIGN::POS, true, _equalities, FormulaParseInfo()));
			savevars.insert(_variables.cbegin(), _variables.cend());
			_equalities.clear();
			_variables.clear();
		}
	}

	_context = Context::POSITIVE;
	setAllowedToUnnest(false);
	for (size_t n = 0; n < s->subformulas().size(); ++n) {
		s->subformula(n, s->subformulas()[n]->accept(this));
	}
	_context = savecontext;
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
	Context savecontext = _context;
	_context = Context::POSITIVE;

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
	_context = Context::POSITIVE;
	s->subformula(0, s->subformulas()[0]->accept(this));

	_variables = savevars;
	_equalities = saveequalities;
	_context = savecontext;
	setAllowedToUnnest(savemovecontext);
	return s;
}
