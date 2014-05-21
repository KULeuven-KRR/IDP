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
#include "commontypes.hpp"

class Definition;

/*
 * Returns for all symbols that occur in which context they occur.
 * CONTEXT::BOTH means: in aggregates, definitions, equivalences,...
 * Returned context are relative to input object!
 */
class CollectSymbolOccurences: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	std::set<std::pair<PFSymbol*, Context>> _result;
	Context _context;

public:
	template<typename T>
	std::set<std::pair<PFSymbol*, Context>> execute(T f) {
		_context = Context::POSITIVE;
		f->accept(this);
		return _result;
	}

protected:
	void visit(const PredForm*);
	void visit(const BoolForm*);
	void visit(const QuantForm*);
	void visit(const EqChainForm*);
	void visit(const AggForm*);
	void visit(const EquivForm*);
	void visit(const FuncTerm* ft);
	void visit(const Definition*);
	void visit(const Rule*);

};
