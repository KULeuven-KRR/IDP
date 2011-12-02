#include "common.hpp"
#include "theorytransformations/DeriveSorts.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "error.hpp"

using namespace std;

Formula* DeriveSorts::visit(QuantForm* qf) {
	if (_firstvisit) {
		for (auto it = qf->quantVars().cbegin(); it != qf->quantVars().cend(); ++it) {
			if (not (*it)->sort()) {
				_untyped[*it] = set<Sort*>();
				_changed = true;
			}
		}
	}
	return traverse(qf);
}

Formula* DeriveSorts::visit(PredForm* pf) {
	PFSymbol* p = pf->symbol();

	// At first visit, insert the atoms over overloaded predicates
	if (_firstvisit && p->overloaded()) {
		_overloadedatoms.insert(pf);
		_changed = true;
	}

	// Visit the children while asserting the sorts of the predicate
	vector<Sort*>::const_iterator it = p->sorts().cbegin();
	vector<Term*>::const_iterator jt = pf->subterms().cbegin();
	for (; it != p->sorts().cend(); ++it, ++jt) {
		_assertsort = *it;
		(*jt)->accept(this);
	}
	return pf;
}

Formula* DeriveSorts::visit(EqChainForm* ef) {
	Sort* s = 0;
	if (not _firstvisit) {
		for (auto it = ef->subterms().cbegin(); it != ef->subterms().cend(); ++it) {
			Sort* temp = (*it)->sort();
			if (temp && temp->parents().empty() && temp->children().empty()) {
				s = temp;
				break;
			}
		}
	}
	for (auto it = ef->subterms().cbegin(); it != ef->subterms().cend(); ++it) {
		_assertsort = s;
		(*it)->accept(this);
	}
	return ef;
}

Rule* DeriveSorts::visit(Rule* r) {
	if (_firstvisit) {
		for (auto it = r->quantVars().cbegin(); it != r->quantVars().cend(); ++it) {
			if (not (*it)->sort()) {
				_untyped[*it] = set<Sort*>();
			}
			_changed = true;
		}
	}
	r->head()->accept(this);
	r->body()->accept(this);
	return r;
}

Term* DeriveSorts::visit(VarTerm* vt) {
	if (not vt->sort() && _assertsort) {
		_untyped[vt->var()].insert(_assertsort);
	}
	return vt;
}

Term* DeriveSorts::visit(DomainTerm* dt) {
	if (_firstvisit && (!(dt->sort()))) {
		_domelements.insert(dt);
	}

	if (not dt->sort() && _assertsort) {
		dt->sort(_assertsort);
		_changed = true;
		_domelements.erase(dt);
	}
	return dt;
}

Term* DeriveSorts::visit(FuncTerm* ft) {
	Function* f = ft->function();

	// At first visit, insert the terms over overloaded functions
	if (f->overloaded()) {
		if (_firstvisit || _overloadedterms.find(ft) == _overloadedterms.cend() || _assertsort != _overloadedterms[ft]) {
			_changed = true;
			_overloadedterms[ft] = _assertsort;
		}
	}

	// Visit the children while asserting the sorts of the predicate
	vector<Sort*>::const_iterator it = f->insorts().cbegin();
	vector<Term*>::const_iterator jt = ft->subterms().cbegin();
	for (; it != f->insorts().cend(); ++it, ++jt) {
		_assertsort = *it;
		(*jt)->accept(this);
	}
	return ft;
}

SetExpr* DeriveSorts::visit(QuantSetExpr* qs) {
	if (_firstvisit) {
		for (auto it = qs->quantVars().cbegin(); it != qs->quantVars().cend(); ++it) {
			if (not (*it)->sort()) {
				_untyped[*it] = set<Sort*>();
				_changed = true;
			}
		}
	}
	return traverse(qs);
}

void DeriveSorts::derivesorts() {
	for (auto it = _untyped.begin(); it != _untyped.end();) {
		map<Variable*, std::set<Sort*> >::iterator jt = it;
		++jt;
		if (not (*it).second.empty()) {
			std::set<Sort*>::iterator kt = (*it).second.cbegin();
			Sort* s = *kt;
			++kt;
			for (; kt != (*it).second.cend(); ++kt) {
				s = SortUtils::resolve(s, *kt, _vocab);
				if (not s) { // In case of conflicting sorts, assign the first sort.
							 // Error message will be given during final check.
					s = *((*it).second.cbegin());
					break;
				}
			}Assert(s);
			if ((*it).second.size() > 1 || s->builtin()) { // Warning when the sort was resolved or builtin
				Warning::derivevarsort(it->first->name(), s->name(), (*it).first->pi());
			}
			(*it).first->sort(s);
			_untyped.erase(it);
			_changed = true;
		}
		it = jt;
	}
}

void DeriveSorts::derivefuncs() {
	for (auto it = _overloadedterms.begin(); it != _overloadedterms.end();) {
		map<FuncTerm*, Sort*>::iterator jt = it;
		++jt;
		Function* f = it->first->function();
		vector<Sort*> vs;
		for (auto kt = it->first->subterms().cbegin(); kt != it->first->subterms().cend(); ++kt) {
			vs.push_back((*kt)->sort());
		}
		vs.push_back(it->second);
		Function* rf = f->disambiguate(vs, _vocab);
		if (rf) {
			it->first->function(rf);
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
		std::set<PredForm*>::iterator jt = it;
		++jt;
		PFSymbol* p = (*it)->symbol();
		vector<Sort*> vs;
		for (auto kt = (*it)->subterms().cbegin(); kt != (*it)->subterms().cend(); ++kt) {
			vs.push_back((*kt)->sort());
		}
		PFSymbol* rp = p->disambiguate(vs, _vocab);
		if (rp) {
			(*it)->symbol(rp);
			if (not rp->overloaded()) {
				_overloadedatoms.erase(it);
			}
			_changed = true;
		}
		it = jt;
	}
}

void DeriveSorts::deriveSorts(Formula* f) {
	_changed = false;
	_firstvisit = true;
	f->accept(this); // First visit: collect untyped symbols, set types of variables that occur in typed positions.
	_firstvisit = false;
	while (_changed) {
		_changed = false;
		derivesorts();
		derivefuncs();
		derivepreds();
		f->accept(this); // Next visit: type derivation over overloaded predicates or functions.
	}
	check();
}

void DeriveSorts::deriveSorts(Term* t) {
	_changed = false;
	_firstvisit = true;
	t->accept(this);
	_firstvisit = false;
	while (_changed) {
		_changed = false;
		derivesorts();
		derivefuncs();
		derivepreds();
		t->accept(this);
	}
	check();
}

void DeriveSorts::deriveSorts(Rule* r) {
	// Set the sort of the terms in the head
	vector<Sort*>::const_iterator jt = r->head()->symbol()->sorts().cbegin();
	for (unsigned int n = 0; n < r->head()->subterms().size(); ++n, ++jt) {
		Sort* hs = r->head()->subterms()[n]->sort();
		if (hs) {
			if (!SortUtils::isSubsort(hs, *jt)) {
				Variable* nv = new Variable(*jt);
				VarTerm* nvt1 = new VarTerm(nv, TermParseInfo());
				VarTerm* nvt2 = new VarTerm(nv, TermParseInfo());
				EqChainForm* ecf = new EqChainForm(SIGN::POS, true, nvt1, FormulaParseInfo());
				ecf->add(CompType::EQ, r->head()->subterms()[n]);
				BoolForm* bf = new BoolForm(SIGN::POS, true, r->body(), ecf, FormulaParseInfo());
				r->body(bf);
				r->head()->arg(n, nvt2);
				r->addvar(nv);
			}
		} else {
			r->head()->subterms()[n]->sort(*jt);
		}
	}
	// Rest of the algorithm
	_changed = false;
	_firstvisit = true;
	r->accept(this);
	_firstvisit = false;
	while (_changed) {
		_changed = false;
		derivesorts();
		derivefuncs();
		derivepreds();
		r->accept(this);
	}
	check();
}

void DeriveSorts::check() {
	for (auto it = _untyped.cbegin(); it != _untyped.cend(); ++it) {
		Assert((it->second).empty());
		Error::novarsort(it->first->name(), it->first->pi());
	}
	for (auto it = _overloadedatoms.cbegin(); it != _overloadedatoms.cend(); ++it) {
		if (typeid(*((*it)->symbol())) == typeid(Predicate))
			Error::nopredsort((*it)->symbol()->name(), (*it)->pi());
		else
			Error::nofuncsort((*it)->symbol()->name(), (*it)->pi());
	}
	for (auto it = _overloadedterms.cbegin(); it != _overloadedterms.cend(); ++it) {
		Error::nofuncsort(it->first->function()->name(), it->first->pi());
	}
	for (auto it = _domelements.cbegin(); it != _domelements.cend(); ++it) {
		Error::nodomsort((*it)->toString(), (*it)->pi());
	}
}
