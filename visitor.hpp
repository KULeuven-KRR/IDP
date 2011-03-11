/************************************
	visitor.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef VISITOR_HPP
#define VISITOR_HPP

class Formula;
class Term;
class Rule;
class Definition;
class FixpDef;
class SetExpr;
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
		void traverse(const Formula* f);
		void traverse(const Term* t);
		void traverse(const Rule* r);
		void traverse(const Definition* d);
		void traverse(const FixpDef* f);
		void traverse(const SetExpr* s);
		void traverse(const Theory* t);
		void traverse(const Structure* s);
		void traverse(const Vocabulary* v);

		/** Theories **/
		virtual void visit(const Theory* t);
		virtual void visit(const GroundTheory* t);
		virtual void visit(const SolverTheory* t);

		// Formulas     
		virtual void visit(const PredForm* a);			
		virtual void visit(const BracketForm* a);			
		virtual void visit(const EqChainForm* a);
		virtual void visit(const EquivForm* a);
		virtual void visit(const BoolForm* a);
		virtual void visit(const QuantForm* a);
		virtual void visit(const AggForm* a);

		// Definitions 
		virtual void visit(const Rule* a);
		virtual void visit(const Definition* a);
		virtual void visit(const FixpDef* a);

		// Terms
		virtual void visit(const VarTerm* a);
		virtual void visit(const FuncTerm* a);
		virtual void visit(const DomainTerm* a);
		virtual void visit(const AggTerm* a);

		// Set expressions
		virtual void visit(const EnumSetExpr* a);
		virtual void visit(const QuantSetExpr* a);

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
		virtual Formula* visit(PredForm* a);	
		virtual Formula* visit(BracketForm* a);	
		virtual Formula* visit(EqChainForm* a);	
		virtual Formula* visit(EquivForm* a);	
		virtual Formula* visit(BoolForm* a);	
		virtual Formula* visit(QuantForm* a);	
		virtual Formula* visit(AggForm* a);

		// Definitions 
		virtual Rule* visit(Rule* a);		// NOTE: the head of a rule is not visited by the default implementation!
		virtual Definition* visit(Definition* a);	
		virtual FixpDef* visit(FixpDef* a);		

		// Terms
		virtual Term* visit(VarTerm* a);		
		virtual Term* visit(FuncTerm* a);	
		virtual Term* visit(DomainTerm* a);	
		virtual Term* visit(AggTerm* a);		

		// Set expressions
		virtual SetExpr* visit(EnumSetExpr* a);	
		virtual SetExpr* visit(QuantSetExpr* a);

		// Theories
		virtual Theory* visit(Theory* t);	
		virtual GroundTheory* visit(GroundTheory* t);
		virtual SolverTheory* visit(SolverTheory* t);


};


#endif
