/************************************
	structure.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef STRUCTURE_HPP
#define STRUCTURE_HPP

#include <string>
#include <vector>
#include <set>
#include <map>
#include <cassert>
#include <ostream>
#include <limits>

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

std::ostream& operator<< (std::ostream&,const DomainElementType&);

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

struct StrictWeakNTupleOrdering {
	unsigned int _arity;
	StrictWeakNTupleOrdering(unsigned int arity) : _arity(arity) { }
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

		Compound(Function* function, const ElementTuple& arguments);

	public:
		~Compound();

		Function*				function()				const;	//!< Returns the function of the compound
		const DomainElement*	arg(unsigned int n)		const;	//!< Returns the n'th argument of the compound
		const ElementTuple&		args()					const { return _arguments;	}

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


	public:
		~DomainElementFactory();

		static DomainElementFactory*	instance();

		const DomainElement*	create(int value);
		const DomainElement*	create(double value, bool certnotint = false);
		const DomainElement*	create(const std::string* value, bool certnotdouble = false);
		const DomainElement*	create(const Compound* value);
		const DomainElement*	create(Function*,const ElementTuple&);
		
		const Compound*		compound(Function*,const ElementTuple&);
};

/*******************
	Domain atoms
*******************/

class DomainAtomFactory;

class DomainAtom {
	private:
		PFSymbol*		_symbol;
		ElementTuple	_args;

		DomainAtom(PFSymbol* symbol, const ElementTuple& args) : _symbol(symbol), _args(args) { }

	public:
		~DomainAtom() { }

		PFSymbol*			symbol()	const { return _symbol;	}
		const ElementTuple&	args()		const { return _args;	} 

		std::ostream&	put(std::ostream&)	const;
		std::string		to_string()			const;

		friend class DomainAtomFactory;
		
};

class DomainAtomFactory {
	private:
		static DomainAtomFactory*								_instance;
		std::map<PFSymbol*,std::map<ElementTuple,DomainAtom*> >	_atoms;	
		DomainAtomFactory() { }

	public:
		~DomainAtomFactory();
		static	DomainAtomFactory*	instance();
				const DomainAtom*	create(PFSymbol*,const ElementTuple&);
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
		TableIterator() : _iterator(0) { }
		TableIterator(const TableIterator&);
		TableIterator(InternalTableIterator* iter) : _iterator(iter) { }
		TableIterator& operator=(const TableIterator&);
		bool					hasNext()	const;
		const ElementTuple&		operator*()	const;
		void					operator++();
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

class CartesianInternalTableIterator : public InternalTableIterator {
	private:
		std::vector<SortIterator>	_iterators;
		std::vector<SortIterator>	_lowest;
		mutable ElementTable		_deref;
		bool						_hasNext;
		bool						hasNext()	const;
		const ElementTuple&			operator*()	const;
		void						operator++();
	public:
		CartesianInternalTableIterator(const std::vector<SortIterator>& vsi, const std::vector<SortIterator>& low, bool h = true); 
		~CartesianInternalTableIterator() { }
		CartesianInternalTableIterator*	clone()	const;

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
class InternalPredTable; 

typedef std::pair<bool,unsigned int> tablesize;

class Universe {
	private:
		std::vector<SortTable*>		_tables;
	public:
		Universe() { }
		Universe(const std::vector<SortTable*>& tables) : _tables(tables) { }
		Universe(const Universe& univ) : _tables(univ.tables()) { }
		~Universe() { }

		const std::vector<SortTable*>& tables() const { return _tables;	}
		unsigned int					arity()	const { return _tables.size();	}

		bool		empty()							const;
		bool		finite()						const;
		bool		approxempty()					const;
		bool		approxfinite()					const;
		bool		contains(const ElementTuple&)	const;
		tablesize	size()							const;
};

class InternalFuncTable;

class InternalFuncIterator : public InternalTableIterator {
	private:
		TableIterator					_curr;
		mutable ElementTable			_deref;
		const InternalFuncTable*		_function;
		bool							hasNext()	const { return _curr.hasNext();	}
		const ElementTuple&				operator*()	const;
		void							operator++();
	public:
		InternalFuncIterator(const InternalFuncTable* f, const Universe& univ);
		InternalFuncIterator(const InternalFuncTable* f, const TableIterator& c) : 
			_curr(c), _function(f) { }
		~InternalFuncIterator() {	}
		InternalFuncIterator* clone() const { return new InternalFuncIterator(_function,_curr);	}
		
};

class InternalPredTable;

class ProcInternalTableIterator : public InternalTableIterator {
	private:
		TableIterator					_curr;
		Universe						_univ;
		mutable ElementTable			_deref;
		const InternalPredTable*		_predicate;
		bool							hasNext()	const { return _curr.hasNext();	}
		const ElementTuple&				operator*()	const;
		void							operator++();
	public:
		ProcInternalTableIterator(const InternalPredTable* p, const Universe& univ);
		ProcInternalTableIterator(const InternalPredTable* p, const TableIterator& c, const Universe& univ) : 
			_curr(c), _univ(univ), _predicate(p) { }
		~ProcInternalTableIterator() {	}
		ProcInternalTableIterator* clone() const { return new ProcInternalTableIterator(_predicate,_curr,_univ);	}
		
};

class UnionInternalIterator : public InternalTableIterator {
	private:
		std::vector<TableIterator>				_iterators;
		Universe								_universe;
		std::vector<InternalPredTable*>			_outtables;
		std::vector<TableIterator>::iterator	_curriterator;

		bool contains(const ElementTuple&)	const;
		void setcurriterator();

		bool					hasNext()	const;	
		const ElementTuple&		operator*()	const;
		void					operator++();	
	public:
		UnionInternalIterator(const std::vector<TableIterator>&, const std::vector<InternalPredTable*>&, const Universe&);
		~UnionInternalIterator() { }
		UnionInternalIterator*	clone() const;
};

class UNAInternalIterator : public InternalTableIterator {
	private:
		std::vector<SortIterator>	_curr;
		std::vector<SortIterator>	_lowest;
		Function*					_function;
		mutable bool						_end;
		mutable ElementTuple				_currtuple;
		mutable std::vector<ElementTuple>	_deref;

		bool					hasNext()	const;	
		const ElementTuple&		operator*()	const;
		void					operator++();	
		UNAInternalIterator(const std::vector<SortIterator>&, const std::vector<SortIterator>&, Function*, bool);
	public:
		UNAInternalIterator(const std::vector<SortIterator>&, Function*);
		~UNAInternalIterator() { }
		UNAInternalIterator* clone() const;
};

class InverseInternalIterator : public InternalTableIterator {
	private:
		std::vector<SortIterator>	_curr;
		std::vector<SortIterator>	_lowest;
		Universe					_universe;
		InternalPredTable*			_outtable;
		mutable bool						_end;
		mutable ElementTuple				_currtuple;
		mutable std::vector<ElementTuple>	_deref;

		bool					hasNext()	const;	
		const ElementTuple&		operator*()	const;
		void					operator++();	
		InverseInternalIterator(const std::vector<SortIterator>&, const std::vector<SortIterator>&, InternalPredTable*, const Universe&, bool);
	public:
		InverseInternalIterator(const std::vector<SortIterator>&, InternalPredTable*, const Universe&);
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
		StrLessThanInternalIterator(const SortIterator& l, const SortIterator& r) :
			_leftiterator(l), _rightiterator(r) { }
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
		StrGreaterThanInternalIterator(const SortIterator& l, const SortIterator& r, const SortIterator& m) :
			_leftiterator(l), _rightiterator(r), _lowest(m) { }
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
		NatInternalSortIterator(int iter = 0) : _iter(iter) { }
		~NatInternalSortIterator() { }
		NatInternalSortIterator* clone()	const { return new NatInternalSortIterator(_iter);	}
};

class IntInternalSortIterator : public InternalSortIterator {
	private:
		int _iter;
		bool					hasNext()	const { return true;	}
		const DomainElement*	operator*()		const	{ return DomainElementFactory::instance()->create(_iter);	}
		void					operator++()			{ ++_iter;		}	
	public:
		IntInternalSortIterator(int iter = std::numeric_limits<int>::min()) : _iter(iter) { }
		~IntInternalSortIterator() { }
		IntInternalSortIterator* clone()	const { return new IntInternalSortIterator(_iter);	}

};

class FloatInternalSortIterator : public InternalSortIterator {
	private:
		double _iter;
		bool					hasNext()		const { return true;	}
		const DomainElement*	operator*()		const	{ return DomainElementFactory::instance()->create(_iter);	}
		void					operator++()	{ ++_iter;	}
	public:
		FloatInternalSortIterator(double iter = std::numeric_limits<double>::min()) : _iter(iter) { }
		~FloatInternalSortIterator() { }
		FloatInternalSortIterator* clone()	const { return new FloatInternalSortIterator(_iter);	}

};

class StringInternalSortIterator : public InternalSortIterator {
	private:
		std::string _iter;
		bool					hasNext()		const { return true;	}
		const DomainElement*	operator*()		const;	
		void					operator++();
	public:
		StringInternalSortIterator(const std::string& iter = "") : _iter(iter) { }
		~StringInternalSortIterator() { }
		StringInternalSortIterator* clone()	const { return new StringInternalSortIterator(_iter);	}

};

class CharInternalSortIterator : public InternalSortIterator {
	private:
		char _iter;
		bool _end;
		bool					hasNext()		const { return !_end;	}
		const DomainElement*	operator*()		const;	
		void					operator++();
	public:
		CharInternalSortIterator(char iter = std::numeric_limits<char>::min(), bool end = false) : _iter(iter), _end(end) { }
		~CharInternalSortIterator() { }
		CharInternalSortIterator* clone()	const { return new CharInternalSortIterator(_iter);	}

};

class EnumInternalSortIterator : public InternalSortIterator {
	private:
		SortedElementTuple::const_iterator	_iter;
		SortedElementTuple::const_iterator	_end;
		bool					hasNext()	const	{ return _iter != _end;	}
		const DomainElement*	operator*()	const	{ return *_iter;		}
		void					operator++()		{ ++_iter;				}
	public:
		EnumInternalSortIterator(SortedElementTuple::iterator it, SortedElementTuple::iterator end) : _iter(it), _end(end) { }
		~EnumInternalSortIterator() { }
		EnumInternalSortIterator* clone()	const { return new EnumInternalSortIterator(_iter,_end);	}
};

class RangeInternalSortIterator : public InternalSortIterator {
	private:
		int	_current;
		int _last;
		bool					hasNext()	const	{ return _current <= _last;	}
		const DomainElement*	operator*()	const	{ return DomainElementFactory::instance()->create(_current);	}
		void					operator++()		{ ++_current;				}
	public:
		RangeInternalSortIterator(int current, int last) : _current(current), _last(last) { }
		~RangeInternalSortIterator() { }
		RangeInternalSortIterator* clone()	const { return new RangeInternalSortIterator(_current,_last);	}
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
		virtual bool	finite(const Universe&)		const = 0;	//!< Returns true iff the table is finite
		virtual	bool	empty(const Universe&)		const = 0;	//!< Returns true iff the table is empty

		virtual bool	approxfinite(const Universe&)	const = 0;
			//!< Returns false if the table size is infinite. May return true if the table size is finite.
		virtual bool	approxempty(const Universe&)	const = 0;
			//!< Returns false if the table is non-empty. May return true if the table is empty.

		virtual	bool	contains(const ElementTuple& tuple, const Universe&) const = 0;	
			//!< Returns true iff the table contains the tuple. 
		virtual tablesize	size(const Universe&)	const = 0;	


		// Mutators
		virtual InternalPredTable*	add(const ElementTuple& tuple) = 0;		//!< Add a tuple to the table
		virtual InternalPredTable*	remove(const ElementTuple& tuple) = 0;	//!< Remove a tuple from the table

		void decrementRef();	//!< Delete one reference. Deletes the table if the number of references becomes zero.
		void incrementRef();	//!< Add one reference

		// Iterators
		virtual InternalTableIterator*		begin(const Universe&)	const = 0;

		InternalPredTable() : _nrRefs(0)	{ }
		virtual ~InternalPredTable()		{ }

	friend class PredTable;
	friend class SortTable;
};

class ProcInternalPredTable : public InternalPredTable {
	private:
		std::string*	_procedure;
	public:
		ProcInternalPredTable(std::string* proc) :
			InternalPredTable(), _procedure(proc) { }

		~ProcInternalPredTable();

		bool			finite(const Universe&)			const;
		bool			empty(const Universe&)			const;
		bool			approxfinite(const Universe&)	const;
		bool			approxempty(const Universe&)	const;
		tablesize		size(const Universe&)			const { return tablesize(false,0);	}

		bool	contains(const ElementTuple& tuple,const Universe&)		const;

		InternalPredTable*	add(const ElementTuple& tuple);		//!< Add a tuple to the table
		InternalPredTable*	remove(const ElementTuple& tuple);	//!< Remove a tuple from the table

		InternalTableIterator*	begin(const Universe&)	const;

};

class FullInternalPredTable : public InternalPredTable {
	private:
	public:
		FullInternalPredTable() : InternalPredTable() { }

		bool			finite(const Universe&)			const;
		bool			empty(const Universe&)			const;
		bool			approxfinite(const Universe&)	const;
		bool			approxempty(const Universe&)	const;
		tablesize		size(const Universe& univ)		const { return univ.size();	}

		bool	contains(const ElementTuple& tuple,const Universe&)		const;

		InternalPredTable*	add(const ElementTuple& tuple);		//!< Add a tuple to the table
		InternalPredTable*	remove(const ElementTuple& tuple);	//!< Remove a tuple from the table

		InternalTableIterator*	begin(const Universe&)	const;

		~FullInternalPredTable();

};

class FuncTable;

class FuncInternalPredTable : public InternalPredTable {
	private:
		FuncTable*	_table;
		bool		_linked;
	public:
		FuncInternalPredTable(FuncTable* table, bool linked);

		bool			finite(const Universe&)			const;
		bool			empty(const Universe&)			const;
		bool			approxfinite(const Universe&)	const;
		bool			approxempty(const Universe&)	const;
		tablesize		size(const Universe& univ)		const;

		bool	contains(const ElementTuple& tuple,const Universe&)		const;

		InternalPredTable*	add(const ElementTuple& tuple);		//!< Add a tuple to the table
		InternalPredTable*	remove(const ElementTuple& tuple);	//!< Remove a tuple from the table

		InternalTableIterator*	begin(const Universe&)	const;

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
		std::vector<InternalPredTable*>	_intables;	
			//!< a tuple of the table does belong to at least one of the tables in _intables
		std::vector<InternalPredTable*>	_outtables;	
			//!< a tuple of the table does not belong to any of the tables in _outtables

		bool			finite(const Universe&)			const;
		bool			empty(const Universe&)			const;
		bool			approxfinite(const Universe&)	const;
		bool			approxempty(const Universe&)	const;
		tablesize		size(const Universe& )			const { return tablesize(false,0);	}

		bool	contains(const ElementTuple& tuple,const Universe&) const;	

		InternalTableIterator*	begin(const Universe&)	const;

	public:
		UnionInternalPredTable();
		UnionInternalPredTable(const std::vector<InternalPredTable*>& in, const std::vector<InternalPredTable*>& out);
		~UnionInternalPredTable();
		void	addInTable(InternalPredTable* t)	{ _intables.push_back(t); t->incrementRef();	}
		void	addOutTable(InternalPredTable* t)	{ _outtables.push_back(t); t->incrementRef();	}
		InternalPredTable*	add(const ElementTuple& tuple);		
		InternalPredTable*	remove(const ElementTuple& tuple);	
	
};


/**
 *	This class implements a finite, enumerated InternalPredTable
 */
class EnumeratedInternalPredTable : public InternalPredTable {
	private:
		SortedElementTable	_table;		//!< the actual table

		bool			finite(const Universe&)			const { return true;			}
		bool			empty(const Universe&)			const { return _table.empty();	}
		bool			approxfinite(const Universe&)	const { return true;			}
		bool			approxempty(const Universe&)	const { return _table.empty();	}
		tablesize		size(const Universe&)			const { return tablesize(true,_table.size());	}

		bool			contains(const ElementTuple& tuple, const Universe&) const;


		InternalTableIterator*		begin(const Universe&) const;

	public:
		EnumeratedInternalPredTable(const SortedElementTable& tab) :
			InternalPredTable(), _table(tab) { }
		EnumeratedInternalPredTable() : InternalPredTable() { }
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
	public:
		ComparisonInternalPredTable();
		virtual ~ComparisonInternalPredTable();
		InternalPredTable*	add(const ElementTuple& tuple);
		InternalPredTable*	remove(const ElementTuple& tuple);
};

/**
 *	Internal table for '=/2'
 */
class EqualInternalPredTable : public ComparisonInternalPredTable {
	public:
		EqualInternalPredTable() : ComparisonInternalPredTable() { } 
		~EqualInternalPredTable() { }

		bool		contains(const ElementTuple&, const Universe&)	const;
		bool		finite(const Universe&)							const;
		bool		empty(const Universe&)							const;
		bool		approxfinite(const Universe&)					const;
		bool		approxempty(const Universe&)					const;
		tablesize	size(const Universe&)							const;

		InternalTableIterator*		begin(const Universe&) const;
};

/**
 *	Internal table for '</2'
 */
class StrLessInternalPredTable : public ComparisonInternalPredTable {
	public:
		StrLessInternalPredTable() : ComparisonInternalPredTable() { } 
		~StrLessInternalPredTable() { }

		bool		contains(const ElementTuple&, const Universe&)	const;
		bool		finite(const Universe&)							const;
		bool		empty(const Universe&)							const;
		bool		approxfinite(const Universe&)					const;
		bool		approxempty(const Universe&)					const;
		tablesize	size(const Universe&)							const;

		InternalTableIterator*		begin(const Universe&) const;
};

/**
 *	Internal table for '>/2'
 */
class StrGreaterInternalPredTable : public ComparisonInternalPredTable {
	public:
		StrGreaterInternalPredTable() : ComparisonInternalPredTable() { } 
		~StrGreaterInternalPredTable() { }

		bool		contains(const ElementTuple&, const Universe&)	const;
		bool		finite(const Universe&)							const;
		bool		empty(const Universe&)							const;
		bool		approxfinite(const Universe&)					const;
		bool		approxempty(const Universe&)					const;
		tablesize	size(const Universe&)							const;

		InternalTableIterator*		begin(const Universe&) const;
};

/**
 *	This class implements the complement of an internal predicate table
 */
class InverseInternalPredTable : public InternalPredTable {
	private:
		InternalPredTable*	_invtable;		//!< the inverse of the actual table

	public:
		InverseInternalPredTable(InternalPredTable* inv) :
			InternalPredTable(), _invtable(inv) { inv->incrementRef(); }
		~InverseInternalPredTable();

		bool			finite(const Universe&)					const;
		bool			empty(const Universe&)					const;
		bool			approxfinite(const Universe&)			const;
		bool			approxempty(const Universe&)			const;
		tablesize		size(const Universe&)					const;

		bool	contains(const ElementTuple& tuple, const Universe&) const;

		InternalPredTable*	add(const ElementTuple& tuple);
		InternalPredTable*	remove(const ElementTuple& tuple);

		InternalTableIterator*	begin(const Universe&) const;

		void	interntable(InternalPredTable*);
};

/********************************
	Internal tables for sorts
********************************/

/**
 *	This class implements a concrete one-dimensional table
 */
class InternalSortTable : public InternalPredTable {
	protected:

	public:
		InternalSortTable() { }

		unsigned int arity()	const	{ return 1;	}

		virtual bool	contains(const DomainElement*) const = 0;
				bool	contains(const ElementTuple& tuple)						const { return contains(tuple[0]);	}
				bool	contains(const ElementTuple& tuple, const Universe&)	const { return contains(tuple[0]);	}

		virtual bool		empty()			const = 0;
		virtual bool		finite()		const = 0;
		virtual bool		approxempty()	const = 0;
		virtual bool		approxfinite()	const = 0;
		virtual tablesize	size()			const = 0;
				bool		empty(const Universe&)			const { return empty();			}
				bool		finite(const Universe&)			const { return finite();		}
				bool		approxempty(const Universe&)	const { return approxempty();	}
				bool		approxfinite(const Universe&)	const { return approxfinite();	}
				tablesize	size(const Universe&)			const { return size();			}

		virtual InternalSortTable*	add(const DomainElement*)			= 0;
				InternalSortTable*	add(const ElementTuple& tuple)		{ return add(tuple[0]);		}
		virtual InternalSortTable*	remove(const DomainElement*)		= 0;
				InternalSortTable*	remove(const ElementTuple& tuple)	{ return remove(tuple[0]);	}
		virtual InternalSortTable*	add(int i1, int i2)					= 0;

		virtual InternalSortIterator*	sortbegin()				const = 0;
				InternalTableIterator*	begin()					const;
				InternalTableIterator*	begin(const Universe&)	const { return begin();	}

		virtual	const DomainElement*	first()		const = 0;
		virtual	const DomainElement*	last()		const = 0;
		virtual bool					isRange()	const = 0;
	
		virtual ~InternalSortTable() { }

};

class UnionInternalSortTable : public InternalSortTable {
	public:
		std::vector<SortTable*>	_intables;	
			//!< an element of the table does belong to at least one of the tables in _intables
		std::vector<SortTable*>	_outtables;	
			//!< an element of the table does not belong to any of the tables in _outtables

		bool		finite()		const;
		bool		empty()			const;
		bool		approxfinite()	const;
		bool		approxempty()	const;
		tablesize	size()			const { return tablesize(false,0);	}

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
		InternalSortTable*	add(int i1, int i2);

		const DomainElement*	first()		const;
		const DomainElement*	last()		const;
		bool					isRange()	const;
	
};

class InfiniteInternalSortTable : public InternalSortTable {
	private:
		InternalSortTable*	add(const DomainElement*);
		InternalSortTable*	remove(const DomainElement*);
		bool		finite()		const { return false;	}
		bool		empty()			const { return false;	}
		bool		approxfinite()	const { return false;	}
		bool		approxempty()	const { return false;	}
		tablesize	size()			const { return tablesize(false,0);	}
	protected:
		virtual ~InfiniteInternalSortTable() { }
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
		InternalSortTable*	add(int i1, int i2);
		const DomainElement*	first()	const;
		const DomainElement*	last()	const;
		bool					isRange()	const { return true;	}
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
		InternalSortTable*	add(int i1, int i2);
		const DomainElement*	first()	const;
		const DomainElement*	last()	const;
		bool					isRange()	const { return true;	}
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
		InternalSortTable*	add(int i1, int i2);
		const DomainElement*	first()	const;
		const DomainElement*	last()	const;
		bool					isRange()	const { return true;	}
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
		InternalSortTable*	add(int i1, int i2);
		const DomainElement*	first()	const;
		const DomainElement*	last()	const;
		bool					isRange()	const { return true;	}
};

/**
 * All characters
 */
class AllChars : public InternalSortTable {
	private:
		bool					contains(const DomainElement*)	const;
		InternalSortTable*		add(const DomainElement*);
		InternalSortTable*		remove(const DomainElement*);
		InternalSortTable*		add(int i1, int i2);

		InternalSortIterator*	sortbegin()	const;

		bool		finite()		const { return true;	}
		bool		empty()			const { return false;	}
		bool		approxfinite()	const { return true;	}
		bool		approxempty()	const { return false;	}
		tablesize	size()			const;
	protected:
		~AllChars() { }
		const DomainElement*	first()	const;
		const DomainElement*	last()	const;
		bool					isRange()	const { return true;	}
};

/**
 *	A finite, enumerated SortTable
 */
class EnumeratedInternalSortTable : public InternalSortTable {
	private:
		SortedElementTuple	_table;

		bool					contains(const DomainElement*)	const;

		InternalSortIterator*	sortbegin()	const;

		bool finite()		const { return true;			}
		bool empty()		const { return _table.empty();	}	
		bool approxfinite()	const { return true;			}
		bool approxempty()	const { return _table.empty();	}
		tablesize	size()	const { return tablesize(true,_table.size());	}
	protected:
	public:
		EnumeratedInternalSortTable() { }
		~EnumeratedInternalSortTable() { }
		EnumeratedInternalSortTable(const SortedElementTuple& d) : _table(d) { }
		InternalSortTable*		add(const DomainElement*);
		InternalSortTable*		remove(const DomainElement*);
		InternalSortTable*		add(int i1, int i2);
		const DomainElement*	first()		const;
		const DomainElement*	last()		const;
		bool					isRange()	const;
};

/**
 *		A range of integers
 */
class IntRangeInternalSortTable : public InternalSortTable {
	private:
		int _first;		//!< first element in the range
		int _last;		//!< last element in the range
	public:
		IntRangeInternalSortTable(int f, int l) : _first(f), _last(l) { }
		bool				finite()		const	{ return approxfinite();	}
		bool				empty()			const	{ return approxempty();		}
		bool				approxfinite()	const	{ return true;				}
		bool				approxempty()	const	{ return _first > _last;	}
		tablesize			size()			const	{ return tablesize(true,_last - _first + 1);	}
		InternalSortTable*	add(const DomainElement*);
		InternalSortTable*	remove(const DomainElement*);
		InternalSortTable*	add(int i1, int i2);
		const DomainElement*	first()	const;
		const DomainElement*	last()	const;

		bool contains(const DomainElement*) const;
		bool isRange()						const { return true;	}

		InternalSortIterator*	sortbegin()	const;

};

/************************************
	Internal tables for functions
************************************/

/**
 *		This class implements a concrete associative array mapping tuples of elements to elements
 */
class InternalFuncTable {
	protected:
		unsigned int	_nrRefs;
	public:
		InternalFuncTable() : _nrRefs(0) { }
		virtual ~InternalFuncTable() { }

		void decrementRef();	//!< Delete one reference. Deletes the table if the number of references becomes zero.
		void incrementRef();	//!< Add one reference

		virtual bool			finite(const Universe&)	const = 0;	//!< Returns true iff the table is finite
		virtual	bool			empty(const Universe&)		const = 0;	//!< Returns true iff the table is empty

		virtual bool			approxfinite(const Universe&)			const = 0;
			//!< Returns false if the table size is infinite. May return true if the table size is finite.
		virtual bool			approxempty(const Universe&)			const = 0;
			//!< Returns false if the table is non-empty. May return true if the table is empty.
		virtual tablesize		size(const Universe&)					const = 0;
		
				bool				 contains(const ElementTuple& tuple,const Universe&)	const;
		virtual const DomainElement* operator[](const ElementTuple& tuple)	const = 0;	
			//!< Returns the value of the tuple according to the array.

		virtual	InternalFuncTable*	add(const ElementTuple&)	= 0;	//!< Add a tuple to the table
		virtual InternalFuncTable*	remove(const ElementTuple&)	= 0;	//!< Remove a tuple from the table

		virtual InternalTableIterator*	begin(const Universe&)	const = 0;
};

class ProcInternalFuncTable : public InternalFuncTable {
	private:
		std::string*			_procedure;
	public:
		ProcInternalFuncTable(std::string* proc) :
			InternalFuncTable(), _procedure(proc) { }

		~ProcInternalFuncTable();

		bool		finite(const Universe&)			const;
		bool		empty(const Universe&)			const; 
		bool		approxfinite(const Universe&)	const;
		bool		approxempty(const Universe&)	const; 
		tablesize	size(const Universe&)			const { return tablesize(false,0);	}
		
		const DomainElement*	operator[](const ElementTuple& tuple) const;
		InternalFuncTable*		add(const ElementTuple&);	
		InternalFuncTable*		remove(const ElementTuple&);

		InternalTableIterator*	begin(const Universe&)	const;
};

class UNAInternalFuncTable : public InternalFuncTable {
	private:
		Function*	_function;
	public:
		UNAInternalFuncTable(Function* f) :
			InternalFuncTable(), _function(f) { }

		~UNAInternalFuncTable() { }

		bool		finite(const Universe&)			const;
		bool		empty(const Universe&)			const; 
		bool		approxfinite(const Universe&)	const;
		bool		approxempty(const Universe&)	const; 
		tablesize	size(const Universe&)			const;
		
		const DomainElement*	operator[](const ElementTuple& tuple) const;
		InternalFuncTable*		add(const ElementTuple&);	
		InternalFuncTable*		remove(const ElementTuple&);

		InternalTableIterator*	begin(const Universe&)	const;
};

/**
 *		A finite, enumerated InternalFuncTable
 */
class EnumeratedInternalFuncTable : public InternalFuncTable {
	private:
		ElementFunc	_table;
	public:
		EnumeratedInternalFuncTable() : InternalFuncTable() { }
		EnumeratedInternalFuncTable(const ElementFunc& tab) : 
			InternalFuncTable(), _table(tab) { }
		~EnumeratedInternalFuncTable() { }

		bool		finite(const Universe&)			const { return true;							}
		bool		empty(const Universe&)			const { return _table.empty();					}
		bool		approxfinite(const Universe&)	const { return true;							}
		bool		approxempty(const Universe&)	const { return _table.empty();					}
		tablesize	size(const Universe&)			const { return tablesize(true,_table.size());	}
		
		const DomainElement*	operator[](const ElementTuple& tuple) const;
		InternalFuncTable*		add(const ElementTuple&);	
		InternalFuncTable*		remove(const ElementTuple&);

		InternalTableIterator*	begin(const Universe&)	const;
};

class IntFloatInternalFuncTable : public InternalFuncTable {
	protected:
		bool	_int;
	public:

		IntFloatInternalFuncTable(bool i) : _int(i) { }

		bool		finite(const Universe&)			const { return false;	}
		bool		empty(const Universe&)			const { return false;	}
		bool		approxfinite(const Universe&)	const { return false;	}
		bool		approxempty(const Universe&)	const { return false;	}
		tablesize	size(const Universe&)			const { return tablesize(false,0);	}

		InternalFuncTable*	add(const ElementTuple&);
		InternalFuncTable*	remove(const ElementTuple&);

		virtual InternalTableIterator*	begin(const Universe&)	const = 0;

};

class PlusInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		PlusInternalFuncTable(bool i) : IntFloatInternalFuncTable(i) { }
		const DomainElement*	operator[](const ElementTuple&)	const;
		InternalTableIterator*	begin(const Universe&)	const;
};

class MinusInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		MinusInternalFuncTable(bool i) : IntFloatInternalFuncTable(i) { }
		const DomainElement*	operator[](const ElementTuple&)	const;
		InternalTableIterator*	begin(const Universe&)	const;
};

class TimesInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		TimesInternalFuncTable(bool i) : IntFloatInternalFuncTable(i) { }
		const DomainElement*	operator[](const ElementTuple&)	const;
		InternalTableIterator*	begin(const Universe&)	const;
};

class DivInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		DivInternalFuncTable(bool i) : IntFloatInternalFuncTable(i) { }
		const DomainElement*	operator[](const ElementTuple&)	const;
		InternalTableIterator*	begin(const Universe&)	const;
};

class AbsInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		AbsInternalFuncTable(bool i) : IntFloatInternalFuncTable(i) { }
		const DomainElement*	operator[](const ElementTuple&)	const;
		InternalTableIterator*	begin(const Universe&)	const;
};

class UminInternalFuncTable : public IntFloatInternalFuncTable {
	public:
		UminInternalFuncTable(bool i) : IntFloatInternalFuncTable(i) { }
		const DomainElement*	operator[](const ElementTuple&)	const;
		InternalTableIterator*	begin(const Universe&)	const;
};

class ExpInternalFuncTable : public InternalFuncTable {
	public:
		bool		finite(const Universe&)			const { return false;	}
		bool		empty(const Universe&)			const { return false;	}
		bool		approxfinite(const Universe&)	const { return false;	}
		bool		approxempty(const Universe&)	const { return false;	}
		tablesize	size(const Universe&)			const { return tablesize(false,0);	}
		const DomainElement*	operator[](const ElementTuple&)	const;

		InternalFuncTable*	add(const ElementTuple&);
		InternalFuncTable*	remove(const ElementTuple&);

		InternalTableIterator*	begin(const Universe&)	const;

		ExpInternalFuncTable() { }
		~ExpInternalFuncTable() { }
};

class ModInternalFuncTable : public InternalFuncTable {
	public:
		ModInternalFuncTable() { }
		~ModInternalFuncTable() { }
		bool		finite(const Universe&)			const { return false;	}
		bool		empty(const Universe&)			const { return false;	}
		bool		approxfinite(const Universe&)	const { return false;	}
		bool		approxempty(const Universe&)	const { return false;	}
		tablesize	size(const Universe&)			const { return tablesize(false,0);	}
		const DomainElement*	operator[](const ElementTuple&)	const;

		InternalFuncTable*	add(const ElementTuple&);
		InternalFuncTable*	remove(const ElementTuple&);

		InternalTableIterator*	begin(const Universe&)	const;
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
		Universe			_universe;
	public:
		PredTable(InternalPredTable* table, const Universe&);
		~PredTable();

		bool				finite()							const	{ return _table->finite(_universe);			}
		bool				empty()								const	{ return _table->empty(_universe);			}
		unsigned int		arity()								const	{ return _universe.arity();					}
		bool				approxfinite()						const	{ return _table->approxfinite(_universe);	}
		bool				approxempty()						const	{ return _table->approxempty(_universe);	}
		bool				contains(const ElementTuple& tuple)	const	{ return _table->contains(tuple,_universe);	}
		tablesize			size()								const	{ return _table->size(_universe);			}
		void				add(const ElementTuple& tuple);
		void				remove(const ElementTuple& tuple);

		TableIterator		begin() const;

		InternalPredTable*	interntable()	const { return _table;	}

		const Universe&		universe()	const { return _universe;	}
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

		void	interntable(InternalSortTable*);

		bool			finite()							const	{ return _table->finite();			}
		bool			empty()								const	{ return _table->empty();			}
		bool			approxfinite()						const	{ return _table->approxfinite();	}
		bool			approxempty()						const	{ return _table->approxempty();		}
		unsigned int	arity()								const	{ return 1;							}
		tablesize		size()								const	{ return _table->size();			}
		bool			contains(const ElementTuple& tuple)	const	{ return _table->contains(tuple);	}
		bool			contains(const DomainElement* el)	const	{ return _table->contains(el);		}
		void			add(const ElementTuple& tuple);	
		void			add(const DomainElement* el);	
		void			add(int i1, int i2);			
		void			remove(const ElementTuple& tuple);	
		void			remove(const DomainElement* el);	
		TableIterator 	begin()								const;
		SortIterator 	sortbegin()							const;

		const DomainElement*	first()		const { return _table->first();	}
		const DomainElement*	last()		const { return _table->last();	}
		bool					isRange()	const { return _table->isRange();	}

		InternalSortTable*	interntable()	const { return _table;	}
};

/**
 *	This class implements tables for function symbols
 */
class FuncTable : public AbstractTable {
	private:
		InternalFuncTable*	_table;	//!< Points to the actual table
		Universe			_universe;
	public:
		FuncTable(InternalFuncTable* table, const Universe&);
		~FuncTable();

		bool			finite()				const	{ return _table->finite(_universe);			}
		bool			empty()					const	{ return _table->empty(_universe);			}
		unsigned int	arity()					const	{ return _universe.arity() - 1;				}
		bool			approxfinite()			const	{ return _table->approxfinite(_universe);	}
		bool			approxempty()			const	{ return _table->approxempty(_universe);	}
		tablesize		size()					const	{ return _table->size(_universe);			}

		const DomainElement* operator[](const ElementTuple& tuple) const {
			return _table->operator[](tuple);
		}
		bool	contains(const ElementTuple& tuple)	const;
		void	add(const ElementTuple& tuple);	
		void	remove(const ElementTuple& tuple);

		TableIterator	begin()	const;

		InternalFuncTable*	interntable()	const { return _table;	}

		const Universe&	universe() const { return _universe;	}

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
		
		PredInter(PredTable* ctpf,PredTable* cfpt,bool ct, bool cf);
		PredInter(PredTable* ctpf, bool ct);

		// Destructor
		~PredInter();

		// Mutators
		void ct(PredTable*);		//!< Replace the certainly true (and possibly false) tuples
		void cf(PredTable*);		//!< Replace the certainly false (and possibly true) tuples
		void pt(PredTable*);		//!< Replace the possibly true (and certainly false) tuples
		void pf(PredTable*);        //!< Replace the possibly false (and certainly true) tuples
		void ctpt(PredTable*);		//!< Replace the certainly and possibly true tuples

		void makeTrue(const ElementTuple&);		//!< Make the given tuple true
		void makeFalse(const ElementTuple&);	//!< Make the given tuple false
		void makeUnknown(const ElementTuple&);	//!< Make the given tuple unknown

		// Inspectors
		const PredTable*	ct()										const { return _ct;		}
		const PredTable*	cf()										const { return _cf;		}
		const PredTable*	pt()										const { return _pt;		}
		const PredTable*	pf()										const { return _pf;		}
		bool				istrue(const ElementTuple& tuple)			const;
		bool				isfalse(const ElementTuple& tuple)			const;
		bool				isunknown(const ElementTuple& tuple)		const;
		bool				isinconsistent(const ElementTuple& tuple)	const;
		bool				approxtwovalued()							const;
		const Universe&		universe()									const { return _ct->universe();	}
		PredInter*			clone(const Universe&)						const;

};

class AbstractStructure;

class PredInterGenerator {
	public:
		virtual PredInter* get(const AbstractStructure* structure) = 0;
};

class Sort;

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
		
		FuncInter(FuncTable* ft);
		FuncInter(PredInter* pt) : _functable(0), _graphinter(pt) { }

		~FuncInter();

		void	graphinter(PredInter*);
		void	functable(FuncTable*);
		void	makeTrue(const ElementTuple&);
		void	makeFalse(const ElementTuple&);

		PredInter*	graphinter()		const { return _graphinter;			}
		FuncTable*	functable()			const { return _functable;			}
		bool		approxtwovalued()	const { return _functable != 0;		}

		const Universe&	universe()	const { return _graphinter->universe();	}
		FuncInter*	clone(const Universe&)	const;
};

class FuncInterGenerator {
	public:
		virtual FuncInter* get(const AbstractStructure* structure) = 0;
		virtual ~FuncInterGenerator() { }
};

class SingleFuncInterGenerator : public FuncInterGenerator {
	private:
		FuncInter*	_inter;
	public:
		SingleFuncInterGenerator(FuncInter* inter) : _inter(inter) { }
		~SingleFuncInterGenerator() { delete(_inter);	}
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

class Predicate;

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

		virtual void	inter(Predicate* p, PredInter* i) = 0;	//!< set the interpretation of p to i
		virtual void	inter(Function* f, FuncInter* i) = 0;	//!< set the interpretation of f to i
		virtual void	clean()	= 0;							//!< make three-valued interpretations that are in fact
																//!< two-valued, two-valued.

		// Inspectors
				const std::string&	name()						const { return _name;		}
				ParseInfo			pi()						const { return _pi;			}
				Vocabulary*			vocabulary()				const { return _vocabulary;	}
		virtual SortTable*			inter(Sort* s)				const = 0;	// Return the domain of s.
		virtual PredInter*			inter(Predicate* p)			const = 0;	// Return the interpretation of p.
		virtual FuncInter*			inter(Function* f)			const = 0;	// Return the interpretation of f.
		virtual PredInter*			inter(PFSymbol* s)			const = 0;	// Return the interpretation of s.

		virtual AbstractStructure*	clone() const = 0;	// take a clone of this structure

		virtual Universe	universe(PFSymbol*)	const = 0;

};

/** Structures as constructed by the parser **/

class Structure : public AbstractStructure {
	private:
		std::map<Sort*,SortTable*>		_sortinter;		//!< The domains of the structure. 
		std::map<Predicate*,PredInter*>	_predinter;		//!< The interpretations of the predicate symbols.
		std::map<Function*,FuncInter*>	_funcinter;		//!< The interpretations of the function symbols.

	public:
		// Constructors
		Structure(const std::string& name, const ParseInfo& pi) : AbstractStructure(name,pi) { }

		// Destructor
		~Structure();

		// Mutators
		void	vocabulary(Vocabulary* v);			//!< set the vocabulary of the structure
		void	inter(Predicate* p, PredInter* i);	//!< set the interpretation of p to i
		void	inter(Function* f, FuncInter* i);	//!< set the interpretation of f to i
		void	addStructure(AbstractStructure*);	
		void	clean();

		void	functioncheck();	//!< check the correctness of the function tables
		void	autocomplete();		//!< make the domains consistent with the predicate and function tables				

		// Inspectors
		SortTable*		inter(Sort* s)				const; //!< Return the domain of s.
		PredInter*		inter(Predicate* p)			const; //!< Return the interpretation of p.
		FuncInter*		inter(Function* f)			const; //!< Return the interpretation of f.
		PredInter*		inter(PFSymbol* s)			const; //!< Return the interpretation of s.
		Structure*		clone()						const; //!< take a clone of this structure

		Universe	universe(PFSymbol*)	const;
};

/************************
	Auxiliary methods
************************/

namespace TableUtils {
	PredInter*	leastPredInter(const Universe& univ);	
		//!< construct a new, least precise predicate interpretation
	FuncInter*	leastFuncInter(const Universe& univ);		
		//!< construct a new, least precise function interpretation
	Universe	fullUniverse(unsigned int arity);
}

#endif
