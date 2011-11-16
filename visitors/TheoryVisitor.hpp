/************************************
	TheoryVisitor.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef THEORYVISITOR_HPP
#define THEORYVISITOR_HPP

class Theory;

class GroundPolicy;
class PrintGroundPolicy;
class SolverPolicy;
template<class T> class GroundTheory;

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
 * The theory is NOT changed.
 *
 * By default, it is traversed depth-first, allowing subimplementations
 * to only implement some of the traversals specifically.
 */
class TheoryVisitor {
protected:
	void traverse(const Formula*);
	void traverse(const Term*);
	void traverse(const SetExpr*);

public:
	virtual ~TheoryVisitor() {}
	// Theories
	virtual void visit(const Theory*);

	virtual void visit(const GroundTheory<GroundPolicy>*);
	virtual void visit(const GroundTheory<PrintGroundPolicy>*);
	virtual void visit(const GroundTheory<SolverPolicy>*);

	// Formulas
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

	virtual void visit(const CPReification*);

	// Definitions
	virtual void visit(const Rule*);
	virtual void visit(const Definition*);
	virtual void visit(const FixpDef*);

	// Terms
	virtual void visit(const VarTerm*);
	virtual void visit(const FuncTerm*);
	virtual void visit(const DomainTerm*);
	virtual void visit(const AggTerm*);

	virtual void visit(const CPVarTerm*);
	virtual void visit(const CPWSumTerm*);
	virtual void visit(const CPSumTerm*);

	// Set expressions
	virtual void visit(const EnumSetExpr*);
	virtual void visit(const QuantSetExpr*);
};

#endif
