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
#include "theorytransformations/PushNegations.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"

using namespace std;

// DELETES original predform!
PredForm* getDerivedPredForm(PredForm* pf, SymbolType t){
	Assert(t!=ST_NONE);
	auto newsymbol = pf->symbol()->derivedSymbol(t);
	PredForm* newpf = new PredForm(SIGN::POS, newsymbol, pf->subterms(), pf->pi().clone());
	delete (pf);
	return newpf;
}

Formula* PushNegations::visit(PredForm* pf) {
	if (isPos(pf->sign())) {
		return traverse(pf);
	}
	if (sametypeid<Predicate>(*(pf->symbol()))) {
		auto p = dynamic_cast<Predicate*>(pf->symbol());
		switch (p->type()) {
		case ST_CT:
			pf = getDerivedPredForm(pf, ST_PF);
			break;
		case ST_CF:
			pf = getDerivedPredForm(pf, ST_PT);
			break;
		case ST_PT:
			pf = getDerivedPredForm(pf, ST_CF);
			break;
		case ST_PF:
			pf = getDerivedPredForm(pf, ST_CT);
			break;
		case ST_NONE:
			break;
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
