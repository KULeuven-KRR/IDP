/************************************
	generator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GENERATORFACTORY_HPP
#define GENERATORFACTORY_HPP

#include <vector>
#include <structure.hpp>
#include "generators/InstGenerator.hpp"

class PredTable;
class PredInter;
class SortTable;
class DomainElement;
class InstanceChecker;
class GeneratorNode;
class Universe;

class GeneratorFactory : public StructureVisitor {
private:
	const PredTable*					_table;
	std::vector<Pattern>					_pattern;	//!< _pattern[n] == true iff the n'th column is an input column
	std::vector<const DomElemContainer*>	_vars;		//!< the variables corresponding to each column
	Universe							_universe;	//!< the domains of the variables
	std::vector<unsigned int>			_firstocc;	//!< for each of the variables, the position in _vars where this
													//!< variable occurs first
	InstGenerator*						_generator;

	// NOTE: for any function, if the range is an output variable, we can use the simple func generator
	// if the range is input, we need more specialized generators depending on the function type
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

	InstGenerator* internalCreate(const PredTable*, std::vector<Pattern> pattern, const std::vector<const DomElemContainer*>&, const Universe&);

public:
	static InstGenerator* create(const std::vector<const DomElemContainer*>&, const std::vector<SortTable*>&);
	static InstGenerator* create(const PredTable*, std::vector<Pattern> pattern, const std::vector<const DomElemContainer*>&, const Universe&);
};

#endif /* GENERATORFACTORY_HPP */
