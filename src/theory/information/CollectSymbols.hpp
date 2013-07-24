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

#include <set>

#include "visitors/TheoryVisitor.hpp"

class PFSymbol;

class CollectSymbols: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	std::set<PFSymbol*> _result;

public:
	template<typename T>
	std::set<PFSymbol*> execute(T f) {
		f->accept(this);
		return _result;
	}

protected:
	void visit(const PredForm* pf);
	void visit(const FuncTerm* ft);
};

