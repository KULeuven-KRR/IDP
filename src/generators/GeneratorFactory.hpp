/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef GENERATORFACTORY_HPP
#define GENERATORFACTORY_HPP

#include <vector>
#include "InstGenerator.hpp"
#include "visitors/StructureVisitor.hpp"

class PredTable;
class PredInter;
class SortTable;
class DomainElement;
class InstanceChecker;
class GeneratorNode;
class Universe;
class PredForm;
class Structure;
class PFSymbol;

class GeneratorFactory: public StructureVisitor {
private:
	const PredTable* _table;
	std::vector<Pattern> _pattern; //!< _pattern[n] == true iff the n'th column is an input column
	std::vector<const DomElemContainer*> _vars; //!< the variables corresponding to each column
	Universe _universe; //!< the domains of the variables
	std::vector<unsigned int> _firstocc; //!< for each of the variables, the position in _vars where this
										 //!< variable occurs first
	InstGenerator* _generator;

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
	void visit(const ConstructedInternalSortTable*);

	InstGenerator* internalCreate(const PredTable*, std::vector<Pattern> pattern, const std::vector<const DomElemContainer*>&, const Universe&);

public:
	static InstGenerator* create(const std::vector<const DomElemContainer*>&, const std::vector<SortTable*>&, const Formula* original = NULL);
	// NOTE: becomes predtable owner!
	static InstGenerator* create(const PredTable*, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*>&, const Universe&);
	static InstGenerator* create(const PFSymbol* symbol,const  Structure* structure, bool inverse, const std::vector<Pattern>& pattern,
			const std::vector<const DomElemContainer*>& vars, const Universe& universe);
};

#endif /* GENERATORFACTORY_HPP */
