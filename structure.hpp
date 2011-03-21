/************************************
	structure.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef STRUCTURE_HPP
#define STRUCTURE_HPP

/**
 * \file structure.hpp
 * DESCRIPTION
 *		This file contains the classes concerning structures:
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
 * DESCRIPTION
 *		The different types of domain elements
 *		- DET_INT: integers
 *		- DET_DOUBLE: floating point numbers
 *		- DET_STRING: strings
 *		- DET_COMPOUND: a function applied to domain elements
 */
enum DomainElementType { DET_INT, DET_DOUBLE, DET_STRING, DET_COMPOUND };

/**
 * DESCRIPTION
 *		A value for a single domain element. 
 */
union DomainElementValue {
	int				_int;		//!< Value if the domain element is an integer
	double			_double;	//!< Value if the domain element is a floating point number
	const string*	_string;	//!< Value if the domein element is a string
	const Compound*	_compound;	//!< Value if the domain element is a function applied to domain elements
};

/**
 * DESCRIPTION
 *		A domain element
 */
class DomainElement {
	private:
		DomainElementType	_type;		//!< The type of the domain element
		DomainElementValue	_value;		//!< The value of the domain element

		DomainElement(int value);
		DomainElement(double value);
		DomainElement(string* value);
		DomainElement(Compound* value);

	public:
		~DomainElement();

		DomainElementType	type()	const	{ return _type;		}
		DomainElementValue	value()	const	{ return _value;	}

		friend class DomainElementFactory;
};

/**
 * DESCRIPTION
 *		The value of a domain element that consists of a function applied to domain elements.
 */
class Compound {
	private:
		Function*				_function;
		vector<DomainElement*>	_arguments;

		Compound(Function* function, const vector<DomainElement*> arguments) : 
			_function(function), _arguments(arguments) { assert(function != 0); }
	public:
		~Compound();
};

/**
 * DESCRIPTION
 *		Class to create domain elements. This class is a singleton class that ensures all domain elements
 *		with the same value are stored at the same address in memory. As a result, two domain elements are equal
 *		iff they have the same address.
 *
 *		Obtaining the address of a domain element with a given value and type should take logaritmic time in the number
 *		of created domain elements of that type. For a specified integer range, obtaining the address is optimized to
 *		constant time.
 */
class DomainElementFactory {
	private:
		static DomainElementFactory*	_instance;			//!< The single instance of DomainElementFactory
		
		int								_firstfastint;		//!< The first integer in the optimized range
		int								_lastfastint;		//!< One past the last integer in the optimized range
		vector<DomainElement*>			_fastintelements;	//!< Stores pointers to integers in the optimized range.
															//!< The domain element with value n is stored at
															//!< _fastintelements[n+_firstfastint]

		map<int,DomainElement*>			_intelements;		//!< Maps an integer outside of the optimized range to its corresponding doman element address.
		map<double,DomainElement*>		_doubleelements;	//!< Maps a floating point number to its corresponding domain element address.
		map<string*,DomainElement*>		_stringelements;	//!< Maps a string pointer to its corresponding domain element address.
		map<Compound*,DomainElement*>	_compoundelements;	//!< Maps a compound pointer to its corresponding domain element address.
		
		DomainElementFactory(int firstfastint = 0, int lastfastint = 10001);

	public:
		~DomainElementFactory();

		static DomainElementFactory*	instance();

		DomainElement*	create(int value);
		DomainElement*	create(double value, bool certnotint = false);
		DomainElement*	create(const string* value, bool certnotdouble = false);
		DomainElement*	create(const Compound* value);
};


/*********************************
	Tables for logical symbols
*********************************/

typedef vector<vector<DomainElement*> >	ElementTable;

/**
 * DESCRIPTION
 *		This class implements the common functionality of tables for sorts, predicate and function symbols.
 */
class AbstractTable {
	private:
	protected:
	public:
		virtual	ElementType		type(unsigned int col)	const = 0;	//!< Returns the type of elements in a column of the table
		virtual bool			finite()				const = 0;	//!< Returns true iff the table is finite
		virtual	bool			empty()					const = 0;	//!< Returns true iff the table is empty
		virtual	unsigned int	arity()					const = 0;	//!< Returns the number of columns in the table

		virtual const vector<ElementType>&	types()	const = 0;	//!< Returns all types of the table

		virtual bool	approxfinite()			const = 0;	
			//!< Returns false if the table size is infinite. May return true if the table size is finite.
		virtual bool	approxempty()			const = 0;
			//!< Returns false if the table is non-empty. May return true if the table is empty.

		virtual	bool	contains(const vector<Element>& tuple)		const = 0;	
			//!< Returns true iff the table contains the tuple. 
			//!< The types of the elements in the tuple should be the same as the types 
			//!< of the corresponding columns in the table
				bool	contains(const vector<TypedElement>& tuple)	const;	//!< Returns true iff the table contains the tuple
};

/***********************************
	Tables for predicate symbols
***********************************/

/**
 * DESCRIPTION
 *		This class implements a concrete two-dimensional table
 */
class InternalPredTable {
	private:
		vector<ElementType>	_types;		//!< the types of elements in the columns
	proteced:
	public:
		ElementType					type(unsigned int col)	const { return _types[col];		}
		const vector<ElementType>&	types()					const { return _types;			}
		unsigned int				arity()					const { return _types.size();	}	
		virtual bool				finite()				const = 0;	//!< Returns true iff the table is finite
		virtual	bool				empty()					const = 0;	//!< Returns true iff the table is empty

		virtual bool				approxfinite()			const = 0;
			//!< Returns false if the table size is infinite. May return true if the table size is finite.
		virtual bool				approxempty()			const = 0;
			//!< Returns false if the table is non-empty. May return true if the table is empty.

		virtual	bool	contains(const vector<Element>& tuple) = 0;	
			//!< Returns true iff the table contains the tuple. 
			//!< The types of the elements in the tuple should be the same as the types 
			//!< of the corresponding columns in the table
};

/**
 * DESCRIPTION
 *		This class implements a finite, enumerated InternalPredTable
 */
class EnumeratedInternalPredTable : public InternalPredTable {
	private:
		ElementTable		_table;		//!< the actual table

		ElementWeakOrdering	_smaller;	//!< less-than-or-equal relation on the tuples of the table
		ElementEquality		_equality;	//!< equality relation on the tuples of the table

		bool				_sorted;	//!< true iff it is certain that the table is sorted and does not contain duplicates

	protected:
	public:
		bool	finite()				const { return true;			}
		bool	empty()					const { return _table.empty();	}
		bool	approxfinite()			const { return true;			}
		bool	approxempty()			const { return _table.empty();	}
		bool	contains(const vector<Element>& tuple);
		void	sortunique();	
};

/**
 * DESCRIPTION
 *		Abstract base class for implementing InternalPredTable for '=/2', '</2', and '>/2'
 */
class ComparisonInternalPredTable : public InternalPredTable {
	private:
	protected:
		SortTable*	_lefttable;		//!< the elements that possibly occur in the first column of the table
		SortTable*	_righttable;	//!< the elements that possibly occur in the second column of the table
	public:
}

/**
 * DESCRIPTION
 *		Internal table for '=/2'
 */
class EqualInternalPredTable : public ComparisonInternalPredTable {
	private:
	protected:
	public:
		bool	contains(const vector<Element>&)	const;
		bool	finite()							const;
		bool	empty()								const;
		bool	approxfinite()						const;
		bool	approxempty()						const;
};

/**
 * DESCRIPTION
 *		This class implements tables for predicate symbols.
 */
class PredTable : public AbstractTable {
	private:
		InternalPredTable*	_table;	//!< Points to the actual table
	protected:
	public:
		ElementType		type(unsigned int col)	const	{ return _table->type(col);			}
		bool			finite()				const	{ return _table->finite();			}
		bool			empty()					const	{ return _table->empty();			}
		unsigned int	arity()					const	{ return _table->arity();			}
		bool			approxfinite()			const	{ return _table->approxfinite();	}
		bool			approxempty()			const	{ return _table->approxfinite();	}

		const vector<ElementType>&	types()	const { return _table->types();	}

		bool	contains(const vector<Element>& tuple)	const	{ return _table->contains(tuple);	}
};

/***********************
	Tables for sorts
***********************/

/**
 * DESCRIPTION
 *		This class implements a concrete one-dimensional table
 */
class InternalSortTable : public InternalPredTable {
	private:
	proteced:
	public:
		virtual ElementType	type()	const = 0;	//!< Returns the type of the elements in the table
};

/**
 * DESCRIPTION
 *		This class implements tables for sorts
 */
class SortTable : public AbstractTable {
	private:
		InternalSortTable*	_table;	//!< Points to the actual table
	protected:
	public:
		ElementType		type(unsigned int col)	const	{ return _table->type(col);			}
		ElementType		type()					const	{ return _table->type();			}
		bool			finite()				const	{ return _table->finite();			}
		bool			empty()					const	{ return _table->empty();			}
		unsigned int	arity()					const	{ return 1;							}
		bool			approxfinite()			const	{ return _table->approxfinite();	}
		bool			approxempty()			const	{ return _table->approxfinite();	}

		bool	contains(const vector<Element>& tuple)	const	{ return _table->contains(tuple);	}
};

/**********************************
	Tables for function symbols
**********************************/

/**
 * DESCRIPTION
 *		This class implements a concrete associative array mapping tuples of elements to elements
 */
class InternalFuncTable {
	private:
	proteced:
	public:
		virtual	ElementType		type(unsigned int col)	const = 0;	//!< Return the type of elements in a column of the table
		virtual bool			finite()				const = 0;	//!< Returns true iff the table is finite
		virtual	bool			empty()					const = 0;	//!< Returns true iff the table is empty
		virtual	unsigned int	arity()					const = 0;	//!< Returns the number of columns in the table

		virtual Element operator[](const vector<Element>& tuple)	const = 0;	
			//!< Returns the value of the tuple according to the array.
			//!< The types of the elements in the tuple should be the same as the types of the corresponding
			//!< tables of the array.
};


/**
 * DESCRIPTION
 *		This class implements tables for function symbols
 */
class FuncTable : public AbstractTable {
	private:
		InternalFuncTable*	_table;	//!< Points to the actual table
	protected:
	public:
		ElementType		type(unsigned int col)	const	{ return _table->type(col);			}
		bool			finite()				const	{ return _table->finite();			}
		bool			empty()					const	{ return _table->empty();			}
		unsigned int	arity()					const	{ return _table->arity();			}
		bool			approxfinite()			const	{ return _table->approxfinite();	}
		bool			approxempty()			const	{ return _table->approxfinite();	}

		Element	operator[](const vector<Element>& tuple)		const	{ return (*_table)[tuple];	}
		Element	operator[](const vector<TypedElement>& tuple)	const;	

		bool	contains(const vector<Element>& tuple)	const;

};


/********************* VANAF HIER OUD ******************/

#include "vocabulary.hpp"
#include "visitor.hpp"

typedef vector<vector<Element> > VVE;

/*************************************
	Interpretations for predicates
*************************************/

/** 
 * DESCRIPTION
 *		This class implements common functionality of tables for sorts and predicate symbols.
 */
class PredTable : public AbstractTable {

	private:
		bool	_sorted;	///< _sorted is true iff it is certain that the table is sorted and does not contain duplicates.

	protected:
		PredTable(bool sorted = false) : AbstractTable(), _sorted(sorted) { }

	public:
		virtual ~PredTable() { }

		// Mutators
		virtual void		sortunique() = 0;	// Sort and remove duplicates
		virtual PredTable*	add(const vector<TypedElement>& tuple) = 0;
		virtual PredTable*	remove(const vector<TypedElement>& tuple) = 0;	// NOTE: Expensive operation!

		// Constructors
		virtual PredTable* clone() const = 0;

		// Inspectors
		//		unsigned int		nrofrefs()				const { return _nrofrefs;	}
		virtual bool				finite()				const = 0;	// true iff the table is finite
		virtual	bool				empty()					const = 0;	// true iff the table is empty
		virtual	unsigned int		arity()					const = 0;	// the number of columns in the table
		virtual	ElementType			type(unsigned int n)	const = 0;	// the type of elements in the n'th column
				vector<ElementType>	types()					const;		// all types of the table

		// Check if the table contains a given tuple
		//	precondition: the table is sorted and contains no duplicates
		virtual	bool	contains(const vector<Element>&)		const = 0;	// true iff the table contains the tuple
																			// precondition: the given tuple has the same
																			// types as the types of the table
				bool	contains(const vector<TypedElement>&)	const;		// true iff the table contains the tuple
																			// works also if the types of the tuple do not
																			// match the types of the table
				bool	contains(const vector<TypedElement*>&)	const;
		virtual	bool	contains(const vector<compound*>&)		const;

		// Inspectors for finite tables
		virtual	unsigned int		size()									const = 0;	// the size of the table
		virtual	vector<Element>		tuple(unsigned int n)					const = 0;	// the n'th tuple
		virtual Element				element(unsigned int r,unsigned int c)	const = 0;	// the element at position (r,c)
				domelement			delement(unsigned int r,unsigned int c)	const;

		// Debugging
		virtual string to_string(unsigned int spaces = 0)	const = 0;

};

class CopyPredTable : public PredTable {

	private:
		PredTable*	_table;

	public:
		
		// Constructors
		CopyPredTable(PredTable*);
		CopyPredTable* clone() const { return new CopyPredTable(_table);	}

		// Destructor
		~CopyPredTable() { _table->removeref(); }

		// Mutators
		void	sortunique() { _table->sortunique();	}
		PredTable*	add(const vector<TypedElement>& tuple);
		PredTable*	remove(const vector<TypedElement>& tuple);

		// Inspectors
		PredTable*			table()					const { return _table;				}
		bool				finite()				const { return _table->finite();	} 
		bool				empty()					const { return _table->empty();		} 
		unsigned int		arity()					const { return _table->arity();		} 
		ElementType			type(unsigned int n)	const { return _table->type(n);		} 

		// Check if the table contains a given tuple
		bool	contains(const vector<Element>& ve)		const { return _table->contains(ve);	} 
		bool	contains(const vector<compound*>& vd)	const { return _table->contains(vd);	}

		// Inspectors for finite tables
		unsigned int		size()									const { return _table->size();			}	 
		vector<Element>		tuple(unsigned int n)					const { return _table->tuple(n);		} 
		Element				element(unsigned int r,unsigned int c)	const { return _table->element(r,c);	} 

		// Debugging
		string to_string(unsigned int spaces = 0) const { return _table->to_string(spaces);	} 

};


/*****************************************************
	Interpretations for sorts and unary predicates
*****************************************************/

/** Abstract base class **/

class SortTable : public PredTable {
	
	public:

		// Constructors
		SortTable() : PredTable() { }
		virtual SortTable* clone() const = 0;

		// Destructor
		virtual ~SortTable() { }

		// Mutators
		virtual void sortunique() = 0; 
		virtual SortTable*	add(const vector<TypedElement>& tuple) = 0;
		virtual SortTable*	remove(const vector<TypedElement>& tuple) = 0;

		// Inspectors
		virtual bool			finite()				const = 0;	// Return true iff the size of the table is finite
		virtual bool			empty()					const = 0;	// True iff the sort contains no elements
		virtual ElementType		type()					const = 0;	// return the type of the elements in the table
				unsigned int	arity()					const { return 1;		}
				ElementType		type(unsigned int )		const { return type();	}

		// Check if the table contains a given element
		//	precondition: the table is sorted and contains no duplicates
				bool	contains(const vector<Element>& ve)			const { return contains(ve[0]);	}
				bool	contains(const vector<TypedElement>& ve)	const { return contains(ve[0]);	}	
		virtual	bool	contains(string*)							const = 0;	// true iff the table contains the string.
		virtual bool	contains(int)								const = 0;	// true iff the table contains the integer
		virtual bool	contains(double)							const = 0;	// true iff the table contains the double
		virtual	bool	contains(compound*)							const = 0;	// true iff the table contains the compound
				bool	contains(Element,ElementType)				const;		// true iff the table contains the element
				bool	contains(Element e)							const { return contains(e,type());				}
				bool	contains(const TypedElement& te)			const { return contains(te._element,te._type);	}


		// Inspectors for finite tables
		virtual unsigned int	size()									const = 0;	// Returns the number of elements.
		virtual Element			element(unsigned int n)					const = 0;	// Return the n'th element
				TypedElement	telement(unsigned int n)				const { TypedElement te(element(n),type()); return te;	}
				vector<Element>	tuple(unsigned int n)					const { return vector<Element>(1,element(n));			}
				Element			element(unsigned int r,unsigned int)	const { return element(r);								}
				domelement		delement(unsigned int n)				const;
		virtual unsigned int	position(Element,ElementType)			const = 0;	// Return the position of the given element
				unsigned int	position(Element e)						const { return position(e,type());					}
				unsigned int	position(TypedElement te)				const { return position(te._element,te._type);		}

		// Visitor
		void accept(Visitor*) const;

		// Debugging
		virtual string to_string(unsigned int spaces = 0) const = 0;

};

class CopySortTable : public SortTable {

	private: 
		SortTable*	_table;
	
	public:

		// Constructors
		CopySortTable(SortTable* s); 
		CopySortTable* clone() const { return new CopySortTable(_table);	}

		// Destructor
		~CopySortTable() { _table->removeref(); }

		// Mutators
		void sortunique() { _table->sortunique();	}
		SortTable* add(const vector<TypedElement>& tuple);
		SortTable* remove(const vector<TypedElement>& tuple);

		// Inspectors
		SortTable*		table()					const { return _table;	}
		bool			finite()				const { return _table->finite();	}
		bool			empty()					const { return _table->empty();		}
		ElementType		type()					const { return _table->type();		}

		// Check if the table contains a given element
		//	precondition: the table is sorted and contains no duplicates
		bool	contains(string* s)		const { return _table->contains(s);	}
		bool	contains(int n)			const { return _table->contains(n);	}
		bool	contains(double d)		const { return _table->contains(d);	}
		bool	contains(compound* c)	const { return _table->contains(c);	}


		// Inspectors for finite tables
		unsigned int	size()								const	{ return _table->size();		}
		Element			element(unsigned int n)				const	{ return _table->element(n);	}
		unsigned int	position(Element e,ElementType t)	const	{ return _table->position(e,t);	}

		// Debugging
		string to_string(unsigned int spaces = 0) const { return _table->to_string(spaces);	}


};

/** Abstract class for finite unary tables **/
class FiniteSortTable : public SortTable {

	public:
	
		// Constructors
		FiniteSortTable() : SortTable() { }
		virtual FiniteSortTable* clone() const = 0;

		// Destructor
		virtual ~FiniteSortTable() { }

		// Add elements to a table
		// A pointer to the resulting table is returned. This pointer may point to 'this', but this is not
		// neccessarily the case. No pointers are deleted when calling these methods.
		// The result of add(...) is not necessarily sorted and may contain duplicates.
				FiniteSortTable*	add(Element,ElementType);
		virtual FiniteSortTable*	add(int)		= 0;	// Add an integer to the table.
		virtual FiniteSortTable*	add(double)		= 0;	// Add a floating point number to the table
		virtual FiniteSortTable*	add(string*)	= 0;	// Add a string to the table.
		virtual FiniteSortTable*	add(int,int)	= 0;	// Add a range of integers to the table.
		virtual FiniteSortTable*	add(char,char)	= 0;	// Add a range of characters to the table.
		virtual FiniteSortTable*	add(compound*)	= 0;	// Add a compound element to the table

		// Mutators
		virtual void sortunique() = 0; // Sort the table and remove duplicates.
		FiniteSortTable* add(const vector<TypedElement>& tuple) { return add(tuple[0]._element,tuple[0]._type);	}
		virtual FiniteSortTable* remove(const vector<TypedElement>& tuple) = 0;

		// Inspectors
		virtual bool			finite()	const { return true;	}
		virtual bool			empty()		const = 0;	
		virtual ElementType		type()		const = 0;

		// Check if the table contains a given element
		//	precondition: the table is sorted and contains no duplicates
		virtual	bool	contains(string*)					const = 0;
		virtual bool	contains(int)						const = 0;
		virtual bool	contains(double)					const = 0;
		virtual	bool	contains(compound*)					const = 0;
				bool	contains(const TypedElement& te)	const { return SortTable::contains(te);	}

		// Inspectors for finite tables
		virtual unsigned int	size()							const = 0;	// Returns the number of elements.
		virtual Element			element(unsigned int n)			const = 0;	// Return the n'th element
		virtual unsigned int	position(Element,ElementType)	const = 0;	// returns the position of the given element

		// Debugging
		virtual string to_string(unsigned int spaces = 0) const = 0;

};

/** Empty table **/

class EmptySortTable : public FiniteSortTable {

	public:

		// Constructors
		EmptySortTable() : FiniteSortTable() { }
		EmptySortTable* clone() const { return new EmptySortTable();	}

		// Destructor
		virtual ~EmptySortTable() { }

		// Add elements to a table
		FiniteSortTable*	add(int);	
		FiniteSortTable*	add(double);	
		FiniteSortTable*	add(string*);	
		FiniteSortTable*	add(int,int);	
		FiniteSortTable*	add(char,char);	
		FiniteSortTable*	add(compound*);

		// Mutators
		void sortunique() { }
		EmptySortTable* remove(const vector<TypedElement>&) { return this;	}

		// Inspectors
		bool			empty()								const { return true;						}	
		ElementType		type()								const { return ElementUtil::leasttype();	}
		bool			contains(string*)					const { return false;						}	
		bool			contains(int)						const { return false;						}	
		bool			contains(double)					const { return false;						}	
		bool			contains(compound*)					const { return false;						}
		bool			contains(const TypedElement& )		const { return false;						}
		unsigned int	size()								const { return 0;							}	
		Element			element(unsigned int )				const { assert(false); Element e; return e; } 
		unsigned int	position(Element,ElementType)		const { assert(false); return 0;			}
															
		// Debugging
		string to_string(unsigned int) const { return "";	}

};

/** Domain is an interval of integers **/

class RanSortTable : public FiniteSortTable {
	
	private:
		int _first;		// first element in the range
		int _last;		// last element in the range

	public:

		// Constructors
		RanSortTable(int f, int l) : FiniteSortTable(), _first(f), _last(l) { }
		RanSortTable* clone() const { return new RanSortTable(_first,_last);	}

		// Destructor
		~RanSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);
		FiniteSortTable*	add(compound*);

		// Mutators
		void				sortunique() { }
		FiniteSortTable*	remove(const vector<TypedElement>& tuple);

		// Inspectors
		bool			empty()								const { return _first > _last;	}
		ElementType		type()								const { return ELINT;			}
		bool			contains(string*)					const;
		bool			contains(int)						const;
		bool			contains(double)					const;
		bool			contains(compound*)					const;
		bool			contains(const TypedElement& te)	const { return SortTable::contains(te);	}
		unsigned int	size()								const { return _last-_first+1;	}
		Element			element(unsigned int n)				const { Element e; e._int = _first+n; return e;	}
		unsigned int	position(Element,ElementType)		const;

		int	operator[](unsigned int n)	const { return _first+n;	}
		int	first()						const { return _first;		}
		int	last()						const { return _last;		}

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/** Domain is a set of integers, but not necessarily an interval **/

class IntSortTable : public FiniteSortTable {
	
	private:
		vector<int> _table;

	public:

		// Constructors
		IntSortTable() : FiniteSortTable(), _table(0) { }
		IntSortTable* clone() const;

		// Destructor
		~IntSortTable() { }

		// Add elements to the table
		IntSortTable*		add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);
		FiniteSortTable*	add(compound*);

		// Mutators
		void			sortunique();
		void			table(const vector<int>& t)	{ _table = t;	}
		IntSortTable*	remove(const vector<TypedElement>& tuple);

		// Inspectors
		bool			empty()								const { return _table.empty();	}
		ElementType		type()								const { return ELINT;			}
		bool			contains(string*)					const;
		bool			contains(int)						const;
		bool			contains(double)					const;
		bool			contains(compound*)					const;
		bool			contains(const TypedElement& te)	const { return SortTable::contains(te);	}
		unsigned int	size()								const { return _table.size();	}
		Element			element(unsigned int n)				const { Element e; e._int = _table[n]; return e;	}
		unsigned int	position(Element,ElementType)		const;

		int	operator[](unsigned int n)	const { return _table[n];			}
		int	first()						const { return _table.front();		}
		int	last()						const { return _table.back();		}

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};

/*
 * Domain is a set of doubles 
 *		Invariant: at least one element in the table is not an integer.
 *
 */
class FloatSortTable : public FiniteSortTable {
	
	private:
		vector<double> _table;

	public:

		// Constructors
		FloatSortTable() : FiniteSortTable(), _table(0) { }
		FloatSortTable* clone() const;

		// Destructor
		~FloatSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);
		FiniteSortTable*	add(compound*);

		// Mutators
		void			sortunique();
		void			table(const vector<double>& t)	{ _table = t;	}
		FloatSortTable*	remove(const vector<TypedElement>& tuple);

		// Inspectors
		bool			empty()								const { return _table.empty();		}
		ElementType		type()								const { return ELDOUBLE;			}
		bool			contains(string*)					const;
		bool			contains(int)						const;
		bool			contains(double)					const;
		bool			contains(compound*)					const;
		bool			contains(const TypedElement& te)	const { return SortTable::contains(te);	}
		unsigned int	size()								const { return _table.size();		}
		Element			element(unsigned int n)				const { Element e; e._double = _table[n]; return e;	}
		unsigned int	position(Element,ElementType)		const;

		double	operator[](unsigned int n)	const { return _table[n];			}
		double	first()						const { return _table.front();		}
		double	last()						const { return _table.back();		}

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/*
 * Domain is a set of strings
 *		Invariant: no string in the table is a number (int or double)
 */
class StrSortTable : public FiniteSortTable {
	
	private:
		vector<string*> _table;

	public:

		// Constructors
		StrSortTable() : _table(0) { }
		StrSortTable(const vector<string*>& t) : _table(t) { }
		StrSortTable* clone() const;

		// Destructor
		~StrSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);
		FiniteSortTable*	add(compound*);

		// Cleanup
		void			sortunique();
		void			table(const vector<string*>& t) { _table = t;	}
		StrSortTable*	remove(const vector<TypedElement>& tuple);

		// Inspectors
		bool			empty()								const { return _table.empty();		}
		ElementType		type()								const { return ELSTRING;			}
		bool			contains(int)						const;
		bool			contains(double)					const;
		bool			contains(string*)					const;
		bool			contains(compound*)					const;
		bool			contains(const TypedElement& te)	const { return SortTable::contains(te);	}
		unsigned int	size()								const { return _table.size();		}
		Element			element(unsigned int n)				const { Element e; e._string = _table[n]; return e;	}
		unsigned int	position(Element,ElementType)		const;

		string*	operator[](unsigned int n)	const { return _table[n];			}
		string*	first()						const { return _table.front();		}
		string*	last()						const { return _table.back();		}

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};

/*
 * Domain contains numbers, strings, and/or compounds
 *		Invariant: there are compounds or both numbers and strings in the table
 */
class MixedSortTable : public FiniteSortTable {
	
	private:
		vector<double>		_numtable;
		vector<string*>		_strtable;
		vector<compound*>	_comtable;

	public:

		// Constructors
		MixedSortTable() : _numtable(0), _strtable(0) { }
		MixedSortTable(const vector<string*>& t) : _numtable(0), _strtable(t)	{ }
		MixedSortTable(const vector<double>& t) : _numtable(t), _strtable(0)	{ }
		MixedSortTable* clone() const;

		// Destructor
		~MixedSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);
		FiniteSortTable*	add(compound*);

		// Mutators
		void				sortunique();
		void				numtable(const vector<double>&	t)		{ _numtable = t;	}
		void				strtable(const vector<string*>&	t)		{ _strtable = t;	}
		void				comtable(const vector<compound*>& t)	{ _comtable = t;	}
		FiniteSortTable*	remove(const vector<TypedElement>& tuple);

		// Inspectors
		bool			empty()								const { return (_numtable.empty() && _strtable.empty() && _comtable.empty()); }
		ElementType		type()								const;
		bool			contains(int)						const;
		bool			contains(double)					const;
		bool			contains(string*)					const;
		bool			contains(compound*)					const;
		bool			contains(const TypedElement& te)	const { return SortTable::contains(te);	}
		unsigned int	size()								const { return _numtable.size() + _strtable.size() + _comtable.size();	}
		Element			element(unsigned int n)				const; 
		unsigned int	position(Element,ElementType)		const;

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};

class UnionSortTable : public SortTable {
	
	private:
		vector<SortTable*>	_tables;
		FiniteSortTable*	_blacklist;

	public:

		// Constructors
		UnionSortTable() : SortTable() { }
		UnionSortTable* clone() const;

		// Destructor
		~UnionSortTable();

		// Mutators
		void			sortunique(); 
		void			add(SortTable* pt)				{ _tables.push_back(pt);	}
		void			blacklist(FiniteSortTable* pt)	{ _blacklist = pt;			}
		UnionSortTable*	add(const vector<TypedElement>& tuple);
		UnionSortTable*	remove(const vector<TypedElement>& tuple);

		// Inspectors
		bool			finite()	const { return false;		}
		bool			empty()		const;
		ElementType		type()		const { return ELCOMPOUND;	}

		// Check if the table contains a given element
		//	precondition: the table is sorted and contains no duplicates
		bool	contains(string*)	const;
		bool	contains(int)		const;
		bool	contains(double)	const;
		bool	contains(compound*)	const;

		// Inspectors for finite tables
		unsigned int	size()							const { assert(false); return 0;			}
		Element			element(unsigned int)			const { assert(false); Element e; return e; }
		unsigned int	position(Element,ElementType)	const { assert(false); return 0; }
		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/***********************************************
	Interpretations for non-unary predicates
***********************************************/

/** Finite, enumerated tables  **/

class FinitePredTable : public PredTable {
	
	private:
		vector<ElementType>			_types;		// the types of elements in the columns
		vector<vector<Element> >	_table;		// the actual table
		ElementWeakOrdering			_order;		// less-than-or-equal relation on the tuples of this table
		ElementEquality				_equality;	// equality relation on the tuples of this table

		mutable	set<vector<compound*> >	_dyntable;
		mutable	set<vector<compound*> >	_invdyntable;	// complement of _dyntable
														// OPTIMIZATION? do not keep this table?
	public:

		// Constructors 
		FinitePredTable(const vector<ElementType>& t) : PredTable(), _types(t), _table(0), _order(t), _equality(t) { }
		FinitePredTable(const FinitePredTable&);
		FinitePredTable(const FiniteSortTable&);
		FinitePredTable* clone() const { return new FinitePredTable(*this);	}

		// Destructor
		~FinitePredTable() { }

		// Mutators
		void	sortunique();
		FinitePredTable*	add(const vector<TypedElement>& tuple);
		FinitePredTable*	remove(const vector<TypedElement>& tuple);

		// Parsing
		void				addRow()								{ _table.push_back(vector<Element>(_types.size()));	}
		void				addRow(const vector<Element>& ve, const vector<ElementType>&);
		void				addColumn(ElementType);
		void				changeElType(unsigned int,ElementType);
		vector<Element>&	operator[](unsigned int n)				{ return _table[n];	}

		// Iterators
		VVE::iterator		begin()									{ return _table.begin();	}
		VVE::iterator		end()									{ return _table.end();		}

		// Inspectors
		bool				finite()								const { return true;			}
		bool				empty()									const { return _table.empty();	}
		unsigned int		arity()									const { return _types.size();	}	
		ElementType			type(unsigned int n)					const { return _types[n];		}
		unsigned int		size()									const { return _table.size();	}
		vector<Element>		tuple(unsigned int n)					const { return _table[n];		}
		Element				element(unsigned int r,unsigned int c)	const { return _table[r][c];	}

		const vector<vector<Element> >& table()	const { return _table;	}

		// Check if the table contains a given tuple
		bool	contains(const vector<Element>&)	const;
		bool	contains(const vector<compound*>&)	const;

		// Other inspectors
		VVE::const_iterator		begin()									const { return _table.begin();	}
		VVE::const_iterator		end()									const { return _table.end();	}
		const vector<Element>&	operator[](unsigned int n)				const { return _table[n];		}
		
		// Debugging
		string to_string(unsigned int spaces = 0)	const;
};

/*
	A UnionPredTable contains all tuples that 
		are in at least one of the table in _tables 
		AND that are not in the blacklist.
	
	Invariant: _tables is not empty
*/
class UnionPredTable : public PredTable {

	private:
		vector<PredTable*>	_tables;
		FinitePredTable*	_blacklist;

	public:
		
		// Constructors
		UnionPredTable() : PredTable() { }
		UnionPredTable* clone() const;

		// Destructor
		~UnionPredTable();

		// Mutators
		void			add(PredTable* pt)				{ _tables.push_back(pt);	}
		void			blacklist(FinitePredTable* pt)	{ _blacklist = pt;			}
		void			sortunique();	// Sort and remove duplicates
		UnionPredTable*	add(const vector<TypedElement>& tuple);
		UnionPredTable*	remove(const vector<TypedElement>& tuple);

		// Inspectors
		bool			finite()				const { return false;	}
		bool			empty()					const;
		unsigned int	arity()					const { return _tables[0]->arity();	}
		ElementType		type(unsigned int)		const { return ELCOMPOUND;			} //TODO try to assign more specific type?

		// Check if the table contains a given tuple
		//	precondition: the tables are sorted and contain no duplicates
		bool	contains(const vector<Element>&)	const;

		// Inspectors for finite tables
		unsigned int	size()								const { assert(false); return 0;					}
		vector<Element>	tuple(unsigned int)					const { assert(false); return vector<Element>();	}
		Element			element(unsigned int,unsigned int)	const { assert(false); Element e; return e;			}

		// Debugging
		string to_string(unsigned int spaces = 0)	const;

};


/********************************************
	Four-valued predicate interpretations
********************************************/

/*
 * Four-valued predicate interpretation, represented by two pointers to tables.
 *	If the two pointers are equal and _ct != _cf, the interpretation is certainly two-valued.
 */ 
class PredInter {
	
	private:
		PredTable*	_ctpf;	// stores certainly true or possibly false tuples
		PredTable*	_cfpt;	// stores certainly false or possibly true tuples
		bool		_ct;	// true iff _ctpf stores certainly true tuples, false iff _ctpf stores possibly false tuples
		bool		_cf;	// ture iff _cfpt stores certainly false tuples, false iff _cfpt stores possibly true tuples

	public:
		
		// Constructors
		PredInter(PredTable* p1,PredTable* p2,bool b1, bool b2) : _ctpf(p1), _cfpt(p2), _ct(b1), _cf(b2) { }
		PredInter(PredTable* p, bool b) : _ctpf(p), _cfpt(p), _ct(b), _cf(!b) { }

		// Destructor
		~PredInter();

		// Mutators
		void replace(PredTable* pt,bool ctpf, bool c);	// If ctpf is true, replace _ctpf by pt and set _ct to c
														// Else, replace cfpt by pt and set _cf to c
		void sortunique()	{ if(_ctpf) _ctpf->sortunique(); if(_cfpt && _ctpf != _cfpt) _cfpt->sortunique();	}
		PredInter*	clone();
		void	forcetwovalued();		// delete cfpt table and replace by ctpf
		void	add(const vector<TypedElement>& tuple, bool ctpf, bool c);


		// Inspectors
		PredTable*	ctpf()									const { return _ctpf;	}
		PredTable*	cfpt()									const { return _cfpt;	}
		bool		ct()									const { return _ct;		}
		bool		cf()									const { return _cf;		}
		bool		istrue(const vector<Element>& vi)		const { return (_ct ? _ctpf->contains(vi) : !(_ctpf->contains(vi)));	}
		bool		isfalse(const vector<Element>& vi)		const { return (_cf ? _cfpt->contains(vi) : !(_cfpt->contains(vi)));	}
		bool		istrue(const vector<TypedElement>& vi)	const;	// return true iff the given tuple is true or inconsistent
		bool		isfalse(const vector<TypedElement>& vi)	const;	// return false iff the given tuple is false or inconsistent
		bool		fasttwovalued()							const { return _ctpf == _cfpt;	}

		// Visitor
		void accept(Visitor*) const;

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/************************************
	Interpretations for functions 
************************************/

/** abstract base class **/

class FuncTable {

	private:
		unsigned int	_nrofrefs;	// Number of references to this table
		mutable	map<vector<domelement>,domelement>	_dyntable;

	public:

		// Constructor
		FuncTable() : _nrofrefs(0) { }
		virtual FuncTable* clone() const = 0;

		// Destructor
		virtual ~FuncTable() { }
	
		// Mutators
		void	removeref()	{ --_nrofrefs; if(!_nrofrefs) delete(this);	}
		void	addref()	{ ++_nrofrefs;								}

		// Inspectors
				unsigned int	nrofrefs()								const { return _nrofrefs;	}
		virtual bool			finite()								const = 0;	// true iff the table is finite
		virtual bool			empty()									const = 0;	// true iff the table is empty
		virtual unsigned int	arity()									const = 0;	// arity of the table - 1
		virtual unsigned int	size()									const = 0;	// size of the table
		virtual ElementType		type(unsigned int n)					const = 0;	// type of the n'th column of the table
				ElementType		outtype()								const { return type(arity());	}
		virtual vector<Element>	tuple(unsigned int n)					const = 0;	// return the n'th tuple
		virtual Element			element(unsigned int r,unsigned int c)	const = 0;	// return the element at row r and column c

		virtual	Element	operator[](const vector<Element>& vi)		const = 0;	// return the value of vi according to the function 
																				// NOTE: the type of the n'th element in vi should be
																				// the type of the n'th column in the table. 
				Element	operator[](const vector<TypedElement>& vi)	const;		// return the value of vi according to the function
				Element operator[](const vector<TypedElement*>& vi)	const;
				domelement operator[](const vector<domelement>& vi)	const;

		// Debugging
		virtual string to_string(unsigned int spaces = 0) const = 0;

};

/** cloned function tables **/

class CopyFuncTable : public FuncTable {

	private:
		FuncTable*	_table;

	public:

		// Constructor
		CopyFuncTable(FuncTable*);
		CopyFuncTable* clone() const { return new CopyFuncTable(_table);	}

		// Destructor
		~CopyFuncTable() { _table->removeref();	}

		// Inspectors
		FuncTable*		table()									const { return _table;					}
		bool			finite()								const { return _table->finite();		} 
		bool			empty()									const { return _table->empty();			} 
		unsigned int	arity()									const { return _table->arity();			} 
		unsigned int	size()									const { return _table->size();			} 
		ElementType		type(unsigned int n)					const { return _table->type(n);			} 
		vector<Element>	tuple(unsigned int n)					const { return _table->tuple(n);		} 
		Element			element(unsigned int r,unsigned int c)	const { return _table->element(r,c);	} 

		Element	operator[](const vector<Element>& vi)		const	{ return (*_table)[vi];	}

		// Debugging
		string to_string(unsigned int spaces = 0) const { return _table->to_string(spaces);	} 

};

/** finite, enumerated functions **/
class FiniteFuncTable : public FuncTable {

	private:
		FinitePredTable*	_ftable;	// the actual table
		ElementWeakOrdering	_order;		// less-than-or-equal on a tuple of domain elements with arity n = (_ftable->arity() - 1)
										// and types (_ftable->type(0),...,_ftable->type(n)).
		ElementEquality		_equality;	// equality on the on a tuple of domain elements with arity n = (_ftable->arity() - 1)
										// and types (_ftable->type(0),...,_ftable->type(n)).

	public:
		
		// Constructors
		FiniteFuncTable(FinitePredTable* ft);
		FiniteFuncTable* clone() const { return new FiniteFuncTable(_ftable->clone());	}

		// Destructor
		~FiniteFuncTable() { }

		// Inspectors
		bool				finite()								const { return true;					}
		bool				empty()									const { return _ftable->finite();		}
		unsigned int		arity()									const { return _ftable->arity() - 1;	}
		unsigned int		size()									const { return _ftable->size();			}
		FinitePredTable*	ftable()								const { return _ftable;					}
		ElementType			type(unsigned int n)					const { return _ftable->type(n);		}
		vector<Element>		tuple(unsigned int n)					const { return _ftable->tuple(n);		}
		Element				element(unsigned int r,unsigned int c)	const { return _ftable->element(r,c);	}

		Element		operator[](const vector<Element>& vi)		const;

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};

/** predicate table derived from function table **/
class FuncPredTable : public PredTable {

	private:
		FuncTable* _ftable;

	public:

		// Constructors
		FuncPredTable(FuncTable* ft) : PredTable(), _ftable(ft) { }
		FuncPredTable* clone() const { return new FuncPredTable(_ftable->clone());	}

		// Destructor
		~FuncPredTable() { }

		// Mutators
		void sortunique() { }
		PredTable*	add(const vector<TypedElement>& tuple);
		PredTable*	remove(const vector<TypedElement>& tuple);

		// Inspectors
//		virtual Element		operator[](const vector<Element>& vi)		const = 0;
//				Element		operator[](const vector<TypedElement>& vi)	const;
//		virtual PredInter*	predinter()									const = 0;

		bool			finite()				const { return _ftable->finite();		}
		bool			empty()					const { return _ftable->empty();		}
		unsigned int	arity()					const { return _ftable->arity() + 1;	}
		ElementType		type(unsigned int n)	const { return _ftable->type(n);		}
		FuncTable*		ftable()				const { return _ftable;					}

		// Check if the table contains a given tuple
		bool	contains(const vector<Element>&)	const;

		// Inspectors for finite tables
		unsigned int		size()									const { return _ftable->size();			}
		vector<Element>		tuple(unsigned int n)					const { return _ftable->tuple(n);		}
		Element				element(unsigned int r,unsigned int c)	const { return _ftable->element(r,c);	}

		// Visitor
		void accept(Visitor*) const;

		// Debugging
		string to_string(unsigned int spaces = 0)	const { return _ftable->to_string(spaces);	}

};

/** four-valued function interpretations **/
class FuncInter {

	private:
		FuncTable*	_ftable;	// the function (if it is two-valued, nullpointer otherwise).
		PredInter*	_pinter;	// the interpretation for the graph of the function

	public:
		
		// Constructors
		FuncInter(FuncTable* ft, PredInter* pt) : _ftable(ft), _pinter(pt) { }

		// Destructor
		~FuncInter() { if(_ftable) delete(_ftable); delete(_pinter); }

		// Mutators
		void sortunique() { if(_pinter) _pinter->sortunique(); }
		FuncInter* clone();
		void	forcetwovalued();
		void	add(const vector<TypedElement>& tuple,bool ctpf,bool c);	// IMPORTANT NOTE: This method deletes _ftable if it is finite!

		// Inspectors
		PredInter*	predinter()		const { return _pinter;			}
		FuncTable*	functable()		const { return _ftable;			}
		bool		fasttwovalued()	const { return _ftable != 0;	}

		// Debugging
		string to_string(unsigned int spaces = 0) const;

		// Visitor
        void accept(Visitor*) const;

};

/************************
	Auxiliary methods
************************/

namespace TableUtils {

	PredInter*	leastPredInter(unsigned int n);		// construct a new, least precise predicate interpretation with arity n
	FuncInter*	leastFuncInter(unsigned int n);		// construct a new, least precise function interpretation with arity n
	PredTable*	intersection(PredTable*,PredTable*);
	PredTable*	difference(PredTable*,PredTable*);

	FiniteSortTable*	singletonSort(Element,ElementType);	// construct a sort table containing only the given element
	FiniteSortTable*	singletonSort(TypedElement);		// construct a sort table containing only the given element

	// Return a table containing all tuples (a_i1,...,a_in) such that
	//			vb[ik] is false for every 1 <= k <= n	
	//		AND	(a_1,...,a_m) is a tuple of pt
	//		AND for every 1 <= j <= m, if vb[j] = true, then vet[j] = a_j
	//	Precondition: pt is finite and sorted
	PredTable*	project(PredTable* pt,const vector<TypedElement>& vet, const vector<bool>& vb);
}

/*****************
	Structures
*****************/

/** Abstract base class **/

class AbstractStructure {

	protected:

		string			_name;			// The name of the structure
		ParseInfo		_pi;			// The place where this structure was parsed.
		Vocabulary*		_vocabulary;	// The vocabulary of the structure.

	public:

		// Constructors
		AbstractStructure(string name, const ParseInfo& pi) : _name(name), _pi(pi), _vocabulary(0) { }

		// Destructor
		virtual ~AbstractStructure() { }

		// Mutators
		virtual void	vocabulary(Vocabulary* v) { _vocabulary = v;	}	// set the vocabulary
		virtual AbstractStructure*	clone() = 0;	// take a clone of this structure
		virtual void	forcetwovalued() = 0;		// delete all cfpt tables and replace by ctpf
		virtual void	sortall() = 0;				// sort all tables

		// Inspectors
				const string&	name()						const { return _name;		}
				ParseInfo		pi()						const { return _pi;			}
				Vocabulary*		vocabulary()				const { return _vocabulary;	}
		virtual SortTable*		inter(Sort* s)				const = 0;	// Return the domain of s.
		virtual PredInter*		inter(Predicate* p)			const = 0;	// Return the interpretation of p.
		virtual FuncInter*		inter(Function* f)			const = 0;	// Return the interpretation of f.
		virtual PredInter*		inter(PFSymbol* s)			const = 0;	// Return the interpretation of s.

		// Lua
		TypedInfArg		getObject(set<Sort*>*)	const;
		TypedInfArg		getObject(set<Predicate*>* predicate) const;
		TypedInfArg		getObject(set<Function*>* function) const;

		// Visitor
		virtual void accept(Visitor* v) const	= 0;

		// Debugging
		virtual string	to_string(unsigned int spaces = 0) const = 0;

};

/** Structures as constructed by the parser **/

class Structure : public AbstractStructure {

	private:

		//TODO: these should not be mutable!
		mutable map<Sort*,SortTable*>		_sortinter;		// The domains of the structure. 
		mutable map<Predicate*,PredInter*>	_predinter;		// The interpretations of the predicate symbols.
		mutable map<Function*,FuncInter*>	_funcinter;		// The interpretations of the function symbols.
	
	public:
		
		// Constructors
		Structure(const string& name, const ParseInfo& pi) : AbstractStructure(name,pi) { }

		// Destructor
		~Structure();

		// Mutators
		void	vocabulary(Vocabulary* v);					// set the vocabulary
		void	inter(Sort* s,SortTable* d) const;			// set the domain of s to d. TODO: should not be const!
		void	inter(Predicate* p, PredInter* i) const;	// set the interpretation of p to i. TODO: should not be const!
		void	inter(Function* f, FuncInter* i) const;		// set the interpretation of f to i. TODO: should not be const!
		void	addElement(Element,ElementType,Sort*);		// add the given element to the interpretation of the given sort
		void	functioncheck();					// check the correctness of the function tables
		void	autocomplete();						// set the interpretation of all predicates and functions that 
													// do not yet have an interpretation to the least precise 
													// interpretation.
		Structure*	clone();						// take a clone of this structure
		void	forcetwovalued();		
		void	sortall();

		// Inspectors
		Vocabulary*		vocabulary()				const { return AbstractStructure::vocabulary();	}
		SortTable*		inter(Sort* s)				const; // Return the domain of s.
		PredInter*		inter(Predicate* p)			const; // Return the interpretation of p.
		FuncInter*		inter(Function* f)			const; // Return the interpretation of f.
		PredInter*		inter(PFSymbol* s)			const; // Return the interpretation of s.
		bool			hasInter(Sort* s)		{ return _sortinter.find(s) != _sortinter.end();	}
		bool			hasInter(Predicate* p)	{ return _predinter.find(p) != _predinter.end();	}
		bool			hasInter(Function* f)	{ return _funcinter.find(f) != _funcinter.end();	}
//		unsigned int	nrSortInters()				const { return _sortinter.size();	}
//		unsigned int	nrPredInters()				const { return _predinter.size();	}
//		unsigned int	nrFuncInters()				const { return _funcinter.size();	}

		// Visitor
		void accept(Visitor* v) const;

		// Debugging
		string	to_string(unsigned int spaces = 0) const;

};

class AbstractTheory;
namespace StructUtils {

	// Make a theory containing all literals that are true according to the given structure
	AbstractTheory*		convert_to_theory(const AbstractStructure*);	

	// Change the vocabulary of a structure
	void	changevoc(AbstractStructure*,Vocabulary*);

	// Compute the complement of the given table in the given structure
	PredTable*	complement(const PredTable*,const vector<Sort*>&,const AbstractStructure*);

}

/**
 *	Iterate over all elements in the cross product of a tuple of SortTables.
 *		Precondition: all tables in the tuple are finite
 */
class SortTableTupleIterator {
	
	private:
		vector<unsigned int>	_currvalue;
		vector<unsigned int>	_limits;
		vector<SortTable*>		_tables;

	public:
		SortTableTupleIterator(const vector<SortTable*>&);
		SortTableTupleIterator(const vector<Variable*>&, AbstractStructure*);	// tuple of tables are the tables
																				// for the sorts of the given variables
																				// in the given structure
		bool nextvalue();							// go to the next value. Returns false iff there is no next value.
		bool empty()						const;	// true iff the cross product is empty
		bool singleton()					const;  // true iff the cross product is a singleton
		ElementType type(unsigned int n)	const;	// Get the type of the values of the n'th table in the tuple
		Element		value(unsigned int n)	const;	// Get the n'th element in the current tuple

};

#endif
