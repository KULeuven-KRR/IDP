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

bool operator<(const DomainElement&,const DomainElement&);
bool operator>(const DomainElement&,const DomainElement&);
bool operator==(const DomainElement&,const DomainElement&);
bool operator!=(const DomainElement&,const DomainElement&);
bool operator<=(const DomainElement&,const DomainElement&);
bool operator>=(const DomainElement&,const DomainElement&);

typedef vector<DomainElement*>	ElementTuple;
typedef vector<ElementTuple>	ElementTable;

/**
 * DESCRIPTION
 *		The value of a domain element that consists of a function applied to domain elements.
 */
class Compound {
	private:
		Function*		_function;
		ElementTuple	_arguments;

		Compound(Function* function, const vector<DomainElement*> arguments) : 
			_function(function), _arguments(arguments) { assert(function != 0); }
	public:
		~Compound();

		const Function*			function()				const { return _function;			}
		const DomainElement&	arg(unsigned int n)		const { return *(_arguments[n]);	}

		friend class DomainElementFactory;
};

bool operator< (const Compound&,const Compound&);
bool operator> (const Compound&,const Compound&);
bool operator==(const Compound&,const Compound&);
bool operator!=(const Compound&,const Compound&);
bool operator<=(const Compound&,const Compound&);
bool operator>=(const Compound&,const Compound&);

/**
 * DESCRIPTION
 *		Class to create domain elements. This class is a singleton class that ensures all domain elements
 *		with the same value are stored at the same address in memory. As a result, two domain elements are equal
 *		iff they have the same address. It also ensures that all Compounds with the same function and arguments are
 *		stored at the same address.
 *
 *		Obtaining the address of a domain element with a given value and type should take logaritmic time in the number
 *		of created domain elements of that type. For a specified integer range, obtaining the address is optimized to
 *		constant time.
 */
class DomainElementFactory {
	private:
		static DomainElementFactory*	_instance;			//!< The single instance of DomainElementFactory
		
		map<const Function*,map<ElementTuple,Compound*> >	_compounds;	//!< Maps a function and tuple of elements to the
																		//!< corresponding compound.
																
		int						_firstfastint;		//!< The first integer in the optimized range
		int						_lastfastint;		//!< One past the last integer in the optimized range
		vector<DomainElement*>	_fastintelements;	//!< Stores pointers to integers in the optimized range.
															//!< The domain element with value n is stored at
															//!< _fastintelements[n+_firstfastint]

		map<int,DomainElement*>				_intelements;		//!< Maps an integer outside of the optimized range to its corresponding doman element address.
		map<double,DomainElement*>			_doubleelements;	//!< Maps a floating point number to its corresponding domain element address.
		map<const string*,DomainElement*>	_stringelements;	//!< Maps a string pointer to its corresponding domain element address.
		map<const Compound*,DomainElement*>	_compoundelements;	//!< Maps a compound pointer to its corresponding domain element address.
		
		DomainElementFactory(int firstfastint = 0, int lastfastint = 10001);

		Compound*		compound(const Function*,const ElementTuple&);

	public:
		~DomainElementFactory();

		static DomainElementFactory*	instance();

		DomainElement*	create(int value);
		DomainElement*	create(double value, bool certnotint = false);
		DomainElement*	create(const string* value, bool certnotdouble = false);
		DomainElement*	create(const Compound* value);
		DomainElement*	create(const Function*,const ElementTuple&);
};


/*********************************
	Tables for logical symbols
*********************************/

/**
 * DESCRIPTION
 *		This class implements the common functionality of tables for sorts, predicate and function symbols.
 */
class AbstractTable {
	private:
	protected:
	public:
		virtual ~AbstractTable() { }

		virtual bool			finite()		const = 0;	//!< Returns true iff the table is finite
		virtual	bool			empty()			const = 0;	//!< Returns true iff the table is empty
		virtual	unsigned int	arity()			const = 0;	//!< Returns the number of columns in the table

		virtual bool	approxfinite()			const = 0;	
			//!< Returns false if the table size is infinite. May return true if the table size is finite.
		virtual bool	approxempty()			const = 0;
			//!< Returns false if the table is non-empty. May return true if the table is empty.

		virtual	bool	contains(const ElementTuple& tuple)	const = 0;	
			//!< Returns true iff the table contains the tuple. 
			
		virtual void	add(const ElementTuple& tuple)		= 0;	//!< Add a tuple to the table
		virtual void	remove(const ElementTuple& tuple)	= 0;	//!< Remove a tuple from the table

};

/***********************************
	Tables for predicate symbols
***********************************/

/**
 * DESCRIPTION
 *		This class implements a concrete two-dimensional table
 */
class InternalPredTable {
	public:
		virtual ~InternalPredTable() { }

		virtual unsigned int	arity()		const = 0;
		virtual bool			finite()	const = 0;	//!< Returns true iff the table is finite
		virtual	bool			empty()		const = 0;	//!< Returns true iff the table is empty

		virtual bool	approxfinite()	const = 0;
			//!< Returns false if the table size is infinite. May return true if the table size is finite.
		virtual bool	approxempty()	const = 0;
			//!< Returns false if the table is non-empty. May return true if the table is empty.

		virtual	bool	contains(const ElementTuple& tuple) = 0;	//!< Returns true iff the table contains the tuple. 

		virtual InternalPredTable*	add(const ElementTuple& tuple) = 0;		//!< Add a tuple to the table
		virtual InternalPredTable*	remove(const ElementTuple& tuple) = 0;	//!< Remove a tuple from the table
};

/**
 * DESCRIPTION
 *		This class implements a shared pointer to an InternalPredTable
 */
class SharedInternalPredTable {
	private:
		InternalPredTable*	_table;		//!< The actual table
		int*				_counter;	//!< Pointer to the number of SharedInternalPredTable pointing to _table
	public:
		~SharedInternalPredTable();

		unsigned int	arity()			const	{ return _table->arity();			}
		bool			finite()		const	{ return _table->finite();			}
		bool			empty()			const	{ return _table->empty();			}
		bool			approxfinite()	const	{ return _table->approxfinite();	}
		bool			approxempty()	const	{ return _table->approxempty();		}

		bool	contains(const ElementTuple& tuple)	{ return _table->contains(tuple);	}

		InternalPredTable*	add(const ElementTuple& tuple);
		InternalPredTable*	remove(const ElementTuple& tuple);	
};

/**
 * DESCRIPTION
 *		This class implements a finite, enumerated InternalPredTable
 */
class EnumeratedInternalPredTable : public InternalPredTable {
	private:
		unsigned int	_arity;		//!< the number of columns of the table
		ElementTable	_table;		//!< the actual table
		bool			_sorted;	//!< true iff it is certain that the table is sorted and does not contain duplicates

	protected:
	public:
		~EnumeratedInternalPredTable() { }

		unsigned int	arity()					const { return _arity;			}
		bool			finite()				const { return true;			}
		bool			empty()					const { return _table.empty();	}
		bool			approxfinite()			const { return true;			}
		bool			approxempty()			const { return _table.empty();	}

		bool			contains(const ElementTuple& tuple);
		void			sortunique();	

		EnumeratedInternalPredTable*	add(const ElementTuple& tuple);
		EnumeratedInternalPredTable*	remove(const ElementTuple& tuple);
};

/**
 * DESCRIPTION
 *		Abstract base class for implementing InternalPredTable for '=/2', '</2', and '>/2'
 */
class ComparisonInternalPredTable : public InternalPredTable {
	protected:
		SortTable*	_lefttable;		//!< the elements that possibly occur in the first column of the table
		SortTable*	_righttable;	//!< the elements that possibly occur in the second column of the table
	public:
		virtual ~ComparisonInternalPredTable() { }
		unsigned int		arity()	const { return 2;	}
		InternalPredTable*	add(const ElementTuple& tuple);
		InternalPredTable*	remove(const ElementTuple& tuple);
};

/**
 * DESCRIPTION
 *		Internal table for '=/2'
 */
class EqualInternalPredTable : public ComparisonInternalPredTable {
	public:
		~EqualInternalPredTable() { }

		bool	contains(const ElementTuple&)	const;
		bool	finite()						const;
		bool	empty()							const;
		bool	approxfinite()					const;
		bool	approxempty()					const;
};

/**
 * DESCRIPTION
 *		Internal table for '</2'
 */
class StrLessInternalPredTable : public ComparisonInternalPredTable {
	public:
		~StrLessInternalPredTable() { }

		bool	contains(const ElementTuple&)	const;
		bool	finite()						const;
		bool	empty()							const;
		bool	approxfinite()					const;
		bool	approxempty()					const;
};

/**
 * DESCRIPTION
 *		Internal table for '>/2'
 */
class StrGreaterInternalPredTable : public ComparisonInternalPredTable {
	public:
		~StrGreaterInternalPredTable() { }

		bool	contains(const ElementTuple&)	const;
		bool	finite()						const;
		bool	empty()							const;
		bool	approxfinite()					const;
		bool	approxempty()					const;
};

/**
 * DESCRIPTION
 *		This class implements the complement of an internal predicate table
 */
class InverseInternalPredTable : public InternalPredTable {
	private:
		InternalPredTable*	_invtable;	//!< the inverse of the actual table
		vector<SortTable*>	_universe;	//!< the actual table is the complement of _table with respect to 
										//!< the cartesian product of the tables in _universe

	public:
		~InverseInternalPredTable() { delete(_invtable);	}

		unsigned int	arity()					const { return _invtable->arity();	}
		bool			finite()				const;
		bool			empty()					const;
		bool			approxfinite()			const;
		bool			approxempty()			const;

		bool	contains(const ElementTuple& tuple);

		InternalPredTable*	add(const ElementTuple& tuple);
		InternalPredTable*	remove(const ElementTuple& tuple);
};

/**
 * DESCRIPTION
 *		This class implements table that consists of all tuples that belong to the union of a set of tables,
 *		but not to the union of another set of tables.
 */
class UnionInternalPredTable : public InternalPredTable {
	private:
		vector<InternalPredTable*>	_intables;	//!< a tuple of the table does belong to at least one of the tables in _intables
		vector<InternalPredTable*>	_outtables;	//!< a tuple of the table does not belong to any of the tables in _outtables

	public:
		~UnionInternalPredTable();

		unsigned int	arity()			const;
		bool			finite()		const;
		bool			empty()			const;
		bool			approxfinite()	const;
		bool			approxempty()	const;

		bool	contains(const ElementTuple& tuple);	

		void	addInTable(InternalPredTable* t)	{ _intables.push_back(t);	}
		void	addOutTable(InternalPredTable* t)	{ _outtables.push_back(t);	}

		InternalPredTable*	add(const ElementTuple& tuple);		
		InternalPredTable*	remove(const ElementTuple& tuple);	
	
};


/**
 * DESCRIPTION
 *		This class implements tables for predicate symbols.
 */
class PredTable : public AbstractTable {
	private:
		InternalPredTable*	_table;	//!< Points to the actual table
	public:
		~PredTable();

		bool			finite()							const	{ return _table->finite();			}
		bool			empty()								const	{ return _table->empty();			}
		unsigned int	arity()								const	{ return _table->arity();			}
		bool			approxfinite()						const	{ return _table->approxfinite();	}
		bool			approxempty()						const	{ return _table->approxfinite();	}
		bool			contains(const ElementTuple& tuple)	const	{ return _table->contains(tuple);	}
		void			add(const ElementTuple& tuple);
		void			remove(const ElementTuple& tuple);
};

/***********************
	Tables for sorts
***********************/

/**
 * DESCRIPTION
 *		This class implements a concrete one-dimensional table
 */
class InternalSortTable : public InternalPredTable {
	public:
		virtual ~InternalSortTable() { }
		virtual DomainElement*	front()	const = 0;
		virtual DomainElement*	back()	const = 0;

		virtual bool	contains(const DomainElement*)		= 0;
				bool	contains(const ElementTuple& tuple)	{ return contains(tuple[0]);	}

		virtual InternalSortTable*	add(const DomainElement*)			= 0;
				InternalSortTable*	add(const ElementTuple& tuple)		{ return add(tuple[0]);		}
		virtual InternalSortTable*	remove(const DomainElement*)		= 0;
				InternalSortTable*	remove(const ElementTuple& tuple)	{ return remove(tuple[0]);	}

		unsigned int arity()	const	{ return 1;	}
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

/**
 * DESCRIPTION
 *		A finite, enumerated SortTable
 */
class EnumeratedInternalSortTable : public InternalSortTable {
	private:
		vector<DomainElement*>	_table;
};

/**
 * DESCRIPTION
 *		This class implements tables for sorts
 */
class SortTable : public AbstractTable {
	private:
		InternalSortTable*	_table;	//!< Points to the actual table
	public:
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
		DomainElement*	front()								const	{ return _table->front();			}
		DomainElement*	back()								const	{ return _table->back();			}
};

/**********************************
	Tables for function symbols
**********************************/

/**
 * DESCRIPTION
 *		This class implements a concrete associative array mapping tuples of elements to elements
 */
class InternalFuncTable {
	public:
		virtual ~InternalFuncTable();

		virtual bool			finite()				const = 0;	//!< Returns true iff the table is finite
		virtual	bool			empty()					const = 0;	//!< Returns true iff the table is empty
		virtual	unsigned int	arity()					const = 0;	//!< Returns the number of columns in the table

		virtual bool			approxfinite()			const = 0;
			//!< Returns false if the table size is infinite. May return true if the table size is finite.
		virtual bool			approxempty()			const = 0;
			//!< Returns false if the table is non-empty. May return true if the table is empty.

		virtual Element operator[](const vector<DomainElement*>& tuple)	const = 0;	
			//!< Returns the value of the tuple according to the array.

		virtual	InternalFuncTable*	add(const ElementTuple&)	const = 0;	//!< Add a tuple to the table
		virtual InternalFuncTable*	remove(const ElementTuple&)	const = 0;	//!< Remove a tuple from the table
};

/**
 * DESCRIPTION
 *		A finite, enumerated InternalFuncTable
 */
class EnumeratedInternalFuncTable : public InternalFuncTable {
	private:
		map<ElementTuple,DomainElement*>	_table;
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
		~FuncTable();

		bool			finite()				const	{ return _table->finite();			}
		bool			empty()					const	{ return _table->empty();			}
		unsigned int	arity()					const	{ return _table->arity();			}
		bool			approxfinite()			const	{ return _table->approxfinite();	}
		bool			approxempty()			const	{ return _table->approxfinite();	}

		Element	operator[](const vector<DomainElement*>& tuple)	const	{ return (*_table)[tuple];	}
		bool	contains(const vector<DomainElement*>& tuple)	const;
		void	add(const ElementTuple& tuple)							{ _table = _table->add(tuple);		}
		void	remove(const ElementTuple& tuple)						{ _table = _table->remove(tuple);	}

};


/**********************
	Interpretations
*********************/

/**
 * DESCRIPTION
 *		Class to represent a four-valued interpretation for a predicate
 */
class PredInter {
	
	private:
		PredTable*	_ctpf;	//!< stores certainly true or possibly false tuples
		PredTable*	_cfpt;	//!< stores certainly false or possibly true tuples
		bool		_ct;	//!< true iff _ctpf stores certainly true tuples, false iff _ctpf stores possibly false tuples
		bool		_cf;	//!< ture iff _cfpt stores certainly false tuples, false iff _cfpt stores possibly true tuples

	public:
		
		PredInter(PredTable* ctpf,PredTable* cfpt,bool ct, bool cf) : _ctpf(ctpf), _cfpt(cfpt), _ct(ct), _cf(cf) { }
		PredInter(PredTable* ctpf, bool ct) : _ctpf(ctpf), _cfpt(ctpf), _ct(ct), _cf(!ct) { }

		// Destructor
		~PredInter();

		// Mutators
		void replace(PredTable* pt,bool ctpf, bool c);	//!< If ctpf is true, replace _ctpf by pt and set _ct to c
														//!< Else, replace cfpt by pt and set _cf to c
		PredInter*	clone();
		void		makecertainlytrue(const ElementTuple& tuple);
		void		makecertainlyfalse(const ElementTuple& tuple);
		void		makepossiblytrue(const ElementTuple& tuple);
		void		makepossiblyfalse(const ElementTuple& tuple);

		// Inspectors
		PredTable*	ctpf()										const { return _ctpf;	}
		PredTable*	cfpt()										const { return _cfpt;	}
		bool		ct()										const { return _ct;		}
		bool		cf()										const { return _cf;		}
		bool		istrue(const ElementTuple& tuple)			const;
		bool		isfalse(const ElementTuple& tuple)			const;
		bool		isunknown(const ElementTuple& tuple)		const;
		bool		isinconsistent(const ElementTuple& tuple)	const;

		bool		approxtwovalued()							const { return _ctpf == _cfpt;	}

};


/**
 * DESCRIPTION
 *		Class to represent a four-valued interpretation for functions
 */
class FuncInter {

	private:
		FuncTable*	_functable;		//!< the function table (if the interpretation is two-valued, nullpointer otherwise).
		PredInter*	_graphinter;	//!< the interpretation for the graph of the function

	public:
		
		FuncInter(FuncTable* ft, PredInter* pt) : _functable(ft), _graphinter(pt) { }

		~FuncInter();

		PredInter*	graphinter()		const { return _graphinter;			}
		FuncTable*	functable()			const { return _functable;			}
		bool		approxtwovalued()	const { return _functable != 0;		}

};

/************************
	Auxiliary methods
************************/

/*
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
*/

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

