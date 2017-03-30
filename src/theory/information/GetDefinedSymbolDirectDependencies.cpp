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

#include "GetDefinedSymbolDirectDependencies.hpp"

#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"
#include "vocabulary/vocabulary.hpp"
#include "theory/term.hpp"

using namespace std;

const std::set<PFSymbol*>& GetDefinedSymbolDirectDependencies::execute(const Definition* d, PFSymbol* symbol) {
	_defined_symbol = symbol;
	_result.clear();
	d->accept(this);
	return _result;
}
void GetDefinedSymbolDirectDependencies::visit(const Rule* r) {
	if (DefinitionUtils::definesSymbol(r,_defined_symbol)) {
		if (VocabularyUtils::isPredicate(r->head()->symbol(),STDPRED::EQ)) {
			Assert(r->head()->subterms().size() == 2);
			// don't add defined symbol if it occurs as a top-level symbol on the left-hand side of "="
			auto firstarg = *(r->head()->subterms().begin());
			Assert(firstarg->type() == TermType::FUNC);
			for (auto subterm : firstarg->subterms()) {
				subterm->accept(this);
			}
			(*(r->head()->subterms().end()))->accept(this);
			
		} else {
			for (auto subterm : r->head()->subterms()) {
				subterm->accept(this);
			}
		}
		r->body()->accept(this);
	}
}

void GetDefinedSymbolDirectDependencies::visit(const PredForm* pf) {
	_result.insert(pf->symbol());
	traverse(pf);
}
void GetDefinedSymbolDirectDependencies::visit(const FuncTerm* ft) {
	_result.insert(ft->function());
	traverse(ft);
}
