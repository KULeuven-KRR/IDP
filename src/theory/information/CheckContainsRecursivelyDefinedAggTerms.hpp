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

class CheckContainsRecDefAggTerms: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	std::set<PFSymbol*> _definedsymbols;
	bool _result;

public:
	bool execute(const Definition* f, const std::set<PFSymbol*>& definedsymbols) {
		_definedsymbols = definedsymbols;
		_result = false;
		f->accept(this);
		return _result;
	}

protected:
	void visit(const AggTerm* at) {
		auto as = FormulaUtils::collectSymbols(at->set());
		for(auto s: _definedsymbols){
			if(contains(as, s)){
				_result |= true;
			}
		}
	}
};
