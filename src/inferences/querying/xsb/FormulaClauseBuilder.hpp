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
#include "visitors/TheoryVisitor.hpp"

class PrologProgram;
class FormulaClause;
class PrologTerm;
class PrologVariable;
class PrologConstant;
class Variable;
class PFSymbol;
class DomainElement;
class XSBToIDPTranslator;

using std::string;
using std::list;

class FormulaClauseBuilder: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	list<FormulaClause*> _ruleClauses;
	PrologProgram* _pp;
	XSBToIDPTranslator* _translator;
	FormulaClause* _parent;
	void enter(FormulaClause* f);
	void leave();

	string generateNewCompositeClauseName();
	string generateNewAndClauseName();
	string generateNewOrClauseName();
	string generateNewExistsClauseName();
	string generateNewForallClauseName();
	string generateNewAggregateClauseName();
	string generateNewAggregateTermName();
	string generateNewEnumSetExprName();
	string generateNewQuantSetExprName();

	PrologVariable* createPrologVar(const Variable*);
	PrologConstant* createPrologConstant(const DomainElement*);
	PrologTerm* createPrologTerm(const PFSymbol*);
public:
	FormulaClauseBuilder(PrologProgram* p, Definition* d, XSBToIDPTranslator* translator)
			: 	_ruleClauses(),
			  	_pp(p),
			  	_translator(translator),
				_parent(NULL) {
		visit(d);
	}

	list<FormulaClause*>& clauses() {
		return _ruleClauses;
	}

	void visit(const Theory*);
	void visit(const Definition*);
	void visit(const Rule*);

	void visit(const VarTerm*);
	void visit(const DomainTerm*);
	void visit(const AggTerm*);
	void visit(const AggForm*);
	void visit(const BoolForm*);
	void visit(const QuantForm*);
	void visit(const PredForm*);
	void visit(const FuncTerm*);
	void visit(const EnumSetExpr*);
	void visit(const QuantSetExpr*);
	void visit(const EqChainForm*);
};
