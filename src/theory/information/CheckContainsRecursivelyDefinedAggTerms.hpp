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
		if (getOption(BoolType::GUARANTEE_NO_REC_AGG)) { return false; }
		auto voc = Vocabulary("wrapper_voc");
		for(auto s: FormulaUtils::collectSymbols(def)){
			voc.add(s);
		}
		auto theory = new Theory("wrapper_theory", &voc, {});
		auto newdef = def->clone();
		theory->add(newdef);
		DefinitionUtils::splitDefinitions(theory);
		_result = false;
		for (auto d : theory->definitions()) {
			_def = d;
			d->accept(this);
		}
		theory->recursiveDelete();
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
