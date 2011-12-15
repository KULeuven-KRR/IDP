/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

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
	virtual ~StructureVisitor() {}
	virtual void visit(const PredTable* pt);
	virtual void visit(const FuncTable* ft);
	virtual void visit(const SortTable* st);
	virtual void visit(const ProcInternalPredTable*) {}
	virtual void visit(const BDDInternalPredTable*) {}
	virtual void visit(const FullInternalPredTable*) {}
	virtual void visit(const FuncInternalPredTable*) {}
	virtual void visit(const UnionInternalPredTable*) {}
	virtual void visit(const EnumeratedInternalPredTable*) {}
	virtual void visit(const EqualInternalPredTable*) {}
	virtual void visit(const StrLessInternalPredTable*) {}
	virtual void visit(const StrGreaterInternalPredTable*) {}
	virtual void visit(const InverseInternalPredTable*) {}
	virtual void visit(const UnionInternalSortTable*) {}
	virtual void visit(const AllNaturalNumbers*) {}
	virtual void visit(const AllIntegers*) {}
	virtual void visit(const AllFloats*) {}
	virtual void visit(const AllChars*) {}
	virtual void visit(const AllStrings*) {}
	virtual void visit(const EnumeratedInternalSortTable*) {}
	virtual void visit(const IntRangeInternalSortTable*) {}
	virtual void visit(const ProcInternalFuncTable*) {}
	virtual void visit(const UNAInternalFuncTable*) {}
	virtual void visit(const EnumeratedInternalFuncTable*) {}
	virtual void visit(const PlusInternalFuncTable*) {}
	virtual void visit(const MinusInternalFuncTable*) {}
	virtual void visit(const TimesInternalFuncTable*) {}
	virtual void visit(const DivInternalFuncTable*) {}
	virtual void visit(const AbsInternalFuncTable*) {}
	virtual void visit(const UminInternalFuncTable*) {}
	virtual void visit(const ExpInternalFuncTable*) {}
	virtual void visit(const ModInternalFuncTable*) {}
};

#endif
