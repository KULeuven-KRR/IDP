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

class Definition;
class PFSymbol;

/**
 * Given a definition and a symbol, returns the set of symbols that the given symbol directly depends on.
 * If a symbol P is given, this means that for every rule
 * 		P <- \phi
 * 	or
 * 		P = x <- \phi
 * the returned set contains all symbols in \phi.
 * 
 * If the given symbol is not defined in the definition, this returns an empty set.
 * This method does not return INDIRECT dependencies of the symbol.
 */

class GetDefinedSymbolDirectDependencies: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	PFSymbol* _defined_symbol;
	std::set<PFSymbol*> _result;

public:
	const std::set<PFSymbol*>& execute(const Definition* d, PFSymbol* symbol);

protected:
	void visit(const Rule* r);
	void visit(const PredForm* pf);
	void visit(const FuncTerm* ft);
};
