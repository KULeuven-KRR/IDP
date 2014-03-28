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

#pragma once

#include "visitors/TheoryVisitor.hpp"
#include "theory/TheoryUtils.hpp"

class PFSymbol;

class CheckApproxContainsRecDefAggTerms: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	const Definition* _def;
	bool _result;

public:
	bool execute(const Definition* def) {
		_def = def;
		_result = false;
		def->accept(this);
		return _result;
	}

protected:
	void visit(const AggTerm* at) {
		auto as = FormulaUtils::collectSymbols(at->set());
		for(auto s: _def->defsymbols()){
			if(contains(as, s)){
				_result |= true;
			}
		}
	}
};
