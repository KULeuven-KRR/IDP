/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "theorytransformations/DeriveSorts.hpp"

#include "common.hpp"
#include "GlobalData.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "error.hpp"

using namespace std;

void DeriveSorts::checkVars(const set<Variable*>& quantvars) {
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
	return traverse(qf);
}

Rule* DeriveSorts::visit(Rule* r) {
	Assert(_assertsort == NULL);
	checkVars(r->quantVars());
	r->head()->accept(this);
	r->body()->accept(this);
	return r;
}

SetExpr* DeriveSorts::visit(QuantSetExpr* qs) {
	Assert(_assertsort == NULL);
	checkVars(qs->quantVars());
	return traverse(qs);
}

Term* DeriveSorts::visit(VarTerm* vt) {
	if (_underivable) {
		_underivableVariables.insert(vt->var());
		return vt;
	}
	if (_assertsort != NULL && _untypedvariables.find(vt->var()) != _untypedvariables.end()) {
		Sort* newsort = NULL;
		if (vt->sort() == NULL) {
			newsort = _assertsort;
		} else {
			newsort = SortUtils::resolve(vt->sort(), _assertsort);
		}
		if (newsort == NULL) {
			_underivableVariables.insert(vt->var());
		} else if(vt->sort()!=newsort){
			_changed = true;
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
	if (_assertsort != NULL) {
		Assert(SortUtils::resolve(_assertsort, VocabularyUtils::intsort())!=NULL);
	}
	_assertsort = NULL;
	return TheoryMutatingVisitor::visit(t);
}

Term* DeriveSorts::visit(FuncTerm* term) {
	auto f = term->function();

	if (_assertsort != NULL && term->sort() != NULL) {
		Assert(SortUtils::resolve(_assertsort, term->sort())!=NULL);
	}
	_assertsort = NULL;

	auto origunderivable = _underivable;

	if (f->overloaded()) {
		_overloadedterms.insert(term);
		if (not f->builtin()) {
			_underivable = true;
		}
	}

	// Currently, overloading is resolved iteratively, so it can be that a builtin goes from overloaded to set, but the arguments of the builtins, eg +/2:, are set too broadly (int+int:int)
	// TODO review if more builtins are added?
	if (f->builtin()) {
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

	auto origunderivable = _underivable;
	if (p->overloaded()) {
		_overloadedatoms.insert(f);
		if (not p->builtin()) {
			_underivable = true;
		}
	}

	if (p->builtin()) {
		Sort* temp = NULL;
		if (not _firstvisit && p->name() == "=/2") {
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
	return formula;
}

void DeriveSorts::derivesorts() {

}

void DeriveSorts::derivefuncs() {
	for (auto it = _overloadedterms.begin(); it != _overloadedterms.end();) {
		auto jt = it;
		++jt;
		auto f = (*it)->function();
		vector<Sort*> vs;
		for (auto kt = (*it)->subterms().cbegin(); kt != (*it)->subterms().cend(); ++kt) {
			vs.push_back((*kt)->sort());
		}
		vs.push_back(NULL);

		//cerr <<"Disambiguating for " <<f->name() <<" with " <<toString(vs) <<"\n";

		auto rf = f->disambiguate(vs, _vocab);
		if (rf != NULL) {
			(*it)->function(rf);
			if (not rf->overloaded()) {
				_overloadedterms.erase(it);
			}
			_changed = true;
		}
		it = jt;
	}
}

void DeriveSorts::derivepreds() {
	for (auto it = _overloadedatoms.cbegin(); it != _overloadedatoms.cend();) {
		auto jt = it;
		++jt;
		auto p = (*it)->symbol();
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
		it = jt;
	}
}

template<>
void DeriveSorts::execute(Rule* r, Vocabulary* v) {
	_assertsort = NULL;
	_vocab = v;
	// Set the sort of the terms in the head
	auto jt = r->head()->symbol()->sorts().cbegin();
	for (unsigned int n = 0; n < r->head()->subterms().size(); ++n, ++jt) {
		auto hs = r->head()->subterms()[n]->sort();
		if (hs == NULL) {
			r->head()->subterms()[n]->sort(*jt);
			continue;
		}
		if (SortUtils::isSubsort(hs, *jt)) {
			continue;
		}

		auto nv = new Variable(*jt);
		auto nvt1 = new VarTerm(nv, TermParseInfo());
		auto nvt2 = new VarTerm(nv, TermParseInfo());
		auto ecf = new EqChainForm(SIGN::POS, true, nvt1, FormulaParseInfo());
		ecf->add(CompType::EQ, r->head()->subterms()[n]);
		auto bf = new BoolForm(SIGN::POS, true, r->body(), ecf, FormulaParseInfo());
		r->body(bf);
		r->head()->arg(n, nvt2);
		r->addvar(nv);
	}
	deriveSorts(r);
}

void DeriveSorts::check() {
	for (auto i = _untypedvariables.cbegin(); i != _untypedvariables.cend(); ++i) {
		if ((*i)->sort() == NULL) {
			Error::novarsort((*i)->name(), (*i)->pi());
		} else if (getOption(BoolType::SHOWWARNINGS)) {
			Warning::derivevarsort((*i)->name(), (*i)->sort()->name(), (*i)->pi());
		}
	}
	for (auto i = _underivableVariables.cbegin(); i != _underivableVariables.cend(); ++i) {
		Error::novarsort((*i)->name(), (*i)->pi());
	}
	for (auto it = _overloadedatoms.cbegin(); it != _overloadedatoms.cend(); ++it) {
		if (typeid(*((*it)->symbol())) == typeid(Predicate))
			Error::nopredsort((*it)->symbol()->name(), (*it)->pi());
		else
			Error::nofuncsort((*it)->symbol()->name(), (*it)->pi());
	}
	for (auto it = _overloadedterms.cbegin(); it != _overloadedterms.cend(); ++it) {
		Error::nofuncsort((*it)->function()->name(), (*it)->pi());
	}
	for (auto it = _domelements.cbegin(); it != _domelements.cend(); ++it) {
		Error::nodomsort(toString(*it), (*it)->pi());
	}
}
