/************************************
	structure.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "vocabulary.hpp"
#include "visitor.hpp"
typedef vector<vector<Element> > VVE;

/*************************************
	Interpretations for predicates
*************************************/

class PredTable {

	public:
		
		// Constructors
		PredTable() { }

		// Destructor
		virtual ~PredTable() { }

		// Mutators
		virtual void	sortunique() = 0;	// Sort and remove duplicates

		// Inspectors
		virtual bool			finite()				const = 0;	// true iff the table is finite
		virtual	bool			empty()					const = 0;	// true iff the table is empty
		virtual	unsigned int	arity()					const = 0;	// the number of columns in the table
		virtual	ElementType		type(unsigned int n)	const = 0;	// the type of elements in the n'th column

		// Check if the table contains a given tuple
		//	precondition: the table is sorted and contains no duplicates
		virtual	bool	contains(const vector<Element>&)		const = 0;	// true iff the table contains the tuple
																			// precondition: the given tuple has the same
																			// types as the types of the table
				bool	contains(const vector<TypedElement>&)	const;		// true iff the table contains the tuple
																			// works also if the types of the tuple do not
																			// match the types of the table

		// Inspectors for finite tables
		virtual	unsigned int		size()									const = 0;	// the size of the table
		virtual	vector<Element>		tuple(unsigned int n)					const = 0;	// the n'th tuple
		virtual Element				element(unsigned int r,unsigned int c)	const = 0;	// the element at position (r,c)

		// Debugging
		virtual string to_string(unsigned int spaces = 0)	const = 0;

};

/*****************************************************
	Interpretations for sorts and unary predicates
*****************************************************/

/** Abstract base class **/

class SortTable : public PredTable {
	
	public:

		// Constructors
		SortTable() : PredTable() { }

		// Destructor
		virtual ~SortTable() { }

		// Mutators
		virtual void sortunique() = 0; 

		// Inspectors
		virtual bool			finite()				const = 0;	// Return true iff the size of the table is finite
		virtual bool			empty()					const = 0;	// True iff the sort contains no elements
		virtual ElementType		type()					const = 0;	// return the type of the elements in the table
				unsigned int	arity()					const { return 1;		}
				ElementType		type(unsigned int n)	const { return type();	}

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
				bool	contains(TypedElement te)					const { return contains(te._element,te._type);	}


		// Inspectors for finite tables
		virtual unsigned int	size()									const = 0;	// Returns the number of elements.
		virtual Element			element(unsigned int n)					const = 0;	// Return the n'th element
				TypedElement	telement(unsigned int n)				const { TypedElement te(element(n),type()); return te;	}
		virtual	vector<Element>	tuple(unsigned int n)					const { return vector<Element>(1,element(n));			}
		virtual Element			element(unsigned int r,unsigned int c)	const { return element(r);								}
		virtual unsigned int	position(Element,ElementType)			const = 0;	// Return the position of the given element
				unsigned int	position(Element e)						const { return position(e,type());					}
				unsigned int	position(TypedElement te)				const { return position(te._element,te._type);		}

		// Debugging
		virtual string to_string() const = 0;

};

/** Abstract class for finite unary tables **/
class FiniteSortTable : public SortTable {

	public:
	
		// Constructors
		FiniteSortTable() : SortTable() { }

		// Destructor
		virtual ~FiniteSortTable() { }

		// Add elements to a table
		// A pointer to the resulting table is returned. This pointer may point to 'this', but this is not
		// neccessarily the case. No pointers are deleted when calling these methods.
		// The result of add(...) is not necessarily sorted and may contain duplicates.
		virtual FiniteSortTable*	add(int)		= 0;	 // Add an integer to the table.
		virtual FiniteSortTable*	add(double)		= 0;	 // Add a floating point number to the table
		virtual FiniteSortTable*	add(string*)	= 0;	 // Add a string to the table.
		virtual FiniteSortTable*	add(int,int)	= 0;	 // Add a range of integers to the table.
		virtual FiniteSortTable*	add(char,char)	= 0;	 // Add a range of characters to the table.

		// Mutators
		virtual void sortunique() = 0; // Sort the table and remove duplicates.

		// Inspectors
		virtual bool			finite()	const { return true;	}
		virtual bool			empty()		const = 0;	
		virtual ElementType		type()		const = 0;

		// Check if the table contains a given element
		//	precondition: the table is sorted and contains no duplicates
		virtual	bool	contains(string*)	const = 0;
		virtual bool	contains(int)		const = 0;
		virtual bool	contains(double)	const = 0;
		virtual	bool	contains(compound*)	const = 0;

		// Inspectors for finite tables
		virtual unsigned int	size()							const = 0;	// Returns the number of elements.
		virtual Element			element(unsigned int n)			const = 0;	// Return the n'th element
		virtual unsigned int	position(Element,ElementType)	const = 0;	// returns the position of the given element

		// Debugging
		virtual string to_string() const = 0;

};

/** Empty table **/

class EmptySortTable : public FiniteSortTable {

	public:

		// Constructors
		EmptySortTable() : FiniteSortTable() { }

		// Destructor
		virtual ~EmptySortTable() { }

		// Add elements to a table
		FiniteSortTable*	add(int);	
		FiniteSortTable*	add(double);	
		FiniteSortTable*	add(string*);	
		FiniteSortTable*	add(int,int);	
		FiniteSortTable*	add(char,char);	

		// Mutators
		void sortunique() { }

		// Inspectors
		bool			empty()							const { return true;						}	
		ElementType		type()							const { return ElementUtil::leasttype();	}
		bool			contains(string*)				const { return false;						}	
		bool			contains(int)					const { return false;						}	
		bool			contains(double)				const { return false;						}	
		bool			contains(compound*)				const { return false;						}
		unsigned int	size()							const { return 0;							}	
		Element			element(unsigned int n)			const { assert(false); Element e; return e; } 
		unsigned int	position(Element,ElementType)	const { assert(false); return 0;			}
															
		// Debugging
		string to_string() const { return "";	}

};

/** Domain is an interval of integers **/

class RanSortTable : public FiniteSortTable {
	
	private:
		int _first;		// first element in the range
		int _last;		// last element in the range

	public:

		// Constructors
		RanSortTable(int f, int l) : FiniteSortTable(), _first(f), _last(l) { }

		// Destructor
		~RanSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);

		// Mutators
		void			sortunique() { }

		// Inspectors
		bool			empty()							const { return _first > _last;	}
		ElementType		type()							const { return ELINT;			}
		bool			contains(string*)				const;
		bool			contains(int)					const;
		bool			contains(double)				const;
		bool			contains(compound*)				const;
		unsigned int	size()							const { return _last-_first+1;	}
		Element			element(unsigned int n)			const { Element e; e._int = _first+n; return e;	}
		unsigned int	position(Element,ElementType)	const;

		int	operator[](unsigned int n)	const { return _first+n;	}
		int	first()						const { return _first;		}
		int	last()						const { return _last;		}

		// Debugging
		string to_string() const;

};


/** Domain is a set of integers, but not necessarily an interval **/

class IntSortTable : public FiniteSortTable {
	
	private:
		vector<int> _table;

	public:

		// Constructors
		IntSortTable() : FiniteSortTable(), _table(0) { }

		// Destructor
		~IntSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);

		// Mutators
		void	sortunique();

		// Inspectors
		bool			empty()							const { return _table.empty();	}
		ElementType		type()							const { return ELINT;			}
		bool			contains(string*)				const;
		bool			contains(int)					const;
		bool			contains(double)				const;
		bool			contains(compound*)				const;
		unsigned int	size()							const { return _table.size();	}
		Element			element(unsigned int n)			const { Element e; e._int = _table[n]; return e;	}
		unsigned int	position(Element,ElementType)	const;

		int	operator[](unsigned int n)	const { return _table[n];			}
		int	first()						const { return _table.front();		}
		int	last()						const { return _table.back();		}

		// Debugging
		string to_string() const;

};

/** Domain is a set of doubles **/

class FloatSortTable : public FiniteSortTable {
	
	private:
		vector<double> _table;

	public:

		// Constructors
		FloatSortTable() : FiniteSortTable(), _table(0) { }

		// Destructor
		~FloatSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);

		// Mutators
		void	sortunique();

		// Inspectors
		bool			empty()							const { return _table.empty();		}
		ElementType		type()							const { return ELDOUBLE;			}
		bool			contains(string*)				const;
		bool			contains(int)					const;
		bool			contains(double)				const;
		bool			contains(compound*)				const;
		unsigned int	size()							const { return _table.size();		}
		Element			element(unsigned int n)			const { Element e; e._double = _table[n]; return e;	}
		unsigned int	position(Element,ElementType)	const;

		double	operator[](unsigned int n)	const { return _table[n];			}
		double	first()						const { return _table.front();		}
		double	last()						const { return _table.back();		}

		// Debugging
		string to_string() const;

};


/** Domain is a set of strings **/

class StrSortTable : public FiniteSortTable {
	
	private:
		vector<string*> _table;

	public:

		// Constructors
		StrSortTable() : _table(0) { }

		// Destructor
		~StrSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);

		// Cleanup
		void	sortunique();

		// Inspectors
		bool			empty()							const { return _table.empty();		}
		ElementType		type()							const { return ELSTRING;			}
		bool			contains(int)					const;
		bool			contains(double)				const;
		bool			contains(string*)				const;
		bool			contains(compound*)				const;
		unsigned int	size()							const { return _table.size();		}
		Element			element(unsigned int n)			const { Element e; e._string = _table[n]; return e;	}
		unsigned int	position(Element,ElementType)	const;

		string*	operator[](unsigned int n)	const { return _table[n];			}
		string*	first()						const { return _table.front();		}
		string*	last()						const { return _table.back();		}

		// Debugging
		string to_string() const;

};

/** Domain contains numbers, strings, and/or compounds **/

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

		// Destructor
		~MixedSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);

		// Mutators
		void			sortunique();

		// Inspectors
		bool			empty()							const;
		ElementType		type()							const;
		bool			contains(int)					const;
		bool			contains(double)				const;
		bool			contains(string*)				const;
		bool			contains(compound*)				const;
		unsigned int	size()							const;
		Element			element(unsigned int n)			const; 
		unsigned int	position(Element,ElementType)	const;

		// Debugging
		string to_string() const;

};


/***********************************************
	Interpretations for non-unary predicates
***********************************************/

/** Abstract base class **/

class NonUnaryPredTable : public PredTable {

	protected:
		vector<ElementType>	_types;
	
	public:
		
		// Constructors
		NonUnaryPredTable(const vector<ElementType>& t) : PredTable(), _types(t) { }

		// Destructor
		virtual ~NonUnaryPredTable() { }

		// Mutators
		virtual void	sortunique()	= 0;

		// Inspectors
		virtual bool				finite()				const = 0;	// true iff the table is finite
		virtual	bool				empty()					const = 0;	// true iff the table is empty
				unsigned int		arity()					const { return _types.size();	}	
				ElementType			type(unsigned int n)	const { return _types[n];		}

		// Check if the table contains a given tuple
		virtual	bool				contains(const vector<Element>&)		const = 0;

		// Inspectors for finite tables
		virtual	unsigned int		size()									const = 0;	// the size of the table
		virtual	vector<Element>		tuple(unsigned int n)					const = 0;	// the n'th tuple
		virtual Element				element(unsigned int r,unsigned int c)	const = 0;	// the element at position (r,c)

		// Debugging
		virtual string to_string(unsigned int spaces = 0)	const = 0;
};

/** Finite tables  **/

class FinitePredTable : public NonUnaryPredTable {
	
	private:
		vector<vector<Element> >	_table;		// the actual table
		ElementWeakOrdering			_order;		// less-than-or-equal relation on the tuples of this table
		ElementEquality				_equality;	// equality relation on the tuples of this table

	public:

		// Constructors 
		FinitePredTable(const vector<ElementType>& t) : NonUnaryPredTable(t), _table(0), _order(t), _equality(t) { }
		FinitePredTable(const UserPredTable&);

		// Destructor
		~FinitePredTable() { }

		// Mutators
		void	sortunique();

		// Parsing
		void				addRow()								{ _table.push_back(vector<Element>(_types.size()));	}
		void				addRow(const vector<Element>& ve, const vector<ElementType>&);
		void				addColumn(ElementType);
		void				changeElType(unsigned int,ElementType);
		vector<Element>&	operator[](unsigned int n)				{ return _table[n];	}

		// Inspectors
		bool				finite()								const { return true;			}
		bool				empty()									const { return _table.empty();	}
		unsigned int		size()									const { return _table.size();	}
		vector<Element>		tuple(unsigned int n)					const { return _table[n];		}
		Element				element(unsigned int r,unsigned int c)	const { return _table[r][c];	}

		// Check if the table contains a given tuple
		bool	contains(const vector<Element>&)	const;

		// Other inspectors
		VVE::const_iterator		begin()									const { return _table.begin();	}
		VVE::const_iterator		end()									const { return _table.end();	}
		const vector<Element>&	operator[](unsigned int n)				const { return _table[n];		}
		
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
		virtual ~PredInter();

		// Mutators
		void replace(PredTable* pt,bool ctpf, bool c);	// If ctpf is true, replace _ctpf by pt and set _ct to c
														// Else, replace cfpt by pt and set _cf to c
		void sortunique()	{ _ctpf->sortunique(); if(_ctpf != _cfpt) _cfpt->sortunique();	}

		// Inspectors
		PredTable*	ctpf()									const { return _ctpf;	}
		PredTable*	cfpt()									const { return _cfpt;	}
		bool		ct()									const { return _ct;		}
		bool		cf()									const { return _cf;		}
		bool		istrue(const vector<Element>& vi)		const { return (_ct ? _ctpf->contains(vi) : !(_ctpf->contains(vi)));	}
		bool		isfalse(const vector<Element>& vi)		const { return (_cf ? _cfpt->contains(vi) : !(_cfpt->contains(vi)));	}
		bool		istrue(const vector<TypedElement>& vi)	const;	// return true iff the given tuple is true or inconsistent
		bool		isfalse(const vector<TypedElement>& vi)	const;	// return false iff the given tuple is false or inconsistent

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/************************************
	Interpretations for functions 
************************************/

HIER BEZIG

class FuncInter {

	protected:
		vector<ElementType>	_intypes;
		ElementType			_outtype;

	public:
		
		// Constructors
		FuncInter(const vector<ElementType>& in, ElementType out) : _intypes(in), _outtype(out) { }

		// Destructor
		virtual ~FuncInter() { }

		// Mutators
		virtual void sortunique() { }

		// Inspectors
		virtual Element		operator[](const vector<Element>& vi)		const = 0;
				Element		operator[](const vector<TypedElement>& vi)	const;
		virtual PredInter*	predinter()									const = 0;
				ElementType	outtype()									const { return _outtype;	}
		
		// Debugging
		virtual string to_string(unsigned int spaces = 0) const = 0;

};

class UserFuncInter : public FuncInter {

	private:
		UserPredTable*		_ftable;	// the function table, if the function is 2-valued
		PredInter*			_ptable;	// interpretation of the associated predicate
		ElementWeakOrdering	_order;
		ElementEquality		_equality;

	public:
		
		// Constructors
		UserFuncInter(const vector<ElementType>& t, ElementType out, PredInter* pt) : 
			FuncInter(t,out), _ftable(0), _ptable(pt), _order(t), _equality(t) { }
		UserFuncInter(const vector<ElementType>& t, ElementType out, PredInter* pt, UserPredTable* ft) : 
			FuncInter(t,out), _ftable(ft), _ptable(pt), _order(t), _equality(t) { }

		// Destructor
		virtual ~UserFuncInter() { delete(_ptable); } // NOTE: _ftable is also deleted because it is part of _ptable

		// Mutators
		void sortunique() { _ptable->sortunique();	}

		// Inspectors
		bool			istrue(const vector<Element>& vi)			const { return _ptable->istrue(vi);		}
		bool			isfalse(const vector<Element>& vi)			const { return _ptable->isfalse(vi);	}
		Element			operator[](const vector<Element>& vi)		const;
		PredInter*		predinter()									const { return _ptable;					}
		UserPredTable*	ftable()									const { return _ftable;					}

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};

/************************
	Auxiliary methods
************************/

namespace TableUtils {
	PredInter*	leastPredInter(unsigned int n);	// construct a new, least precise predicate interpretation with arity n
	FuncInter*	leastFuncInter(unsigned int n);	// construct a new, least precise function interpretation with arity n
}

/*****************
	Structures
*****************/

class AbstractStructure {

	protected:

		string			_name;			// The name of the structure
		ParseInfo		_pi;			// The place where this structure was parsed.
		Vocabulary*		_vocabulary;	// The vocabulary of the structure.

	public:

		// Constructors
		AbstractStructure(string name, ParseInfo* pi) : _name(name), _pi(pi), _vocabulary(0) { }

		// Destructor
		virtual ~AbstractStructure() { if(_pi) delete(_pi);	}

		// Mutators
		virtual void	vocabulary(Vocabulary* v) = 0;			// set the vocabulary

		// Inspectors
		const string&	name()						const { return _name;		}
		ParseInfo*		pi()						const { return _pi;			}
		Vocabulary*		vocabulary()				const { return _vocabulary;	}

		// Debugging
		virtual string	to_string(unsigned int spaces = 0) const = 0;

};

class Structure : public AbstractStructure {

	private:

		vector<SortTable*>	_sortinter;		// The domains of the structure. 
											// The domain for sort s is stored in _sortinter[n], 
											// where n is the index of s in _vocabulary.
											// If a sort has no domain, a null-pointer is stored.
		vector<PredInter*>	_predinter;		// The interpretations of the predicate symbols.
											// If a predicate has no interpretation, a null-pointer is stored.
		vector<FuncInter*>	_funcinter;		// The interpretations of the function symbols.
											// If a function has no interpretation, a null-pointer is stored.
	
	public:
		
		// Constructors
		Structure(string name, ParseInfo* pi) : AbstractStructure(name,pi) { }

		// Destructor
		~Structure();

		// Mutators
		void	vocabulary(Vocabulary* v);			// set the vocabulary
		void	inter(Sort* s,SortTable* d);		// set the domain of s to d.
		void	inter(Predicate* p, PredInter* i);	// set the interpretation of p to i.
		void	inter(Function* f, FuncInter* i);	// set the interpretation of f to i.
		void	close();							// set the interpretation of all predicates and functions that 
													// do not yet have an interpretation to the least precise 
													// interpretation.

		// Inspectors
		Vocabulary*		vocabulary()				const { return AbstractStructure::vocabulary();	}
		SortTable*		inter(Sort* s)				const;	// Return the domain of s.
		PredInter*		inter(Predicate* p)			const;	// Return the interpretation of p.
		FuncInter*		inter(Function* f)			const;	// Return the interpretation of f.
		PredInter*		inter(PFSymbol* s)			const;  // Return the interpretation of s.
		SortTable*		sortinter(unsigned int n)	const { return _sortinter[n];	}
		PredInter*		predinter(unsigned int n)	const { return _predinter[n];	}
		FuncInter*		funcinter(unsigned int n)	const { return _funcinter[n];	}
		bool			hasInter(Sort* s)			const;	// True iff s has an interpretation
		bool			hasInter(Predicate* p)		const;	// True iff p has an interpretation
		bool			hasInter(Function* f)		const;	// True iff f has an interpretation

		// Visitor
		void accept(Visitor* v)	{ v->visit(this);	}

		// Debugging
		string	to_string(unsigned int spaces = 0) const;

};

class Theory;
namespace StructUtils {
	Theory*	convert_to_theory(Structure*);	// Make a theory containing all literals that are true according to the given structure
	PredTable*	complement(PredTable*,const vector<Sort*>&, Structure*);	// Compute the complement of the given table 
}

#endif
