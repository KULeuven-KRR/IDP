/************************************
	print.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PRINT_HPP
#define PRINT_HPP

#include "visitor.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include <cstdio>

class Printer : public Visitor {

	protected:
		FILE* _out;
		unsigned int _indent;
		Printer(); 
	
	public:
		// Factory method
		static Printer* create();
		virtual ~Printer();

		// Print methods
		void print(Vocabulary* v);
		void print(Theory* t);
		void print(Structure* s);

		// Indentation
		void indent();
		void unindent();
		void printtab();

};

class SimplePrinter : public Printer {

	public:
		void visit(Theory*);
		void visit(Structure*);
		void visit(Vocabulary*);

};

class IDPPrinter : public Printer {

	private:
		PFSymbol* 	_currsymbol;
		Structure* 	_currstructure;
		void print(PredTable*);
		void printInter(const char*,const char*,PredTable*,PredTable*);

	public:
		/** Theories **/
		void visit(Theory*);

		// Formulas
		void visit(PredForm*);
		void visit(EqChainForm*);
		void visit(EquivForm*);
		void visit(BoolForm*);
		void visit(QuantForm*);

		// Definitions
		void visit(Rule*);
		void visit(Definition*);
		void visit(FixpDef*);

		// Terms
		void visit(VarTerm*);
		void visit(FuncTerm*);
		void visit(DomainTerm*);
		void visit(AggTerm*);

		// Set expressions
		void visit(EnumSetExpr*);
		void visit(QuantSetExpr*);

		/** Structures **/
		void visit(Structure*);
		void visit(SortTable*);
		void visit(PredInter*);
		void visit(FuncInter*);

		/** Vocabularies **/
		void visit(Vocabulary*);
		void visit(Sort*);
		void visit(Predicate*);
		void visit(Function*);

};

#endif
