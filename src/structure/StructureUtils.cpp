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

#include "StructureUtils.hpp"
#include "IncludeComponents.hpp"

namespace StructureUtils {

tablesize getNbOfTwoValuedAtoms(Structure* s) {
	tablesize ret;
	for (auto predinter : s->getPredInters()) {
		ret = ret + toDouble(predinter.second->ct()->size());
		ret = ret + toDouble(predinter.second->cf()->size());
		ret = ret - predinter.second->getInconsistentAtoms().size();
	}
	for (auto funcinter : s->getFuncInters()) {
		if (funcinter.second->funcTable() != NULL) {
			ret = ret + funcinter.second->funcTable()->universe().size();
		} else {
			ret = ret + toDouble(funcinter.second->graphInter()->ct()->size());
			ret = ret + toDouble(funcinter.second->graphInter()->cf()->size());
			ret = ret - funcinter.second->graphInter()->getInconsistentAtoms().size();
		}
	}
	return ret;
}

} /* namespace StructureUtils */
