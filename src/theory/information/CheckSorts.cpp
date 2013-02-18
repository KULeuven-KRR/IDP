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

#include "CheckSorts.hpp"

#include "IncludeComponents.hpp"
#include "errorhandling/error.hpp"

using namespace std;

void CheckSorts::visit(const PredForm* pf) {
	PFSymbol* s = pf->symbol();
	vector<Sort*>::const_iterator it = s->sorts().cbegin();
	vector<Term*>::const_iterator jt = pf->subterms().cbegin();
	for (; it != s->sorts().cend(); ++it, ++jt) {
		Sort* s1 = *it;
		Sort* s2 = (*jt)->sort();
		if (s1 && s2) {
			if (!SortUtils::resolve(s1, s2, _vocab)) {
				Error::wrongsort(toString(*jt), s2->name(), s1->name(), (*jt)->pi());
			}
		}
	}
	traverse(pf);
}

void CheckSorts::visit(const FuncTerm* ft) {
	Function* f = ft->function();
	vector<Sort*>::const_iterator it = f->insorts().cbegin();
	vector<Term*>::const_iterator jt = ft->subterms().cbegin();
	for (; it != f->insorts().cend(); ++it, ++jt) {
		Sort* s1 = *it;
		Sort* s2 = (*jt)->sort();
		if (s1 && s2) {
			if (!SortUtils::resolve(s1, s2, _vocab)) {
				Error::wrongsort(toString(*jt), s2->name(), s1->name(), (*jt)->pi());
			}
		}
	}
	traverse(ft);
}

void CheckSorts::visit(const EqChainForm* ef) {
	Sort* s = 0;
	vector<Term*>::const_iterator it = ef->subterms().cbegin();
	while (!s && it != ef->subterms().cend()) {
		s = (*it)->sort();
		++it;
	}
	for (; it != ef->subterms().cend(); ++it) {
		Sort* t = (*it)->sort();
		if (t) {
			if (!SortUtils::resolve(s, t, _vocab)) {
				Error::wrongsort(toString(*it), t->name(), s->name(), (*it)->pi());
			}
		}
	}
	traverse(ef);
}

bool isNumeric(Sort* sort, Vocabulary* voc) {
	return SortUtils::resolve(sort, get(STDSORT::FLOATSORT), voc);
}

void CheckSorts::visit(const AggTerm* at) {
	for (auto i = at->set()->getSets().cbegin(); i < at->set()->getSets().cend(); ++i) {
		auto term = (*i)->getTerm();
		auto sort = term->sort();
		if (sort != NULL && !isNumeric(sort, _vocab)) {
			Error::wrongsort(toString(term), sort->name(), "int or float", term->pi());
		}
	}
	traverse(at);
}
