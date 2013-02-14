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

#ifndef THEORYVISITOR_HPP
#define THEORYVISITOR_HPP

#include "VisitorFriends.hpp"

#include "groundtheories/GroundPolicy.hpp"

class TheoryVisitor {
	VISITORFRIENDS()
public:
	virtual ~TheoryVisitor() {
	}

protected:
	virtual void traverse(const Theory*);
	virtual void traverse(const Formula*);
	virtual void traverse(const Term*);
	virtual void traverse(const QuantSetExpr*);
	virtual void traverse(const EnumSetExpr*);

	virtual void visit(const Theory*) = 0;
	virtual void visit(const AbstractGroundTheory*) = 0;
	virtual void visit(const GroundTheory<GroundPolicy>*) = 0;

	virtual void visit(const PredForm*) = 0;
	virtual void visit(const EqChainForm*) = 0;
	virtual void visit(const EquivForm*) = 0;
	virtual void visit(const BoolForm*) = 0;
	virtual void visit(const QuantForm*) = 0;
	virtual void visit(const AggForm*) = 0;

	virtual void visit(const GroundDefinition*) = 0;
	virtual void visit(const PCGroundRule*) = 0;
	virtual void visit(const AggGroundRule*) = 0;
	virtual void visit(const GroundSet*) = 0;
	virtual void visit(const GroundAggregate*) = 0;

	virtual void visit(const CPReification*) = 0;

	virtual void visit(const Rule*) = 0;
	virtual void visit(const Definition*) = 0;
	virtual void visit(const FixpDef*) = 0;

	virtual void visit(const VarTerm*) = 0;
	virtual void visit(const FuncTerm*) = 0;
	virtual void visit(const DomainTerm*) = 0;
	virtual void visit(const AggTerm*) = 0;

	virtual void visit(const CPVarTerm*) = 0;
	virtual void visit(const CPSetTerm*) = 0;
	virtual void visit(const EnumSetExpr*) = 0;
	virtual void visit(const QuantSetExpr*) = 0;
};

/**
 * A class for visiting all elements in a logical theory. 
 * The theory is NOT changed.
 *
 * By default, it is traversed depth-first, allowing subimplementations
 * to only implement some of the traversals specifically.
 */
class DefaultTraversingTheoryVisitor: public TheoryVisitor {
	VISITORFRIENDS()
protected:
	virtual void visit(const Theory*);
	virtual void visit(const AbstractGroundTheory*);
	virtual void visit(const GroundTheory<GroundPolicy>*);

	virtual void visit(const PredForm*);
	virtual void visit(const EqChainForm*);
	virtual void visit(const EquivForm*);
	virtual void visit(const BoolForm*);
	virtual void visit(const QuantForm*);
	virtual void visit(const AggForm*);

	virtual void visit(const GroundDefinition*);
	virtual void visit(const PCGroundRule*);
	virtual void visit(const AggGroundRule*);
	virtual void visit(const GroundSet*);
	virtual void visit(const GroundAggregate*);

	virtual void visit(const CPReification*) {
		// TODO
	}

	virtual void visit(const Rule*);
	virtual void visit(const Definition*);
	virtual void visit(const FixpDef*);

	virtual void visit(const VarTerm*);
	virtual void visit(const FuncTerm*);
	virtual void visit(const DomainTerm*);
	virtual void visit(const AggTerm*);

	virtual void visit(const CPVarTerm*) {
		// TODO
	}
	virtual void visit(const CPSetTerm*) {
		// TODO
	}

	virtual void visit(const EnumSetExpr*);
	virtual void visit(const QuantSetExpr*);
};

#endif
