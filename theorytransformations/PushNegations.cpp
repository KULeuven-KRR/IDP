#include <vector>
#include <cassert>

#include "theorytransformations/PushNegations.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"

using namespace std;

Formula* PushNegations::visit(PredForm* pf) {
	if (isPos(pf->sign())) {
		return traverse(pf);
	}
	if (sametypeid<Predicate>(*(pf->symbol()))) {
		Predicate* p = dynamic_cast<Predicate*>(pf->symbol());
		if (p->type() != ST_NONE) {
			Predicate* newsymbol = NULL;
			switch (p->type()) {
			case ST_CT:
				newsymbol = pf->symbol()->derivedSymbol(ST_PF);
				break;
			case ST_CF:
				newsymbol = pf->symbol()->derivedSymbol(ST_PT);
				break;
			case ST_PT:
				newsymbol = pf->symbol()->derivedSymbol(ST_CF);
				break;
			case ST_PF:
				newsymbol = pf->symbol()->derivedSymbol(ST_CT);
				break;
			case ST_NONE:
				assert(false);
				break;
			}
			PredForm* newpf = new PredForm(SIGN::POS, newsymbol, pf->subterms(), pf->pi().clone());
			delete (pf);
			pf = newpf;
		}
	}
	return traverse(pf);
}

Formula* PushNegations::traverse(Formula* f) {
	for (auto it = f->subformulas().cbegin(); it != f->subformulas().cend(); ++it) {
		(*it)->accept(this);
	}
	for (auto it = f->subterms().cbegin(); it != f->subterms().cend(); ++it) {
		(*it)->accept(this);
	}
	return f;
}

Term* PushNegations::traverse(Term* t) {
	for (auto it = t->subterms().cbegin(); it != t->subterms().cend(); ++it) {
		(*it)->accept(this);
	}
	for (auto it = t->subsets().cbegin(); it != t->subsets().cend(); ++it) {
		(*it)->accept(this);
	}
	return t;
}

Formula* PushNegations::visit(EqChainForm* f) {
	if (isNeg(f->sign())) {
		f->negate();
		f->conj(!f->conj());
		for (size_t n = 0; n < f->comps().size(); ++n) {
			f->comp(n, negateComp(f->comps()[n]));
		}
	}
	return traverse(f);
}

Formula* PushNegations::visit(EquivForm* f) {
	if (isNeg(f->sign())) {
		f->negate();
		f->right()->negate();
	}
	return traverse(f);
}

Formula* PushNegations::visit(BoolForm* f) {
	if (isNeg(f->sign())) {
		f->negate();
		for (auto it = f->subformulas().cbegin(); it != f->subformulas().cend(); ++it)
			(*it)->negate();
		f->conj(!f->conj());
	}
	return traverse(f);
}

Formula* PushNegations::visit(QuantForm* f) {
	if (isNeg(f->sign())) {
		f->negate();
		f->subformula()->negate();
		f->quant(not f->quant());
	}
	return traverse(f);
}
