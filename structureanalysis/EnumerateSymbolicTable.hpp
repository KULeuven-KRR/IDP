/************************************
	EnumerateSymbolicTable.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef MATERIALIZER_HPP 
#define MATERIALIZER_HPP 

//#include "structure.hpp"
#include "visitors/StructureVisitor.hpp"

/** Replace symbolic table by an enumerated one.
 *	Returns 0 when impossible or when it was enumerated already.
 */
class EnumerateSymbolicTable: public StructureVisitor {
private:
	Universe 			_universe;
	PredTable* 			_predtable;
	FuncTable* 			_functable;
	SortTable* 			_sorttable;
	InternalPredTable* 	_internpredtable;
	InternalFuncTable* 	_internfunctable;
	InternalSortTable* 	_internsorttable;

	void makeEnumeratedPredTable(const InternalPredTable* ipt) {
		if (ipt->approxFinite(_universe)) {
			_internpredtable = new EnumeratedInternalPredTable();
			for (TableIterator it = ipt->begin(_universe); not it.isAtEnd(); ++it) {
				_internpredtable->add(*it);
			}
		}
	}

	void visit(const PredTable* pt) {
		_universe = pt->universe();
		_internpredtable = pt->internTable();
		pt->internTable()->accept(this);
		if (_internpredtable == pt->internTable()) {
			_predtable = 0;
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
			_internpredtable = new InverseInternalPredTable(_internpredtable);
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
		}
	}

	void visit(const UnionInternalSortTable* uist) {
		tablesize ts = uist->size();
		if (ts._type == TST_EXACT || ts._type == TST_APPROXIMATED) {
			_internsorttable = new EnumeratedInternalSortTable();
			for (SortIterator it = SortIterator(uist->sortBegin()); not it.isAtEnd(); ++it) {
				_internsorttable->add(*it);
			}
		}
	}

public:
	SortTable* run(const SortTable* s) {
		visit(s);
		return _sorttable;
	}
	PredTable* run(const PredTable* p) {
		visit(p);
		return _predtable;
	}
	FuncTable* run(const FuncTable* f) {
		visit(f);
		return _functable;
	}
};

#endif /* MATERIALIZER_HPP_ */
