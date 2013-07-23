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

#include "IsTwoValued.hpp"
#include "IncludeComponents.hpp"

bool isTwoValued(const Term* t, const Structure* structure) {
	if (t == NULL) {
		return false;
	}
	switch (t->type()) {
	case TermType::FUNC: {
		auto ft = dynamic_cast<const FuncTerm*>(t);
		auto inter = ft->function()->interpretation(structure);
		if (inter==NULL || not inter->approxTwoValued()) {
			return false;
		}
		auto twoval = true && ft->subterms().size() > 0;
		for (auto st : ft->subterms()) {
			twoval &= isTwoValued(st, structure);
		}
		return twoval;
	}
	case TermType::AGG: {
		// TODO check on two-valuedness?
		return false;
	}
	case TermType::VAR:
		return true;
	case TermType::DOM:
		return true;
	}
	throw IdpException("Invalid code path");
}
