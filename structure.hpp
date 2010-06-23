/************************************
	structure.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef STRUCTURE_HPP
#define STRUCTURE_HPP

#include "vocabulary.hpp"

/******************************************
	Domains (interpretations for sorts)
******************************************/

class SortTable {
	
	public:

		// Destructor
		virtual ~SortTable() { }

		// Inspectors
		virtual bool			finite()				const { return true;	}	// Return true iff the size of the table is finite
		virtual unsigned int	size()					const = 0;		// Returns the number of elements.
		virtual bool			empty()					const = 0;		// True iff the sort contains no elements

		virtual	bool			contains(const string&)	const = 0;		// true iff the table contains the string.
																		// Only works correct if the table is sorted and
																		// contains no doubles. 
		virtual bool			contains(int)			const = 0;		// true iff the table contains the integer
																		// Only works correct if the table is sorted and
																		// contains no doubles. 
		virtual bool			contains(double)		const = 0;		// true iff the table contains the integer
																		// Only works correct if the table is sorted and
																		// contains no doubles.
				bool			contains(Element,ElementType) const;	// true iff the table contains the element
																		// Only works correct if the table is sorted and
																		// contains no doubles. 
																		//
		virtual ElementType		type()					const = 0;		// return the type (int, double or string) of the
																		// elements in the table

		virtual Element			element(unsigned int n)	= 0;			// returns (a pointer to) the n'th element

		// Cleanup
		virtual void sortunique() { } // Sort the table and remove doubles.

		// Visitor
		void accept(Visitor*);

		// Debugging
		virtual string to_string() const = 0;

};

class UserSortTable : public SortTable {

	public:

		// Destructor
		virtual ~UserSortTable() { }

		// Add elements to a table
		// A pointer to the resulting table is returned. This pointer may point to 'this', but this is not
		// neccessarily the case. No pointers are deleted when calling these methods.
		// The result of add(...) is not necessarily sorted and may contain doubles.
		virtual UserSortTable*	add(int)			= 0;	 // Add an integer to the table.
		virtual UserSortTable*	add(double)			= 0;	 // Add a floating point number to the table
		virtual UserSortTable*	add(const string&)	= 0;	 // Add a string to the table.
		virtual UserSortTable*	add(int,int)		= 0;	 // Add a range of integers to the table.
		virtual UserSortTable*	add(char,char)		= 0;	 // Add a range of characters to the table.

		// Cleanup
		virtual void sortunique() = 0; // Sort the table and remove doubles.

		// Inspectors
		Element					element(unsigned int n)		 = 0;	// returns the n'th element

		// Debugging
		virtual string to_string() const = 0;

};

/** Empty table **/

class EmptySortTable : public UserSortTable {

	public:

		// Destructor
		virtual ~EmptySortTable() { }

		UserSortTable*	add(int);	
		UserSortTable*	add(double);	
		UserSortTable*	add(const string&);	
		UserSortTable*	add(int,int);	
		UserSortTable*	add(char,char);	

		// Cleanup
		void sortunique() { }

		// Inspectors
		bool			finite()					const { return true;	} 
		unsigned int	size()						const { return 0;		}	
		bool			empty()						const { return true;	}	
		bool			contains(const string&)		const { return false;	}	
		bool			contains(int)				const { return false;	}	
		bool			contains(double)			const { return false;	}	
		ElementType		type()						const { return ELINT;	}
		Element			element(unsigned int n)		{ assert(false); Element e; return e; } 
															
		// Debugging
		string to_string() const { return "";	}

};

/** Domain is an interval of integers **/

class RanSortTable : public UserSortTable {
	
	private:
		int _first;		// first element in the range
		int _last;		// last element in the range

	public:

		// Destructor
		~RanSortTable() { }

		// Constructors
		RanSortTable(int f, int l) : _first(f), _last(l) { }

		// Mutators
		UserSortTable*	add(int);
		UserSortTable*	add(const string&);
		UserSortTable*	add(double);
		UserSortTable*	add(int,int);
		UserSortTable*	add(char,char);

		// Cleanup
		void			sortunique() { }

		// Inspectors
		unsigned int	size()							const { return _last-_first+1;		}
		bool			empty()							const { return _first > _last;		}
		int				operator[](unsigned int n)		const { return _first+n;			}
		int				first()							const { return _first;				}
		int				last()							const { return _last;				}
		bool			contains(const string&)			const;
		bool			contains(int)					const;
		bool			contains(double)				const;
		ElementType		type()							const { return ELINT;				}
		Element			element(unsigned int n)			{ Element e; e._int = _first+n; return e;	}

		// Debugging
		string to_string() const;

};


/** Domain is a set of integers, but not an interval **/

class IntSortTable : public UserSortTable {
	
	private:
		vector<int> _table;

	public:

		// Constructors
		IntSortTable() : _table(0) { }

		// Destructor
		~IntSortTable() { }

		// Mutators
		UserSortTable*	add(int);
		UserSortTable*  add(const string&);
		UserSortTable*	add(double);
		UserSortTable*  add(int,int);
		UserSortTable*  add(char,char);

		// Cleanup
		void	sortunique();

		// Inspectors
		unsigned int	size()						const { return _table.size();		}
		bool			empty()						const { return _table.empty();		}
		bool			contains(const string&)		const;
		bool			contains(int)				const;
		bool			contains(double)			const;
		int				operator[](unsigned int n)	const { return _table[n];			}
		int				first()						const { return _table.front();		}
		int				last()						const { return _table.back();		}
		ElementType		type()						const { return ELINT;				}
		Element			element(unsigned int n)		{ Element e; e._int = _table[n]; return e;	}

		// Debugging
		string to_string() const;

};

/** Domain is a set of doubles **/

class FloatSortTable : public UserSortTable {
	
	private:
		vector<double> _table;

	public:

		// Constructors
		FloatSortTable() : _table(0) { }

		// Destructor
		~FloatSortTable() { }

		// Mutators
		UserSortTable*	add(int);
		UserSortTable*  add(const string&);
		UserSortTable*	add(double);
		UserSortTable*  add(int,int);
		UserSortTable*  add(char,char);

		// Cleanup
		void	sortunique();

		// Inspectors
		unsigned int	size()						const { return _table.size();		}
		bool			empty()						const { return _table.empty();		}
		bool			contains(const string&)		const;
		bool			contains(int)				const;
		bool			contains(double)			const;
		double			operator[](unsigned int n)	const { return _table[n];			}
		double			first()						const { return _table.front();		}
		double			last()						const { return _table.back();		}
		ElementType		type()						const { return ELDOUBLE;			}
		Element			element(unsigned int n)		{ Element e; e._double = &(_table[n]); return e;	}

		// Debugging
		string to_string() const;

};


/** Domain is a set of strings **/

class StrSortTable : public UserSortTable {
	
	private:
		vector<string> _table;

	public:

		// Constructors
		StrSortTable() : _table(0) { }

		// Destructor
		~StrSortTable() { }

		// Mutators
		UserSortTable*	add(int);
		UserSortTable*	add(const string&);
		UserSortTable*	add(double);
		UserSortTable*	add(int,int);
		UserSortTable*	add(char,char);

		// Cleanup
		void			sortunique();

		// Inspectors
		unsigned int	size()						const { return _table.size();		}
		bool			empty()						const { return _table.empty();		}
		bool			contains(const string&)		const;
		bool			contains(int)				const;
		bool			contains(double)			const;
		const string&	operator[](unsigned int n)	const { return _table[n];			}
		const string&	first()						const { return _table.front();		}
		const string&	last()						const { return _table.back();		}
		ElementType		type()						const { return ELSTRING;			}
		Element			element(unsigned int n)		{ Element e; e._string = &(_table[n]); return e;	}

		// Debugging
		string to_string() const;

};

/** Domain contains both numbers and strings **/

class MixedSortTable : public UserSortTable {
	
	private:
		vector<double>	_numtable;
		vector<string>	_strtable;
		vector<string*>	_numstrmem;

	public:

		// Constructors
		MixedSortTable() : _numtable(0), _strtable(0), _numstrmem(0) { }
		MixedSortTable(const vector<string>& t) : _numtable(0), _strtable(t), _numstrmem(0) { }
		MixedSortTable(const vector<double>& t) : _numtable(t), _strtable(0), _numstrmem(t.size(),0) { }

		// Destructor
		~MixedSortTable() { for(unsigned int n = 0; n < _numstrmem.size(); ++n) { if(_numstrmem[n]) delete(_numstrmem[n]); } }

		// Mutators
		UserSortTable*	add(int);
		UserSortTable*	add(const string&);
		UserSortTable*	add(double);
		UserSortTable*	add(int,int);
		UserSortTable*	add(char,char);

		// Cleanup
		void			sortunique();

		// Inspectors
		unsigned int	size()						const { return _numtable.size() + _strtable.size();			}
		bool			empty()						const { return (_numtable.empty() && _strtable.empty());	}
		bool			contains(const string&)		const;
		bool			contains(int)				const;
		bool			contains(double)			const;
		ElementType		type()						const { return ELSTRING;	}
		Element			element(unsigned int n); 

		// Debugging
		string to_string() const;

};


/*************************************
	Interpretations for predicates
*************************************/

class PredTable {

	protected:
		vector<ElementType>	_types;
	
	public:
		
		// Constructors
		PredTable(const vector<ElementType>& t) : _types(t) { }

		// Destructor
		virtual ~PredTable() { }

		// Mutators
		virtual void	sortunique()	{ }	// Sort and remove doubles

		// Inspectors
		virtual bool				finite()								const = 0;	// true iff the table is finite
		virtual	unsigned int		size()									const = 0;	// the size of the table
		virtual	bool				empty()									const = 0;	// true iff the table is empty
		virtual	bool				contains(const vector<Element>&)		const = 0;
				bool				contains(const vector<TypedElement>&)	const;
		virtual	vector<Element>		tuple(unsigned int n)					const = 0;	// the n'th tuple
		virtual Element				element(unsigned int r,unsigned int c)	const = 0;	// the element at position (r,c)
				unsigned int				arity()							const { return _types.size();	}	
				ElementType					type(unsigned int n)			const { return _types[n];		}
				const vector<ElementType>&	types()							const { return _types;			}

		// Debugging
		virtual string to_string(unsigned int spaces = 0)	const = 0;
};

typedef vector<vector<Element> > VVE;

/** Finite tables **/

class FinitePredTable : public PredTable {

	public:
		
		// Constructors
		FinitePredTable(const vector<ElementType>& t) : PredTable(t) { }

		// Destructor
		virtual ~FinitePredTable() { }

		// Mutators
		virtual void	sortunique()												= 0;	// Sort and remove doubles
		virtual void	addRow(const vector<Element>&, const vector<ElementType>&)	= 0;	// Add a tuple

		// Inspectors
				bool				finite()								const { return true;	}
		virtual	unsigned int		size()									const = 0;	// the size of the table
		virtual	bool				empty()									const = 0;	// true iff the table is empty
		virtual	bool				contains(const vector<Element>&)		const = 0;
		virtual	vector<Element>		tuple(unsigned int n)					const = 0;	// the n'th tuple
		virtual Element				element(unsigned int r,unsigned int c)	const = 0;	// the element at position (r,c)

		// Debugging
		virtual string to_string(unsigned int spaces = 0)	const = 0;

};

/** Tables where all tuples are enumerated **/

class UserPredTable : public FinitePredTable {
	
	private:
		vector<vector<Element> >	_table;	
		ElementWeakOrdering			_order;
		ElementEquality				_equality;

	public:

		// Constructors 
		UserPredTable(const vector<ElementType>& t) : FinitePredTable(t), _table(0), _order(t), _equality(t) { }
		UserPredTable(const UserPredTable&);

		// Destructor
		virtual ~UserPredTable();

		// Mutators
		void			sortunique();		// Sort and remove doubles

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
		unsigned int			size()									const { return _table.size();	}
		bool					empty()									const { return _table.empty();	}
		bool					contains(const vector<Element>&)		const;
		const vector<Element>&	operator[](unsigned int n)				const { return _table[n];		}
		vector<Element>			tuple(unsigned int n)					const { return _table[n];		}
		Element					element(unsigned int r,unsigned int c)	const { return _table[r][c];	}
		
		// Debugging
		string to_string(unsigned int spaces = 0)	const;
};

/** Tables with arity 1 **/

class SortPredTable : public FinitePredTable {
	
	private:
		UserSortTable*	_table;

	public:

		// Constructors
		SortPredTable(UserSortTable* t);

		// Destructor
		virtual ~SortPredTable() { delete(_table);	}

		// Mutators
		void sortunique()						{ _table->sortunique();	}
		void addRow(const vector<Element>&, const vector<ElementType>&);
		void table(UserSortTable* ust)			{ _table = ust;			}

		// Inspectors
		unsigned int	size()									const { return _table->size();	}
		bool			empty()									const { return _table->empty();	}
		bool			contains(const vector<Element>&)		const;
		vector<Element>	tuple(unsigned int n)					const { return vector<Element>(1,_table->element(n));	}
		Element			element(unsigned int r,unsigned int c)	const { assert(!c);	return _table->element(r);			}
		UserSortTable*	table()									const { return _table;									}

		// Debugging
		string to_string(unsigned int spaces = 0)	const;
};

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
		PredTable*	ctpf()	const	{ return _ctpf;	}
		PredTable*	cfpt()	const	{ return _cfpt;	}
		bool		ct()	const	{ return _ct;	}
		bool		cf()	const	{ return _cf;	}
		bool		istrue(const vector<Element>& vi)	const { return (_ct ? _ctpf->contains(vi) : !(_ctpf->contains(vi)));	}
		bool		isfalse(const vector<Element>& vi)	const { return (_cf ? _cfpt->contains(vi) : !(_cfpt->contains(vi)));	}
		bool		istrue(const vector<TypedElement>& vi)	const;
		bool		isfalse(const vector<TypedElement>& vi)	const;

		// Visitor
		void accept(Visitor*);

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/************************************
	Interpretations for functions 
************************************/

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
		virtual const Element& 	operator[](const vector<Element>& vi)			const = 0;
		virtual const Element& 	operator[](const vector<TypedElement>& vi)		const = 0;
		virtual PredInter*		predinter()										const = 0;
				ElementType		outtype()										const { return _outtype;	}
		
		// Visitor
		void accept(Visitor*);

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
		const Element&	operator[](const vector<Element>& vi)		const;
		const Element&  operator[](const vector<TypedElement>& vi)	const;
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
	PredTable*	intersection(PredTable*,PredTable*);
	PredTable*	difference(PredTable*,PredTable*);
}

/*****************
	Structures
*****************/

class Structure {

	private:

		string				_name;			// The name of the structure
		ParseInfo*			_pi;			// The place where this structure was parsed.
		Vocabulary*			_vocabulary;	// The vocabulary of the structure.

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
		Structure(string name, ParseInfo* pi) : _name(name), _pi(pi) { }

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
		const string&	name()						const { return _name;		}
		ParseInfo*		pi()						const { return _pi;			}
		Vocabulary*		vocabulary()				const { return _vocabulary;	}
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
		void accept(Visitor*);

		// Debugging
		string	to_string(unsigned int spaces = 0) const;

};

class Theory;
namespace StructUtils {
	Theory*	convert_to_theory(Structure*);	// Make a theory containing all literals that are true according to the given structure
	PredTable*	complement(PredTable*,const vector<Sort*>&, Structure*);	// Compute the complement of the given table 
}

#endif
