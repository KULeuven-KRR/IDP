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
#include "vocabulary/vocabulary.hpp"

class PFSymbol;

class CheckContainment: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	const PFSymbol* _symbol;
	bool _result;

public:
	template<class T>
	bool execute(const PFSymbol* s, const T* f) {
		_symbol = s;
		_result = false;
		f->accept(this);
		return _result;
	}

protected:
	void visit(const PredForm* pf) {
		if (pf->symbol() == _symbol) {
			_result = true;
			return;
		} else {
			traverse(pf);
		}
	}

	void visit(const FuncTerm* ft) {
		if (ft->function() == _symbol) {
			_result = true;
			return;
		} else {
			traverse(ft);
		}
	}
};
