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

#ifndef STRUCTUREVISITOR_HPP
#define STRUCTUREVISITOR_HPP

class PredTable;
class FuncTable;
class SortTable;

class ProcInternalPredTable;
class BDDInternalPredTable;
class FullInternalPredTable;
class FuncInternalPredTable;
class UnionInternalPredTable;
class EnumeratedInternalPredTable;
class EqualInternalPredTable;
class StrLessInternalPredTable;
class StrGreaterInternalPredTable;
class InverseInternalPredTable;

class UnionInternalSortTable;
class EnumeratedInternalSortTable;
class IntRangeInternalSortTable;
class ConstructedInternalSortTable;

class ProcInternalFuncTable;
class UNAInternalFuncTable;
class EnumeratedInternalFuncTable;
class PlusInternalFuncTable;
class MinusInternalFuncTable;
class TimesInternalFuncTable;
class DivInternalFuncTable;
class AbsInternalFuncTable;
class UminInternalFuncTable;
class ExpInternalFuncTable;
class ModInternalFuncTable;

class AllNaturalNumbers;
class AllIntegers;
class AllFloats;
class AllChars;
class AllStrings;

/*
 * Visitor class for analyzing structures
 */
class StructureVisitor {
public:
	virtual ~StructureVisitor() {
	}
	virtual void visit(const PredTable* pt);
	virtual void visit(const FuncTable* ft);
	virtual void visit(const SortTable* st);
	virtual void visit(const ProcInternalPredTable*) = 0;
	virtual void visit(const BDDInternalPredTable*) = 0;
	virtual void visit(const FullInternalPredTable*) = 0;
	virtual void visit(const FuncInternalPredTable*) = 0;
	virtual void visit(const UnionInternalPredTable*) = 0;
	virtual void visit(const EnumeratedInternalPredTable*) = 0;
	virtual void visit(const EqualInternalPredTable*) = 0;
	virtual void visit(const StrLessInternalPredTable*) = 0;
	virtual void visit(const StrGreaterInternalPredTable*) = 0;
	virtual void visit(const InverseInternalPredTable*) = 0;
	virtual void visit(const UnionInternalSortTable*) = 0;
	virtual void visit(const AllNaturalNumbers*) = 0;
	virtual void visit(const AllIntegers*) = 0;
	virtual void visit(const AllFloats*) = 0;
	virtual void visit(const AllChars*) = 0;
	virtual void visit(const AllStrings*) = 0;
	virtual void visit(const EnumeratedInternalSortTable*) = 0;
	virtual void visit(const IntRangeInternalSortTable*) = 0;
	virtual void visit(const ConstructedInternalSortTable*) = 0;
	virtual void visit(const ProcInternalFuncTable*) = 0;
	virtual void visit(const UNAInternalFuncTable*) = 0;
	virtual void visit(const EnumeratedInternalFuncTable*) = 0;
	virtual void visit(const PlusInternalFuncTable*) = 0;
	virtual void visit(const MinusInternalFuncTable*) = 0;
	virtual void visit(const TimesInternalFuncTable*) = 0;
	virtual void visit(const DivInternalFuncTable*) = 0;
	virtual void visit(const AbsInternalFuncTable*) = 0;
	virtual void visit(const UminInternalFuncTable*) = 0;
	virtual void visit(const ExpInternalFuncTable*) = 0;
	virtual void visit(const ModInternalFuncTable*) = 0;
};

#endif
