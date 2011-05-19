/************************************
	generator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INSTGENERATOR_HPP
#define INSTGENERATOR_HPP

#include <vector>

class PredTable;
class PredInter;
class SortTable;
class DomainElement;
class InstanceChecker;

class InstGenerator {
	public:
		InstGenerator() { }
		virtual bool first() const = 0;
		virtual bool next() const = 0; 
};

/**************
	Factory
**************/

class GeneratorFactory : public StructureVisitor {
	private:
		PredTable*							_table;
		std::vector<bool>					_pattern;	//!< _pattern[n] == true iff the n'th column is an input column
		std::vector<const DomainElement**>	_vars;		//!< the variables corresponding to each column
		InstGenerator*						_generator;

		void visit(const FuncTable* ft);				
		void visit(const ProcInternalPredTable*);
		void visit(const BDDInternalPredTable*);			
		void visit(const FullInternalPredTable*);		
		void visit(const FuncInternalPredTable*);		
		void visit(const UnionInternalPredTable*);		
		void visit(const EnumeratedInternalPredTable*);	
		void visit(const EqualInternalPredTable*);		
		void visit(const StrLessInternalPredTable*);		
		void visit(const StrGreaterInternalPredTable*);	
		void visit(const InverseInternalPredTable*);		
		void visit(const ProcInternalFuncTable*);		
		void visit(const UNAInternalFuncTable*);			
		void visit(const EnumeratedInternalFuncTable*);	
		void visit(const PlusInternalFuncTable*);		
		void visit(const MinusInternalFuncTable*);		
		void visit(const TimesInternalFuncTable*);		
		void visit(const DivInternalFuncTable*);			
		void visit(const AbsInternalFuncTable*);			
		void visit(const UminInternalFuncTable*);		
		void visit(const ExpInternalFuncTable*);			
		void visit(const ModInternalFuncTable*);			

	public:
		InstGenerator*	create(const std::vector<const DomainElement**>&, const std::vector<SortTable*>&);
		InstGenerator*	create(PredTable*, std::vector<bool> pattern, const std::vector<const DomainElement**>&);
};

#endif
