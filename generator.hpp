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
class GeneratorNode;
class Universe;

class InstGenerator {
	public:
		InstGenerator() { }
		virtual ~InstGenerator(){}
		virtual bool first() const = 0;
		virtual bool next() const = 0; 
};

/**************
	Factory
**************/

class GeneratorFactory : public StructureVisitor {
	private:
		const PredTable*					_table;
		std::vector<bool>					_pattern;	//!< _pattern[n] == true iff the n'th column is an input column
		std::vector<const DomElemContainer*>	_vars;		//!< the variables corresponding to each column
		Universe							_universe;	//!< the domains of the variables
		std::vector<unsigned int>			_firstocc;	//!< for each of the variables, the position in _vars where this
														//!< variable occurs first
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

		void visit(const UnionInternalSortTable*);	
		void visit(const AllNaturalNumbers*);				
		void visit(const AllIntegers*);						
		void visit(const AllFloats*);						
		void visit(const AllChars*);					
		void visit(const AllStrings*);					
		void visit(const EnumeratedInternalSortTable*);	
		void visit(const IntRangeInternalSortTable*);	

	public:
		InstGenerator*	create(const std::vector<const DomElemContainer*>&, const std::vector<SortTable*>&);
		InstGenerator*	create(const PredTable*, std::vector<bool> pattern, const std::vector<const DomElemContainer*>&, const Universe&);
};

/******************************
	From BDDs to generators
******************************/

class FOBDDManager;
class FOBDDVariable;
class FOBDD;
class FOBDDKernel;

/**
 * Class to convert a bdd into a generator
 */
class BDDToGenerator {
	private:
		FOBDDManager*	_manager;

		Term* solve(PredForm* atom, Variable* var);
	public:

		// Constructor
		BDDToGenerator(FOBDDManager* manager);

		// Factory methods
		InstGenerator* create(PredForm*, const std::vector<bool>&, const std::vector<const DomElemContainer*>&, const std::vector<Variable*>&, AbstractStructure*, bool, const Universe&);
		InstGenerator* create(const FOBDD*, const std::vector<bool>&, const std::vector<const DomElemContainer*>&, const std::vector<const FOBDDVariable*>&, AbstractStructure* structure, const Universe&);
		InstGenerator* create(const FOBDDKernel*, const std::vector<bool>&, const std::vector<const DomElemContainer*>&, const std::vector<const FOBDDVariable*>&, AbstractStructure* structure, bool, const Universe&);

		GeneratorNode* createnode(const FOBDD*, const std::vector<bool>&, const std::vector<const DomElemContainer*>&, const std::vector<const FOBDDVariable*>&, AbstractStructure* structure, const Universe&);

		// Visit
//		void visit(const FOBDDAtomKernel* kernel);
//		void visit(const FOBDDQuantKernel* kernel);
};

#endif
