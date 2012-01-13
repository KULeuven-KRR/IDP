/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "theoryinformation/CheckSorts.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "error.hpp"

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
	return SortUtils::resolve(sort, VocabularyUtils::floatsort(), voc);
}

void CheckSorts::visit(const AggTerm* at) {
	auto subterms = at->set()->subterms();
	for (auto it = subterms.cbegin(); it != subterms.cend(); ++it) {
		if ((*it)->sort() != NULL && !isNumeric((*it)->sort(), _vocab)) {
			Error::wrongsort(toString(*it), (*it)->sort()->name(), "int or float", (*it)->pi());
		}
	}
	traverse(at);
}
