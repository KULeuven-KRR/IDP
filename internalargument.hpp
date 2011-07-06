/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INTERNALARGUMENT_HPP_
#define INTERNALARGUMENT_HPP_

#include <set>
#include <iostream>
#include <cstdlib>
#include "lua.hpp"
#include "common.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "options.hpp"
#include "namespace.hpp"

/**
 * Types of arguments given to, or results produced by internal procedures
 */
enum ArgType {
	// Vocabulary
	AT_SORT,			//!< a sort
	AT_PREDICATE,		//!< a predicate symbol
	AT_FUNCTION,		//!< a function symbol
	AT_SYMBOL,			//!< a symbol of a vocabulary
	AT_VOCABULARY,		//!< a vocabulary

	// Structure
	AT_COMPOUND,		//!< a compound domain element
	AT_TUPLE,			//!< a tuple in a predicate or function table
	AT_DOMAIN,			//!< a domain of a sort
	AT_PREDTABLE,		//!< a predicate table
	AT_PREDINTER,		//!< a predicate interpretation
	AT_FUNCINTER,		//!< a function interpretation
	AT_STRUCTURE,		//!< a structure
	AT_TABLEITERATOR,	//!< a predicate table iterator
	AT_DOMAINITERATOR,	//!< a domain iterator
	AT_DOMAINATOM,		//!< a domain atom

	// Theory
	AT_QUERY,			//!< a query
	AT_FORMULA,			//!< a formula
	AT_THEORY,			//!< a theory

	// Options
	AT_OPTIONS,			//!< an options block

	// Namespace
	AT_NAMESPACE,		//!< a namespace

	// Lua types
	AT_NIL,				//!< a nil value
	AT_INT,				//!< an integer
	AT_DOUBLE,			//!< a double
	AT_BOOLEAN,			//!< a boolean
	AT_STRING,			//!< a string
	AT_TABLE,			//!< a table
	AT_PROCEDURE,		//!< a procedure

	AT_OVERLOADED,		//!< an overloaded object

	// Additional return values
	AT_MULT,			//!< multiple arguments	(only used as return value)
	AT_REGISTRY			//!< a value stored in the registry of the lua state
};

template<class T>
struct map_init_helper{
	T& container_;

	map_init_helper(T& container): container_(container){}

	map_init_helper& operator()(const typename T::key_type& key, const char* value){
		container_.insert(std::pair<typename T::key_type, const char*>(key, value));
		return *this;
	}
};

template<typename T> map_init_helper<T> map_init(T& container){
	return map_init_helper<T>(container);
}

const char* toCString(ArgType type);

/**
 * Objects to overload sorts, predicate, and function symbols
 */
class OverloadedSymbol {
private:
	std::set<Sort*>			_sorts;
	std::set<Function*>		_funcs;
	std::set<Predicate*>	_preds;
public:
	void insert(Sort* s)		{ _sorts.insert(s);	}
	void insert(Function* f)	{ _funcs.insert(f);	}
	void insert(Predicate* p)	{ _preds.insert(p);	}

	std::set<Sort*>*	sorts()		{ return &_sorts;	}
	std::set<Predicate*>* preds()	{ return &_preds;	}
	std::set<Function*>* funcs()	{ return &_funcs;	}

	std::vector<ArgType>	types() {
		std::vector<ArgType> result;
		if(!_sorts.empty()) result.push_back(AT_SORT);
		if(!_preds.empty()) result.push_back(AT_PREDICATE);
		if(!_funcs.empty()) result.push_back(AT_FUNCTION);
		return result;
	}
};

/**
 * Objects to overload members of namespaces and vocabularies
 */
class OverloadedObject {
private:
	Namespace*			_namespace;
	Vocabulary*			_vocabulary;
	AbstractTheory*		_theory;
	AbstractStructure*	_structure;
	Options*			_options;
	UserProcedure*		_procedure;
	Formula*			_formula;
	Query*				_query;

	std::set<Predicate*>	_predicate;
	std::set<Function*>		_function;
	std::set<Sort*>			_sort;

public:
	// Constructor
	OverloadedObject() :
		_namespace(NULL), _vocabulary(NULL), _theory(NULL),
		_structure(NULL), _options(NULL), _procedure(NULL),
		_formula(NULL), _query(NULL) { }

	void insert(Namespace* n)			{ _namespace = n;	}
	void insert(Vocabulary* n)			{ _vocabulary = n;	}
	void insert(AbstractTheory* n)		{ _theory = n;		}
	void insert(AbstractStructure* n)	{ _structure = n;	}
	void insert(Options* n)				{ _options = n;		}
	void insert(UserProcedure* n)		{ _procedure = n;	}
	void insert(Formula* f)				{ _formula = f;		}
	void insert(Query* q)				{ _query = q;		}

	AbstractStructure*	structure()		const { return _structure;	}
	AbstractTheory*		theory()		const { return _theory;		}
	Options*			options()		const { return _options;	}
	Namespace*			space()			const { return _namespace;	}
	Vocabulary*			vocabulary()	const { return _vocabulary;	}
	Formula*			formula()		const { return _formula;	}
	Query*				query()			const { return _query;		}

	std::vector<ArgType> types() {
		std::vector<ArgType> result;
		if(_namespace) result.push_back(AT_NAMESPACE);
		if(_vocabulary) result.push_back(AT_VOCABULARY);
		if(_theory) result.push_back(AT_THEORY);
		if(_structure) result.push_back(AT_STRUCTURE);
		if(_options) result.push_back(AT_OPTIONS);
		if(_procedure) result.push_back(AT_PROCEDURE);
		if(!_predicate.empty()) result.push_back(AT_PREDICATE);
		if(!_function.empty()) result.push_back(AT_FUNCTION);
		if(!_sort.empty()) result.push_back(AT_SORT);
		if(_formula) result.push_back(AT_FORMULA);
		if(_query) result.push_back(AT_QUERY);
		return result;
	}
};

struct InternalArgument {
	ArgType		_type;
	union {
		std::set<Sort*>*				_sort;
		std::set<Predicate*>*			_predicate;
		std::set<Function*>*			_function;
		Vocabulary*						_vocabulary;

		const Compound*					_compound;
		const DomainAtom*				_domainatom;
		ElementTuple*					_tuple;
		SortTable*						_domain;
		const PredTable*				_predtable;
		PredInter*						_predinter;
		FuncInter*						_funcinter;
		AbstractStructure*				_structure;
		TableIterator*					_tableiterator;
		SortIterator*					_sortiterator;

		AbstractTheory*					_theory;
		Formula*						_formula;
		Query*							_query;
		Options*						_options;
		Namespace*						_namespace;

		int								_int;
		double							_double;
		bool							_boolean;
		std::string*					_string;
		std::vector<InternalArgument>*	_table;

		OverloadedSymbol*				_symbol;
		OverloadedObject*				_overloaded;
	} _value;

	// Constructors
	InternalArgument() { }
	//InternalArgument(const InternalArgument& orig):_type(orig._type), _value(orig._value) { }
	InternalArgument(Vocabulary* v)						: _type(AT_VOCABULARY)	{ _value._vocabulary = v;	}
	InternalArgument(PredInter* p)						: _type(AT_PREDINTER)	{ _value._predinter = p;	}
	InternalArgument(FuncInter* f)						: _type(AT_FUNCINTER)	{ _value._funcinter = f;	}
	InternalArgument(AbstractStructure* s)				: _type(AT_STRUCTURE)	{ _value._structure = s;	}
	InternalArgument(AbstractTheory* t)					: _type(AT_THEORY)		{ _value._theory = t;		}
	InternalArgument(Options* o)						: _type(AT_OPTIONS)		{ _value._options = o;		}
	InternalArgument(Namespace* n)						: _type(AT_NAMESPACE)	{ _value._namespace = n;	}
	InternalArgument(int i)								: _type(AT_INT)			{ _value._int = i;			}
	InternalArgument(double d)							: _type(AT_DOUBLE)		{ _value._double = d;		}
	InternalArgument(std::string* s)					: _type(AT_STRING)		{ _value._string = s;		}
	InternalArgument(std::vector<InternalArgument>* t)	: _type(AT_TABLE)		{ _value._table = t;		}
	InternalArgument(std::set<Sort*>* s)				: _type(AT_SORT)		{ _value._sort = s;			}
	InternalArgument(std::set<Predicate*>* p)			: _type(AT_PREDICATE)	{ _value._predicate = p;	}
	InternalArgument(std::set<Function*>* f)			: _type(AT_FUNCTION)	{ _value._function = f;		}
	InternalArgument(OverloadedSymbol* s)				: _type(AT_SYMBOL)		{ _value._symbol = s;		}
	InternalArgument(const PredTable* t)				: _type(AT_PREDTABLE)	{ _value._predtable = t;	}
	InternalArgument(SortTable* t)						: _type(AT_DOMAIN)		{ _value._domain = t;		}
	InternalArgument(const Compound* c)					: _type(AT_COMPOUND)	{ _value._compound = c;		}
	InternalArgument(const DomainAtom* a)				: _type(AT_DOMAINATOM)	{ _value._domainatom = a;	}
	InternalArgument(OverloadedObject* o)				: _type(AT_OVERLOADED)	{ _value._overloaded = o;	}
	InternalArgument(Formula* f)						: _type(AT_FORMULA)		{ _value._formula = f;		}
	InternalArgument(Query* q)							: _type(AT_QUERY)		{ _value._query = q;		}
	InternalArgument(const DomainElement*);

	// Inspectors
	std::set<Sort*>*	sort() const {
		if(_type == AT_SORT) { return _value._sort; }
		if(_type == AT_SYMBOL) { return _value._symbol->sorts(); }
		return 0;
	}

	AbstractTheory* theory() const {
		if(_type == AT_THEORY){ return _value._theory; }
		if(_type == AT_OVERLOADED){ return _value._overloaded->theory(); }
		return 0;
	}

	AbstractStructure* structure() const {
		if(_type == AT_STRUCTURE){ return _value._structure; }
		if(_type == AT_OVERLOADED){ return _value._overloaded->structure(); }
		return 0;
	}

	Options* options() const {
		if(_type == AT_OPTIONS){ return _value._options; }
		if(_type == AT_OVERLOADED){ return _value._overloaded->options(); }
		return 0;
	}

	Namespace* space() const {
		if(_type == AT_NAMESPACE){ return _value._namespace; }
		if(_type == AT_OVERLOADED){ return _value._overloaded->space(); }
		return 0;
	}

	Vocabulary* vocabulary() const {
		if(_type == AT_VOCABULARY){ return _value._vocabulary; }
		if(_type == AT_OVERLOADED){ return _value._overloaded->vocabulary(); }
		return 0;
	}
};

InternalArgument nilarg();

/**
 * Class to represent user-defined procedures
 */
class UserProcedure {
	protected:
		std::string					_name;				//!< name of the procedure
		ParseInfo					_pi;				//!< place where the procedure was parsed
		std::vector<std::string>	_argnames;			//!< names of the arguments of the procedure
		std::stringstream			_code;				//!< body of the procedure. Empty string if the procedure is not
														//!< yet compiled.
		std::string					_registryindex;		//!< place where the compiled version of the
														//!< procedure is stored in the registry of the lua state
		static int					_compilenumber;		//!< used to create unique registryindexes
		std::string					_description;

	public:
		// Constructors
		UserProcedure(const std::string& name, const ParseInfo& pi, std::stringstream* des) :
				_name(name), _pi(pi), _registryindex(""), _description(des ? des->str() : "(undocumented)\n") { }

		void addarg(const std::string& name)	{ _argnames.push_back(name);	}	//!< add an argument to the procedure

		template<typename Printable> void add(const Printable& s) { _code << s;	}

		// Inspectors
		const ParseInfo&	pi()			const { return _pi;						}
		const std::string&	name()			const { return _name;					}
		unsigned int		arity()			const { return _argnames.size();		}
		bool				iscompiled()	const { return _registryindex != "";	}
		const std::string&	registryindex()	const { return _registryindex;			}
		const std::vector<std::string>&	args()	const { return _argnames;			}
		const std::string&	description()	const { return _description;			}
};

#endif /* INTERNALARGUMENT_HPP_ */
