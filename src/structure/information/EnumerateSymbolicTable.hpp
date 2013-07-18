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

#ifndef MATERIALIZER_HPP 
#define MATERIALIZER_HPP 

#include "IncludeComponents.hpp"
#include "visitors/StructureVisitor.hpp"
#include "structure/StructureComponents.hpp"

/** Replace symbolic table by an enumerated one.
 *	Returns NULL when impossible or when it was enumerated already.
 */
class EnumerateSymbolicTable: public StructureVisitor {
private:
	Universe _universe;
	PredTable* _predtable;
	FuncTable* _functable;
	SortTable* _sorttable;
	InternalPredTable* _internpredtable;
	InternalFuncTable* _internfunctable;
	InternalSortTable* _internsorttable;

	void makeEnumeratedPredTable(const InternalPredTable* ipt) {
		if (ipt->approxFinite(_universe)) {
			_internpredtable = new EnumeratedInternalPredTable();
			for (TableIterator it = ipt->begin(_universe); not it.isAtEnd(); ++it) {
				_internpredtable->add(*it);
			}
		} else {
			throw notyetimplemented("Enumerating infinite tables");
		}
	}

	void visit(const PredTable* pt) {
		_universe = pt->universe();
		_internpredtable = pt->internTable();
		pt->internTable()->accept(this);
		if (_internpredtable == pt->internTable()) {
			_predtable = NULL;
		} else {
			_predtable = new PredTable(_internpredtable, pt->universe());
		}
	}

	void visit(const FuncTable* ft) {
		_universe = ft->universe();
		_internfunctable = ft->internTable();
		ft->internTable()->accept(this);
		if (_internfunctable == ft->internTable()) {
			_functable = 0;
		} else {
			_functable = new FuncTable(_internfunctable, ft->universe());
		}
	}

	void visit(const SortTable* st) {
		_internsorttable = st->internTable();
		st->internTable()->accept(this);
		if (_internsorttable == st->internTable()) {
			_sorttable = 0;
		} else {
			_sorttable = new SortTable(_internsorttable);
		}
	}

	void visit(const ProcInternalPredTable* pift) {
		makeEnumeratedPredTable(pift);
	}
	void visit(const BDDInternalPredTable* bipt) {
		makeEnumeratedPredTable(bipt);
	}
	void visit(const UnionInternalPredTable* uipt) {
		makeEnumeratedPredTable(uipt);
	}

	void visit(const FuncInternalPredTable* fipt) {
		InternalPredTable* save = _internpredtable;
		visit(fipt->table());
		FuncTable* ft = _functable;
		if (ft) {
			_internpredtable = new FuncInternalPredTable(ft, false);
		} else {
			_internpredtable = save;
		}
	}

	void visit(const InverseInternalPredTable* iipt) {
		InternalPredTable* save = _internpredtable;
		_internpredtable = iipt->table();
		iipt->table()->accept(this);
		if (_internpredtable != iipt->table()) {
			_internpredtable = InverseInternalPredTable::getInverseTable(_internpredtable);
		} else {
			_internpredtable = save;
		}
	}

	void visit(const ProcInternalFuncTable* pift) {
		if (pift->approxFinite(_universe)) {
			_internfunctable = new EnumeratedInternalFuncTable();
			for (TableIterator it = pift->begin(_universe); not it.isAtEnd(); ++it) {
				_internfunctable->add(*it);
			}
		} else {
			throw notyetimplemented("Enumerating infinite tables");
		}
	}

	void visit(const UnionInternalSortTable* uist) {
		tablesize ts = uist->size();
		if (not ts.isInfinite()) {
			_internsorttable = new EnumeratedInternalSortTable();
			for (SortIterator it = SortIterator(uist->sortBegin()); not it.isAtEnd(); ++it) {
				_internsorttable->add(*it);
			}
		} else {
			throw notyetimplemented("Enumerating infinite tables");
		}
	}

	virtual void visit(const FullInternalPredTable*){

	}
	virtual void visit(const EnumeratedInternalPredTable*){

	}
	virtual void visit(const EqualInternalPredTable*){

	}
	virtual void visit(const StrLessInternalPredTable*){

	}
	virtual void visit(const StrGreaterInternalPredTable*){

	}
	virtual void visit(const AllNaturalNumbers*){

	}
	virtual void visit(const AllIntegers*){

	}
	virtual void visit(const AllFloats*){

	}
	virtual void visit(const AllChars*){

	}
	virtual void visit(const AllStrings*){

	}
	virtual void visit(const EnumeratedInternalSortTable*){

	}
	virtual void visit(const IntRangeInternalSortTable*){

	}
	virtual void visit(const ConstructedInternalSortTable*){

	}
	virtual void visit(const UNAInternalFuncTable*){

	}
	virtual void visit(const EnumeratedInternalFuncTable*){

	}
	virtual void visit(const PlusInternalFuncTable*){

	}
	virtual void visit(const MinusInternalFuncTable*){

	}
	virtual void visit(const TimesInternalFuncTable*){

	}
	virtual void visit(const DivInternalFuncTable*){

	}
	virtual void visit(const AbsInternalFuncTable*){

	}
	virtual void visit(const UminInternalFuncTable*){

	}
	virtual void visit(const ExpInternalFuncTable*){

	}
	virtual void visit(const ModInternalFuncTable*){

	}

public:
	SortTable* run(const SortTable* s) {
		_sorttable = NULL;
		visit(s);
		return _sorttable;
	}
	PredTable* run(const PredTable* p) {
		_predtable = NULL;
		visit(p);
		return _predtable;
	}
	FuncTable* run(const FuncTable* f) {
		_functable = NULL;
		visit(f);
		return _functable;
	}
};

#endif /* MATERIALIZER_HPP_ */
