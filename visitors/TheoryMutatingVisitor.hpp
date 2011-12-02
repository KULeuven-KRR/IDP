/************************************
	TheoryMutatingVisitor.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef THEORYMUTATINGVISITOR_HPP
#define THEORYMUTATINGVISITOR_HPP

class Theory;

class AbstractGroundTheory;

class Formula;
class PredForm;
class EqChainForm;
class EquivForm;
class BoolForm;
class QuantForm;
class AggForm;

class GroundDefinition;
class GroundRule;
class PCGroundRule;
class AggGroundRule;
class GroundSet;
class GroundAggregate;

class Rule;
class Definition;
class FixpDef;

class Term;
class VarTerm;
class FuncTerm;
class DomainTerm;
class AggTerm;

class CPTerm;
class CPVarTerm;
class CPWSumTerm;
class CPSumTerm;
class CPReification;

class SetExpr;
class EnumSetExpr;
class QuantSetExpr;

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
protected:
	virtual Formula* traverse(Formula*);
	virtual Term* traverse(Term*);
	virtual SetExpr* traverse(SetExpr*);

public:
	virtual ~TheoryMutatingVisitor(){}
	// Theories
	virtual Theory* visit(Theory*);
	virtual AbstractGroundTheory* visit(AbstractGroundTheory*);

	// Formulas
	virtual Formula* visit(PredForm*);
	virtual Formula* visit(EqChainForm*);
	virtual Formula* visit(EquivForm*);
	virtual Formula* visit(BoolForm*);
	virtual Formula* visit(QuantForm*);
	virtual Formula* visit(AggForm*);

	virtual GroundDefinition* visit(GroundDefinition*);
	virtual GroundRule*	visit(AggGroundRule*);
	virtual GroundRule*	visit(PCGroundRule*);

	// Definitions
	virtual Rule*		visit(Rule*);
	virtual Definition*	visit(Definition*);
	virtual FixpDef*	visit(FixpDef*);

	// Terms
	virtual Term* visit(VarTerm*);
	virtual Term* visit(FuncTerm*);
	virtual Term* visit(DomainTerm*);
	virtual Term* visit(AggTerm*);

	// Set expressions
	virtual SetExpr* visit(EnumSetExpr*);
	virtual SetExpr* visit(QuantSetExpr*);
};

#endif
