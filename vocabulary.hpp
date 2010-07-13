/************************************
	vocabulary.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef VOCABULARY_H
#define VOCABULARY_H

#include "common.hpp"
#include <map>
#include <cassert>

/*******************************************
	Argument types for inference methods
*******************************************/

// The types are: void, theory, structure, vocabulary and namespace
enum InfArgType { IAT_VOID, IAT_THEORY, IAT_STRUCTURE, IAT_VOCABULARY, IAT_NAMESPACE };

// Convert a InfArgType to a string (e.g., IAT_VOID converts to "void")
namespace IATUtils { 
	string to_string(InfArgType);
}

/***************************************
	Parse location of parsed objects	
***************************************/

/*
 *		A ParseInfo contains a line number, a column number and a file. 
 *
 *		Almost every object that can be written by a user has a ParseInfo object as attribute. 
 *		The ParseInfo stores where the object was parsed. This is used for producing precise error and warning messages.
 *		Objects that are not parsed (e.g., internally created variables) have line number 0 in their ParseInfo.
 *
 */
class ParseInfo {

	private:

		unsigned int	_line;	// line number where the object is declared (0 for non-parsed objects)
		unsigned int	_col;	// column number where the object is declared
		string*			_file;	// file name where the object is declared
								// NOTE: different ParseInfo objects may point to the same string. Do not call delete(_file) when
								// deleting a ParseInfo object.
								// NOTE: _file == 0 when parsed on stdin
	
	public:

		// Constructors
		ParseInfo() : _line(0), _col(0), _file(0) { }
		ParseInfo(unsigned int line, unsigned int col, string* file) : _line(line), _col(col), _file(file) { }
		ParseInfo(const ParseInfo& p) : _line(p.line()), _col(p.col()), _file(p.file()) { }

		// Destructor
		virtual ~ParseInfo() { }

		// Inspectors
		unsigned int	line()		const { return _line;		}
		unsigned int	col()		const { return _col;		}
		string*			file()		const { return _file;		}
		bool			isParsed()	const { return _line != 0;	}

};

/*
 *		ParseInfo for formulas.
 *		
 *		Besides the attributes of a ParseInfo object, it contains also the originally parsed formula
 *
 */
class Formula;
class FormParseInfo : public ParseInfo {

	private:
		Formula*	_original;

	public:

		// Constructors
		FormParseInfo() : ParseInfo(), _original(0) { }
		FormParseInfo(unsigned int line, unsigned int col, string* file, Formula* orig) : 
			ParseInfo(line,col,file), _original(orig) { }
		FormParseInfo(const FormParseInfo& p) : ParseInfo(p.line(),p.col(),p.file()), _original(p.original()) { }

		// Inspectors
		Formula*	original()	const { return _original;	}

};


/*************
	Sorts
*************/

class Predicate;
class SortTable;

class Sort {

	private:

		string			_name;		// name of the sort
		Sort*			_parent;	// parent sort (equal to 0, if this is a base sort)
		Sort*			_base;		// base sort 
		vector<Sort*>	_children;	// the children of the sort in the sort hierarchy 
		unsigned int	_depth;		// depth of this sort in the sort hierarchy (0 for base sorts)
		Predicate*		_pred;		// the predicate that corresponds to the sort
		ParseInfo		_pi;		// the place where the sort was declared (0 for non user-defined sorts)

	public:

		// Constructors
		Sort(const string& name);
		Sort(const string& name, const ParseInfo& pi);  

		// Destructor
		virtual ~Sort() { }	// NOTE: deleting a sort creates dangling pointers
							// Only delete sorts when the global namespace is deleted

		// Mutators
		void	parent(Sort* p);	// Set the parent of the sort to p. This also changes _base and _depth.
		void	child(Sort* c);		// Add c to the children of the sort. Also set the parent of c to this.
		void	pred(Predicate* p)	{ _pred = p;	}
		
		// Inspectors
		string				name()					const	{ return _name;				} 
		const ParseInfo&	pi()					const	{ return _pi;				}
		Sort*				parent()				const	{ return _parent;			}
		Sort*				base()					const	{ return _base;				}
		Predicate*			pred()					const	{ return _pred;				}
		unsigned int		nrChildren()			const	{ return _children.size();	}
		Sort*				child(unsigned int n)	const	{ return _children[n];		}
		int					depth()					const	{ return _depth;			}
		bool				intsort()				const;  // Returns true if the sort is a subsort of the integers
		bool				floatsort()				const;  // Returns true if the sort is a subsort of the floats
		bool				stringsort()			const;  // Returns true if the sort is a subsort of the strings
		bool				charsort()				const;  // Returns true if the sort is a subsort of the chars
		string				to_string()				const	{ return _name;				}
		virtual bool		builtin()				const	{ return false;				}
		virtual SortTable*	inter()					const	{ return 0;					}	// returns the built-in interpretation for
																							// built-in sorts

};

namespace SortUtils {

	/*
	 * return the common ancestor with maximum depth of the given sorts. 
	 * Return 0 if such an ancestor does not exist.
	 */ 
	Sort* resolve(Sort* s1, Sort* s2);
}


/****************
	Variables	
****************/

class Variable {

	private:
		string		_name;	// name of the variable
		Sort*		_sort;	// sort of the variable (0 if the sort is not derived)
		static int	_nvnr;	// used to create unique new names for internal variables
		ParseInfo	_pi;	// the place where the variable was quantified (0 for non user-defined variables)

	public:

		// Constructors
		Variable(const string& name, Sort* sort, const ParseInfo& pi) : _name(name), _sort(sort), _pi(pi) { }
		Variable(Sort* s);	// constructor for an internal variable 

		// Destructor
		~Variable() { }	// NOTE: deleting variables creates dangling pointers
						// Only delete a variable when deleting its quantifier!

		// Mutators
		void	sort(Sort* s)	{ _sort = s; }

		// Inspectors
		string				name()	const { return _name;	}
		Sort*				sort()	const { return _sort;	}
		const ParseInfo&	pi()	const { return _pi;		}

		// Debugging
		string	to_string()		const;

};

namespace VarUtils {

	/*
	 * Sort and remove duplicates from a given vector of variables
	 */
	void sortunique(vector<Variable*>& vv);
}


/*******************************
	Predicates and functions
*******************************/

/** Abstract base class **/

class PFSymbol {
	
	protected:
		string				_name;	// Name of the symbol (ending with the /arity)
		vector<Sort*>		_sorts;	// Sorts of the arguments
		ParseInfo			_pi;	// the place where the symbol was declared (0 for non user-defined symbols)

	public:

		// Constructors
		PFSymbol(const string& name, const vector<Sort*>& sorts) : 
			_name(name), _sorts(sorts) { }
		PFSymbol(const string& name, const vector<Sort*>& sorts, const ParseInfo& pi) : 
			_name(name), _sorts(sorts), _pi(pi) { }

		// Destructor
		virtual ~PFSymbol() { }		// NOTE: deleting a PFSymbol creates dangling pointers
									// Only delete a PFSymbol if the global namespace is deleted

		// Inspectors
		string					name()					const { return _name;							}
		const ParseInfo&		pi()					const { return _pi;								}
		unsigned int			nrsorts()				const { return _sorts.size();					}
		Sort*					sort(unsigned int n)	const { return _sorts[n];						}
		const vector<Sort*>&	sorts()					const { return _sorts;							}
		string					to_string()				const { return _name.substr(0,_name.find('/'));	}
		virtual bool			ispred()				const = 0;	// true iff the symbol is a predicate

		// Built-in symbols 
		virtual bool			builtin()			const				{ return false;	}	// true iff the symbol is built-in
		virtual bool			overloaded()		const				{ return false;	}	// true iff the symbol is overloaded
		virtual PFSymbol*		disambiguate(const vector<Sort*>&)		{ return this;	}	// this method tries to disambiguate 
																							// overloaded symbols. See builtin.cpp
																							// for more information
																						

};


/** Predicate symbols **/

class PredInter;

class Predicate : public PFSymbol {

	private:
		static int	_npnr;	// used to create unique new names for internal predicates

	public:

		// Constructors
		Predicate(const string& name,const vector<Sort*>& sorts, const ParseInfo& pi) : PFSymbol(name,sorts,pi) { }
		Predicate(const string& name,const vector<Sort*>& sorts) : PFSymbol(name,sorts) { }
		Predicate(const vector<Sort*>& sorts);	// constructor for internal/tseitin predicates

		// Inspectors
						unsigned int	arity()								const { return _sorts.size();	}
						bool			ispred()							const { return true;			}
		virtual			PredInter*		inter(const vector<SortTable*>& vs)	const { return 0;				}

		// Built-in symbols 
		virtual Predicate*		disambiguate(const vector<Sort*>&)		{ return this;	}

};


/** Functions **/

class FuncInter;

class Function : public PFSymbol {

	protected:
		bool	_partial;	// true iff the function is declared as partial function

	public:

		// Constructors
		Function(const string& name, const vector<Sort*>& is, Sort* os, const ParseInfo& pi) : 
			PFSymbol(name,is,pi), _partial(false) { _sorts.push_back(os); }
		Function(const string& name, const vector<Sort*>& sorts, const ParseInfo& pi) : 
			PFSymbol(name,sorts,pi), _partial(false) { }
		Function(const string& name, const vector<Sort*>& is, Sort* os) : 
			PFSymbol(name,is), _partial(false) { _sorts.push_back(os); }
		Function(const string& name, const vector<Sort*>& sorts) : 
			PFSymbol(name,sorts), _partial(false) { }

		// Mutators
		void		partial(const bool& b)	{ _partial = b;		}
	
		// Inspectors
		vector<Sort*>		insorts()							const;	// returns the input sorts of the function
		unsigned int		arity()								const { return _sorts.size()-1;	}
		Sort*				insort(const int& n)				const { return _sorts[n];		}
		Sort*				outsort()							const { return _sorts.back();	}
		bool				partial()							const { return _partial;		}
		bool				ispred()							const { return false;			}
		virtual	FuncInter*	inter(const vector<SortTable*>& vs)	const { return 0;				}

		// Built-in symbols 
		virtual Function*		disambiguate(const vector<Sort*>&)		{ return this;	}

};


/*****************
	Vocabulary
*****************/

class Vocabulary {

	private:

		string		_name;	// name of the vocabulary. Default name is the empty string
		ParseInfo	_pi;	// place where the vocabulary was parsed

		// map symbols of the vocabulary to a unique index
		map<Sort*,unsigned int>			_sorts;		
		map<Predicate*,unsigned int>	_predicates;
		map<Function*,unsigned int>		_functions;
		// map the index of a symbol to the symbol
		vector<Sort*>					_vsorts;
		vector<Predicate*>				_vpredicates;
		vector<Function*>				_vfunctions;
		// map a name to a symbol of the vocabulary
		// NOTE: these lists are not exhaustive. 
		//	They only contain the symbols that are explicitly added by a user to the vocabulary.
		// NOTE: if string <my_name> maps to sort s, then s->name is not necessarily <my_name>
		// NOTE: the strings that map to predicates and functions end on /arity
		map<string,Sort*>		_sortnames;
		map<string,Predicate*>	_prednames;
		map<string,Function*>	_funcnames;

	public:

		// Constructors
		Vocabulary(const string& name) : _name(name) { }
		Vocabulary(const string& name, const ParseInfo& pi) : _name(name), _pi(pi) { }

		// Destructor
		virtual ~Vocabulary() { }

		// Mutators
		void addSort(Sort*);						 // Add the given sort (and its ancestors) to the vocabulary
		void addPred(Predicate*);					 // Add the given predicate (and its sorts) to the vocabulary
		void addFunc(Function*);					 // Add the given function (and its sorts) to the vocabulary
		void addSort(const string& n, Sort* s);		 // Add the given sort (and its ancestors) and set _sortnames[n] = s
		void addPred(const string& n, Predicate* p); // Add the given predicate (and its sorts) to the vocabulary and set _prednames[n] = p
		void addFunc(const string& n, Function* f);	 // Add the given function (and its sorts) to the vocabulary and set _funcnames[n] = f

		// Inspectors
		const string&		name()							const { return _name;					}
		const ParseInfo&	pi()							const { return _pi;						}
		bool				contains(Sort*)					const;	// true iff the vocabulary contains the given sort
		bool				contains(Predicate*)			const;	// true iff the vocabulary contains the given predicate
		bool				contains(Function*)				const;	// true iff the vocabulary contains the given function
		unsigned int		index(Sort*)					const;	// return the index of the given sort
		unsigned int		index(Predicate*)				const;	// return the index of the given predicate
		unsigned int		index(Function*)				const;	// return the index of the given function
		unsigned int		nrSorts()						const	{ return _vsorts.size();		}
		unsigned int		nrPreds()						const	{ return _vpredicates.size();	}
		unsigned int		nrFuncs()						const	{ return _vfunctions.size();	}
		Sort*				sort(unsigned int n)			const	{ return _vsorts[n];			}
		Predicate*			pred(unsigned int n)			const	{ return _vpredicates[n];		}
		Function*			func(unsigned int n)			const	{ return _vfunctions[n];		}
		Sort*				sort(const string&)				const;	// return the sort with the given name
		Predicate*			pred(const string&)				const;	// return the predicate with the given name (ending on /arity)
		Function*			func(const string&)				const;	// return the function with the given name (ending on /arity)
		vector<Predicate*>	pred_no_arity(const string&)	const;	// return all predicates with the given name (not including the arity)
		vector<Function*>	func_no_arity(const string&)	const;	// return all functions with the given name (not including the arity)

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/**********************
	Domain elements
**********************/

/*
 * The three different types of domain elements 
 *		ELINT: an integer 
 *		ELDOUBLE: a floating point number 
 *		ELSTRING: a string (characters are strings of length 1)
 *		ELCOMPOUND: a constructor function applied to a number of domain elements
 *
 *		These types form a hierarchy: ints are also doubles, doubles are also strings, strings are also compounds.
 *
 */

enum ElementType { ELINT, ELDOUBLE, ELSTRING, ELCOMPOUND };

// A single domain element
struct compound;
union Element {
	int			_int;
	double		_double;
	string*		_string;
	compound*	_compound;
};

// A pair of a domain element and its type
struct TypedElement{
	Element		_element;
	ElementType	_type;
	TypedElement(Element e, ElementType t) : _element(e), _type(t) { }
	TypedElement() : _type(ELINT) { }
};

/*
 *		Compound domain elements
 *		
 *		NOTE: sometimes, an int/double/string is represented as a compound domain element
 *		in that case, _function=0 and the only element in _args is the int/double/string
 *
 */
struct compound {
	Function*				_function;
	vector<TypedElement>	_args;

	compound(Function* f, const vector<TypedElement>& a) : _function(f), _args(a) { }
	string to_string() const;
};

// Class that implements the relation 'less-than-or-equal' on tuples of domain elements with the same types
class ElementWeakOrdering {
	
	private:
		vector<ElementType>	_types;	// the types of the two tuples that are compared

	public:
		ElementWeakOrdering(const vector<ElementType>& t) : _types(t) { }
		bool operator()(const vector<Element>&,const vector<Element>&) const;
		void addType(ElementType t) { _types.push_back(t);	}
		void changeType(unsigned int n, ElementType t) { _types[n] = t;	}
};

// Class that implements the relation 'equal' on tuples of domain elements with the same types
class ElementEquality {

	private:
		vector<ElementType>	_types;	// the types of the two tuples that are compared

	public:
		ElementEquality(const vector<ElementType>& t) : _types(t) { }
		bool operator()(const vector<Element>&,const vector<Element>&) const;
		void addType(ElementType t) { _types.push_back(t);	}
		void changeType(unsigned int n, ElementType t) { _types[n] = t;	}

};

// Useful functions on domain elements
namespace ElementUtil {
	
	// Return the least precise elementtype of the two given types
	ElementType	resolve(ElementType,ElementType);

	// Return the most precise elementtype
	ElementType leasttype() { return ELINT;	}

	// Convert a domain element to a string
	string		ElementToString(Element,ElementType);
	string		ElementToString(TypedElement);

	// Return the non-existing domain element. 
	//	Non-existing domain elements are used as return value when a partial function is applied on elements outside its domain
	Element	nonexist(ElementType);				

	// Checks if the element exists
	bool		exists(Element,ElementType);	
	bool		exists(TypedElement);					

	// Convert an element from one type to another
	Element		convert(TypedElement,ElementType newtype);		
	Element		convert(Element,ElementType oldtype,ElementType newtype);

	// Compare elements
	bool	equal(Element e1, ElementType t1, Element e2, ElementType t2);				// equality
	bool	strlessthan(Element e1, ElementType t1, Element e2, ElementType t2);		// strictly less than
	bool	lessthanorequal(Element e1, ElementType t1, Element e2, ElementType t2);	// less than or equal

}

bool operator==(TypedElement e1, TypedElement e2);
bool operator<=(TypedElement e1, TypedElement e2);
bool operator<(TypedElement e1, TypedElement e2);




#endif
