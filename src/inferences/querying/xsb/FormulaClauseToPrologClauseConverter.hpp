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

#include <list>
#include "vocabulary/vocabulary.hpp"
#include "FormulaClauseVisitor.hpp"

class PrologProgram;
class PrologClause;
class PrologTerm;
class FormulaClause;
class XSBToIDPTranslator;

class FormulaClauseToPrologClauseConverter: public FormulaClauseVisitor {
private:
	std::list<PrologClause*> _prologClauses;
	PrologProgram* _pp;
	XSBToIDPTranslator* _translator;
public:
	FormulaClauseToPrologClauseConverter(PrologProgram* p, XSBToIDPTranslator* translator)
			: _pp(p),
			  _translator(translator) {
	}

	std::string generateGeneratorClauseName();

	void visit(FormulaClause* f) {
		f->accept(this);
	}
	virtual void visit(PrologTerm*){
		// TODO?
	}
	void visit(ExistsClause*);
	void visit(ForallClause*);
	void visit(AndClause*);
	void visit(OrClause*);
	void visit(AggregateClause*);
	void visit(AggregateTerm*);
	void visit(QuantSetExpression*);
	void visit(EnumSetExpression*);
};

