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

#include "IncludeComponents.hpp"
#include "SplitComparisonChains.hpp"
#include <cstddef>

using namespace std;

Formula* SplitComparisonChains::visit(EqChainForm* ef) {
	for (auto it = ef->subterms().cbegin(); it != ef->subterms().cend(); ++it) {
		(*it)->accept(this);
	}
	vector<Formula*> vf;
	size_t n = 0;
	for (auto it = ef->comps().cbegin(); it != ef->comps().cend(); ++it, ++n) {
		Predicate* p = 0;
		switch (*it) {
		case CompType::EQ:
		case CompType::NEQ:
			p = get(STDPRED::EQ);
			break;
		case CompType::LT:
		case CompType::GEQ:
			p = get(STDPRED::LT);
			break;
		case CompType::GT:
		case CompType::LEQ:
			p = get(STDPRED::GT);
			break;
		}
		SIGN sign = (*it == CompType::EQ || *it == CompType::LT || *it == CompType::GT) ? SIGN::POS : SIGN::NEG;
		vector<Sort*> vs(2);
		vs[0] = ef->subterms()[n]->sort();
		vs[1] = ef->subterms()[n + 1]->sort();
		p = p->disambiguate(vs, _vocab);
		Assert(p != NULL);
		vector<Term*> vt(2);
		if (n > 0) {
			vt[0] = ef->subterms()[n]->clone();
		} else {
			vt[0] = ef->subterms()[n];
		}
		vt[1] = ef->subterms()[n + 1];
		PredForm* pf = new PredForm(sign, p, vt, ef->pi());
		vf.push_back(pf);
	}
	if (vf.size() == 1) {
		if (isNeg(ef->sign())) {
			vf[0]->negate();
		}
		delete (ef);
		return vf[0];
	} else {
		BoolForm* bf = new BoolForm(ef->sign(), ef->conj(), vf, ef->pi());
		delete (ef);
		return bf;
	}
}
