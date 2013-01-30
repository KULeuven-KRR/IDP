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

#include <vector>

#include "Flatten.hpp"

#include "IncludeComponents.hpp"

using namespace std;

Formula* Flatten::visit(BoolForm* bf) {
	vector<Formula*> newsubf;
	traverse(bf);
	for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
		if (isa<BoolForm>(*(*it))) {
			BoolForm* sbf = dynamic_cast<BoolForm*>(*it);
			if ((bf->conj() == sbf->conj()) && isPos(sbf->sign())) {
				for (auto jt = sbf->subformulas().cbegin(); jt != sbf->subformulas().cend(); ++jt) {
					newsubf.push_back(*jt);
				}
				delete (sbf);
			} else {
				newsubf.push_back(*it);
			}
		} else {
			newsubf.push_back(*it);
		}
	}
	bf->subformulas(newsubf);
	return bf;
}

Formula* Flatten::visit(QuantForm* qf) {
	traverse(qf);
	if (isa<QuantForm>(*(qf->subformula()))) {
		QuantForm* sqf = dynamic_cast<QuantForm*>(qf->subformula());
		if ((qf->quant() == sqf->quant()) && isPos(sqf->sign())) {
			qf->subformula(sqf->subformula());
			for (auto it = sqf->quantVars().cbegin(); it != sqf->quantVars().cend(); ++it) {
				qf->add(*it);
			}
			delete (sqf);
		}
	}
	return qf;
}
