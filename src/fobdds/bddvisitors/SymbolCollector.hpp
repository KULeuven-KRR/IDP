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
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddKernel.hpp"
#include "fobdds/FoBddAtomKernel.hpp"
#include "fobdds/FoBddTerm.hpp"

/**
 * Class to obtain symbols (predsymbols and function symbols) occuring in a bdd
 */

class SymbolCollector: public FOBDDVisitor {
private:
	std::set<const PFSymbol*> _result;
public:
	SymbolCollector(std::shared_ptr<FOBDDManager> m)
			: FOBDDVisitor(m) {
	}
	void visit(const FOBDDAtomKernel* a) {
		_result.insert(a->symbol());
		FOBDDVisitor::visit(a);
	}
	void visit(const FOBDDFuncTerm* f){
		_result.insert(f->func());
		FOBDDVisitor::visit(f);
	}


	template<typename Node>
	const std::set<const PFSymbol*>& collectSymbols(const Node* arg) {
		_result.clear();
		arg->accept(this);
		return _result;
	}
};

