#ifndef THEORYVISITOR_HPP
#define THEORYVISITOR_HPP

#include "visitors/VisitorFriends.hpp"

/**
 * A class for visiting all elements in a logical theory. 
 * The theory is NOT changed.
 *
 * By default, it is traversed depth-first, allowing subimplementations
 * to only implement some of the traversals specifically.
 */
class TheoryVisitor {
	VISITORFRIENDS()
public:
	virtual ~TheoryVisitor() {}

protected:
	virtual void traverse(const Formula*);
	virtual void traverse(const Term*);
	virtual void traverse(const SetExpr*);

	virtual void visit(const Theory*);
	virtual void visit(const AbstractGroundTheory*);

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

	virtual void visit(const CPReification*){
		// TODO
	}

	virtual void visit(const Rule*);
	virtual void visit(const Definition*);
	virtual void visit(const FixpDef*);

	virtual void visit(const VarTerm*);
	virtual void visit(const FuncTerm*);
	virtual void visit(const DomainTerm*);
	virtual void visit(const AggTerm*);

	virtual void visit(const CPVarTerm*){
		// TODO
	}
	virtual void visit(const CPWSumTerm*){
		// TODO
	}
	virtual void visit(const CPSumTerm*){
		// TODO
	}

	virtual void visit(const EnumSetExpr*);
	virtual void visit(const QuantSetExpr*);
};

#endif
