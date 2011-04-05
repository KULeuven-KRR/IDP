/************************************
	execute.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EXECUTE_HPP
#define EXECUTE_HPP

#include "namespace.hpp"
#include <sstream>

/*******************************************
	Argument types for inference methods
*******************************************/

enum InfArgType { 
	IAT_THEORY, 
	IAT_STRUCTURE, 
	IAT_VOCABULARY, 
	IAT_NAMESPACE, 
	IAT_OPTIONS, 
	IAT_NIL, 
	IAT_INT, 
	IAT_DOUBLE,
	IAT_BOOLEAN, 
	IAT_STRING, 
	IAT_TABLE, 
	IAT_PROCEDURE, 
	IAT_OVERLOADED, 
	IAT_SORT,
	IAT_PREDICATE, 
	IAT_FUNCTION,
	IAT_MULT,
	IAT_REGISTRY
};

namespace BuiltinProcs {
	void initialize();
	void cleanup();
}

/** Lua procedures **/

class LuaProcedure {
	protected:
		string				_name;		// name without arity
		ParseInfo			_pi;
		vector<string>		_innames;
		stringstream		_code;
		string				_registryindex;
	public:
		LuaProcedure() { }
		LuaProcedure(const string& name, const ParseInfo& pi) :
			_name(name), _pi(pi), _innames(0), _registryindex("") { }

		// Mutators
		void addarg(const string& name) { _innames.push_back(name);	}
		void add(char* s)				{ _code << s;				}
		void add(const string& s)		{ _code << s;				}
		void compile(lua_State*);
		
		// Inspectors
		const ParseInfo&	pi()			const { return _pi;						}
		const string&		name()			const { return _name;					}
		unsigned int		arity()			const { return _innames.size();			}
		virtual string		code()			const { return _code.str();				}
		bool				iscompiled()	const { return _registryindex != "";	}
		const string&		registryindex()	const { return _registryindex;			}

};

/** class OverloadedObject **/
struct TypedInfArg;
class OverloadedObject {
	private:

		Namespace*			_namespace;
		Vocabulary*			_vocabulary;
		AbstractTheory*		_theory;
		AbstractStructure*	_structure;
		InfOptions*			_options;
		const string*		_procedure;

		Predicate*			_predicate;
		Function*			_function;
		Sort*				_sort;

		string*				_string;
		bool*				_bool;
		double*				_double;
		int*				_int;
		
		vector<TypedInfArg>*	_table;

	public:

		// Constructor
		OverloadedObject() : 
			_namespace(0), _vocabulary(0), _theory(0), _structure(0), _options(0), _procedure(0),
			_predicate(0), _function(0), _sort(0), _string(0), _bool(0), _double(0), _int(0), _table(0) { }

		// Mutators
		void	makenamespace(Namespace* n)			{ _namespace = n;				}
		void	makevocabulary(Vocabulary* v)		{ _vocabulary = v;				}
		void	maketheory(AbstractTheory* t)		{ _theory = t;					}
		void	makestructure(AbstractStructure* s) { _structure = s;				}
		void	makeoptions(InfOptions* o)			{ _options = o;					}
		void	makeprocedure(const string* p)		{ _procedure = p;				}
		void	makepredicate(Predicate* p)			{ _predicate = p;				}
		void	makefunction(Function* f)			{ _function = f;				}
		void	makesort(Sort* s)					{ _sort = s;					}
		void	makestring(string* s)				{ _string = s;					}
		void	makebool(bool b)					{ _bool = new bool(b);			}
		void	makedouble(double d)				{ _double = new double(d);		}
		void	makeint(int i)						{ _int = new int(i);			}
		void	maketable(vector<TypedInfArg>* v)	{ _table = v;					}

		// Inspectors
		bool	isNamespace()	const { return (_namespace != 0);		}
		bool	isVocabulary()	const { return (_vocabulary != 0);		}
		bool	isTheory()		const { return (_theory != 0);			}
		bool	isStructure()	const { return (_structure != 0);		}
		bool	isOptions()		const { return (_options != 0);			}
		bool	isProcedure()	const { return (_procedure != 0);		}
		bool	isPredicate()	const { return (_predicate != 0);		}
		bool	isFunction()	const { return (_function != 0);		}
		bool	isSort()		const { return (_sort != 0);			}
		bool	isString()		const { return (_string != 0);			}
		bool	isBool()		const { return (_bool != 0);			}
		bool	isDouble()		const { return (_double != 0);			}
		bool	isInt()			const { return (_int != 0);				}
		bool	isTable()		const { return (_table != 0);			}

		bool	single()	const;

		Namespace*					getNamespace()		const	{ return _namespace;	}
		Vocabulary*					getVocabulary()		const	{ return _vocabulary;	}
		AbstractTheory*				getTheory()			const	{ return _theory;		}
		AbstractStructure*			getStructure()		const	{ return _structure;	}
		InfOptions*					getOptions()		const	{ return _options;		}
		const string*				getProcedure()		const	{ return _procedure;	}
		Predicate*					getPredicate()		const	{ return _predicate;	}
		Function*					getFunction()		const	{ return _function;		}
		Sort*						getSort()			const	{ return _sort;			}
		string*						getString()			const	{ return _string;		}
		int							getInt()			const	{ return *_int;			}
		bool						getBool()			const	{ return *_bool;		}
		double						getDouble()			const	{ return *_double;		}
		vector<TypedInfArg>*		getTable()			const	{ return _table;		}

};


/** Possible argument or return value of an execute statement **/
struct TypedInfArg;

union InfArg {
	Vocabulary*					_vocabulary;
	AbstractStructure*			_structure;
	AbstractTheory*				_theory;
	Namespace*					_namespace;
	double						_double;
	int							_int;
	bool						_boolean;
	string*						_string;
	InfOptions*					_options;
	const string*				_procedure;		// contains the registry index of a procedure
	OverloadedObject*			_overloaded;
	Predicate*					_predicate;
	Function*					_function;
	Sort*						_sort;
	vector<TypedInfArg>*		_table;
};

struct TypedInfArg {
	InfArg		_value;
	InfArgType	_type;
};



/** An execute statement **/
class Inference {
	protected:
		vector<InfArgType>	_intypes;		// types of the input arguments
		string				_description;	// description of the inference
	public:
		virtual ~Inference() { }
		virtual TypedInfArg execute(const vector<InfArg>& args, lua_State*) const = 0;	// execute the statement
		const vector<InfArgType>&	intypes()		const { return _intypes;		}
		unsigned int				arity()			const { return _intypes.size();	}
		string						description()	const { return _description;	}
};

class LoadFile : public Inference {
	public:
		LoadFile() { _intypes = vector<InfArgType>(1,IAT_STRING);	
					 _description = "Load the given file";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class NewOption : public Inference {
	public:
		NewOption() {	
			_intypes = vector<InfArgType>(0);
			_description = "Create a option block";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class PrintTheory : public Inference {
	public:
		PrintTheory() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the theory";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class PrintVocabulary : public Inference {
	public:
		PrintVocabulary() { 
			_intypes = vector<InfArgType>(1,IAT_VOCABULARY); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the vocabulary";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class PrintStructure : public Inference {
	public:
		PrintStructure() { 
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the structure";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class PrintNamespace : public Inference {
	public:
		PrintNamespace() { 
			_intypes = vector<InfArgType>(1,IAT_NAMESPACE); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the namespace";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class PushNegations : public Inference {
	public:
		PushNegations() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_description = "Push all negations inside until they are in front of atoms";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class RemoveEquivalences : public Inference {
	public:
		RemoveEquivalences() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_description = "Rewrite equivalences into pairs of implications";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class RemoveEqchains : public Inference {
	public:
		RemoveEqchains() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_description = "Rewrite chains of (in)equalities to conjunctions of atoms";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;

};

class FlattenFormulas : public Inference {
	public:
		FlattenFormulas() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_description = "Rewrite ((A & B) & C) to (A & B & C), rewrite (! x : ! y : phi(x,y)) to (! x y : phi(x,y)), etc.";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class FastGrounding : public Inference {
	public:
		FastGrounding();
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class FastMXInference : public Inference {
	public:
		FastMXInference();
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class StructToTheory : public Inference {
	public:
		StructToTheory() {
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE); 
			_description = "Rewrite a structure to a theory";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class MoveQuantifiers : public Inference {
	public:
		MoveQuantifiers() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_description = "Move universal (existential) quantifiers inside conjunctions (disjunctions)";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class ApplyTseitin : public Inference {
	public:
		ApplyTseitin() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_description = "Apply the tseitin transformation to a theory";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class GroundReduce : public Inference {
	public:
		GroundReduce();
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class MoveFunctions : public Inference {
	public:
		MoveFunctions() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_description = "Move functions until no functions are nested";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class CloneStructure : public Inference {
	public:
		CloneStructure() { 
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE);
			_description = "Make a copy of this structure";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class CloneTheory : public Inference {
	public:
		CloneTheory() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_description = "Make a copy of this theory";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class ForceTwoValued : public Inference {
	public:
		ForceTwoValued() {
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE);
			_description = "Force structure to be two-valued";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class DeleteData : public Inference {
	public:
		DeleteData(InfArgType t) {
			_intypes = vector<InfArgType>(1,t);
			_description = "Delete"; 
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class ChangeVoc : public Inference {
	public:
		ChangeVoc();
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class GetIndex : public Inference {
	public:
		GetIndex(InfArgType table, InfArgType index) {
			_intypes = vector<InfArgType>(2);
			_intypes[0] = table; _intypes[1] = index;
			_description = "index function"; 
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class SetIndex : public Inference {
	public:
		SetIndex(InfArgType table, InfArgType key, InfArgType value);
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

class BDDPrinter : public Inference {
	public:
		BDDPrinter() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_description = "Show the given theory as a bdd";
		}
		TypedInfArg execute(const vector<InfArg>& args, lua_State*) const;
};

#endif
