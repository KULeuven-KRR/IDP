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
#include "RemoveEquivalences.hpp"

using namespace std;

/**
 * Splits equivalences. It is probably better to first introduce tseitins because formulas are shared, so this transformation is only used in special cases, the grounding directly handles equivalences.
 */
Formula* RemoveEquivalences::visit(EquivForm* ef) {
	auto nl = ef->left()->accept(this);
	auto nr = ef->right()->accept(this);
	auto newl = nl->clone();
	newl->negate();
	vector<Formula*> vf1 = {newl, nr->clone()};
	auto newr = nr->clone();
	newr->negate();
	vector<Formula*> vf2 = {nl->clone(), newr};
	auto bf1 = new BoolForm(SIGN::POS, false, vf1, ef->pi());
	auto bf2 = new BoolForm(SIGN::POS, false, vf2, ef->pi());
	vector<Formula*> vf = {bf1, bf2};
	auto bf = new BoolForm(ef->sign(), true, vf, ef->pi());
	delete (ef);
	return bf;
}
