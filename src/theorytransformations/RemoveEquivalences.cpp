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
#include "theorytransformations/RemoveEquivalences.hpp"

#include "vocabulary.hpp"
#include "theory.hpp"

using namespace std;

Formula* RemoveEquivalences::visit(EquivForm* ef) {
	Formula* nl = ef->left()->accept(this);
	Formula* nr = ef->right()->accept(this);
	vector<Formula*> vf1(2);
	vector<Formula*> vf2(2);
	vf1[0] = nl;
	vf1[1] = nr;
	vf2[0] = nl->clone();
	vf2[1] = nr->clone();
	vf1[0]->negate();
	vf2[1]->negate();
	BoolForm* bf1 = new BoolForm(SIGN::POS, false, vf1, ef->pi());
	BoolForm* bf2 = new BoolForm(SIGN::POS, false, vf2, ef->pi());
	vector<Formula*> vf(2);
	vf[0] = bf1;
	vf[1] = bf2;
	BoolForm* bf = new BoolForm(ef->sign(), true, vf, ef->pi());
	delete (ef);
	return bf;
}
