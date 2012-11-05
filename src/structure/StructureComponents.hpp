/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef STRUCTURECOMPONENTS_HPP
#define STRUCTURECOMPONENTS_HPP

#include <cstdlib>
#include "parseinfo.hpp"
#include "common.hpp"

#include "fwstructure.hpp"

#include "GlobalData.hpp"
#include "utils/NumericLimits.hpp"
#include "MainStructureComponents.hpp"

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

class DomainAtomFactory;
class PFSymbol;
class Variable;
class Vocabulary;
class AbstractStructure;

class DomainAtom {
private:
	PFSymbol* _symbol;
	ElementTuple _args;

	DomainAtom(PFSymbol* symbol, const ElementTuple& args) :
			_symbol(symbol), _args(args) {
	}

public:
	~DomainAtom() {
	}

	PFSymbol* symbol() const {
		return _symbol;
	}
	const ElementTuple& args() const {
		return _args;
	}

	std::ostream& put(std::ostream&) const;
	std::string toString() const;

	friend class DomainAtomFactory;
};

class DomainAtomFactory {
private:
	static DomainAtomFactory* _instance;
	std::map<PFSymbol*, std::map<ElementTuple, DomainAtom*> > _atoms;
	DomainAtomFactory() {
	}

public:
	~DomainAtomFactory();
	static DomainAtomFactory* instance();
	const DomainAtom* create(PFSymbol*, const ElementTuple&);
};

// ITERATORS

class InternalTableIterator {
private:
	virtual bool hasNext() const = 0;
	virtual const ElementTuple& operator*() const = 0;
	virtual void operator++() = 0;
public:
	virtual ~InternalTableIterator() {
	}
	virtual InternalTableIterator* clone() const = 0;
	friend class TableIterator;
};

class CartesianInternalTableIterator: public InternalTableIterator {
private:
	std::vector<SortIterator> _iterators;
	std::vector<SortIterator> _lowest;
	mutable ElementTable _deref;
	bool _hasNext;
	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
public:
	CartesianInternalTableIterator(const std::vector<SortIterator>& vsi, const std::vector<SortIterator>& low, bool h = true);
	CartesianInternalTableIterator* clone() const;
};

class InstGenerator;

class GeneratorInternalTableIterator: public InternalTableIterator {
private:
	InstGenerator* _generator;
	std::vector<const DomElemContainer*> _vars;
	bool _hasNext;
	mutable ElementTable _deref;
	bool hasNext() const {
		return _hasNext;
	}
	const ElementTuple& operator*() const;
	void operator++();
public:
	GeneratorInternalTableIterator(InstGenerator* generator, const std::vector<const DomElemContainer*>& vars, bool reset = true, bool h = true);
	GeneratorInternalTableIterator* clone() const {
		return new GeneratorInternalTableIterator(_generator, _vars, false, _hasNext);
	}
};

class SortInternalTableIterator: public InternalTableIterator {
private:
	InternalSortIterator* _iter;
	mutable ElementTable _deref;
	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
public:
	SortInternalTableIterator(InternalSortIterator* isi) :
			_iter(isi) {
	}
	~SortInternalTableIterator();
	SortInternalTableIterator* clone() const;
};

class EnumInternalIterator: public InternalTableIterator {
private:
	SortedElementTable::const_iterator _iter;
	SortedElementTable::const_iterator _end;
	bool hasNext() const {
		return _iter != _end;
	}
	const ElementTuple& operator*() const {
		return *_iter;
	}
	void operator++() {
		++_iter;
	}
public:
	EnumInternalIterator(SortedElementTable::const_iterator it, SortedElementTable::const_iterator end) :
			_iter(it), _end(end) {
	}
	~EnumInternalIterator() {
	}
	EnumInternalIterator* clone() const;
};

class EnumInternalFuncIterator: public InternalTableIterator {
private:
	Tuple2Elem::const_iterator _iter;
	Tuple2Elem::const_iterator _end;
	mutable ElementTable _deref;
	bool hasNext() const {
		return _iter != _end;
	}
	const ElementTuple& operator*() const;
	void operator++() {
		++_iter;
	}
public:
	EnumInternalFuncIterator(Tuple2Elem::const_iterator it, Tuple2Elem::const_iterator end) :
			_iter(it), _end(end) {
	}
	~EnumInternalFuncIterator() {
	}
	EnumInternalFuncIterator* clone() const;
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
	bool hasNext() const {
		return not _curr.isAtEnd();
	}
	const ElementTuple& operator*() const;
	void operator++();
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
};

class InternalPredTable;

class ProcInternalTableIterator: public InternalTableIterator {
private:
	TableIterator _curr;
	Universe _univ;
	mutable ElementTable _deref;
	const InternalPredTable* _predicate;
	bool hasNext() const {
		return not _curr.isAtEnd();
	}
	const ElementTuple& operator*() const;
	void operator++();
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
};

class UnionInternalIterator: public InternalTableIterator {
private:
	std::vector<TableIterator> _iterators;
	Universe _universe;
	std::vector<InternalPredTable*> _outtables;
	std::vector<TableIterator>::iterator _curriterator;

	bool contains(const ElementTuple&) const;
	void setcurriterator();

	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
public:
	UnionInternalIterator(const std::vector<TableIterator>&, const std::vector<InternalPredTable*>&, const Universe&);
	~UnionInternalIterator() {
	}
	UnionInternalIterator* clone() const;
};

class UNAInternalIterator: public InternalTableIterator {
private:
	std::vector<SortIterator> _curr;
	std::vector<SortIterator> _lowest;
	Function* _function;
	mutable bool _end;
	mutable ElementTuple _currtuple;
	mutable std::vector<ElementTuple> _deref;

	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
	UNAInternalIterator(const std::vector<SortIterator>&, const std::vector<SortIterator>&, Function*, bool);
public:
	UNAInternalIterator(const std::vector<SortIterator>&, Function*);
	~UNAInternalIterator() {
	}
	UNAInternalIterator* clone() const;
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

	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
	InverseInternalIterator(const std::vector<SortIterator>&, const std::vector<SortIterator>&, InternalPredTable*, const Universe&, bool);
public:
	InverseInternalIterator(const std::vector<SortIterator>&, InternalPredTable*, const Universe&);
	~InverseInternalIterator() {
	}
	InverseInternalIterator* clone() const;
};

class EqualInternalIterator: public InternalTableIterator {
private:
	SortIterator _iterator;
	mutable ElementTable _deref;
	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
public:
	EqualInternalIterator(const SortIterator& iter);
	~EqualInternalIterator() {
	}
	EqualInternalIterator* clone() const;
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
class AbstractStructure;

class BDDInternalPredTable: public InternalPredTable {
private:
	FOBDDManager* _manager;
	const FOBDD* _bdd;
	std::vector<Variable*> _vars;
	const AbstractStructure* _structure;
public:
	BDDInternalPredTable(const FOBDD*, FOBDDManager*, const std::vector<Variable*>&, const AbstractStructure*);
	~BDDInternalPredTable() {
	}

	FOBDDManager* manager() const {
		return _manager;
	}
	const FOBDD* bdd() const {
		return _bdd;
	}
	const std::vector<Variable*>& vars() const {
		return _vars;
	}
	const AbstractStructure* structure() const {
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

	bool contains(const ElementTuple& tuple, const Universe&) const;
	virtual const DomainElement* operator[](const ElementTuple& tuple) const = 0;
	//!< Returns the value of the tuple according to the array.

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
	UNAInternalFuncTable(Function* f) :
			InternalFuncTable(), _function(f) {
	}

	~UNAInternalFuncTable() {
	}

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

class AbstractStructure;

class PredInterGenerator {
public:
	virtual PredInter* get(const AbstractStructure* structure) = 0;
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
	PredInter* get(const AbstractStructure*) {
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
	PredInter* get(const AbstractStructure* structure);
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
	PredInter* get(const AbstractStructure* structure);
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
	PredInter* get(const AbstractStructure* structure);
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
	PredInter* get(const AbstractStructure* structure);
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
	virtual FuncInter* get(const AbstractStructure* structure) = 0;
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
	FuncInter* get(const AbstractStructure*) {
		return _inter;
	}
};

class InconsistentFuncInterGenerator: public FuncInterGenerator {
private:
	Function* _function;
public:
	InconsistentFuncInterGenerator(Function* function) :
			_function(function) {
	}
	FuncInter* get(const AbstractStructure* structure);
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
	FuncInter* get(const AbstractStructure* structure);
};

class MaxInterGenerator: public OneSortInterGenerator {
public:
	MaxInterGenerator(Sort* sort) :
			OneSortInterGenerator(sort) {
	}
	FuncInter* get(const AbstractStructure* structure);
};

class SuccInterGenerator: public OneSortInterGenerator {
public:
	SuccInterGenerator(Sort* sort) :
			OneSortInterGenerator(sort) {
	}
	FuncInter* get(const AbstractStructure* structure);
};

class InvSuccInterGenerator: public OneSortInterGenerator {
public:
	InvSuccInterGenerator(Sort* sort) :
			OneSortInterGenerator(sort) {
	}
	FuncInter* get(const AbstractStructure* structure);
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

#endif
