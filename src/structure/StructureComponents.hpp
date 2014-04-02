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

#ifndef STRUCTURECOMPONENTS_HPP
#define STRUCTURECOMPONENTS_HPP

#include <cstdlib>
#include "parseinfo.hpp"
#include "common.hpp"

#include "fwstructure.hpp"

#include "GlobalData.hpp"
#include "utils/NumericLimits.hpp"
#include "MainStructureComponents.hpp"
#include "Structure.hpp"

/**
 * NAMING CONVENTION
 *		- 'Interpretation' means a possibly three-valued, or even four-valued 
 *			interpretation for a symbol.
 *		- 'Table' means a two-value table
 *		- if a name of a methods begins with 'approx', the method is fast, but 
 *			provides a under approximation of the desired result.
 */

/*******************
 Domain atoms
 *******************/

class PFSymbol;
class Variable;
class Vocabulary;
class Structure;

// ITERATORS

class InternalTableIterator {
public:
	virtual ~InternalTableIterator() {
	}
	virtual InternalTableIterator* clone() const = 0;

	virtual bool hasNext() const = 0;
	virtual const ElementTuple& operator*() const = 0;
	virtual void operator++() = 0;
};

class CartesianInternalTableIterator: public InternalTableIterator {
private:
	std::vector<SortIterator> _iterators;
	std::vector<SortIterator> _lowest;
	mutable ElementTable _deref;
	bool _hasNext;
public:
	CartesianInternalTableIterator(const std::vector<SortIterator>& vsi, const std::vector<SortIterator>& low, bool h = true);
	CartesianInternalTableIterator* clone() const;

	virtual bool hasNext() const;
	virtual const ElementTuple& operator*() const;
	virtual void operator++();

	const std::vector<SortIterator>& getIterators() const {
		return _iterators;
	}
	const std::vector<SortIterator>& getLowest() const {
		return _lowest;
	}
};

class InstGenerator;

class GeneratorInternalTableIterator: public InternalTableIterator {
private:
	InstGenerator* _generator;
	std::vector<const DomElemContainer*> _vars;
	bool _hasNext;
	mutable ElementTable _deref;
public:
	GeneratorInternalTableIterator(InstGenerator* generator, const std::vector<const DomElemContainer*>& vars, bool reset = true, bool h = true);
	GeneratorInternalTableIterator* clone() const {
		return new GeneratorInternalTableIterator(_generator, _vars, false, _hasNext);
	}

	virtual bool hasNext() const {
		return _hasNext;
	}
	virtual const ElementTuple& operator*() const;
	virtual void operator++();
};

class SortInternalTableIterator: public InternalTableIterator {
private:
	InternalSortIterator* _iter;
	mutable ElementTable _deref;
public:
	SortInternalTableIterator(InternalSortIterator* isi) :
			_iter(isi) {
	}
	~SortInternalTableIterator();
	SortInternalTableIterator* clone() const;

	virtual bool hasNext() const;
	virtual const ElementTuple& operator*() const;
	virtual void operator++();
};

class EnumInternalIterator: public InternalTableIterator {
private:
	SortedElementTable::const_iterator _iter;
	SortedElementTable::const_iterator _end;
public:
	EnumInternalIterator(SortedElementTable::const_iterator it, SortedElementTable::const_iterator end) :
			_iter(it), _end(end) {
	}
	~EnumInternalIterator() {
	}
	EnumInternalIterator* clone() const;

	virtual bool hasNext() const {
		return _iter != _end;
	}
	virtual const ElementTuple& operator*() const {
		return *_iter;
	}
	virtual void operator++() {
		++_iter;
	}
};

class EnumInternalFuncIterator: public InternalTableIterator {
private:
	Tuple2Elem::const_iterator _iter;
	Tuple2Elem::const_iterator _end;
	mutable ElementTable _deref;
public:
	EnumInternalFuncIterator(Tuple2Elem::const_iterator it, Tuple2Elem::const_iterator end) :
			_iter(it), _end(end) {
	}
	~EnumInternalFuncIterator() {
	}
	EnumInternalFuncIterator* clone() const;

	virtual bool hasNext() const {
		return _iter != _end;
	}
	virtual const ElementTuple& operator*() const;
	virtual void operator++() {
		++_iter;
	}
};

class SortTable;
class PredTable;
class InternalPredTable;

class InternalFuncTable;

class InternalFuncIterator: public InternalTableIterator {
private:
	TableIterator _curr;
	mutable ElementTable _deref;
	const InternalFuncTable* _function;
public:
	InternalFuncIterator(const InternalFuncTable* f, const Universe& univ);
	InternalFuncIterator(const InternalFuncTable* f, const TableIterator& c) :
			_curr(c), _function(f) {
	}
	~InternalFuncIterator() {
	}
	InternalFuncIterator* clone() const {
		return new InternalFuncIterator(_function, _curr);
	}

	virtual bool hasNext() const {
		return not _curr.isAtEnd();
	}
	virtual const ElementTuple& operator*() const;
	virtual void operator++();
};

class InternalPredTable;

class ProcInternalTableIterator: public InternalTableIterator {
private:
	TableIterator _curr;
	Universe _univ;
	mutable ElementTable _deref;
	const InternalPredTable* _predicate;
public:
	ProcInternalTableIterator(const InternalPredTable* p, const Universe& univ);
	ProcInternalTableIterator(const InternalPredTable* p, const TableIterator& c, const Universe& univ) :
			_curr(c), _univ(univ), _predicate(p) {
	}
	~ProcInternalTableIterator() {
	}
	ProcInternalTableIterator* clone() const {
		return new ProcInternalTableIterator(_predicate, _curr, _univ);
	}

	virtual bool hasNext() const {
		return not _curr.isAtEnd();
	}
	virtual const ElementTuple& operator*() const;
	virtual void operator++();
};

class UnionInternalIterator: public InternalTableIterator {
private:
	std::vector<TableIterator> _iterators;
	Universe _universe;
	std::vector<InternalPredTable*> _outtables;
	std::vector<TableIterator>::iterator _curriterator;

	bool contains(const ElementTuple&) const;
	void setcurriterator();

public:
	UnionInternalIterator(const std::vector<TableIterator>&, const std::vector<InternalPredTable*>&, const Universe&);
	~UnionInternalIterator() {
	}
	UnionInternalIterator* clone() const;

	virtual bool hasNext() const;
	virtual const ElementTuple& operator*() const;
	virtual void operator++();
};

class UNAInternalIterator: public InternalTableIterator {
private:
	CartesianInternalTableIterator cartIt;
	Function* _function;
	mutable std::vector<ElementTuple> _deref2;

	UNAInternalIterator(const std::vector<SortIterator>& vsi, const std::vector<SortIterator>& low, Function* f, bool h = true);
public:
	UNAInternalIterator(const std::vector<SortIterator>& vsi, Function* f);
	UNAInternalIterator* clone() const;

	virtual bool hasNext() const;
	virtual const ElementTuple& operator*() const;
	virtual void operator++();
};

class InverseInternalIterator: public InternalTableIterator {
private:
	std::vector<SortIterator> _curr;
	std::vector<SortIterator> _lowest;
	Universe _universe;
	InternalPredTable* _outtable;
	mutable bool _end;
	mutable ElementTuple _currtuple;
	mutable ElementTuple _deref;

	InverseInternalIterator(const std::vector<SortIterator>&, const std::vector<SortIterator>&, InternalPredTable*, const Universe&, bool);
public:
	InverseInternalIterator(const std::vector<SortIterator>&, InternalPredTable*, const Universe&);
	~InverseInternalIterator() {
	}
	InverseInternalIterator* clone() const;

	virtual bool hasNext() const;
	virtual const ElementTuple& operator*() const;
	virtual void operator++();
};

class EqualInternalIterator: public InternalTableIterator {
private:
	SortIterator _iterator;
	mutable ElementTable _deref;
public:
	EqualInternalIterator(const SortIterator& iter);
	~EqualInternalIterator() {
	}
	EqualInternalIterator* clone() const;

	virtual bool hasNext() const;
	virtual const ElementTuple& operator*() const;
	virtual void operator++();
};

class InternalSortIterator {
public:
	virtual bool hasNext() const = 0; // TODO should become isAtEnd
	virtual const DomainElement* operator*() const = 0;
	virtual void operator++() = 0;
	virtual ~InternalSortIterator() {
	}
	virtual InternalSortIterator* clone() const = 0;
	friend class SortIterator;
	friend class SortInternalTableIterator;
};

class UnionInternalSortIterator: public InternalSortIterator {
private:
	std::vector<SortIterator> _iterators;
	std::vector<SortTable*> _outtables;
	std::vector<SortIterator>::iterator _curriterator;

	bool contains(const DomainElement*) const;
	void setcurriterator();

	bool hasNext() const;
	const DomainElement* operator*() const;
	void operator++();
public:
	UnionInternalSortIterator(const std::vector<SortIterator>&, const std::vector<SortTable*>&);
	~UnionInternalSortIterator() {
	}
	UnionInternalSortIterator* clone() const;
};

class NatInternalSortIterator: public InternalSortIterator {
private:
	int _iter;
	bool hasNext() const {
		return true;
	}
	const DomainElement* operator*() const {
		return createDomElem(_iter);
	}
	void operator++() {
		++_iter;
	}
public:
	NatInternalSortIterator(int iter = 0) :
			_iter(iter) {
	}
	~NatInternalSortIterator() {
	}
	NatInternalSortIterator* clone() const {
		return new NatInternalSortIterator(_iter);
	}
};

class IntInternalSortIterator: public InternalSortIterator {
private:
	int _iter;
	bool hasNext() const {
		return true;
	}
	const DomainElement* operator*() const {
		return createDomElem(_iter);
	}
	void operator++() {
		++_iter;
	}
public:
	IntInternalSortIterator(int iter = getMinElem<int>()) :
			_iter(iter) {
	}
	~IntInternalSortIterator() {
	}
	IntInternalSortIterator* clone() const {
		return new IntInternalSortIterator(_iter);
	}
};

class FloatInternalSortIterator: public InternalSortIterator {
private:
	double _iter;
	bool hasNext() const {
		return true;
	}
	const DomainElement* operator*() const {
		return createDomElem(_iter);
	}
	void operator++() {
		++_iter;
	}
public:
	FloatInternalSortIterator(double iter = getMinElem<double>()) :
			_iter(iter) {
	}
	~FloatInternalSortIterator() {
	}
	FloatInternalSortIterator* clone() const {
		return new FloatInternalSortIterator(_iter);
	}
};

class StringInternalSortIterator: public InternalSortIterator {
private:
	std::string _iter;
	bool hasNext() const {
		return true;
	}
	const DomainElement* operator*() const;
	void operator++();
public:
	StringInternalSortIterator(const std::string& iter = "") :
			_iter(iter) {
	}
	~StringInternalSortIterator() {
	}
	StringInternalSortIterator* clone() const {
		return new StringInternalSortIterator(_iter);
	}
};

class CharInternalSortIterator: public InternalSortIterator {
private:
	char _iter;
	bool _end;
	bool hasNext() const {
		return !_end;
	}
	const DomainElement* operator*() const;
	void operator++();
public:
	CharInternalSortIterator(char iter = getMinElem<char>(), bool end = false) :
			_iter(iter), _end(end) {
	}
	~CharInternalSortIterator() {
	}
	CharInternalSortIterator* clone() const {
		return new CharInternalSortIterator(_iter);
	}
};

// TODO danger of not detecting iterator invalidation when the sortedelementtuple is changed in the associated table
class EnumInternalSortIterator: public InternalSortIterator {
private:
	SortedElementTuple::const_iterator _iter;
	SortedElementTuple::const_iterator _end;
	bool hasNext() const {
		return _iter != _end;
	}
	const DomainElement* operator*() const {
		return *_iter;
	}
	void operator++() {
		++_iter;
	}
public:
	EnumInternalSortIterator(SortedElementTuple::iterator it, SortedElementTuple::iterator end) :
			_iter(it), _end(end) {
	}
	~EnumInternalSortIterator() {
	}
	EnumInternalSortIterator* clone() const {
		return new EnumInternalSortIterator(_iter, _end);
	}
};



class ConstructedInternalSortIterator: public InternalSortIterator {
private:
	std::vector<Function*> _constructors;
	int _constr_index; // Stores the index of the constructor currently in use for iteration, to allow cloning; -1 if we are at the end.
	std::vector<Function*>::const_iterator _constructors_it;
	TableIterator _table_it;
	const Structure* _struct;

	bool hasNext() const ;
	const DomainElement* operator*() const ;
	void operator++();
	void skipToNextElement();

	void initialize(const std::vector<Function*>& constructors);

public:
	ConstructedInternalSortIterator();
	ConstructedInternalSortIterator(const std::vector<Function*>& constructors, const Structure* struc);
	ConstructedInternalSortIterator(const std::vector<Function*>& constructors, const Structure* struc, const DomainElement* domel);
	~ConstructedInternalSortIterator() {
	}
	ConstructedInternalSortIterator* clone() const;
};

class RangeInternalSortIterator: public InternalSortIterator {
private:
	int _current;
	int _last;
	bool hasNext() const {
		return _current <= _last;
	}
	const DomainElement* operator*() const {
		return createDomElem(_current);
	}
	void operator++() {
		++_current;
	}
public:
	RangeInternalSortIterator(int current, int last) :
			_current(current), _last(last) {
	}
	~RangeInternalSortIterator() {
	}
	RangeInternalSortIterator* clone() const {
		return new RangeInternalSortIterator(_current, _last);
	}
};

/********************************************
 Internal tables for predicate symbols
 ********************************************/

class StructureVisitor;

/**
 *	This class implements a concrete two-dimensional table
 */
class InternalPredTable {
protected:
	// Attributes
	unsigned int _nrRefs; //!< The number of references to this table

public:
	// Inspectors
	virtual bool finite(const Universe&) const = 0; //!< Returns true iff the table is finite
	virtual bool empty(const Universe&) const = 0; //!< Returns true iff the table is empty

	virtual bool approxFinite(const Universe&) const = 0;
	//!< Returns false if the table size is infinite. May return true if the table size is finite.
	virtual bool approxEmpty(const Universe&) const = 0;
	//!< Returns false if the table is non-empty. May return true if the table is empty.
	virtual bool approxEqual(const InternalPredTable*, const Universe&) const;
	//!< Returns false if the tables are different. May return true if the tables are equal.
	virtual bool approxInverse(const InternalPredTable*, const Universe&) const;
	//!< Returns false if the tables are not each other inverse. May return true if the tables are complementary.


	virtual bool contains(const ElementTuple& tuple, const Universe&) const = 0;
	//!< Returns true iff the table contains the tuple.
	virtual tablesize size(const Universe&) const = 0;

	// Mutators
	virtual InternalPredTable* add(const ElementTuple& tuple) = 0; //!< Add a tuple to the table
	virtual InternalPredTable* remove(const ElementTuple& tuple) = 0; //!< Remove a tuple from the table

	void decrementRef(); //!< Delete one reference. Deletes the table if the number of references becomes zero.
	void incrementRef(); //!< Add one reference

	// Iterators
	// TODO: what object is responsible for the memory management of this iterator?
	virtual InternalTableIterator* begin(const Universe&) const = 0;

	InternalPredTable() :
			_nrRefs(0) {
	}

	// Visitor
	virtual void accept(StructureVisitor* v) const = 0;

	friend class PredTable;
	friend class SortTable;

	virtual void put(std::ostream&) const;

protected:
	virtual ~InternalPredTable() {
	}
};

class ProcInternalPredTable: public InternalPredTable {
private:
	std::string* _procedure;
public:
	ProcInternalPredTable(std::string* proc) :
			InternalPredTable(), _procedure(proc) {
	}

	~ProcInternalPredTable();

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe& univ) const;

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalPredTable* add(const ElementTuple& tuple); //!< Add a tuple to the table
	InternalPredTable* remove(const ElementTuple& tuple); //!< Remove a tuple from the table

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class FOBDDManager;
class FOBDD;
class Structure;

class BDDInternalPredTable: public InternalPredTable {
private:
	std::shared_ptr<FOBDDManager> _manager;
	const FOBDD* _bdd;
	std::vector<Variable*> _vars;
	const Structure* _structure;
public:
	BDDInternalPredTable(const FOBDD*, std::shared_ptr<FOBDDManager>, const std::vector<Variable*>&, const Structure*);
	~BDDInternalPredTable();

	std::shared_ptr<FOBDDManager> manager() const {
		return _manager;
	}
	const FOBDD* bdd() const {
		return _bdd;
	}
	const std::vector<Variable*>& vars() const {
		return _vars;
	}
	const Structure* structure() const {
		return _structure;
	}

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	bool approxEqual(const InternalPredTable*, const Universe&) const;
	bool approxInverse(const InternalPredTable*, const Universe&) const;
	tablesize size(const Universe&) const;

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalPredTable* add(const ElementTuple& tuple);
	InternalPredTable* remove(const ElementTuple& tuple);

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class FullInternalPredTable: public InternalPredTable {
private:
public:
	FullInternalPredTable() :
			InternalPredTable() {
	}

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe& univ) const {
		return univ.size();
	}

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalPredTable* add(const ElementTuple& tuple); //!< Add a tuple to the table
	InternalPredTable* remove(const ElementTuple& tuple); //!< Remove a tuple from the table

	InternalTableIterator* begin(const Universe&) const;

	~FullInternalPredTable();

	// Visitor
	void accept(StructureVisitor* v) const;
};

class FuncTable;

class FuncInternalPredTable: public InternalPredTable {
private:
	FuncTable* _table;
	bool _linked;
public:
	FuncInternalPredTable(FuncTable* table, bool linked);

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe& univ) const;

	bool contains(const ElementTuple& tuple, const Universe&) const;
	FuncTable* table() const {
		return _table;
	}

	InternalPredTable* add(const ElementTuple& tuple); //!< Add a tuple to the table
	InternalPredTable* remove(const ElementTuple& tuple); //!< Remove a tuple from the table

	InternalTableIterator* begin(const Universe&) const;

	~FuncInternalPredTable();

	// Visitor
	void accept(StructureVisitor* v) const;
};

class PredTable;

/**
 *	This class implements table that consists of all tuples that belong to the union of a set of tables,
 *	but not to the union of another set of tables.
 *
 *	INVARIANT: the first table of _intables and of _outtables has an enumerated internal table
 */
class UnionInternalPredTable: public InternalPredTable {
private:
	std::vector<InternalPredTable*> _intables;
	//!< a tuple of the table does belong to at least one of the tables in _intables
	std::vector<InternalPredTable*> _outtables;
	//!< a tuple of the table does not belong to any of the tables in _outtables

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalTableIterator* begin(const Universe&) const;

public:
	const std::vector<InternalPredTable*>& inTables() const {
		return _intables;
	}
	const std::vector<InternalPredTable*>& outTables() const {
		return _outtables;
	}

	UnionInternalPredTable();
	UnionInternalPredTable(const std::vector<InternalPredTable*>& in, const std::vector<InternalPredTable*>& out);
	~UnionInternalPredTable();
	void addInTable(InternalPredTable* t) {
		_intables.push_back(t);
		t->incrementRef();
	}
	void addOutTable(InternalPredTable* t) {
		_outtables.push_back(t);
		t->incrementRef();
	}
	InternalPredTable* add(const ElementTuple& tuple);
	InternalPredTable* remove(const ElementTuple& tuple);

	// Visitor
	void accept(StructureVisitor* v) const;
};

/**
 *	This class implements a finite, enumerated InternalPredTable
 */
class EnumeratedInternalPredTable: public InternalPredTable {
private:
	SortedElementTable _table; //!< the actual table

	bool finite(const Universe&) const {
		return true;
	}
	bool empty(const Universe&) const {
		return _table.empty();
	}
	bool approxFinite(const Universe&) const {
		return true;
	}
	bool approxEmpty(const Universe&) const {
		return _table.empty();
	}
	tablesize size(const Universe&) const {
		return tablesize(TST_EXACT, _table.size());
	}

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalTableIterator* begin(const Universe&) const;

public:
	EnumeratedInternalPredTable(const SortedElementTable& tab) :
			InternalPredTable(), _table(tab) {
	}
	EnumeratedInternalPredTable() :
			InternalPredTable() {
	}
	~EnumeratedInternalPredTable() {
	}
	EnumeratedInternalPredTable* add(const ElementTuple& tuple);
	EnumeratedInternalPredTable* remove(const ElementTuple& tuple);

	// Visitor
	void accept(StructureVisitor* v) const;
};

class InternalSortTable;

/**
 *	Abstract base class for implementing InternalPredTable for '=/2', '</2', and '>/2'
 */
class ComparisonInternalPredTable: public InternalPredTable {
protected:
public:
	ComparisonInternalPredTable();
	virtual ~ComparisonInternalPredTable();
	InternalPredTable* add(const ElementTuple& tuple);
	InternalPredTable* remove(const ElementTuple& tuple);
};

/**
 *	Internal table for '=/2'
 */
class EqualInternalPredTable: public ComparisonInternalPredTable {
public:
	EqualInternalPredTable() :
			ComparisonInternalPredTable() {
	}
	~EqualInternalPredTable() {
	}

	bool contains(const ElementTuple&, const Universe&) const;
	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/**
 *	Internal table for '</2'
 */
class StrLessInternalPredTable: public ComparisonInternalPredTable {
public:
	StrLessInternalPredTable() :
			ComparisonInternalPredTable() {
	}
	~StrLessInternalPredTable() {
	}

	bool contains(const ElementTuple&, const Universe&) const;
	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/**
 *	Internal table for '>/2'
 */
class StrGreaterInternalPredTable: public ComparisonInternalPredTable {
public:
	StrGreaterInternalPredTable() :
			ComparisonInternalPredTable() {
	}
	~StrGreaterInternalPredTable() {
	}

	bool contains(const ElementTuple&, const Universe&) const;
	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;

};


/**
 *	This class implements the complement of an internal predicate table
 */
class InverseInternalPredTable: public InternalPredTable {
private:
	InternalPredTable* _invtable; //!< the inverse of the actual table

private:
	InverseInternalPredTable(InternalPredTable* inv);
public:
	static InternalPredTable* getInverseTable(InternalPredTable* inv);
	~InverseInternalPredTable();

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	bool approxInverse(const InternalPredTable*, const Universe&) const;
	tablesize size(const Universe&) const;
	InternalPredTable* table() const {
		return _invtable;
	}

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalPredTable* add(const ElementTuple& tuple);
	InternalPredTable* remove(const ElementTuple& tuple);

	InternalTableIterator* begin(const Universe&) const;

	void internTable(InternalPredTable*);

	// Visitor
	void accept(StructureVisitor* v) const;
};

/********************************
 Internal tables for sorts
 ********************************/

/**
 *	This class implements a concrete one-dimensional table
 */
class InternalSortTable: public InternalPredTable {
protected:

public:
	InternalSortTable() {
	}

	unsigned int arity() const {
		return 1;
	}

	virtual bool contains(const DomainElement*) const = 0;
	bool contains(const ElementTuple& tuple) const {
		return contains(tuple[0]);
	}
	bool contains(const ElementTuple& tuple, const Universe&) const {
		return contains(tuple[0]);
	}

	virtual bool empty() const = 0;
	virtual bool finite() const = 0;
	virtual bool approxEmpty() const = 0;
	virtual bool approxFinite() const = 0;
	virtual tablesize size() const = 0;
	bool empty(const Universe&) const {
		return empty();
	}
	bool finite(const Universe&) const {
		return finite();
	}
	bool approxEmpty(const Universe&) const {
		return approxEmpty();
	}
	bool approxFinite(const Universe&) const {
		return approxFinite();
	}
	tablesize size(const Universe&) const {
		return size();
	}

	virtual InternalSortTable* add(const DomainElement*) = 0;
	InternalSortTable* add(const ElementTuple& tuple) {
		return add(tuple[0]);
	}
	virtual InternalSortTable* remove(const DomainElement*) = 0;
	InternalSortTable* remove(const ElementTuple& tuple) {
		return remove(tuple[0]);
	}
	virtual InternalSortTable* add(int i1, int i2) = 0;

	virtual InternalSortIterator* sortBegin() const = 0;
	virtual InternalSortIterator* sortIterator(const DomainElement*) const = 0;
	InternalTableIterator* begin() const;
	InternalTableIterator* begin(const Universe&) const {
		return begin();
	}

	// Returns true if non-empty and is a range
	virtual bool isRange() const = 0;
	// Returns NULL if empty or not a range
	virtual const DomainElement* first() const = 0;
	// Returns NULL if empty or not a range
	virtual const DomainElement* last() const = 0;

	// Visitor
	virtual void accept(StructureVisitor* v) const = 0;

protected:
	// SortTable is responsible for ref management and deletion!
	virtual ~InternalSortTable() {
	}

};

class FullInternalSortTable: public InternalSortTable {
public:
	FullInternalSortTable() {
	}

	virtual bool contains(const DomainElement*) const{
		return true;
	}

	virtual bool empty() const {
		return false;
	}
	virtual bool finite() const {
		return false;
	}
	virtual bool approxEmpty() const {
		return false;
	}
	virtual bool approxFinite() const {
		return false;
	}
	virtual tablesize size() const {
		tablesize ts;
		ts._size=0;
		ts._type=TST_INFINITE;
		return ts;
	}

	virtual InternalSortTable* add(const DomainElement*) {
		return this;
	}
	virtual InternalSortTable* remove(const DomainElement*){
		throw IdpException("Cannot remove from the universal table.");
	}
	virtual InternalSortTable* add(int, int){
		return this;
	}

	virtual InternalSortIterator* sortBegin() const{
		throw notyetimplemented("Iterating over a FullInternalSortTable.");
	}
	virtual InternalSortIterator* sortIterator(const DomainElement*) const{
		throw notyetimplemented("Iterating over a FullInternalSortTable.");
	}

	// Returns true if non-empty and is a range
	virtual bool isRange() const{
		return false;
	}
	// Returns NULL if empty or not a range
	virtual const DomainElement* first() const{
		return NULL;
	}
	// Returns NULL if empty or not a range
	virtual const DomainElement* last() const{
		return NULL;
	}

	// Visitor
	virtual void accept(StructureVisitor* v) const;

	virtual ~FullInternalSortTable() {
	}
};

class UnionInternalSortTable: public InternalSortTable {
public:
	std::vector<SortTable*> _intables;
	//!< an element of the table does belong to at least one of the tables in _intables
	std::vector<SortTable*> _outtables;
	//!< an element of the table does not belong to any of the tables in _outtables

	bool finite() const;
	bool empty() const;
	bool approxFinite() const;
	bool approxEmpty() const;
	tablesize size() const;

	bool contains(const DomainElement*) const;

	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~UnionInternalSortTable();

public:
	UnionInternalSortTable();
	UnionInternalSortTable(const std::vector<SortTable*>& in, const std::vector<SortTable*>& out) :
			_intables(in), _outtables(out) {
	}
	void addInTable(SortTable* t) {
		_intables.push_back(t);
	}
	void addOutTable(SortTable* t) {
		_outtables.push_back(t);
	}
	InternalSortTable* add(const DomainElement*);
	InternalSortTable* remove(const DomainElement*);
	InternalSortTable* add(int i1, int i2);

	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class InfiniteInternalSortTable: public InternalSortTable {
public:
	InternalSortTable* add(const DomainElement*);
	InternalSortTable* remove(const DomainElement*);
	bool finite() const {
		return false;
	}
	bool empty() const {
		return false;
	}
	bool approxFinite() const {
		return false;
	}
	bool approxEmpty() const {
		return false;
	}
	tablesize size() const {
		return tablesize(TST_INFINITE, 0);
	}
protected:
	// SortTable is responsible for ref management and deletion!
	virtual ~InfiniteInternalSortTable() {
	}
};

/**
 *	All natural numbers
 */
class AllNaturalNumbers: public InfiniteInternalSortTable {
public:
	bool contains(const DomainElement*) const;
	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const {
		return true;
	}

	// Visitor
	void accept(StructureVisitor* v) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~AllNaturalNumbers() {
	}
};

/**
 * All integers
 */
class AllIntegers: public InfiniteInternalSortTable {
public:
	bool contains(const DomainElement*) const;
	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const {
		return true;
	}

	// Visitor
	void accept(StructureVisitor* v) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~AllIntegers() {
	}
};

/**
 * All floating point numbers
 */
class AllFloats: public InfiniteInternalSortTable {
public:
	bool contains(const DomainElement*) const;
	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const {
		return true;
	}

	// Visitor
	void accept(StructureVisitor* v) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~AllFloats() {
	}
};

/**
 * All strings
 */
class AllStrings: public InfiniteInternalSortTable {
public:
	bool contains(const DomainElement*) const;
	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const {
		return true;
	}

	// Visitor
	void accept(StructureVisitor* v) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~AllStrings() {
	}
};

/**
 * All characters
 */
class AllChars: public InternalSortTable {
public:
	bool contains(const DomainElement*) const;
	InternalSortTable* add(const DomainElement*);
	InternalSortTable* remove(const DomainElement*);
	InternalSortTable* add(int i1, int i2);

	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	bool finite() const {
		return true;
	}
	bool empty() const {
		return false;
	}
	bool approxFinite() const {
		return true;
	}
	bool approxEmpty() const {
		return false;
	}
	tablesize size() const;

	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const {
		return true;
	}

	// Visitor
	void accept(StructureVisitor* v) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~AllChars() {
	}
};

class IntRangeInternalSortTable;

/**
 *	A finite, enumerated SortTable
 */
class EnumeratedInternalSortTable: public InternalSortTable {
private:
	SortedElementTuple _table;
	int nbNotIntElements; // Used for efficient range operations

	bool contains(const DomainElement*) const;

	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	bool finite() const {
		return approxFinite();
	}
	bool empty() const {
		return approxEmpty();
	}
	bool approxFinite() const {
		return true;
	}
	bool approxEmpty() const {
		return _table.empty();
	}
	tablesize size() const {
		return tablesize(TST_EXACT, _table.size());
	}
	void countNBNotIntElements();

protected:
	// SortTable is responsible for ref management and deletion!
	~EnumeratedInternalSortTable() {
	}

	friend class IntRangeInternalSortTable;
	void addNonRange(const DomainElement*  elem, int start, int end);
	void addNonRange(int start1, int end1, int start2, int end2);

public:
	EnumeratedInternalSortTable(): nbNotIntElements(0) {
	}
	EnumeratedInternalSortTable(const SortedElementTuple& d)
			: 	_table(d),
				nbNotIntElements(0) {
		countNBNotIntElements();
	}
	InternalSortTable* add(const DomainElement*);
	InternalSortTable* remove(const DomainElement*);
	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;
	/**
	 * Returns true if the table is not empty, only contains integers and the integers cover a full interval and nothing outside
	 */
	bool isRange() const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/**
 *		A range of integers
 */
class IntRangeInternalSortTable: public InternalSortTable {
private:
	int _first; //!< first element in the range
	int _last; //!< last element in the range
protected:
	~IntRangeInternalSortTable() {
	}
public:
	// Takes the non-empty interval, regardless of the order
	IntRangeInternalSortTable(int f, int l) :
			_first(f), _last(l) {
		if(_first>_last){
			_last = f;
			_first = l;
		}
	}
	bool finite() const {
		return approxFinite();
	}
	bool empty() const {
		return approxEmpty();
	}
	bool approxFinite() const {
		return true;
	}
	bool approxEmpty() const {
		return _first > _last;
	}
	tablesize size() const {
		return tablesize(TST_EXACT, _last - _first + 1);
	}
	InternalSortTable* add(const DomainElement*);
	InternalSortTable* remove(const DomainElement*);
	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;

	bool contains(const DomainElement*) const;
	bool isRange() const {
		return true;
	}

	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/**
 *		A set of constructor functions generating the InternalSortTable
 */
class ConstructedInternalSortTable: public InternalSortTable {
private:
	// Note: implementing this as a list of FuncTables or InternalFuncTables will result in a bug,
	// since the construction of such tables requires the outSort's interpretation (and hence this table) to be known.
	const std::vector<Function*> _constructors;
	const Structure* _struc;

	FuncTable* getTable(const Function* f) const{
		return _struc->inter(f)->funcTable();
	}

protected:
	~ConstructedInternalSortTable() {
	}
public:
	ConstructedInternalSortTable(const Structure* s, const std::vector<Function*>& funcs) :
			_constructors(funcs),
			_struc(s){
	}
	bool isRecursive() const;

	bool finite() const;
	bool empty() const;
	bool approxFinite() const;
	bool approxEmpty() const;
	tablesize size() const;

	InternalSortTable* add(const DomainElement*){
		throw notyetimplemented("Addition of domain elements to a constructed sort is not yet implemented.");
	}
	InternalSortTable* remove(const DomainElement*){
		throw notyetimplemented("Removal of domain elements to a constructed sort is not yet implemented.");
	}
	InternalSortTable* add(int, int){
		throw notyetimplemented("Addition of domain elements to a constructed sort is not yet implemented.");
	}

	//NOTE: what happens when the sort is empty?
	const DomainElement* first() const;

	//NOTE: what happens when the sort is empty?
	const DomainElement* last() const;

	bool contains(const DomainElement*) const;
	bool isRange() const {
		return false;
	}

	int nrOfConstructors() const {
		return _constructors.size();
	}

	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/************************************
 Internal tables for functions
 ************************************/

/**
 *		This class implements a concrete associative array mapping tuples of elements to elements
 */
class InternalFuncTable {
protected:
	unsigned int _nrRefs;
public:
	InternalFuncTable() :
			_nrRefs(0) {
	}
	virtual ~InternalFuncTable() {
	}

	void decrementRef(); //!< Delete one reference. Deletes the table if the number of references becomes zero.
	void incrementRef(); //!< Add one reference

	virtual bool finite(const Universe&) const = 0; //!< Returns true iff the table is finite
	virtual bool empty(const Universe&) const = 0; //!< Returns true iff the table is empty

	virtual bool approxFinite(const Universe&) const = 0;
	//!< Returns false if the table size is infinite. May return true if the table size is finite.
	virtual bool approxEmpty(const Universe&) const = 0;
	//!< Returns false if the table is non-empty. May return true if the table is empty.
	virtual tablesize size(const Universe&) const = 0;

	bool contains(const ElementTuple& tuple) const;

	// Returns the value of the function given the instantiation tuple.
	// NOTE: type check has already been done externally
	virtual const DomainElement* operator[](const ElementTuple& tuple) const = 0;

	virtual InternalFuncTable* add(const ElementTuple&) = 0; //!< Add a tuple to the table
	virtual InternalFuncTable* remove(const ElementTuple&) = 0; //!< Remove a tuple from the table

	virtual InternalTableIterator* begin(const Universe&) const = 0;

	// Visitor
	virtual void accept(StructureVisitor* v) const = 0;

	virtual void put(std::ostream& stream) const;
};

class ProcInternalFuncTable: public InternalFuncTable {
private:
	std::string* _procedure;
public:
	ProcInternalFuncTable(std::string* proc) :
			InternalFuncTable(), _procedure(proc) {
	}

	~ProcInternalFuncTable();

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	const DomainElement* operator[](const ElementTuple& tuple) const;
	InternalFuncTable* add(const ElementTuple&);
	InternalFuncTable* remove(const ElementTuple&);

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class UNAInternalFuncTable: public InternalFuncTable {
private:
	Function* _function;
public:
	UNAInternalFuncTable(Function* f) : _function(f) {
	}

	Function* getFunction() const {
		return _function;
	}

	// Correctness of constructed iterator code depends on the fact that "empty" and "approxFinite" of constructed functions do NOT work by requesting an iterator
	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	const DomainElement* operator[](const ElementTuple& tuple) const;
	InternalFuncTable* add(const ElementTuple&);
	InternalFuncTable* remove(const ElementTuple&);

	InternalTableIterator* begin(const Universe&) const;

	void accept(StructureVisitor* v) const;
};

/**
 *		A finite, enumerated InternalFuncTable
 */
class EnumeratedInternalFuncTable: public InternalFuncTable {
private:
	Tuple2Elem _table;
public:
	EnumeratedInternalFuncTable() :
			InternalFuncTable() {
	}
	EnumeratedInternalFuncTable(const Tuple2Elem& tab) :
			InternalFuncTable(), _table(tab) {
	}
	virtual ~EnumeratedInternalFuncTable() {
	}

	bool finite(const Universe&) const {
		return true;
	}
	bool empty(const Universe&) const {
		return _table.empty();
	}
	bool approxFinite(const Universe&) const {
		return true;
	}
	bool approxEmpty(const Universe&) const {
		return _table.empty();
	}
	tablesize size(const Universe&) const {
		return tablesize(TST_EXACT, _table.size());
	}

	const DomainElement* operator[](const ElementTuple& tuple) const;
	EnumeratedInternalFuncTable* add(const ElementTuple&);
	EnumeratedInternalFuncTable* remove(const ElementTuple&);

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;

	virtual void put(std::ostream& stream) const;

};

class IntFloatInternalFuncTable: public InternalFuncTable {
protected:
	NumType _type;
public:

	IntFloatInternalFuncTable(bool i) :
			_type(i ? NumType::CERTAINLYINT : NumType::POSSIBLYINT) {
	}

	bool finite(const Universe&) const {
		return false;
	}
	bool empty(const Universe&) const {
		return false;
	}
	bool approxFinite(const Universe&) const {
		return false;
	}
	bool approxEmpty(const Universe&) const {
		return false;
	}
	tablesize size(const Universe&) const {
		return tablesize(TST_INFINITE, 0);
	}

	NumType getType() const {
		return _type;
	}

	InternalFuncTable* add(const ElementTuple&);
	InternalFuncTable* remove(const ElementTuple&);

	virtual InternalTableIterator* begin(const Universe&) const = 0;
};

class PlusInternalFuncTable: public IntFloatInternalFuncTable {
public:
	PlusInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class MinusInternalFuncTable: public IntFloatInternalFuncTable {
public:
	MinusInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class TimesInternalFuncTable: public IntFloatInternalFuncTable {
public:
	TimesInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class DivInternalFuncTable: public IntFloatInternalFuncTable {
public:
	DivInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class AbsInternalFuncTable: public IntFloatInternalFuncTable {
public:
	AbsInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class UminInternalFuncTable: public IntFloatInternalFuncTable {
public:
	UminInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class ExpInternalFuncTable: public InternalFuncTable {
public:
	bool finite(const Universe&) const {
		return false;
	}
	bool empty(const Universe&) const {
		return false;
	}
	bool approxFinite(const Universe&) const {
		return false;
	}
	bool approxEmpty(const Universe&) const {
		return false;
	}
	tablesize size(const Universe&) const {
		return tablesize(TST_INFINITE, 0);
	}
	const DomainElement* operator[](const ElementTuple&) const;

	InternalFuncTable* add(const ElementTuple&);
	InternalFuncTable* remove(const ElementTuple&);

	InternalTableIterator* begin(const Universe&) const;

	ExpInternalFuncTable() {
	}
	~ExpInternalFuncTable() {
	}

	// Visitor
	void accept(StructureVisitor* v) const;
};

class ModInternalFuncTable: public InternalFuncTable {
public:
	ModInternalFuncTable() {
	}
	~ModInternalFuncTable() {
	}
	bool finite(const Universe&) const {
		return false;
	}
	bool empty(const Universe&) const {
		return false;
	}
	bool approxFinite(const Universe&) const {
		return false;
	}
	bool approxEmpty(const Universe&) const {
		return false;
	}
	tablesize size(const Universe&) const {
		return tablesize(TST_INFINITE, 0);
	}
	const DomainElement* operator[](const ElementTuple&) const;

	InternalFuncTable* add(const ElementTuple&);
	InternalFuncTable* remove(const ElementTuple&);

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/**********************
 Interpretations
 *********************/

class Structure;

class PredInterGenerator {
public:
	virtual PredInter* get(const Structure* structure) = 0;
	virtual ~PredInterGenerator() {
	}
};

class SinglePredInterGenerator: public PredInterGenerator {
private:
	PredInter* _inter;
public:
	SinglePredInterGenerator(PredInter* inter) :
			_inter(inter) {
	}
	~SinglePredInterGenerator() {
		delete (_inter);
	}
	PredInter* get(const Structure*) {
		return _inter;
	}
};

class Sort;
class Predicate;

class InconsistentPredInterGenerator: public PredInterGenerator {
private:
	Predicate* _predicate;
public:
	InconsistentPredInterGenerator(Predicate* predicate) :
			_predicate(predicate) {
	}
	PredInter* get(const Structure* structure);
};

class EqualInterGenerator: public PredInterGenerator {
private:
	Sort* _sort;
	std::vector<PredInter*> generatedInters;
public:
	EqualInterGenerator(Sort* sort) :
			_sort(sort) {
	}
	~EqualInterGenerator();
	PredInter* get(const Structure* structure);
};

class StrLessThanInterGenerator: public PredInterGenerator {
private:
	Sort* _sort;
	std::vector<PredInter*> generatedInters;
public:
	StrLessThanInterGenerator(Sort* sort) :
			_sort(sort) {
	}
	~StrLessThanInterGenerator();
	PredInter* get(const Structure* structure);
};

class StrGreaterThanInterGenerator: public PredInterGenerator {
private:
	Sort* _sort;
	std::vector<PredInter*> generatedInters;
public:
	StrGreaterThanInterGenerator(Sort* sort) :
			_sort(sort) {
	}
	~StrGreaterThanInterGenerator();
	PredInter* get(const Structure* structure);
};

class PredInterGeneratorGenerator {
public:
	virtual PredInterGenerator* get(const std::vector<Sort*>&) = 0;
	virtual ~PredInterGeneratorGenerator() {
	}
};

class EqualInterGeneratorGenerator: public PredInterGeneratorGenerator {
public:
	EqualInterGenerator* get(const std::vector<Sort*>&);
};

class StrGreaterThanInterGeneratorGenerator: public PredInterGeneratorGenerator {
public:
	StrGreaterThanInterGenerator* get(const std::vector<Sort*>&);
};

class StrLessThanInterGeneratorGenerator: public PredInterGeneratorGenerator {
public:
	StrLessThanInterGenerator* get(const std::vector<Sort*>&);
};

class FuncInterGenerator {
public:
	virtual FuncInter* get(const Structure* structure) = 0;
	virtual ~FuncInterGenerator() {
	}
};

class SingleFuncInterGenerator: public FuncInterGenerator {
private:
	FuncInter* _inter;
public:
	SingleFuncInterGenerator(FuncInter* inter) :
			_inter(inter) {
	}
	~SingleFuncInterGenerator() {
		delete (_inter);
	}
	FuncInter* get(const Structure*) {
		return _inter;
	}
};

class ConstructorFuncInterGenerator: public FuncInterGenerator {
private:
	Function* _function;
public:
	ConstructorFuncInterGenerator(Function* function) :
			_function(function) {
	}
	FuncInter* get(const Structure* structure);
};

class OneSortInterGenerator: public FuncInterGenerator {
protected:
	Sort* _sort;
public:
	OneSortInterGenerator(Sort* sort) :
			_sort(sort) {
	}
};

class MinInterGenerator: public OneSortInterGenerator {
public:
	MinInterGenerator(Sort* sort) :
			OneSortInterGenerator(sort) {
	}
	FuncInter* get(const Structure* structure);
};

class MaxInterGenerator: public OneSortInterGenerator {
public:
	MaxInterGenerator(Sort* sort) :
			OneSortInterGenerator(sort) {
	}
	FuncInter* get(const Structure* structure);
};

class SuccInterGenerator: public OneSortInterGenerator {
public:
	SuccInterGenerator(Sort* sort) :
			OneSortInterGenerator(sort) {
	}
	FuncInter* get(const Structure* structure);
};

class InvSuccInterGenerator: public OneSortInterGenerator {
public:
	InvSuccInterGenerator(Sort* sort) :
			OneSortInterGenerator(sort) {
	}
	FuncInter* get(const Structure* structure);
};

class FuncInterGeneratorGenerator {
public:
	virtual FuncInterGenerator* get(const std::vector<Sort*>&) = 0;
	virtual ~FuncInterGeneratorGenerator() {
	}
};

class MinInterGeneratorGenerator: public FuncInterGeneratorGenerator {
public:
	MinInterGenerator* get(const std::vector<Sort*>&);
};

class MaxInterGeneratorGenerator: public FuncInterGeneratorGenerator {
public:
	MaxInterGenerator* get(const std::vector<Sort*>&);
};

class SuccInterGeneratorGenerator: public FuncInterGeneratorGenerator {
public:
	SuccInterGenerator* get(const std::vector<Sort*>&);
};

class InvSuccInterGeneratorGenerator: public FuncInterGeneratorGenerator {
public:
	InvSuccInterGenerator* get(const std::vector<Sort*>&);
};

// Table utils:

bool intersectionEmpty(PredTable* left, PredTable* right);
bool isConsistentWith(PredTable* table, PredInter* inter);
bool isConsistentWith(PredInter* inter, PredInter* inter2);

#endif
