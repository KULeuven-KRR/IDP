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
#include "options.hpp"
#include <cstdio>
#include <sstream>

class Printer : public Visitor {

	protected:
		stringstream _out;
		unsigned int _indent;
		Printer(); 
	
	public:
		// Factory method
		static Printer* create(InfOptions* opts);

		// Print methods
		string print(const Vocabulary* v);
		string print(const AbstractTheory* t);
		string print(const AbstractStructure* s);

		// Indentation
		void indent();
		void unindent();
		void printtab();

};

class SimplePrinter : public Printer {

	public:
		void visit(const Vocabulary*);
		void visit(const AbstractTheory*);
		void visit(const AbstractStructure*);

};

class IDPPrinter : public Printer {

	private:
		const PFSymbol* 	_currsymbol;
		const Structure* 	_currstructure;
		void print(const PredTable*);
		void printInter(const char*,const char*,const PredTable*,const PredTable*);

		bool _printtypes;

	public:

		IDPPrinter(bool printtypes) : _printtypes(printtypes) { }

		/** Theories **/
		void visit(const Theory*);
		void visit(const EcnfTheory*);

		// Formulas
		void visit(const PredForm*);
		void visit(const EqChainForm*);
		void visit(const EquivForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);

		// Definitions
		void visit(const Rule*);
		void visit(const Definition*);
		void visit(const FixpDef*);

		// Terms
		void visit(const VarTerm*);
		void visit(const FuncTerm*);
		void visit(const DomainTerm*);
		void visit(const AggTerm*);

		// Set expressions
		void visit(const EnumSetExpr*);
		void visit(const QuantSetExpr*);

		/** Structures **/
		void visit(const Structure*);
		void visit(const SortTable*);
		void visit(const PredInter*);
		void visit(const FuncInter*);

		/** Vocabularies **/
		void visit(const Vocabulary*);
		void visit(const Sort*);
		void visit(const Predicate*);
		void visit(const Function*);

};

#endif
