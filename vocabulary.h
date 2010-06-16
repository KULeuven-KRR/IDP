/************************************
	vocabulary.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef VOCABULARY_H
#define VOCABULARY_H

#include <string>
#include <vector>
#include <map>
#include <cassert>
using namespace std;

/*********************
	Argument types
*********************/

enum InfArgType { IAT_VOID, IAT_THEORY, IAT_STRUCTURE, IAT_VOCABULARY, IAT_NAMESPACE };

namespace IATUtils { 
	string to_string(InfArgType);
}

/***************************************
	Parse location of parsed objects	
***************************************/

class ParseInfo {

	private:

		int			_line;		// line number where the object is declared
		int			_col;		// column number where the object is declared
		string*		_file;		// file name where the object is declared	
	
	public:

		// Constructors
		ParseInfo(unsigned int line, unsigned int col, string* file) :
			_line(line), _col(col), _file(file) { }
		ParseInfo(ParseInfo& p) : _line(p.line()), _col(p.col()), _file(p.file()) { }
		ParseInfo(ParseInfo* p) : _line(p->line()), _col(p->col()), _file(p->file()) { }

		// Destructor
		~ParseInfo() { }

		// Inspectors
		unsigned int	line()	const { return _line;	}
		unsigned int	col()	const { return _col;	}
		string*			file()	const { return _file;	}

};

/**********************
	Domain elements
**********************/

// The three different types of domain elements
//		ELINT: an integer
//		ELDOUBLE: a floating point number
//		ELSTRING: a string (characters are strings of length 1)
enum ElementType { ELINT, ELDOUBLE, ELSTRING };

// A single domain element
union Element {
	int		_int;
	double*	_double;
	string*	_string;
};

// A domain element and its type
struct TypedElement{
	Element		_element;
	ElementType	_type;
};

// Class that implements the relation 'less-than-or-equal' on tuples of domain elements
class ElementWeakOrdering {
	
	private:
		vector<ElementType>	_types;	// the types of the two tuples that are compared

	public:
		ElementWeakOrdering(const vector<ElementType>& t) : _types(t) { }
		bool operator()(const vector<Element>&,const vector<Element>&) const;
		void addType(ElementType t) { _types.push_back(t);	}
		void changeType(unsigned int n, ElementType t) { _types[n] = t;	}
};

// Class that implements the relation 'equal' on tuples of domain elements
class ElementEquality {

	private:
		vector<ElementType>	_types;	// the types of the two tuples that are compared

	public:
		ElementEquality(const vector<ElementType>& t) : _types(t) { }
		bool operator()(const vector<Element>&,const vector<Element>&) const;
		void addType(ElementType t) { _types.push_back(t);	}
		void changeType(unsigned int n, ElementType t) { _types[n] = t;	}

};

namespace ElementUtil {
	
	ElementType	resolve(ElementType,ElementType);

	string		ElementToString(Element,ElementType);	// Convert a domain element to a string
	string		ElementToString(TypedElement);

	Element&	nonexist(ElementType);					// Return the non-existing domain element (used for partial functions)

	bool		exists(Element,ElementType);			// Checks if the element exists
	bool		exists(TypedElement);					

	Element		convert(TypedElement,ElementType);			// Convert an element from one type to another
	Element		convert(Element,ElementType,ElementType);

	Element		clone(Element,ElementType);		// Clone an element (creates a new pointer in case of doubles and strings)
	Element		clone(TypedElement);
}



/*************
	Sorts
*************/

class Predicate;

class Sort {

	private:

		string			_name;		// name of the sort
		Sort*			_parent;	// parent sort (equal to 0, if this is a base sort)
		Sort*			_base;		// base sort 
		vector<Sort*>	_children;	// the children of the sort
		unsigned int	_depth;		// depth of this sort in the sort hierarchy
		Predicate*		_pred;		// the predicate that corresponds to the sort
		ParseInfo*		_pi;		// the place where the sort was declared (0 for non user-defined sorts)

	public:

		// Constructors
		Sort(const string& name, ParseInfo* pi);  

		// Destructor
		virtual ~Sort() { if(_pi) delete(_pi); }	// NOTE: deleting a sort creates dangling pointers
													// Only delete sorts when the global namespace is deleted

		// Mutators
		void	parent(Sort* p);	// Set the parent of the sort to p. This also changes _base and _depth.
		void	child(Sort* c);		// Add c to the children of the sort.
		void	pred(Predicate* p)	{ _pred = p;	}
		
		// Inspectors
		string			name()					const	{ return _name;				} 
		ParseInfo*		pi()					const	{ return _pi;				}
		Sort*			parent()				const	{ return _parent;			}
		Sort*			base()					const	{ return _base;				}
		Predicate*		pred()					const	{ return _pred;				}
		unsigned int	nrChildren()			const	{ return _children.size();	}
		Sort*			child(unsigned int n)	const	{ return _children[n];		}
		int				depth()					const	{ return _depth;			}
		bool			intsort()				const;  // Returns true if the sort is a subsort of the integers
		bool			floatsort()				const;  // Returns true if the sort is a subsort of the floats
		bool			stringsort()			const;  // Returns true if the sort is a subsort of the strings
		bool			charsort()				const;  // Returns true if the sort is a subsort of the chars
		string			to_string()				const	{ return _name;		}
		virtual bool	builtin()				const	{ return false;	}

};

namespace SortUtils {

	/*
	 * return the common ancestor with maximum depth of the sort and s. 
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
		ParseInfo*	_pi;	// the place where the variable was declared (0 for non user-defined variables)

	public:

		// Constructors
		Variable(const string& name, Sort* sort, ParseInfo* pi) : _name(name), _sort(sort), _pi(pi) { }
		Variable(Sort* s);	// constructor for an internal variable 

		// Destructor
		~Variable() { if(_pi) delete(_pi);	}	// NOTE: deleting variables creates dangling pointers
										// Only delete a variable when deleting its quantifier!

		// Mutators
		void	sort(Sort* s)	{ _sort = s; }

		// Inspectors
		string		name()		const { return _name;	}
		Sort*		sort()		const { return _sort;	}
		ParseInfo*	pi()		const { return _pi;		}

		// Debugging
		string	to_string()		const;

};

namespace VarUtils {

	/*
	 * Sort and remove doubles from a given vector of variables
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
		ParseInfo*			_pi;	// the place where the symbol was declared (0 for non user-defined symbols)

	public:

		// Constructors
		PFSymbol(const string& name, const vector<Sort*>& sorts, ParseInfo* pi) : 
			_name(name), _sorts(sorts), _pi(pi) { }

		// Destructor
		virtual ~PFSymbol() { if(_pi) delete(_pi);	}	// NOTE: deleting a PFSymbol creates dangling pointers
														// Only delete a PFSymbol if the global namespace is deleted

		// Inspectors
		string					name()					const { return _name;							}
		ParseInfo*				pi()					const { return _pi;								}
		unsigned int			nrsorts()				const { return _sorts.size();					}
		Sort*					sort(unsigned int n)	const { return _sorts[n];						}
		const vector<Sort*>&	sorts()					const { return _sorts;							}
		string					to_string()				const { return _name.substr(0,_name.find('/'));	}
		virtual bool			ispred()				const = 0;	// true iff the symbol is a predicate

		// Built-in symbols 
		virtual bool			builtin()			const				{ return false;	}	// true iff the symbol is built-in
		virtual bool			overloaded()		const				{ return false;	}	// true iff the symbol is overloaded
		virtual PFSymbol*		disambiguate(const vector<Sort*>&)		{ return this;	}

};


/** Predicates **/

class Predicate : public PFSymbol {

	private:

	public:

		// Constructors
		Predicate(const string& name,const vector<Sort*>& sorts, ParseInfo* pi) :
			PFSymbol(name,sorts,pi) { }

		// Inspectors
		unsigned int	arity()		const { return _sorts.size();	}
		bool			ispred()	const { return true;			}

		// Built-in symbols 
		virtual Predicate*		disambiguate(const vector<Sort*>&)		{ return this;	}

		// Debugging of GidL
		void	inspect()	const;

};


/** Functions **/

class Function : public PFSymbol {

	private:
		bool	_partial;	// true iff the function is declared as partial function

	public:

		// Constructors
		Function(const string& name, const vector<Sort*>& is, Sort* os, ParseInfo* pi) : 
			PFSymbol(name,is,pi), _partial(false) { _sorts.push_back(os); }
		Function(const string& name, const vector<Sort*>& sorts, ParseInfo* pi) : 
			PFSymbol(name,sorts,pi), _partial(false) { }

		// Mutators
		void		partial(const bool& b)	{ _partial = b;		}
	
		// Inspectors
		vector<Sort*>		insorts()				const;	// returns the input sorts of the function
		unsigned int		arity()					const { return _sorts.size()-1;	}
		Sort*				insort(const int& n)	const { return _sorts[n];		}
		Sort*				outsort()				const { return _sorts.back();	}
		bool				partial()				const { return _partial;		}
		bool				ispred()				const { return false;			}

		// Built-in symbols 
		virtual Function*		disambiguate(const vector<Sort*>&)		{ return this;	}

		// Debugging of GidL
		void	inspect()	const;

};


/*****************
	Vocabulary
*****************/

class Vocabulary {

	private:

		string		_name;	// name of the vocabulary. Default name is the empty string
		ParseInfo*	_pi;	// place where the vocabulary was parsed

		// map symbols of the vocabulary to an index
		map<Sort*,unsigned int>			_sorts;		
		map<Predicate*,unsigned int>	_predicates;
		map<Function*,unsigned int>		_functions;
		// map the index of a symbol to the symbol
		vector<Sort*>					_vsorts;
		vector<Predicate*>				_vpredicates;
		vector<Function*>				_vfunctions;
		// map name of a symbol to the symbol
		map<string,Sort*>		_sortnames;
		map<string,Predicate*>	_prednames;
		map<string,Function*>	_funcnames;

	public:

		// Constructors
		Vocabulary(const string& name, ParseInfo* pi) : _name(name), _pi(pi) { }

		// Destructor
		~Vocabulary() { if(_pi) delete(_pi);	}

		// Mutators
		void addSort(Sort*);
		void addPred(Predicate*);
		void addFunc(Function*);
		void addSort(const string&, Sort*);
		void addPred(const string&, Predicate*);
		void addFunc(const string&, Function*);

		// Inspectors
		const string&		name()							const { return _name;					}
		ParseInfo*			pi()							const { return _pi;						}
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

#endif
