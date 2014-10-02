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

#include "DeriveSorts.hpp"

#include "IncludeComponents.hpp"
#include "GlobalData.hpp"

#include "errorhandling/error.hpp"

using namespace std;

void DeriveSorts::checkVars(const varset& quantvars) {
	if (_firstvisit) {
		for (auto it = quantvars.cbegin(); it != quantvars.cend(); ++it) {
			if ((*it)->sort() == NULL) {
				_untypedvariables.insert(*it);
				_changed = true;
			}
		}
	}
}

Formula* DeriveSorts::visit(QuantForm* qf) {
	Assert(_assertsort == NULL);
	checkVars(qf->quantVars());
	checkVars(qf->freeVars());
	return traverse(qf);
}

Rule* DeriveSorts::visit(Rule* r) {
	Assert(_assertsort == NULL);
	checkVars(r->quantVars());
	r->head()->accept(this);
	r->body()->accept(this);
	return r;
}

QuantSetExpr* DeriveSorts::visit(QuantSetExpr* qs) {
	Assert(_assertsort == NULL);
	checkVars(qs->freeVars());
	checkVars(qs->quantVars());
	return traverse(qs);
}

Term* DeriveSorts::visit(VarTerm* vt) {
	if (_underivable) {
		_underivableVariables.insert(vt->var());
		return vt;
	}
	if (_assertsort != NULL && _untypedvariables.find(vt->var()) != _untypedvariables.cend()) {
		Sort* newsort = NULL;
		if (vt->sort() == NULL) {
			newsort = _assertsort;
		} else {
			newsort = SortUtils::resolve(vt->sort(), _assertsort);
		}
		if (newsort == NULL) {
			_underivableVariables.insert(vt->var());
		} else if (vt->sort() != newsort) {
			_changed = true;
			if (vt->var()->sort() != newsort) {
				if (_assertsort != NULL) {
					derivations[vt->var()].insert(_assertsort);
				}
				if (vt->sort() != NULL) {
					derivations[vt->var()].insert(vt->sort());
				}
			}
			vt->sort(newsort);
		}
	}
	return vt;
}

Term* DeriveSorts::visit(DomainTerm* dt) {
	if (_firstvisit && dt->sort() == NULL) {
		_domelements.insert(dt);
	}

	if (dt->sort() == NULL && _assertsort != NULL) {
		dt->sort(_assertsort);
		_changed = true;
		_domelements.erase(dt);
	}
	return dt;
}

Term* DeriveSorts::visit(AggTerm* t) {
	if (_assertsort != NULL && SortUtils::resolve(_assertsort, get(STDSORT::INTSORT)) == NULL) {
		_underivable = true;
		return t;
	}
	_assertsort = NULL;
	return TheoryMutatingVisitor::visit(t);
}

Term* DeriveSorts::visit(FuncTerm* term) {
	auto f = term->function();
	if (not _useBuiltIns && (f->builtin() && not f->isConstructorFunction())) {
		return term;
	}
	if (_assertsort != NULL && term->sort() != NULL && SortUtils::resolve(_assertsort, term->sort()) == NULL) {
		_underivable = true;
		return term;
	}

	auto origunderivable = _underivable;

	if (f->overloaded()) {
		_overloadedterms.insert({term,_assertsort});
		if (not f->builtin()) {
			_underivable = true;
		}
	}
	_assertsort = NULL;


	// Currently, overloading is resolved iteratively, so it can be that a builtin goes from overloaded to set, but the arguments of the builtins, eg +/2:, are set too broadly (int+int:int)
	// TODO review if more builtins are added?
	if (_useBuiltIns && (f->builtin() && not f->isConstructorFunction())) {
		for (auto i = term->subterms().cbegin(); i != term->subterms().cend(); ++i) {
			(*i)->accept(this);
			_assertsort = NULL;
		}
		return term;
	}

	auto it = f->insorts().cbegin();
	auto jt = term->subterms().cbegin();
	for (; it != f->insorts().cend(); ++it, ++jt) {
		_assertsort = *it;
		(*jt)->accept(this);
		_assertsort = NULL;
	}
	_underivable = origunderivable;
	return term;
}

Formula* DeriveSorts::visit(PredForm* f) {
	auto p = f->symbol();
	if (not _useBuiltIns && p->builtin()) {
		return f;
	}
	auto origunderivable = _underivable;
	if (p->overloaded()) {
		_overloadedatoms.insert(f);
		if (not p->builtin()) {
			_underivable = true;
		}
	}

	if (p->builtin()) {
		Sort* temp = NULL;
		if (not _firstvisit && is(p, STDPRED::EQ)) {
			for (auto i = f->subterms().cbegin(); i != f->subterms().cend(); ++i) {
				if ((*i)->sort() != NULL) {
					if (temp == NULL) {
						temp = (*i)->sort();
					} else {
						temp = SortUtils::resolve(temp, (*i)->sort());
					}
				}
			}
		}
		for (auto i = f->subterms().cbegin(); i != f->subterms().cend(); ++i) {
			_assertsort = temp;
			(*i)->accept(this);
			_assertsort = NULL;
		}
	} else {
		auto it = p->sorts().cbegin();
		auto jt = f->subterms().cbegin();
		for (; it != p->sorts().cend(); ++it, ++jt) {
			_assertsort = *it;
			(*jt)->accept(this);
			_assertsort = NULL;
		}
	}

	_underivable = origunderivable;
	return f;
}

Formula* DeriveSorts::visit(EqChainForm* formula) {
	if (_useBuiltIns) {
		Sort* temp = NULL;
		if (not _firstvisit) {
			for (auto i = formula->subterms().cbegin(); i != formula->subterms().cend(); ++i) {
				if ((*i)->sort() != NULL) {
					if (temp == NULL) {
						temp = (*i)->sort();
					} else {
						temp = SortUtils::resolve(temp, (*i)->sort());
					}
				}
			}
		}
		for (auto i = formula->subterms().cbegin(); i != formula->subterms().cend(); ++i) {
			_assertsort = temp;
			(*i)->accept(this);
			_assertsort = NULL;
		}
	}
	return formula;
}

void DeriveSorts::derivefuncs() {

	for (auto it = _overloadedterms.begin(); it != _overloadedterms.end();) {
		auto jt = it;
		++jt;
		auto term = (*it).first;
		auto outsort = (*it).second;
		auto f = term->function();
		if (not f->builtin() || _useBuiltIns) {
			vector<Sort*> vs;
			for (auto kt = term->subterms().cbegin(); kt != term->subterms().cend(); ++kt) {
				vs.push_back((*kt)->sort());
			}
			vs.push_back(outsort);

			auto rf = f->disambiguate(vs, _vocab);
			if (rf != NULL) {
				term->function(rf);
				if (not rf->overloaded()) {
					_overloadedterms.erase(it);
				}
				_changed = true;
			}
		}
		it = jt;
	}
}

void DeriveSorts::derivepreds() {
	for (auto it = _overloadedatoms.cbegin(); it != _overloadedatoms.cend();) {
		auto jt = it;
		++jt;
		auto p = (*it)->symbol();
		if (not p->builtin() || _useBuiltIns) {
			vector<Sort*> vs;
			for (auto kt = (*it)->subterms().cbegin(); kt != (*it)->subterms().cend(); ++kt) {
				vs.push_back((*kt)->sort());
			}
			auto rp = p->disambiguate(vs, _vocab);
			if (rp != NULL) {
				(*it)->symbol(rp);
				if (not rp->overloaded()) {
					_overloadedatoms.erase(it);
				}
				_changed = true;
			}
		}
		it = jt;
	}
}

template<>
void DeriveSorts::execute(Rule* r, Vocabulary* v, bool useBuiltins) {
	_useBuiltIns = useBuiltins;
	_assertsort = NULL;
	_vocab = v;
	auto& head = *(r->head());
	// Set the sort of the terms in the head
	auto jt = head.symbol()->sorts().cbegin();
	for (size_t n = 0; n < head.subterms().size(); ++n, ++jt) {
		auto subterm = head.subterms()[n];
		auto hs = subterm->sort();
		if (hs == NULL) {
			subterm->sort(*jt);
			auto varterm = dynamic_cast<VarTerm*>(subterm);
			if (varterm != NULL) {
				_untypedvariables.insert(varterm->var());
			}
			continue;
		}
		if (SortUtils::isSubsort(hs, *jt)) {
			continue;
		}

		// Make sure the terms in the head are type safe.
		Assert(hs != NULL);
		// If domain element, we certainly do not need to add an additional variable, we only need a type check.
		if (subterm->type() == TermType::DOM) {
			// If the subterm is a domainterm, then add a sort check to the body of the rule.
			if (not useBuiltins) {
				// Note: Everytime this method is called, a sort check will be added to the body!
				// Typically it is called twice (with and without using builtins), so we do it in only one of these cases.
				// Note: there is a special case in which the type of the predicate is not yet derived. If this is the case, we throw an exception.
				if (*jt == NULL) {
					Error::nopredsort(head.symbol()->name(), head.pi());
				} else {
					auto sortpred = (*jt)->pred();
					auto pf = new PredForm(SIGN::POS, sortpred, { subterm->clone() }, FormulaParseInfo());
					auto bf = new BoolForm(SIGN::POS, true, r->body(), pf, FormulaParseInfo());
					r->body(bf);
				}
			}
		} else {
			// For functions or aggregates, we expect the cost of duplication to be too high in general, so we add the variable instead.
			// If it can be in fact evaluated, the variable will hopefully only ever get assigned one value.

			// For any other type of term, introduce a variable and move the subterm to the body.
			auto nv = new Variable(*jt);
			auto nvt1 = new VarTerm(nv, TermParseInfo());
			auto nvt2 = new VarTerm(nv, TermParseInfo());
			auto ecf = new EqChainForm(SIGN::POS, true, nvt1, FormulaParseInfo());
			ecf->add(CompType::EQ, subterm);
			auto bf = new BoolForm(SIGN::POS, true, r->body(), ecf, FormulaParseInfo());
			r->body(bf);
			head.arg(n, nvt2);
			r->addvar(nv);
		}
	}
	deriveSorts(r);
}

void DeriveSorts::check() {
	for (auto var2sort : derivations) {
		if (var2sort.second.size() > 1) {
			stringstream ss;
			auto var = var2sort.first;
			auto sorts = var2sort.second;
			ss << "Derived sort " << var->sort()->name() << " for variable " << var->name() << " as nearest parent of ";
			printList(ss, sorts, ", ", false);
			ss << " and " << (*sorts.rbegin())->name() << ".";
			Warning::warning(ss.str(), var->pi());
		}
	}
	derivations.clear();
	for (auto i = _underivableVariables.cbegin(); i != _underivableVariables.cend(); ++i) {
		Error::novarsort((*i)->name(), (*i)->pi());
	}
	if (not _useBuiltIns) {
		return;
	}
	for (auto i = _untypedvariables.cbegin(); i != _untypedvariables.cend(); ++i) {
		if ((*i)->sort() == NULL) {
			Error::novarsort((*i)->name(), (*i)->pi());
		} else if (getOption(BoolType::SHOWWARNINGS)) {
			if (_useBuiltIns) { // NOTE this is used to prevent too many warnings in places were it is quite(!) unambiguous, as all positions it occurs in have the same sort.
				Warning::derivevarsort((*i)->name(), (*i)->sort()->name(), (*i)->pi());
			}
		}
	}
	for (auto it = _overloadedatoms.cbegin(); it != _overloadedatoms.cend(); ++it) {
		if ((*it)->symbol()->isPredicate()) {
			Error::nopredsort((*it)->symbol()->name(), (*it)->pi());
		} else {
			Error::nofuncsort((*it)->symbol()->name(), (*it)->pi());
		}
	}
	for (auto it = _overloadedterms.cbegin(); it != _overloadedterms.cend(); ++it) {
		Error::nofuncsort((*it).first->function()->name(), (*it).first->pi());
	}
	for (auto it = _domelements.cbegin(); it != _domelements.cend(); ++it) {
		Error::nodomsort(toString(*it), (*it)->pi());
	}
}
