/************************************
	visitor.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef VISITOR_HPP
#define VISITOR_HPP

class Namespace;
class Formula;
class Term;
class Rule;
class AbstractDefinition;
class Definition;
class FixpDef;
class SetExpr;
class AbstractTheory;
class Theory;
class PredForm;
class BracketForm;
class EqChainForm;
class EquivForm;	
class BoolForm;
class QuantForm;
class AggForm;
class VarTerm;
class AggTerm;
class DomainTerm;
class FuncTerm;
class QuantSetExpr;
class EnumSetExpr;
class SortTable;
class PredInter;
class FuncInter;
class Structure;
class Sort;
class Predicate;
class Function;
class Vocabulary;
class GroundTheory;
class SolverTheory;
class GroundDefinition;
class GroundAggregate;
class GroundSet;
class PCGroundRuleBody;
class AggGroundRuleBody;
class CPReification;
class CPSumTerm;
class CPWSumTerm;
class CPVarTerm;

/*
 *	class Visitor
 *		This class implements a visitor pattern for namespaces, theories, etc.
 *		By default, if 'visit' is called on an object A, then A is traversed depth-first, from left to right.
 */
class Visitor {
	public:
		Visitor() { }
		virtual ~Visitor() { }

		// Traverse the parse tree
		void traverse(const Formula*);
		void traverse(const Term*);
		void traverse(const Rule*);
		void traverse(const Definition*);
		void traverse(const FixpDef*);
		void traverse(const SetExpr*);
		void traverse(const Theory*);
		void traverse(const Structure*);
		void traverse(const Vocabulary*);
		void traverse(const GroundTheory*);
		void traverse(const SolverTheory*);
		void traverse(const GroundDefinition*);
		void traverse(const GroundSet*);
		void traverse(const Namespace*);

		/** Namespaces **/
		virtual void visit(const Namespace*);
	
		/** Theories **/
		virtual void visit(const Theory*);
		virtual void visit(const GroundTheory*);
		virtual void visit(const SolverTheory*);

		// Formulas     
		virtual void visit(const PredForm*);			
		virtual void visit(const BracketForm*);			
		virtual void visit(const EqChainForm*);
		virtual void visit(const EquivForm*);
		virtual void visit(const BoolForm*);
		virtual void visit(const QuantForm*);
		virtual void visit(const AggForm*);

		// Definitions 
		virtual void visit(const Rule*);
		virtual void visit(const Definition*);
		virtual void visit(const FixpDef*);

		// Terms
		virtual void visit(const VarTerm*);
		virtual void visit(const FuncTerm*);
		virtual void visit(const DomainTerm*);
		virtual void visit(const AggTerm*);

		// Set expressions
		virtual void visit(const EnumSetExpr*);
		virtual void visit(const QuantSetExpr*);

		/** Structures **/
		virtual void visit(const Structure*);
		virtual void visit(const SortTable*);
		virtual void visit(const PredInter*);
		virtual void visit(const FuncInter*);

		/** Vocabularies **/
		virtual void visit(const Vocabulary*);
		virtual void visit(const Sort*);
		virtual void visit(const Predicate*);
		virtual void visit(const Function*);

		/** Grounding **/
		virtual void visit(const GroundDefinition*);
		virtual void visit(const GroundAggregate*);
		virtual void visit(const GroundSet*);
		virtual void visit(const PCGroundRuleBody*);
		virtual void visit(const AggGroundRuleBody*);

		/* Constraint Programming */
		virtual void visit(const CPReification*);
		virtual void visit(const CPSumTerm*);
		virtual void visit(const CPWSumTerm*);
		virtual void visit(const CPVarTerm*);
};

/*
 *	class MutatingVisitor
 *		This class implements a visitor pattern for namespaces, theories, etc. that changes the object it visits.
 *		By default, if 'visit' is called on an object A, then A is traversed depth-first, from left to right.
 *		During the traversal, a call of 'visit' to a direct child B of A gives a returnvalue B'. 
 *		If B' is equal to B, nothing happens. Else, B' becomes a child of A instead of B, and B is deleted non-recursively. 
 */
class MutatingVisitor {
	public:
		MutatingVisitor() { }
		virtual ~MutatingVisitor() { }

		/** Theories **/ 

		// Formulas 
		virtual Formula* visit(PredForm*);	
		virtual Formula* visit(BracketForm*);	
		virtual Formula* visit(EqChainForm*);	
		virtual Formula* visit(EquivForm*);	
		virtual Formula* visit(BoolForm*);	
		virtual Formula* visit(QuantForm*);	
		virtual Formula* visit(AggForm*);

		// Definitions 
		virtual Rule* visit(Rule*);		// NOTE: the head of a rule is not visited by the default implementation!
		virtual Definition* visit(Definition*);	
		virtual FixpDef* visit(FixpDef*);
		virtual AbstractDefinition* visit(GroundDefinition*);

		// Terms
		virtual Term* visit(VarTerm*);		
		virtual Term* visit(FuncTerm*);	
		virtual Term* visit(DomainTerm*);	
		virtual Term* visit(AggTerm*);		

		// Set expressions
		virtual SetExpr* visit(EnumSetExpr*);	
		virtual SetExpr* visit(QuantSetExpr*);

		// Theories
		virtual Theory* visit(Theory*);	
		virtual AbstractTheory* visit(GroundTheory*);
		virtual AbstractTheory* visit(SolverTheory*);
};

#endif
