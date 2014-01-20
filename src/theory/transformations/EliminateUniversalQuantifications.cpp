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
#include "EliminateUniversalQuantifications.hpp"

#include "theory/TheoryUtils.hpp"

using namespace std;

Formula* EliminateUniversalQuantifications::visit(QuantForm* qf) {
	if(qf->isUniv()) {
		qf->subformula()->negate();
		FormulaUtils::pushNegations(qf->subformula());
		if(qf->sign() == SIGN::POS) {
			auto newqf = new QuantForm(SIGN::NEG,QUANT::EXIST,qf->quantVars(),qf->subformula(),qf->pi());
			return newqf;
		} else {
			auto newqf = new QuantForm(SIGN::POS,QUANT::EXIST,qf->quantVars(),qf->subformula(),qf->pi());
			return newqf;
		}
	} else {
		return qf;
	}
}
