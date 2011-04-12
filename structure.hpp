/************************************
	structure.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef STRUCTURE_HPP
#define STRUCTURE_HPP

#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <ostream>

/**
 * \file structure.hpp
 *
 *		This file contains the classes concerning structures:
 *		- domain elements
 *		- tables and interpretations for sorts, predicate, and function symbols
 *		- classes to represent a structure, i.e., a mapping from symbols of a 
 *		  vocabulary to corresponding interpretations of the correct type.
 *
 * NAMING CONVENTION
 *		- 'Interpretation' means a possibly three-valued, or even four-valued interpretation for a symbol.
 *		- 'Table' means a two-value table
 *		- if a name of a methods begins with 'approx', the method is fast, but provides a under approximation of
 *		  the desired result.
 */

/**********************
	Domain elements
**********************/

class Compound;
class DomainElementFactory;

/**
 *	The different types of domain elements
 *	- DET_INT: integers
 *	- DET_DOUBLE: floating point numbers
 *	- DET_STRING: strings
 *	- DET_COMPOUND: a function applied to domain elements
 */
enum DomainElementType { DET_INT, DET_DOUBLE, DET_STRING, DET_COMPOUND };

/**
 *	A value for a single domain element. 
 */
union DomainElementValue {
	int					_int;		//!< Value if the domain element is an integer
	double				_double;	//!< Value if the domain element is a floating point number
	const std::string*	_string;	//!< Value if the domein element is a string
	const Compound*		_compound;	//!< Value if the domain element is a function applied to domain elements
};

/**
 *	A domain element
 */
class DomainElement {
	private:
		DomainElementType	_type;		//!< The type of the domain element
		DomainElementValue	_value;		//!< The value of the domain element

		DomainElement(int value);
		DomainElement(double value);
		DomainElement(const std::string* value);
		DomainElement(const Compound* value);

	public:
		~DomainElement();	//!< Destructor (does not delete the value of the domain element)

		DomainElementType	type()	const;	//!< Returns the type of the element
		DomainElementValue	value()	const;	//!< Returns the value of the element

		std::ostream& put(std::ostream&)	const;
		std::string to_string()				const;

		friend class DomainElementFactory;
};

bool operator<(const DomainElement&,const DomainElement&);
bool operator>(const DomainElement&,const DomainElement&);
bool operator==(const DomainElement&,const DomainElement&);
bool operator!=(const DomainElement&,const DomainElement&);
bool operator<=(const DomainElement&,const DomainElement&);
bool operator>=(const DomainElement&,const DomainElement&);

std::ostream& operator<< (std::ostream&,const DomainElement&);

typedef std::vector<const DomainElement*>	ElementTuple;
typedef std::vector<ElementTuple>			ElementTable;

struct StrictWeakElementOrdering {
	bool operator()(const DomainElement* d1, const DomainElement* d2) const { return *d1 < *d2;	}
};
typedef std::set<const DomainElement*,StrictWeakElementOrdering>	SortedElementTuple;

struct StrictWeakTupleOrdering {
	bool operator()(const ElementTuple&, const ElementTuple&) const;
};
typedef std::set<ElementTuple,StrictWeakTupleOrdering>	SortedElementTable;

typedef std::map<ElementTuple,const DomainElement*,StrictWeakTupleOrdering>	ElementFunc;

struct StrictWeakNTupleEquality {
	unsigned int _arity;
	StrictWeakNTupleEquality(unsigned int arity) : _arity(arity) { }
	bool operator()(const ElementTuple&, const ElementTuple&) const;
};

class Function;

/**
 *	The value of a domain element that consists of a function applied to domain elements.
 */
class Compound {
	private:
		Function*		_function;
		ElementTuple	_arguments;

		Compound(Function* function, const std::vector<const DomainElement*> arguments);

	public:
		~Compound();

		Function*				function()				const;	//!< Returns the function of the compound
		const DomainElement*	arg(unsigned int n)		const;	//!< Returns the n'th argument of the compound

		std::ostream&	put(std::ostream&)	const;
		std::string		to_string()			const;

		friend class DomainElementFactory;
};

bool operator< (const Compound&,const Compound&);
bool operator> (const Compound&,const Compound&);
bool operator==(const Compound&,const Compound&);
bool operator!=(const Compound&,const Compound&);
bool operator<=(const Compound&,const Compound&);
bool operator>=(const Compound&,const Compound&);

std::ostream& operator<< (std::ostream&,const Compound&);

/**
 *	Class to create domain elements. This class is a singleton class that ensures all domain elements
 *	with the same value are stored at the same address in memory. As a result, two domain elements are equal
 *	iff they have the same address. It also ensures that all Compounds with the same function and arguments are
 *	stored at the same address.
 *
 *	Obtaining the address of a domain element with a given value and type should take logaritmic time in the number
 *	of created domain elements of that type. For a specified integer range, obtaining the address is optimized to
 *	constant time.
 */
class DomainElementFactory {
	private:
		static DomainElementFactory*	_instance;			//!< The single instance of DomainElementFactory
		
		std::map<Function*,std::map<ElementTuple,Compound*> >	_compounds;	
			//!< Maps a function and tuple of elements to the corresponding compound.
																
		int							_firstfastint;		//!< The first integer in the optimized range
		int							_lastfastint;		//!< One past the last integer in the optimized range
		std::vector<DomainElement*>	_fastintelements;	//!< Stores pointers to integers in the optimized range.
														//!< The domain element with value n is stored at
														//!< _fastintelements[n+_firstfastint]

		std::map<int,DomainElement*>				_intelements;		
			//!< Maps an integer outside of the optimized range to its corresponding doman element address.
		std::map<double,DomainElement*>				_doubleelements;	
			//!< Maps a floating point number to its corresponding domain element address.
		std::map<const std::string*,DomainElement*>	_stringelements;	
			//!< Maps a string pointer to its corresponding domain element address.
		std::map<const Compound*,DomainElement*>	_compoundelements;	
			//!< Maps a compound pointer to its corresponding domain element address.
		
		DomainElementFactory(int firstfastint = 0, int lastfastint = 10001);

		Compound*		compound(Function*,const ElementTuple&);

	public:
		~DomainElementFactory();

		static DomainElementFactory*	instance();

		const DomainElement*	create(int value);
		const DomainElement*	create(double value, bool certnotint = false);
		const DomainElement*	create(const std::string* value, bool certnotdouble = false);
		const DomainElement*	create(const Compound* value);
		const DomainElement*	create(Function*,const ElementTuple&);
};


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
		InternalTableIterator*	_iterator;
	public:
		TableIterator(const TableIterator&);
		TableIterator(InternalTableIterator* iter) : _iterator(iter) { }
		TableIterator& operator=(const TableIterator&);
		bool					hasNext()	const;
		const ElementTuple&		operator*()	const;
		TableIterator&			operator++();
		~TableIterator();
		const InternalTableIterator*	iterator()	const { return _iterator;	}
};

class SortIterator {
	private:
		InternalSortIterator*	_iterator;
	public:
		SortIterator(InternalSortIterator* iter) : _iterator(iter) { }
		SortIterator(const SortIterator&);
		SortIterator& operator=(const SortIterator&);
		bool					hasNext()	const;
		const DomainElement*	operator*()	const;
		SortIterator&			operator++();
		~SortIterator();
		const InternalSortIterator*	iterator()	const { return _iterator;	}
};

class InternalTableIterator {
	private:
		virtual bool					hasNext()	const = 0;
		virtual const ElementTuple&		operator*()	const = 0;
		virtual void					operator++() = 0;
	public:
		virtual ~InternalTableIterator() { }
		virtual InternalTableIterator*	clone()	const = 0; 
	friend class TableIterator;
};

class SortInternalTableIterator : public InternalTableIterator {
	private:
		InternalSortIterator*	_iter;
		mutable ElementTable	_deref;
		bool					hasNext()	const;
		const ElementTuple&		operator*()	const;
		void					operator++();
	public:
		SortInternalTableIterator(InternalSortIterator* isi) : _iter(isi) { }
		~SortInternalTableIterator();
		SortInternalTableIterator*	clone()	const;

};

class EnumInternalIterator : public InternalTableIterator {
	private:
		SortedElementTable::const_iterator	_iter;
		SortedElementTable::const_iterator	_end;
		bool					hasNext()	const	{ return _iter != _end;	}
		const ElementTuple&		operator*()	const	{ return *_iter;		}
		void					operator++()		{ ++_iter;				}
	public:
		EnumInternalIterator(SortedElementTable::const_iterator it, SortedElementTable::const_iterator end) : _iter(it), _end(end) { }
		~EnumInternalIterator() { }
		EnumInternalIterator*	clone()	const;
};

class EnumInternalFuncIterator : public InternalTableIterator {
	private:
		ElementFunc::const_iterator _iter;
		ElementFunc::const_iterator _end;
		mutable ElementTable	_deref;
		bool	hasNext()	const { return _iter != _end;	}
		const ElementTuple&	operator*()	const;
		void	operator++()	{ ++_iter;	}
	public:
		EnumInternalFuncIterator(ElementFunc::const_iterator it, ElementFunc::const_iterator end) : 
			_iter(it), _end(end) { }
		~EnumInternalFuncIterator() { }
		EnumInternalFuncIterator*	clone()	const;

};

class SortTable;
class PredTable;

class UnionInternalIterator : public InternalTableIterator {
	private:
		std::vector<TableIterator>				_iterators;
		std::vector<PredTable*>					_outtables;
		std::vector<TableIterator>::iterator	_curriterator;

		bool contains(const ElementTuple&)	const;
		void setcurriterator();

		bool					hasNext()	const;	
		const ElementTuple&		operator*()	const;
		void					operator++();	
	public:
		UnionInternalIterator(const std::vector<TableIterator>&, const std::vector<PredTable*>&);
		~UnionInternalIterator() { }
		UnionInternalIterator*	clone() const;
};

class InverseInternalIterator : public InternalTableIterator {
	private:
		std::vector<SortIterator>	_curr;
		std::vector<SortIterator>	_lowest;
		PredTable*					_outtable;
		mutable bool						_end;
		mutable ElementTuple				_currtuple;
		mutable std::vector<ElementTuple>	_deref;

		bool					hasNext()	const;	
		const ElementTuple&		operator*()	const;
		void					operator++();	
		InverseInternalIterator(const std::vector<SortIterator>&, const std::vector<SortIterator>&, PredTable*, bool);
	public:
		InverseInternalIterator(const std::vector<SortIterator>&, PredTable*);
		~InverseInternalIterator() { }
		InverseInternalIterator* clone() const;
};

class EqualInternalIterator : public InternalTableIterator {
	private:
		SortIterator			_iterator;
		mutable	ElementTable	_deref;
		bool					hasNext()	const;	
		const ElementTuple&		operator*()	const;
		void					operator++();	
	public:
		EqualInternalIterator(const SortIterator& iter);
		~EqualInternalIterator() { }
		EqualInternalIterator*	clone()	const;
};

class StrLessThanInternalIterator : public InternalTableIterator {
	private:
		SortIterator			_leftiterator;
		SortIterator			_rightiterator;
		mutable	ElementTable	_deref;

		bool					hasNext()	const;
		const ElementTuple&		operator*()	const;
		void					operator++();	
	public:
		StrLessThanInternalIterator(const SortIterator& si);
		StrLessThanInternalIterator(const SortIterator& l, const SortIterator& r);
		~StrLessThanInternalIterator() { }
		StrLessThanInternalIterator*	clone()	const;
};

class StrGreaterThanInternalIterator : public InternalTableIterator {
	private:
		SortIterator			_leftiterator;
		SortIterator			_rightiterator;
		SortIterator			_lowest;
		mutable	ElementTable	_deref;

		bool					hasNext()	const;
		const ElementTuple&		operator*()	const;
		void					operator++();	
	public:
		StrGreaterThanInternalIterator(const SortIterator& si);
		~StrGreaterThanInternalIterator() { }
		StrGreaterThanInternalIterator*	clone()	const;
};

class InternalSortIterator {
	public:
		virtual bool					hasNext()	const = 0;
		virtual const DomainElement*	operator*()	const = 0;
		virtual void					operator++() = 0;
		virtual ~InternalSortIterator() { }
		virtual InternalSortIterator*	clone()	const = 0;
	friend class SortIterator;
	friend class SortInternalTableIterator;
};

class UnionInternalSortIterator : public InternalSortIterator {
	private:
		std::vector<SortIterator>				_iterators;
		std::vector<SortTable*>					_outtables;
		std::vector<SortIterator>::iterator		_curriterator;

		bool contains(const DomainElement*)	const;
		void setcurriterator();

		bool					hasNext()	const;	
		const DomainElement*	operator*()	const;
		void					operator++();	
	public:
		UnionInternalSortIterator(const std::vector<SortIterator>&, const std::vector<SortTable*>&);
		~UnionInternalSortIterator() { }
		UnionInternalSortIterator*	clone() const;
};

class NatInternalSortIterator : public InternalSortIterator {
	private:
		int	_iter;
		bool					hasNext()		const	{ return true;	}
		const DomainElement*	operator*()		const	{ return DomainElementFactory::instance()->create(_iter);	}
		void					operator++()			{ ++_iter;		}	
	public:
		NatInternalSortIterator() : _iter(0) { }
		~NatInternalSortIterator() { }
		NatInternalSortIterator* clone()	const;
};

class EnumInternalSortIterator : public InternalSortIterator {
	private:
		ElementTuple::const_iterator	_iter;
		ElementTuple::const_iterator	_end;
		bool					hasNext()	const	{ return _iter != _end;	}
		const DomainElement*	operator*()	const	{ return *_iter;		}
		void					operator++()		{ ++_iter;				}
	public:
		EnumInternalSortIterator(ElementTuple::iterator it, ElementTuple::iterator end) : _iter(it), _end(end) { }
		~EnumInternalSortIterator() { }
		EnumInternalSortIterator* clone()	const;
};


/********************************************
	Internal tables for predicate symbols
********************************************/

/**
 *	This class implements a concrete two-dimensional table
 */
class InternalPredTable {
	protected:
		// Attributes
		unsigned int	_nrRefs;	//!< The number of references to this table

	public:
		// Inspectors
		virtual unsigned int	arity()		const = 0;
		virtual bool			finite()	const = 0;	//!< Returns true iff the table is finite
		virtual	bool			empty()		const = 0;	//!< Returns true iff the table is empty

		virtual bool	approxfinite()	const = 0;
			//!< Returns false if the table size is infinite. May return true if the table size is finite.
		virtual bool	approxempty()	const = 0;
			//!< Returns false if the table is non-empty. May return true if the table is empty.

		virtual	bool	contains(const ElementTuple& tuple) const = 0;	//!< Returns true iff the table contains the tuple. 

		// Mutators
		virtual InternalPredTable*	add(const ElementTuple& tuple) = 0;		//!< Add a tuple to the table
		virtual InternalPredTable*	remove(const ElementTuple& tuple) = 0;	//!< Remove a tuple from the table

		void decrementRef();	//!< Delete one reference. Deletes the table if the number of references becomes zero.
		void incrementRef();	//!< Add one reference

		// Iterators
		virtual InternalTableIterator*		begin()	const = 0;

		InternalPredTable() : _nrRefs(0)	{ }
		virtual ~InternalPredTable()		{ }

	friend class PredTable;
	friend class SortTable;
};

class SortInternalPredTable : public InternalPredTable {
	private:
		SortTable*	_table;
		bool		_linked;
	public:
		SortInternalPredTable(SortTable* table, bool linked) : _table(table), _linked(linked) { }

		unsigned int	arity()			const;
		bool			finite()		const;
		bool			empty()			const;
		bool			approxfinite()	const;
		bool			approxempty()	const;

		bool	contains(const ElementTuple& tuple)		const;

		InternalPredTable*	add(const ElementTuple& tuple);		//!< Add a tuple to the table
		InternalPredTable*	remove(const ElementTuple& tuple);	//!< Remove a tuple from the table

		InternalTableIterator*	begin()	const;

		~SortInternalPredTable();

};

class CartesianInternalPredTable : public InternalPredTable {
	private:
		std::vector<SortTable*>		_tables;
		std::vector<bool>			_linked;
	public:
		CartesianInternalPredTable(const std::vector<SortTable*>& tables, const std::vector<bool>& linked) :
			_tables(tables), _linked(linked) { }

		unsigned int	arity()			const { return _tables.size();	}
		bool			finite()		const;
		bool			empty()			const;
		bool			approxfinite()	const;
		bool			approxempty()	const;

		bool	contains(const ElementTuple& tuple)		const;

		InternalPredTable*	add(const ElementTuple& tuple);		//!< Add a tuple to the table
		InternalPredTable*	remove(const ElementTuple& tuple);	//!< Remove a tuple from the table

		InternalTableIterator*	begin()	const;

		~CartesianInternalPredTable();

};

class FuncTable;

class FuncInternalPredTable : public InternalPredTable {
	private:
		FuncTable*	_table;
		bool		_linked;
	public:
		FuncInternalPredTable(FuncTable* table, bool linked) : _table(table), _linked(linked) { }

		unsigned int	arity()			const;
		bool			finite()		const;
		bool			empty()			const;
		bool			approxfinite()	const;
		bool			approxempty()	const;

		bool	contains(const ElementTuple& tuple)		const;

		InternalPredTable*	add(const ElementTuple& tuple);		//!< Add a tuple to the table
		InternalPredTable*	remove(const ElementTuple& tuple);	//!< Remove a tuple from the table

		InternalTableIterator*	begin()	const;

		~FuncInternalPredTable();
};

class PredTable;

/**
 *	This class implements table that consists of all tuples that belong to the union of a set of tables,
 *	but not to the union of another set of tables.
 *
 *	INVARIANT: the first table of _intables and of _outtables has an enumerated internal table
 */
class UnionInternalPredTable : public InternalPredTable {
	private:
		unsigned int			_arity;
		std::vector<PredTable*>	_intables;	//!< a tuple of the table does belong to at least one of the tables in _intables
		std::vector<PredTable*>	_outtables;	//!< a tuple of the table does not belong to any of the tables in _outtables

		unsigned int	arity()			const;
		bool			finite()		const;
		bool			empty()			const;
		bool			approxfinite()	const;
		bool			approxempty()	const;

		bool	contains(const ElementTuple& tuple) const;	

		InternalTableIterator*	begin()	const;

	public:
		UnionInternalPredTable(unsigned int arity);
		UnionInternalPredTable(unsigned int arity, const std::vector<PredTable*>& in, const std::vector<PredTable*>& out) :
			_arity(arity), _intables(in), _outtables(out) { }
		~UnionInternalPredTable();
		void	addInTable(PredTable* t)	{ _intables.push_back(t);	}
		void	addOutTable(PredTable* t)	{ _outtables.push_back(t);	}
		InternalPredTable*	add(const ElementTuple& tuple);		
		InternalPredTable*	remove(const ElementTuple& tuple);	
	
};


/**
 *	This class implements a finite, enumerated InternalPredTable
 */
class EnumeratedInternalPredTable : public InternalPredTable {
	private:
		unsigned int		_arity;		//!< the number of columns of the table
		SortedElementTable	_table;		//!< the actual table

		unsigned int	arity()			const { return _arity;			}
		bool			finite()		const { return true;			}
		bool			empty()			const { return _table.empty();	}
		bool			approxfinite()	const { return true;			}
		bool			approxempty()	const { return _table.empty();	}

		bool			contains(const ElementTuple& tuple) const;


		InternalTableIterator*		begin() const;

	public:
		EnumeratedInternalPredTable(const SortedElementTable& tab, unsigned int arity) :
			_arity(arity), _table(tab) { }
		EnumeratedInternalPredTable(unsigned int arity) : 
			_arity(arity) { }
		~EnumeratedInternalPredTable() { }
		EnumeratedInternalPredTable*	add(const ElementTuple& tuple);
		EnumeratedInternalPredTable*	remove(const ElementTuple& tuple);
};

class InternalSortTable;

/**
 *	Abstract base class for implementing InternalPredTable for '=/2', '</2', and '>/2'
 */
class ComparisonInternalPredTable : public InternalPredTable {
	protected:
		SortTable*	_table;		//!< the elements that possibly occur in the columns of the table
		bool		_linked;	//!< if true, _table is not deleted when the table is deleted
	public:
		ComparisonInternalPredTable(SortTable* t, bool linked);
		virtual ~ComparisonInternalPredTable();
		unsigned int		arity()	const { return 2;	}
		InternalPredTable*	add(const ElementTuple& tuple);
		InternalPredTable*	remove(const ElementTuple& tuple);
};

/**
 *	Internal table for '=/2'
 */
class EqualInternalPredTable : public ComparisonInternalPredTable {
	public:
		EqualInternalPredTable(SortTable* s, bool l) : ComparisonInternalPredTable(s,l) { } 
		~EqualInternalPredTable() { }

		bool	contains(const ElementTuple&)	const;
		bool	finite()						const;
		bool	empty()							const;
		bool	approxfinite()					const;
		bool	approxempty()					const;

		InternalTableIterator*		begin() const;
};

/**
 *	Internal table for '</2'
 */
class StrLessInternalPredTable : public ComparisonInternalPredTable {
	public:
		StrLessInternalPredTable(SortTable* s, bool l) : ComparisonInternalPredTable(s,l) { } 
		~StrLessInternalPredTable() { }

		bool	contains(const ElementTuple&)	const;
		bool	finite()						const;
		bool	empty()							const;
		bool	approxfinite()					const;
		bool	approxempty()					const;

		InternalTableIterator*		begin() const;
};

/**
 *	Internal table for '>/2'
 */
class StrGreaterInternalPredTable : public ComparisonInternalPredTable {
	public:
		StrGreaterInternalPredTable(SortTable* s, bool l) : ComparisonInternalPredTable(s,l) { } 
		~StrGreaterInternalPredTable() { }

		bool	contains(const ElementTuple&)	const;
		bool	finite()						const;
		bool	empty()							const;
		bool	approxfinite()					const;
		bool	approxempty()					const;

		InternalTableIterator*		begin() const;
};

/**
 *	This class implements the complement of an internal predicate table
 */
class InverseInternalPredTable : public InternalPredTable {
	private:
		PredTable*				_invtable;		//!< the inverse of the actual table
		std::vector<SortTable*>	_universe;		//!< the actual table is the complement of _table with respect to 
												//!< the cartesian product of the tables in _universe
		bool					_invlinked;		//!< if true, _invtable will not be deleted when the table is deleted
		std::vector<bool>		_univlinked;	//!< if _univlinked[n] is true, _universe[n] will not be deleted

	public:
		InverseInternalPredTable(PredTable* inv, const std::vector<SortTable*>& univ, bool linked, const std::vector<bool>& univlinked) :
			_invtable(inv), _universe(univ), _invlinked(linked), _univlinked(univlinked) { }
		~InverseInternalPredTable();

		unsigned int	arity()					const;
		bool			finite()				const;
		bool			empty()					const;
		bool			approxfinite()			const;
		bool			approxempty()			const;

		bool	contains(const ElementTuple& tuple) const;

		InternalPredTable*	add(const ElementTuple& tuple);
		InternalPredTable*	remove(const ElementTuple& tuple);

		InternalTableIterator*	begin() const;
};

/********************************
	Internal tables for sorts
********************************/

/**
 *	This class implements a concrete one-dimensional table
 */
class InternalSortTable : public InternalPredTable {
	public:
		unsigned int arity()	const	{ return 1;	}

		virtual bool	contains(const DomainElement*) const = 0;
				bool	contains(const ElementTuple& tuple)	const { return contains(tuple[0]);	}

		virtual InternalSortTable*	add(const DomainElement*)			= 0;
				InternalSortTable*	add(const ElementTuple& tuple)		{ return add(tuple[0]);		}
		virtual InternalSortTable*	remove(const DomainElement*)		= 0;
				InternalSortTable*	remove(const ElementTuple& tuple)	{ return remove(tuple[0]);	}

		virtual InternalSortIterator*	sortbegin() const = 0;
				InternalTableIterator*	begin()		const;
	
		virtual ~InternalSortTable() { }

};

class UnionInternalSortTable : public InternalSortTable {
	public:
		std::vector<SortTable*>	_intables;	
			//!< an element of the table does belong to at least one of the tables in _intables
		std::vector<SortTable*>	_outtables;	
			//!< an element of the table does not belong to any of the tables in _outtables

		bool	finite()		const;
		bool	empty()			const;
		bool	approxfinite()	const;
		bool	approxempty()	const;

		bool	contains(const DomainElement*) const;	

		InternalSortIterator*	sortbegin()	const;

	public:
		UnionInternalSortTable();
		UnionInternalSortTable(const std::vector<SortTable*>& in, const std::vector<SortTable*>& out) :
			_intables(in), _outtables(out) { }
		~UnionInternalSortTable();
		void	addInTable(SortTable* t)	{ _intables.push_back(t);	}
		void	addOutTable(SortTable* t)	{ _outtables.push_back(t);	}
		InternalSortTable*	add(const DomainElement*);		
		InternalSortTable*	remove(const DomainElement*);	
	
};

class InfiniteInternalSortTable : public InternalSortTable {
	private:
		InternalSortTable*	add(const DomainElement*);
		InternalSortTable*	remove(const DomainElement*);
		bool	finite()		const { return false;	}
		bool	empty()			const { return false;	}
		bool	approxfinite()	const { return false;	}
		bool	approxempty()	const { return false;	}
	protected:
		~InfiniteInternalSortTable() { }
};

/**
 *	All natural numbers
 */
class AllNaturalNumbers : public InfiniteInternalSortTable {
	private:
		bool					contains(const DomainElement*)	const;
		InternalSortIterator*	sortbegin()	const;

	protected:
		~AllNaturalNumbers() { }
};

/**
 * All integers
 */
class AllIntegers : public InfiniteInternalSortTable {
	private:
		bool					contains(const DomainElement*)	const;
		InternalSortIterator*	sortbegin()	const;

	protected:
		~AllIntegers() { }
};

/**
 * All floating point numbers
 */
class AllFloats : public InfiniteInternalSortTable {
	private:
		bool					contains(const DomainElement*)	const;
		InternalSortIterator*	sortbegin()	const;

	protected:
		~AllFloats() { }
};

/**
 * All strings
 */
class AllStrings : public InfiniteInternalSortTable {
	private:
		bool					contains(const DomainElement*)	const;
		InternalSortIterator*	sortbegin()	const;

	protected:
		~AllStrings() { }
};

/**
 * All characters
 */
class AllChars : public InternalSortTable {
	private:
		bool					contains(const DomainElement*)	const;
		InternalSortTable*		add(const DomainElement*);
		InternalSortTable*		remove(const DomainElement*);

		InternalSortIterator*	sortbegin()	const;

		bool	finite()		const { return true;	}
		bool	empty()			const { return false;	}
		bool	approxfinite()	const { return true;	}
		bool	approxempty()	const { return false;	}
	protected:
		~AllChars() { }
};

/**
 *	A finite, enumerated SortTable
 */
class EnumeratedInternalSortTable : public InternalSortTable {
	private:
		SortedElementTuple	_table;

		bool					contains(const DomainElement*)	const;
		InternalSortTable*		add(const DomainElement*);
		InternalSortTable*		remove(const DomainElement*);

		InternalSortIterator*	sortbegin()	const;

		bool finite()		const { return true;			}
		bool empty()		const { return _table.empty();	}	
		bool approxfinite()	const { return true;			}
		bool approxempty()	const { return _table.empty();	}
	protected:
		~EnumeratedInternalSortTable() { }
};

/**
 * DESCRIPTION
 *		A range of integers
 */
class IntRangeInternalSortTable : public InternalSortTable {
	private:
		int _first;		//!< first element in the range
		int _last;		//!< last element in the range
	public:
		bool			finite()		const	{ return approxfinite();	}
		bool			empty()			const	{ return approxempty();		}
		bool			approxfinite()	const	{ return true;				}
		bool			approxempty()	const	{ return _first > _last;	}

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
		InternalFuncTable() : _nrRefs(0) { }
		virtual ~InternalFuncTable() { }

		void decrementRef();	//!< Delete one reference. Deletes the table if the number of references becomes zero.
		void incrementRef();	//!< Add one reference

		virtual bool			finite()				const = 0;	//!< Returns true iff the table is finite
		virtual	bool			empty()					const = 0;	//!< Returns true iff the table is empty
		virtual	unsigned int	arity()					const = 0;	//!< Returns the number of columns in the table

		virtual bool			approxfinite()			const = 0;
			//!< Returns false if the table size is infinite. May return true if the table size is finite.
		virtual bool			approxempty()			const = 0;
			//!< Returns false if the table is non-empty. May return true if the table is empty.

		virtual const DomainElement* operator[](const ElementTuple& tuple)	const = 0;	
			//!< Returns the value of the tuple according to the array.

		virtual	InternalFuncTable*	add(const ElementTuple&)	= 0;	//!< Add a tuple to the table
		virtual InternalFuncTable*	remove(const ElementTuple&)	= 0;	//!< Remove a tuple from the table

		virtual InternalTableIterator*	begin()	const = 0;
};

/**
 *		A finite, enumerated InternalFuncTable
 */
class EnumeratedInternalFuncTable : public InternalFuncTable {
	private:
		unsigned int _arity;
		ElementFunc	_table;
	public:
		EnumeratedInternalFuncTable(unsigned int arity, const ElementFunc& tab) : 
			_arity(arity), _table(tab) { }
		~EnumeratedInternalFuncTable() { }

		unsigned int	arity()	const { return _arity;			}
		bool	finite()		const { return true;			}
		bool	empty()			const { return _table.empty();	}
		bool	approxfinite()	const { return true;			}
		bool	approxempty()	const { return _table.empty();	}
		
		const DomainElement*	operator[](const ElementTuple& tuple) const;
		InternalFuncTable*		add(const ElementTuple&);	
		InternalFuncTable*		remove(const ElementTuple&);

		InternalTableIterator*	begin()	const;
};

class IntFloatInternalFuncTable : public InternalFuncTable {
	private:
		bool	_int;
	public:

		IntFloatInternalFuncTable(bool);

				bool			finite()		const { return false;	}
				bool			empty()			const { return false;	}
				bool			approxfinite()	const { return false;	}
				bool			approxempty()	const { return false;	}
		virtual	unsigned int	arity()			const = 0;

		InternalFuncTable*	add(const ElementTuple&);
		InternalFuncTable*	remove(const ElementTuple&);

		InternalTableIterator*	begin()	const;

};

class PlusInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		PlusInternalFuncTable(bool);
		unsigned int arity()	const { return 2;	}
		const DomainElement*	operator[](const std::vector<const DomainElement*>&	)	const;
};

class MinusInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		MinusInternalFuncTable(bool);
		unsigned int arity()	const { return 2;	}
		const DomainElement*	operator[](const std::vector<const DomainElement*>&	)	const;
};

class TimesInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		TimesInternalFuncTable(bool);
		unsigned int arity()	const { return 2;	}
		const DomainElement*	operator[](const std::vector<const DomainElement*>&	)	const;
};

class DivInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		DivInternalFuncTable(bool);
		unsigned int arity()	const { return 2;	}
		const DomainElement*	operator[](const std::vector<const DomainElement*>&	)	const;
};

class AbsInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		AbsInternalFuncTable(bool);
		unsigned int arity()	const { return 1;	}
		const DomainElement*	operator[](const std::vector<const DomainElement*>&	)	const;
};

class UminInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		UminInternalFuncTable(bool);
		unsigned int arity()	const { return 1;	}
		const DomainElement*	operator[](const std::vector<const DomainElement*>&	)	const;
};

class ExpInternalFuncTable : public InternalFuncTable {
	public:
		bool			finite()		const { return false;	}
		bool			empty()			const { return false;	}
		bool			approxfinite()	const { return false;	}
		bool			approxempty()	const { return false;	}
		unsigned int	arity()			const { return 2;		}
		const DomainElement*	operator[](const std::vector<const DomainElement*>&	)	const;

		InternalFuncTable*	add(const ElementTuple&);
		InternalFuncTable*	remove(const ElementTuple&);

		InternalTableIterator*	begin()	const;
};

class ModInternalFuncTable : public InternalFuncTable {
	public:
		bool			finite()		const { return false;	}
		bool			empty()			const { return false;	}
		bool			approxfinite()	const { return false;	}
		bool			approxempty()	const { return false;	}
		unsigned int	arity()			const { return 2;		}
		const DomainElement*	operator[](const std::vector<const DomainElement*>&	)	const;

		InternalFuncTable*	add(const ElementTuple&);
		InternalFuncTable*	remove(const ElementTuple&);

		InternalTableIterator*	begin()	const;
};




/*********************************************************
	Tables for sorts, predicates, and function symbols
*********************************************************/

/**
 *	This class implements the common functionality of tables for sorts, predicate, and function symbols.
 */
class AbstractTable {
	public:
		virtual ~AbstractTable() { }

		virtual bool			finite()	const = 0;	//!< Returns true iff the table is finite
		virtual	bool			empty()		const = 0;	//!< Returns true iff the table is empty
		virtual	unsigned int	arity()		const = 0;	//!< Returns the number of columns in the table

		virtual bool	approxfinite()		const = 0;	
			//!< Returns false if the table size is infinite. May return true if the table size is finite.
		virtual bool	approxempty()		const = 0;
			//!< Returns false if the table is non-empty. May return true if the table is empty.

		virtual	bool	contains(const ElementTuple& tuple)	const = 0;	
			//!< Returns true iff the table contains the tuple. 
			
		virtual void	add(const ElementTuple& tuple)		= 0;	//!< Add a tuple to the table
		virtual void	remove(const ElementTuple& tuple)	= 0;	//!< Remove a tuple from the table

		virtual TableIterator		begin()	const = 0;

};


/**
 *	This class implements tables for predicate symbols.
 */
class PredTable : public AbstractTable {
	private:
		InternalPredTable*	_table;	//!< Points to the actual table
	public:
		PredTable(InternalPredTable* table);
		~PredTable();

		bool				finite()							const	{ return _table->finite();			}
		bool				empty()								const	{ return _table->empty();			}
		unsigned int		arity()								const	{ return _table->arity();			}
		bool				approxfinite()						const	{ return _table->approxfinite();	}
		bool				approxempty()						const	{ return _table->approxfinite();	}
		bool				contains(const ElementTuple& tuple)	const	{ return _table->contains(tuple);	}
		void				add(const ElementTuple& tuple);
		void				remove(const ElementTuple& tuple);

		TableIterator		begin() const;

		InternalPredTable*	interntable()	const { return _table;	}
};

/**
 *		This class implements tables for sorts
 */
class SortTable : public AbstractTable {
	private:
		InternalSortTable*	_table;	//!< Points to the actual table
	public:
		SortTable(InternalSortTable* table); 
		~SortTable();

		bool			finite()							const	{ return _table->finite();			}
		bool			empty()								const	{ return _table->empty();			}
		bool			approxfinite()						const	{ return _table->approxfinite();	}
		bool			approxempty()						const	{ return _table->approxfinite();	}
		unsigned int	arity()								const	{ return 1;							}
		bool			contains(const ElementTuple& tuple)	const	{ return _table->contains(tuple);	}
		bool			contains(const DomainElement* el)	const	{ return _table->contains(el);		}
		void			add(const ElementTuple& tuple)				{ _table = _table->add(tuple);		}
		void			add(const DomainElement* el)				{ _table = _table->add(el);			}
		void			remove(const ElementTuple& tuple)			{ _table = _table->remove(tuple);	}
		void			remove(const DomainElement* el)				{ _table = _table->remove(el);		}
		TableIterator 	begin()								const;
		SortIterator 	sortbegin()							const;

		InternalSortTable*	interntable()	const { return _table;	}
};

/**
 *	This class implements tables for function symbols
 */
class FuncTable : public AbstractTable {
	private:
		InternalFuncTable*	_table;	//!< Points to the actual table
	public:
		FuncTable(InternalFuncTable* table);
		~FuncTable();

		bool			finite()				const	{ return _table->finite();			}
		bool			empty()					const	{ return _table->empty();			}
		unsigned int	arity()					const	{ return _table->arity();			}
		bool			approxfinite()			const	{ return _table->approxfinite();	}
		bool			approxempty()			const	{ return _table->approxfinite();	}

		const DomainElement*	operator[](const std::vector<const DomainElement*>& tuple)	const	{ return (*_table)[tuple];	}
		bool	contains(const std::vector<const DomainElement*>& tuple)		const;
		void	add(const ElementTuple& tuple);	
		void	remove(const ElementTuple& tuple);

		TableIterator	begin()	const;

		InternalFuncTable*	interntable()	const { return _table;	}

};


/**********************
	Interpretations
*********************/

/**
 *	Class to represent a four-valued interpretation for a predicate
 */
class PredInter {
	
	private:
		PredTable*	_ct;	//!< stores certainly true tuples
		PredTable*	_cf;	//!< stores certainly false tuples
		PredTable*	_pt;	//!< stores possibly true tuples
		PredTable*	_pf;	//!< stores possibly false tuples

	public:
		
		PredInter(PredTable* ctpf,PredTable* cfpt,bool ct, bool cf, const std::vector<SortTable*>& univ);
		PredInter(PredTable* ctpf, bool ct, const std::vector<SortTable*>& univ);

		// Destructor
		~PredInter();

		// Mutators
		void ct(PredTable*);
		void cf(PredTable*);
		void pt(PredTable*);
		void pf(PredTable*);

		// Inspectors
		PredTable*	ct()										const { return _ct;	}
		PredTable*	cf()										const { return _cf;	}
		PredTable*	pt()										const { return _pt;	}
		PredTable*	pf()										const { return _pf;	}
		bool		istrue(const ElementTuple& tuple)			const;
		bool		isfalse(const ElementTuple& tuple)			const;
		bool		isunknown(const ElementTuple& tuple)		const;
		bool		isinconsistent(const ElementTuple& tuple)	const;
		bool		approxtwovalued()							const;

};

class AbstractStructure;

class PredInterGenerator {
	public:
		virtual PredInter* get(const AbstractStructure* structure) = 0;
};

class EqualInterGenerator : public PredInterGenerator {
	private:
		Sort*	_sort;
	public:
		EqualInterGenerator(Sort* sort) : _sort(sort) { }
		PredInter* get(const AbstractStructure* structure);
};

class StrLessThanInterGenerator : public PredInterGenerator {
	private:
		Sort*	_sort;
	public:
		StrLessThanInterGenerator(Sort* sort) : _sort(sort) { }
		PredInter* get(const AbstractStructure* structure);
};

class StrGreaterThanInterGenerator : public PredInterGenerator {
	private:
		Sort*	_sort;
	public:
		StrGreaterThanInterGenerator(Sort* sort) : _sort(sort) { }
		PredInter* get(const AbstractStructure* structure);
};

class PredInterGeneratorGenerator {
	public:
		virtual PredInterGenerator* get(const std::vector<Sort*>&) = 0;
};

class EqualInterGeneratorGenerator : public PredInterGeneratorGenerator {
	public:
		 EqualInterGenerator* get(const std::vector<Sort*>&);
};

class StrGreaterThanInterGeneratorGenerator : public PredInterGeneratorGenerator {
	public:
		 StrGreaterThanInterGenerator* get(const std::vector<Sort*>&);
};

class StrLessThanInterGeneratorGenerator : public PredInterGeneratorGenerator {
	public:
		 StrLessThanInterGenerator* get(const std::vector<Sort*>&);
};

/**
 *	Class to represent a four-valued interpretation for functions
 */
class FuncInter {

	private:
		FuncTable*	_functable;		//!< the function table (if the interpretation is two-valued, nullpointer otherwise).
		PredInter*	_graphinter;	//!< the interpretation for the graph of the function

	public:
		
		FuncInter(FuncTable* ft, const std::vector<SortTable*>& univ);
		FuncInter(PredInter* pt) : _functable(0), _graphinter(pt) { }

		~FuncInter();

		void	graphinter(PredInter*);

		PredInter*	graphinter()		const { return _graphinter;			}
		FuncTable*	functable()			const { return _functable;			}
		bool		approxtwovalued()	const { return _functable != 0;		}

};

class FuncInterGenerator {
	public:
		virtual FuncInter* get(const AbstractStructure* structure) = 0;
};

class SingleFuncInterGenerator : public FuncInterGenerator {
	private:
		FuncInter*	_inter;
	public:
		SingleFuncInterGenerator(FuncInter* inter) : _inter(inter) { }
		FuncInter* get(const AbstractStructure* ) { return _inter;	}
};

class OneSortInterGenerator : public FuncInterGenerator {
	protected:
		Sort*	_sort;
	public:
		OneSortInterGenerator(Sort* sort) : _sort(sort) { }
};

class MinInterGenerator : public OneSortInterGenerator {
	public:
		MinInterGenerator(Sort* sort) : OneSortInterGenerator(sort) { }
		FuncInter* get(const AbstractStructure* structure);
};

class MaxInterGenerator : public OneSortInterGenerator {
	public:
		MaxInterGenerator(Sort* sort) : OneSortInterGenerator(sort) { }
		FuncInter* get(const AbstractStructure* structure);
};

class SuccInterGenerator : public OneSortInterGenerator {
	public:
		SuccInterGenerator(Sort* sort) : OneSortInterGenerator(sort) { }
		FuncInter* get(const AbstractStructure* structure);
};

class InvSuccInterGenerator : public OneSortInterGenerator {
	public:
		InvSuccInterGenerator(Sort* sort) : OneSortInterGenerator(sort) { }
		FuncInter* get(const AbstractStructure* structure);
};

class FuncInterGeneratorGenerator {
	public:
		virtual FuncInterGenerator* get(const std::vector<Sort*>&) = 0;
};

class MinInterGeneratorGenerator : public FuncInterGeneratorGenerator {
	public:
		 MinInterGenerator* get(const std::vector<Sort*>&);
};

class MaxInterGeneratorGenerator : public FuncInterGeneratorGenerator {
	public:
		 MaxInterGenerator* get(const std::vector<Sort*>&);
};

class SuccInterGeneratorGenerator : public FuncInterGeneratorGenerator {
	public:
		 SuccInterGenerator* get(const std::vector<Sort*>&);
};

class InvSuccInterGeneratorGenerator : public FuncInterGeneratorGenerator {
	public:
		 InvSuccInterGenerator* get(const std::vector<Sort*>&);
};

/*****************
	Structures
*****************/

/** Abstract base class **/

class AbstractStructure {
	protected:

		std::string		_name;			// The name of the structure
		ParseInfo		_pi;			// The place where this structure was parsed.
		Vocabulary*		_vocabulary;	// The vocabulary of the structure.

	public:
		// Constructors
		AbstractStructure(std::string name, const ParseInfo& pi) : _name(name), _pi(pi), _vocabulary(0) { }

		// Destructor
		virtual ~AbstractStructure() { }

		// Mutators
		virtual void	vocabulary(Vocabulary* v) { _vocabulary = v;	}	// set the vocabulary
		virtual AbstractStructure*	clone() = 0;	// take a clone of this structure

		// Inspectors
				const std::string&	name()						const { return _name;		}
				ParseInfo			pi()						const { return _pi;			}
				Vocabulary*			vocabulary()				const { return _vocabulary;	}
		virtual SortTable*			inter(Sort* s)				const = 0;	// Return the domain of s.
		virtual PredInter*			inter(Predicate* p)			const = 0;	// Return the interpretation of p.
		virtual FuncInter*			inter(Function* f)			const = 0;	// Return the interpretation of f.
		virtual PredInter*			inter(PFSymbol* s)			const = 0;	// Return the interpretation of s.

};

/** Structures as constructed by the parser **/

class Structure : public AbstractStructure {
	private:
		std::map<Sort*,SortTable*>		_sortinter;		//!< The domains of the structure. 
		std::map<Predicate*,PredInter*>	_predinter;		//!< The interpretations of the predicate symbols.
		std::map<Function*,FuncInter*>	_funcinter;		//!< The interpretations of the function symbols.
	
		void	functioncheck();	//!< check the correctness of the function tables
		void	autocomplete();		//!< make the domains consistent with the predicate and function tables				

	public:
		// Constructors
		Structure(const std::string& name, const ParseInfo& pi) : AbstractStructure(name,pi) { }

		// Destructor
		~Structure();

		// Mutators
		void	vocabulary(Vocabulary* v);			//!< set the vocabulary of the structure
		void	inter(Sort* s,SortTable* d);		//!< set the domain of s to d
		void	inter(Predicate* p, PredInter* i);	//!< set the interpretation of p to i
		void	inter(Function* f, FuncInter* i);	//!< set the interpretation of f to i
		Structure*	clone();						//!< take a clone of this structure

		// Inspectors
		SortTable*		inter(Sort* s)				const; //!< Return the domain of s.
		PredInter*		inter(Predicate* p)			const; //!< Return the interpretation of p.
		FuncInter*		inter(Function* f)			const; //!< Return the interpretation of f.
		PredInter*		inter(PFSymbol* s)			const; //!< Return the interpretation of s.
};

/************************
	Auxiliary methods
************************/

namespace TableUtils {
	PredInter*	leastPredInter(const std::vector<SortTable*>& sorts);	
		//!< construct a new, least precise predicate interpretation
	FuncInter*	leastFuncInter(const std::vector<SortTable*>& sorts);		
		//!< construct a new, least precise function interpretation
}

#endif
