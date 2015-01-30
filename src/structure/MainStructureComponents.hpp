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

#ifndef MAINSTRUCTURECOMPONENTS_HPP
#define MAINSTRUCTURECOMPONENTS_HPP

#include <cstdlib>
#include "parseinfo.hpp"
#include "common.hpp"

#include "fwstructure.hpp"

#include "GlobalData.hpp"
#include "utils/NumericLimits.hpp"

/**
 * NAMING CONVENTION
 *		- 'Interpretation' means a possibly three-valued, or even four-valued 
 *			interpretation for a symbol.
 *		- 'Table' means a two-value table
 *		- if a name of a methods begins with 'approx', the method is fast, but 
 *			provides a under approximation of the desired result.
 */

/**********************
 Domain elements
 **********************/

class Compound;
class DomainElementFactory;

std::ostream& operator<<(std::ostream&, const DomainElementType&);

class DomElemContainer {
private:
	mutable const DomainElement* domelem_;
	mutable bool del;
	static std::vector<const DomElemContainer*> containers;

	~DomElemContainer() {
		if (del) {
			delete (domelem_);
		}
	}

public:
	static void deleteAllContainers();
	DomElemContainer()
			: 	domelem_(new DomainElement()),
				del(true) {
		containers.push_back(this);
	}

	const DomElemContainer* operator=(const DomElemContainer* container) const {
		if (del) {
			delete (domelem_);
		}
		del = false;
		domelem_ = container->get();
		return this;
	}
	const DomElemContainer* operator=(const DomElemContainer& container) const {
		if (del) {
			delete (domelem_);
		}
		del = false;
		domelem_ = container.get();
		return this;
	}
	const DomElemContainer* operator=(const DomainElement* domelem) const {
		if (del) {
			delete (domelem_);
		}
		del = false;
		domelem_ = domelem;
		return this;
	}

	inline const DomainElement* get() const {
		return domelem_;
	}

	bool operator==(const DomElemContainer& right) const {
		return get() == right.get();
	}
	bool operator<(const DomElemContainer& right) const {
		return get() < right.get();
	}
	bool operator>(const DomElemContainer& right) const {
		return get() > right.get();
	}

	std::ostream& put(std::ostream& stream) const {
		return get()->put(stream);
	}
};

bool operator<(const DomainElement&, const DomainElement&);
inline bool operator>(const DomainElement& d1, const DomainElement& d2) {
	return d2 < d1;
}

inline bool operator==(const DomainElement& d1, const DomainElement& d2) {
	return &d1 == &d2; // FIXME: underapproximation of what is equal: just comparing pointers to get it fast, but NOT exact!
}

inline bool operator!=(const DomainElement& d1, const DomainElement& d2) {
	return not (d1 == d2);
}

inline bool operator<=(const DomainElement& d1, const DomainElement& d2) {
	return d1 == d2 || d1 < d2;
}

inline bool operator>=(const DomainElement& d1, const DomainElement& d2) {
	return d1 == d2 || d1 > d2;
}

std::ostream& operator<<(std::ostream&, const DomainElement&);

std::ostream& operator<<(std::ostream&, const ElementTuple&);

template<class T>
struct Compare {
	bool operator()(const T& t1, const T& t2) const {
		if (t1.size() < t2.size()) {
			return true;
		} else if (t1.size() > t2.size()) {
			return false; //TODO: right?
		}

		for (size_t n = 0; n < t1.size(); ++n) {
			if (*(t1[n]) < *(t2[n])) {
				return true;
			} else if (*(t1[n]) > *(t2[n])) {
				return false;
			}
		}
		return false;
	}
	bool operator()(const T* t1, const T* t2) const {
		return *t1 < *t2;
	}
};

typedef std::set<ElementTuple, Compare<ElementTuple> > SortedElementTable;
typedef std::set<const DomainElement*, Compare<DomainElement> > SortedElementTuple; // TODO naam trekt er niet op?
typedef std::map<ElementTuple, const DomainElement*, Compare<ElementTuple> > Tuple2Elem;

struct FirstNElementsEqual {
	unsigned int _arity;
	FirstNElementsEqual(unsigned int arity)
			: _arity(arity) {
	}
	bool operator()(const ElementTuple&, const ElementTuple&) const;
};

struct StrictWeakNTupleOrdering {
	unsigned int _arity;
	StrictWeakNTupleOrdering(unsigned int arity)
			: _arity(arity) {
	}
	bool operator()(const ElementTuple&, const ElementTuple&) const;
};

// Important: builds upon the fact that identical compounds always have identical pointers
bool operator<(const Compound&, const Compound&);
bool operator>(const Compound&, const Compound&);
bool operator==(const Compound&, const Compound&);
bool operator!=(const Compound&, const Compound&);
bool operator<=(const Compound&, const Compound&);
bool operator>=(const Compound&, const Compound&);

std::ostream& operator<<(std::ostream&, const Compound&);

/****************
 Iterators
 ****************/

class InternalTableIterator;
class InternalSortIterator;

/**
 * Constant iterator over tables for sorts, predicate, and function symbols.
 */
class TableIterator {
private:
	InternalTableIterator* _iterator;
public:
	TableIterator()
			: _iterator(0) {
	}
	TableIterator(const TableIterator&);
	TableIterator(InternalTableIterator* iter)
			: _iterator(iter) {
	}
	TableIterator& operator=(const TableIterator&);
	//bool					hasNext()	const;
	bool isAtEnd() const;
	const ElementTuple& operator*() const;
	void operator++();
	~TableIterator();
	const InternalTableIterator* iterator() const {
		return _iterator;
	}
};

class SortIterator {
private:
	InternalSortIterator* _iterator;
public:
	SortIterator(InternalSortIterator* iter)
			: _iterator(iter) {
		Assert(iter!=NULL);
	}
	SortIterator(const SortIterator&);
	SortIterator& operator=(const SortIterator&);
//		bool					hasNext()	const;
	bool isAtEnd() const;
	const DomainElement* operator*() const;
	SortIterator& operator++();
	~SortIterator();
	const InternalSortIterator* iterator() const {
		return _iterator;
	}
};

/*********************************************************
 Tables for sorts, predicates, and function symbols
 *********************************************************/

/**
 *	This class implements the common functionality of tables for sorts, predicate, and function symbols.
 */
class AbstractTable {
public:
	virtual ~AbstractTable() {
	}

	virtual bool finite() const = 0; //!< Returns true iff the table is finite
	virtual bool empty() const = 0; //!< Returns true iff the table is empty
	virtual unsigned int arity() const = 0; //!< Returns the number of columns in the table

	virtual bool approxFinite() const = 0;
	//!< Returns false if the table size is infinite. May return true if the table size is finite.
	virtual bool approxEmpty() const = 0;
	//!< Returns false if the table is non-empty. May return true if the table is empty.

	virtual bool contains(const ElementTuple& tuple) const = 0;
	//!< Returns true iff the table contains the tuple.

	virtual AbstractTable* materialize() const = 0; //!< Try to replace a symbolic table by an enumerated one.
													//!< Returns 0 in case this is impossible, or the table was
													//!< already enumerated

	virtual void add(const ElementTuple& tuple, bool ignoresortsortchecks = false) = 0; //!< Add a tuple to the table
	virtual void remove(const ElementTuple& tuple) = 0; //!< Remove a tuple from the table

	virtual TableIterator begin() const = 0;

	virtual void put(std::ostream& stream) const = 0;
};

class InternalPredTable;

/**
 *	This class implements tables for predicate symbols.
 */
class PredTable: public AbstractTable {
private:
	InternalPredTable* _table; //!< Points to the actual table
	Universe _universe;
public:
	PredTable(InternalPredTable* table, const Universe&);
	~PredTable();

	bool finite() const;
	bool empty() const;
	unsigned int arity() const;
	bool approxFinite() const;
	bool approxEmpty() const;
	bool approxEqual(const PredTable*) const;
	bool approxInverse(const PredTable*) const;
	bool contains(const ElementTuple& tuple) const;
	tablesize size() const;
	void add(const ElementTuple& tuple, bool ignoresortsortchecks = false);
	void remove(const ElementTuple& tuple);

	TableIterator begin() const;

	const Universe& universe() const;
	PredTable* materialize() const; // Returns NULL if no materialization is possible OR if it would result in the same table

	virtual void put(std::ostream& stream) const;

	InternalPredTable* internTable() const;

private:
	void setTable(InternalPredTable* table);
};

class InternalSortTable;

/**
 *		This class implements tables for sorts
 */
class SortTable: public AbstractTable {
private:
	InternalSortTable* _table; //!< Points to the actual table
public:
	SortTable(InternalSortTable* table);
	~SortTable();

	void internTable(InternalSortTable*);

	bool finite() const;
	bool empty() const;
	bool approxFinite() const;
	bool approxEmpty() const;
	unsigned int arity() const;
	tablesize size() const;
	bool contains(const ElementTuple& tuple) const;
	bool contains(const DomainElement* el) const;
	void add(const ElementTuple& tuple, bool ignoresortsortchecks = false);
	void add(const DomainElement* el);
	void add(int i1, int i2);
	void remove(const ElementTuple& tuple);
	void remove(const DomainElement* el);
	TableIterator begin() const;
	SortIterator sortBegin() const;
	SortIterator sortIterator(const DomainElement*) const;

	// Returns true if non-empty and a range
	bool isRange() const;
	// NOTE: first and last are guaranteed NOT NULL if the table is not empty
	const DomainElement* first() const;
	const DomainElement* last() const;

	InternalSortTable* internTable() const;
	SortTable* materialize() const; // Returns NULL if no materialization is possible OR if it would result in the same table

	virtual void put(std::ostream& stream) const;
	virtual SortTable* clone() const;
};

class InternalFuncTable;

/**
 *	This class implements tables for function symbols
 */
class FuncTable: public AbstractTable {
private:
	InternalFuncTable* _table; //!< Points to the actual table
	Universe _universe;
public:
	FuncTable(InternalFuncTable* table, const Universe&);
	~FuncTable();

	bool finite() const;
	bool empty() const;
	unsigned int arity() const;
	bool approxFinite() const;
	bool approxEmpty() const;
	tablesize size() const;

	// !!! RETURNS NULL iff the given tuple does not map to a domainelement within the range sort
	const DomainElement* operator[](const ElementTuple& tuple) const;
	bool contains(const ElementTuple& tuple) const;
	void add(const ElementTuple& tuple, bool ignoresortsortchecks = false);
	void remove(const ElementTuple& tuple);

	TableIterator begin() const;

	InternalFuncTable* internTable() const;

	const Universe& universe() const;
	FuncTable* materialize() const;  // Returns NULL if no materialization is possible OR if it would result in the same table

	virtual void put(std::ostream& stream) const;
};

/**********************
 Interpretations
 *********************/

/**
 *	Class to represent a four-valued interpretation for a predicate
 */
class PredInter {
private:
	PredTable* _ct; //!< stores certainly true tuples
	PredTable* _cf; //!< stores certainly false tuples
	PredTable* _pt; //!< stores possibly true tuples
	PredTable* _pf; //!< stores possibly false tuples
	std::set<ElementTuple> _inconsistentElements; //!<stores all elements that are inconsistent in this interpretation

	void checkConsistency();

public:
	/**
	 * \brief Create a three- or four-valued interpretation
	 *
	 * PARAMETERS
	 *	- ctpf	: the certainly true or possibly false tuples
	 *	- cfpt	: the certainly false or possibly true tuples
	 *	- ct	: if true (false), ctpf stores the certainly true (possibly false) tuples
	 *	- cf	: if true (false), cfpt stores the certainly false (possibly true) tuples
	 *	- univ	: all possible domain elements of the sorts of the columns of the table
	 */
	PredInter(PredTable* ctpf, PredTable* cfpt, bool ct, bool cf);

	/**
	 * \brief Create a two-valued interpretation
	 *
	 * PARAMETERS
	 *	- table : the true or false tuples
	 *	- ct	: if true (false), table stores the true (false) tuples
	 *	- univ	: all possible domain elements of the sorts of the columns of the table
	 */
	PredInter(PredTable* ctpf, bool ct);

	~PredInter();

	// Mutators
	void ct(PredTable*); //!< Replace the certainly true (and possibly false) tuples
	void cf(PredTable*); //!< Replace the certainly false (and possibly true) tuples
	void pt(PredTable*); //!< Replace the possibly true (and certainly false) tuples
	void pf(PredTable*); //!< Replace the possibly false (and certainly true) tuples
	void ctpt(PredTable*); //!< Replace the certainly and possibly true tuples
	void cfpf(PredTable*); //!< Replace the certainly and possibly false tuples
	void setTables(PredTable* ctpf, PredTable* cfpt, bool ct, bool cf); //Sets the tables if initialized with these values
	void materialize(); //!< Replace symbolic tables by enumerated ones if possible

	// Make the given tuple true, independent of its current value
	void makeTrueExactly(const ElementTuple&, bool ignoresortchecks = false);
	// Make the given tuple false, independent of its current value
	void makeFalseExactly(const ElementTuple&, bool ignoresortchecks = false);
	// Make the given tuple unknown, independent of its current value
	void makeUnknownExactly(const ElementTuple&, bool ignoresortchecks = false);

	// Make the given tuple true or inconsistent if it was already false
	void makeTrueAtLeast(const ElementTuple&, bool ignoresortchecks = false);
	// Make the given tuple false or inconsistent if it was already true
	void makeFalseAtLeast(const ElementTuple&, bool ignoresortchecks = false);

	// Inspectors
	PredTable* ct() const {
		return _ct;
	}
	PredTable* cf() const {
		return _cf;
	}
	PredTable* pt() const {
		return _pt;
	}
	PredTable* pf() const {
		return _pf;
	}
	bool isTrue(const ElementTuple& tuple, bool ignoresortchecks = false) const;
	bool isFalse(const ElementTuple& tuple, bool ignoresortchecks = false) const;
	bool isUnknown(const ElementTuple& tuple, bool ignoresortchecks = false) const;
	bool isInconsistent(const ElementTuple& tuple) const;
	bool isConsistent() const;
	const std::set<ElementTuple>& getInconsistentAtoms() const;
	bool approxTwoValued() const;
	const Universe& universe() const {
		return _ct->universe();
	}
	PredInter* clone(const Universe&) const;
	PredInter* clone() const;
	void put(std::ostream& stream) const;

private:
	//Can only be called if from and to are inverse tables
	void moveTupleFromTo(const ElementTuple& tuple, PredTable* from, PredTable* to, bool ignoresortchecks);
};

/**
 * Given a ct and cf table, checks which truth tables are the cheapest to store and stores those.
 * IMPORTANT: only use if creating the inverse tables of the given ones is cheap (e.g. when represented as bdds)
 */
PredInter* createSmallestPredInter(PredTable* ct, PredTable* cf, bool known_twovalued = false);

std::ostream& operator<<(std::ostream& stream, const PredInter& interpretation);

class Structure;

class Sort;
class Predicate;

/**
 *	Class to represent a four-valued interpretation for functions
 */
class FuncInter {
private:
	FuncTable* _functable; //!< the function table (if the interpretation is two-valued, nullpointer otherwise).
	PredInter* _graphinter; //!< the interpretation for the graph of the function

public:
	/**
	 * Creates a FuncInter with the given Functable,
	 * As graphinter, it creates a new predtable consistent with ft
	 * Takes responsibility of ft (will delete ft when needed)
	 */
	FuncInter(FuncTable* ft);
	/**
	 * Creates a FuncInter with pt as graphinter.
	 * FuncTable is set to be NULL
	 * Takes responsibility of pt (will delete pt when needed)
	 *
	 */
	FuncInter(PredInter* pt)
			: 	_functable(0),
				_graphinter(pt) {
	}
	~FuncInter();

	/**
	 * Deletes the old functable and graphinter.
	 * Creates a FuncInter with pt as graphinter.
	 * FuncTable is set to be NULL
	 * Takes responsibility of pt (will delete pt when needed)
	 */
	void graphInter(PredInter* pt);

	// NOTE: only implemented if functable is set!!!
	const DomainElement* value(const ElementTuple& tuple) const;

	/**
	 * Deletes the old functable and graphinter.
	 * Sets functable to be the given argument
	 * As graphinter, it creates a new predtable consistent with ft
	 * Takes responsibility of ft (will delete ft when needed)
	 */
	void funcTable(FuncTable* ft);

	void add(const ElementTuple& tuple, bool ignoresortchecks = false);

	bool isConsistent() const;
	const std::set<ElementTuple>& getInconsistentAtoms() const;
	void materialize();

	PredInter* graphInter() const {
		return _graphinter;
	}
	FuncTable* funcTable() const {
		return _functable;
	}
	bool approxTwoValued() const {
		return _functable != NULL;
	}

	const Universe& universe() const {
		return _graphinter->universe();
	}
	FuncInter* clone(const Universe&) const;
	FuncInter* clone() const;
	void put(std::ostream& stream) const;
};

// Contents ownership to receiver
std::vector<Structure*> generateEnoughTwoValuedExtensions(const std::vector<Structure*>& s);

/*********************
 * Auxiliary methods
 *********************/

namespace TableUtils {
PredInter* leastPredInter(const Universe& univ);
//!< construct a new, least precise predicate interpretation
FuncInter* leastFuncInter(const Universe& univ);
//!< construct a new, least precise function interpretation
Universe fullUniverse(unsigned int arity);

bool approxTotalityCheck(const FuncInter*);
//!< Check whether there is a value for every tuple in the given function interpretation.

// NOTE: Precondition: pt1 and pt2 do NOT share tuples!!!
bool isInverse(const PredTable* pt1, const PredTable* pt2);

SortTable* createSortTable();
SortTable* createSortTable(int start, int end);
PredTable* createPredTable(const Universe& universe);
PredTable* createFullPredTable(const Universe& universe);

} /* namespace TableUtils */

#endif
