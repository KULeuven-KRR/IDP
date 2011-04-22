/************************************
	execute.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EXECUTE_HPP
#define EXECUTE_HPP

/**
 * \file execute.hpp
 *
 * This file contains the classes concerning inference methods and communication with lua
 *
 */

#include <string>
#include <vector>
#include <sstream>
#include "parseinfo.hpp"

/******************************
	User defined procedures
******************************/

class lua_State;

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
		UserProcedure(const std::string& name, const ParseInfo& pi, std::stringstream* des = 0) :
			_name(name), _pi(pi), _registryindex(""), _description(des ? des->str() : "(undocumented)") { }

		// Mutators
		void compile(lua_State*);	//!< compile the procedure

		void addarg(const std::string& name)	{ _argnames.push_back(name);	}	//!< add an argument to the procedure

		void add(char* s)				{ _code << s;	}	//!< add a c-style string to the body of the procedure
		void add(const std::string& s)	{ _code << s;	}	//!< add a string to the body of the procedure
		
		// Inspectors
		const ParseInfo&	pi()			const { return _pi;						}
		const std::string&	name()			const { return _name;					}
		unsigned int		arity()			const { return _argnames.size();		}
		bool				iscompiled()	const { return _registryindex != "";	}
		const std::string&	registryindex()	const { return _registryindex;			}
		const std::vector<std::string>&	args()	const { return _argnames;			}
		const std::string&	description()	const { return _description;		}
};

class Vocabulary;
class AbstractStructure;
class AbstractTheory;
class Options;
class Namespace;

namespace LuaConnection {
	void makeLuaConnection();
	void closeLuaConnection();

	void addGlobal(Vocabulary*);
	void addGlobal(AbstractStructure*);
	void addGlobal(AbstractTheory*);
	void addGlobal(Options*);
	void addGlobal(UserProcedure*);
	void addGlobal(Namespace*);

	Vocabulary*			vocabulary(InternalArgument*);
	AbstractTheory*		theory(InternalArgument*);
	AbstractStructure*	structure(InternalArgument*);

	InternalArgument*	call(const std::vector<std::string>&, const std::vector<std::vector<std::string> >&, const ParseInfo&);
	const DomainElement*	funccall(std::string*, const std::vector<const DomainElement*>& tuple);
	bool					predcall(std::string*, const std::vector<const DomainElement*>& tuple);

	std::string*		getProcedure(const std::vector<std::string>&, const ParseInfo&);

	void				execute(std::stringstream* chunk);
	void				compile(UserProcedure*);
}

#endif



#ifdef OLD
class OverloadedObject {
	private:
		Namespace*			_namespace;
		Vocabulary*			_vocabulary;
		AbstractTheory*		_theory;
		AbstractStructure*	_structure;
		InfOptions*			_options;
		const std::string*	_procedure;

		std::set<Predicate*>*	_predicate;
		std::set<Function*>*	_function;
		std::set<Sort*>*		_sort;

	public:
		// Constructor
		OverloadedObject() : 
			_namespace(0), _vocabulary(0), _theory(0), _structure(0), _options(0), _procedure(0),
			_predicate(0), _function(0), _sort(0) { }

		// Mutators
		void	makenamespace(Namespace* n)			{ _namespace = n;				}
		void	makevocabulary(Vocabulary* v)		{ _vocabulary = v;				}
		void	maketheory(AbstractTheory* t)		{ _theory = t;					}
		void	makestructure(AbstractStructure* s) { _structure = s;				}
		void	makeoptions(InfOptions* o)			{ _options = o;					}
		void	makeprocedure(const std::string* p)	{ _procedure = p;				}
		void	makepredicate(Predicate* p);	
		void	makefunction(Function* f);	
		void	makesort(Sort* s);		

		void	setpredicate(std::set<Predicate*>* s)	{ _predicate = s;	}
		void	setfunction(std::set<Function*>* s)		{ _function = s;	}

		// Inspectors
		bool	isNamespace()	const { return (_namespace != 0);		}
		bool	isVocabulary()	const { return (_vocabulary != 0);		}
		bool	isTheory()		const { return (_theory != 0);			}
		bool	isStructure()	const { return (_structure != 0);		}
		bool	isOptions()		const { return (_options != 0);			}
		bool	isProcedure()	const { return (_procedure != 0);		}
		bool	isPredicate()	const;
		bool	isFunction()	const;
		bool	isSort()		const;

		bool	single()		const;

		Namespace*				getNamespace()		const	{ return _namespace;	}
		Vocabulary*				getVocabulary()		const	{ return _vocabulary;	}
		AbstractTheory*			getTheory()			const	{ return _theory;		}
		AbstractStructure*		getStructure()		const	{ return _structure;	}
		InfOptions*				getOptions()		const	{ return _options;		}
		const std::string*		getProcedure()		const	{ return _procedure;	}
		std::set<Predicate*>*	getPredicate()		const	{ return _predicate;	}
		std::set<Function*>*	getFunction()		const	{ return _function;		}
		std::set<Sort*>*		getSort()			const	{ return _sort;			}
};


/** Possible argument or return value of an execute statement **/
struct TypedInfArg;

struct PredTableTuple {
	PredTable*	_table;
	int			_index;
	PredTableTuple(PredTable* table, int index) : _table(table), _index(index) { }
};


struct TypedInfArg {
	InfArg		_value;
	InfArgType	_type;
};


class LoadFile : public Inference {
	public:
		LoadFile() {
			_intypes = std::vector<InfArgType>(1,IAT_STRING);	
			_description = "Load the given file";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class NewOption : public Inference {
	public:
		NewOption() {	
			_intypes = std::vector<InfArgType>(0);
			_description = "Create a option block";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class PrintTheory : public Inference {
	public:
		PrintTheory() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the theory";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class PrintVocabulary : public Inference {
	public:
		PrintVocabulary() { 
			_intypes = std::vector<InfArgType>(1,IAT_VOCABULARY); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the vocabulary";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class PrintStructure : public Inference {
	public:
		PrintStructure() { 
			_intypes = std::vector<InfArgType>(1,IAT_STRUCTURE); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the structure";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class PrintNamespace : public Inference {
	public:
		PrintNamespace() { 
			_intypes = std::vector<InfArgType>(1,IAT_NAMESPACE); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the namespace";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class PushNegations : public Inference {
	public:
		PushNegations() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY); 
			_description = "Push all negations inside until they are in front of atoms";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class RemoveEquivalences : public Inference {
	public:
		RemoveEquivalences() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY); 
			_description = "Rewrite equivalences into pairs of implications";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class RemoveEqchains : public Inference {
	public:
		RemoveEqchains() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY); 
			_description = "Rewrite chains of (in)equalities to conjunctions of atoms";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;

};

class FlattenFormulas : public Inference {
	public:
		FlattenFormulas() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY); 
			_description = "Rewrite ((A & B) & C) to (A & B & C), rewrite (! x : ! y : phi(x,y)) to (! x y : phi(x,y)), etc.";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class FastGrounding : public Inference {
	public:
		FastGrounding();
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class FastMXInference : public Inference {
	public:
		FastMXInference();
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class StructToTheory : public Inference {
	public:
		StructToTheory() {
			_intypes = std::vector<InfArgType>(1,IAT_STRUCTURE); 
			_description = "Rewrite a structure to a theory";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class MoveQuantifiers : public Inference {
	public:
		MoveQuantifiers() {
			_intypes = std::vector<InfArgType>(1,IAT_THEORY);
			_description = "Move universal (existential) quantifiers inside conjunctions (disjunctions)";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class ApplyTseitin : public Inference {
	public:
		ApplyTseitin() {
			_intypes = std::vector<InfArgType>(1,IAT_THEORY);
			_description = "Apply the tseitin transformation to a theory";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class GroundReduce : public Inference {
	public:
		GroundReduce();
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class MoveFunctions : public Inference {
	public:
		MoveFunctions() {
			_intypes = std::vector<InfArgType>(1,IAT_THEORY);
			_description = "Move functions until no functions are nested";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class CloneStructure : public Inference {
	public:
		CloneStructure() { 
			_intypes = std::vector<InfArgType>(1,IAT_STRUCTURE);
			_description = "Make a copy of this structure";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class CloneTheory : public Inference {
	public:
		CloneTheory() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY);
			_description = "Make a copy of this theory";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class ForceTwoValued : public Inference {
	public:
		ForceTwoValued() {
			_intypes = std::vector<InfArgType>(1,IAT_STRUCTURE);
			_description = "Force structure to be two-valued";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class DeleteData : public Inference {
	public:
		DeleteData(InfArgType t) {
			_intypes = std::vector<InfArgType>(1,t);
			_description = "Delete"; 
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class ChangeVoc : public Inference {
	public:
		ChangeVoc();
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class GetIndex : public Inference {
	public:
		GetIndex(InfArgType table, InfArgType index) {
			_intypes = std::vector<InfArgType>(2);
			_intypes[0] = table; _intypes[1] = index;
			_description = "index function"; 
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class SetIndex : public Inference {
	public:
		SetIndex(InfArgType table, InfArgType key, InfArgType value);
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class LenghtOperator : public Inference {
	public:
		LenghtOperator(InfArgType t) {
			_intypes = std::vector<InfArgType>(1,t);
			_description = "# operator";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class CastOperator : public Inference {
	public:
		CastOperator() {
			_intypes = std::vector<InfArgType>(2); _intypes[0] = IAT_OVERLOADED; _intypes[1] = IAT_STRING;
			_description = "disambiguate overloaded object";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class ArityCastOperator : public Inference {
	public:
		ArityCastOperator(InfArgType t) {
			_intypes = std::vector<InfArgType>(2); _intypes[0] = t; _intypes[1] = IAT_INT;
			_description = "disambiguate overloaded object";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class BDDPrinter : public Inference {
	public:
		BDDPrinter() {
			_intypes = std::vector<InfArgType>(1,IAT_THEORY);
			_description = "Show the given theory as a bdd";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

#endif
