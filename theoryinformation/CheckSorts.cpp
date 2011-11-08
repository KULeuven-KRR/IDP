#include "theoryinformation/CheckSorts.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "error.hpp"

using namespace std;

CheckSorts::CheckSorts(Formula* f, Vocabulary* v) :
		_vocab(v) {
	f->accept(this);
}
CheckSorts::CheckSorts(Term* t, Vocabulary* v) :
		_vocab(v) {
	t->accept(this);
}
CheckSorts::CheckSorts(Definition* d, Vocabulary* v) :
		_vocab(v) {
	d->accept(this);
}
CheckSorts::CheckSorts(FixpDef* d, Vocabulary* v) :
		_vocab(v) {
	d->accept(this);
}

void CheckSorts::visit(const PredForm* pf) {
	PFSymbol* s = pf->symbol();
	vector<Sort*>::const_iterator it = s->sorts().cbegin();
	vector<Term*>::const_iterator jt = pf->subterms().cbegin();
	for (; it != s->sorts().cend(); ++it, ++jt) {
		Sort* s1 = *it;
		Sort* s2 = (*jt)->sort();
		if (s1 && s2) {
			if (!SortUtils::resolve(s1, s2, _vocab)) {
				Error::wrongsort((*jt)->toString(), s2->name(), s1->name(), (*jt)->pi());
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
				Error::wrongsort((*jt)->toString(), s2->name(), s1->name(), (*jt)->pi());
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
				Error::wrongsort((*it)->toString(), t->name(), s->name(), (*it)->pi());
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
			Error::wrongsort((*it)->toString(), (*it)->sort()->name(), "int or float", (*it)->pi());
		}
	}
	traverse(at);
}
