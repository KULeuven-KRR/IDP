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

class GroundTranslator;

/*************************
	Printer base class
*************************/

class Printer : public Visitor {
	protected:
		stringstream _out;
		unsigned int _indent;
		Printer(); 
	
	public:
		// Factory method
		static Printer* create(InfOptions* opts);

		// Print methods
		string print(const Vocabulary*);
		string print(const AbstractTheory*);
		string print(const AbstractStructure*);
//TODO	string print(const Namespace*);

		// Indentation
		void indent();
		void unindent();
		void printtab();
};

/*********************
	Simple printer
*********************/

class SimplePrinter : public Printer {
	public:
		void visit(const Vocabulary*);
		void visit(const Theory*);
		void visit(const GroundTheory*);
		void visit(const Structure*);
};

/******************
	IDP printer
******************/

class IDPPrinter : public Printer {
	private:
		bool 					_printtypes;
		const PFSymbol* 		_currsymbol;
		const Structure* 		_currstructure;
		const GroundTranslator*	_translator;

		void print(const PredTable*);
		void printInter(const char*,const char*,const PredTable*,const PredTable*);

	public:
		IDPPrinter() : _printtypes(false) { }
		IDPPrinter(bool printtypes) : _printtypes(printtypes) { }

		/** Theories **/
		void visit(const Theory*);

		// Definitions
		void visit(const Definition*);
		void visit(const Rule*);
		void visit(const FixpDef*);

		// Formulas
		void visit(const PredForm*);
		void visit(const EqChainForm*);
		void visit(const EquivForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);

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

		/** Grounding **/
		void visit(const GroundTheory*);
		void visit(const GroundDefinition*);
		void visit(const GroundAggregate*);
		void visit(const GroundSet*);
};

class EcnfPrinter : public Printer {
	public:
		void visit(const GroundTheory*);
		void visit(const GroundDefinition*);
		void visit(const GroundAggregate*);
		void visit(const GroundSet*);
}; 

#endif
