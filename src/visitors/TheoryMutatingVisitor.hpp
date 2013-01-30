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

#ifndef THEORYMUTATINGVISITOR_HPP
#define THEORYMUTATINGVISITOR_HPP

#include "VisitorFriends.hpp"

/**
 * A class for visiting all elements in a logical theory. 
 * The theory can be changed and is cloned by default.
 *
 * By default, it is traversed depth-first, allowing subimplementations to 
 * only implement some of the traversals specifically.
 *
 * NOTE: ALWAYS take the return value as the result of the visitation!
 */
class TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	virtual ~TheoryMutatingVisitor() {
	}

protected:
	virtual Formula* traverse(Formula*);
	virtual Term* traverse(Term*);
	virtual QuantSetExpr* traverse(QuantSetExpr*);
	virtual EnumSetExpr* traverse(EnumSetExpr*);

	virtual Theory* visit(Theory*);
	virtual AbstractGroundTheory* visit(AbstractGroundTheory*);

	virtual Formula* visit(PredForm*);
	virtual Formula* visit(EqChainForm*);
	virtual Formula* visit(EquivForm*);
	virtual Formula* visit(BoolForm*);
	virtual Formula* visit(QuantForm*);
	virtual Formula* visit(AggForm*);

	virtual GroundDefinition* visit(GroundDefinition*);
	virtual GroundRule* visit(AggGroundRule*);
	virtual GroundRule* visit(PCGroundRule*);

	virtual Rule* visit(Rule*);
	virtual Definition* visit(Definition*);
	virtual FixpDef* visit(FixpDef*);

	virtual Term* visit(VarTerm*);
	virtual Term* visit(FuncTerm*);
	virtual Term* visit(DomainTerm*);
	virtual Term* visit(AggTerm*);

	virtual EnumSetExpr* visit(EnumSetExpr*);
	virtual QuantSetExpr* visit(QuantSetExpr*);
};

#endif
