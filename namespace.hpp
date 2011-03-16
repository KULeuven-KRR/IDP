/************************************
	namespace.hpp	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef NAMESPACE_HPP
#define NAMESPACE_HPP

#include "theory.hpp"
#include "options.hpp"
#include "lua.hpp"
#include <set>

class LuaProcedure;

/*****************
	Namespaces
*****************/

class Namespace {

	private:
		string							_name;			// The name of the namespace
		Namespace*						_superspace;	// The parent of the namespace in the namespace hierarchy
														// Null-pointer if top of the hierarchy
		map<string,Namespace*>			_subspaces;		// Map a name to the corresponding subspace
		map<string,Vocabulary*>			_vocabularies;	// Map a name to the corresponding vocabulary
		map<string,AbstractStructure*>	_structures;	// Map a name to the corresponding structure
		map<string,AbstractTheory*>		_theories;		// Map a name to the corresponding theory
		map<string,InfOptions*>			_options;		// Map a name to the corresponding options
		map<string,LuaProcedure*>		_procedures;	// Map a name+arity to the corresponding procedure
		vector<Namespace*>				_subs;			// The children of the namespace in the namespace hierarchy
		vector<Vocabulary*>				_vocs;			// The vocabularies in the namespace
		vector<AbstractStructure*>		_structs;		// The structures in the namespace                   
		vector<AbstractTheory*>			_theos;			// The theories in the namespace                           
		ParseInfo						_pi;			// the place where the namespace was parsed

		static Namespace*				_global;		// The global namespace

	public:

		// Constructors
		Namespace(string name, Namespace* super, const ParseInfo& pi) : _name(name), _superspace(super), _pi(pi)
			{ if(super) super->add(this); }

		// Destructor
		~Namespace();

		// Inspectors
		static Namespace*	global();	

		const string&		name()						const	{ return _name;				}
		Namespace*			super()						const	{ return _superspace;		}
		const ParseInfo&	pi()						const	{ return _pi;				}
		bool				isGlobal()					const	{ return this == _global;	}
		string				fullname()					const;	// return the full name of the namespace 
		vector<string>		fullnamevector()			const;
		bool				isSubspace(const string&)	const;
		bool				isVocab(const string&)		const;
		bool				isTheory(const string&)		const;
		bool				isStructure(const string&)	const;
		bool				isOption(const string&)		const;
		bool				isProc(const string&)		const;
		Namespace*			subspace(const string&)		const;
		Vocabulary*			vocabulary(const string&)	const;
		AbstractTheory*		theory(const string&)		const;
		AbstractStructure*	structure(const string&)	const;
		InfOptions*			option(const string&)		const;
		LuaProcedure*		procedure(const string&)	const;
		unsigned int		nrSubs()					const { return _subs.size();	}
		unsigned int		nrVocs()					const { return _vocs.size();	}
		unsigned int		nrStructs()					const { return _structs.size();	}
		unsigned int		nrTheos()					const { return _theos.size();	}
		Namespace*			subspace(unsigned int n)	const { return _subs[n];		}
		Vocabulary*			vocabulary(unsigned int n)	const { return _vocs[n];		}
		AbstractTheory*		theory(unsigned int n)		const { return _theos[n];		}
		AbstractStructure*	structure(unsigned int n)	const { return _structs[n];		}
		set<Sort*>			allSorts()					const;
		set<Predicate*>		allPreds()					const;
		set<Function*>		allFuncs()					const;

		// Mutators	
		void	add(Vocabulary* v)			{ _vocabularies[v->name()] = v;	_vocs.push_back(v);		}
		void	add(Namespace* n)			{ _subspaces[n->name()] = n; _subs.push_back(n);		}
		void	add(AbstractStructure* s)	{ _structures[s->name()] = s; _structs.push_back(s);	}
		void	add(AbstractTheory* t)		{ _theories[t->name()] = t; _theos.push_back(t);		}
		void	add(InfOptions* o)			{ _options[o->_name] = o;								}
		void	add(LuaProcedure* l);

		// Lua communication
		int 	tolua(lua_State*,const vector<string>&) const;
		int		tolua(lua_State*) const;

		void	toLuaGlobal(lua_State*) const;
		void	toLuaLocal(lua_State*) const;

		// Visitors
		void	accept(Visitor*) const;
};


/*
	class GlobalName
	
	This class is used to communicate all userdata from the input files to lua
*/
class GlobalName {
	private:
		Namespace*					_namespace;
		Vocabulary*					_vocabulary;
		AbstractTheory*				_theory;
		AbstractStructure*			_structure;
		InfOptions*					_options;
		LuaProcedure*				_procedure;
		Predicate*					_predicate;
		Function*					_function;
		Sort*						_sort;
		
		map<string,GlobalName*>		_subnames;
	public:

		// Constructor
		GlobalName() : 
			_namespace(0), _vocabulary(0), _theory(0), _structure(0), _options(0), _procedure(0),
			_predicate(0), _function(0), _sort(0) { }

		// Mutators
		void	makenamespace(Namespace* n)			{ _namespace = n;	}
		void	makevocabulary(Vocabulary* v)		{ _vocabulary = v;	}
		void	maketheory(AbstractTheory* t)		{ _theory = t;		}
		void	makestructure(AbstractStructure* s) { _structure = s;	}
		void	makeoptions(InfOptions* o)			{ _options = o;		}
		void	makeprocedure(LuaProcedure* p)		{ _procedure = p;	}
		void	makepredicate(Predicate* p)			{ _predicate = p;	}
		void	makefunction(Function* f)			{ _function = f;	}
		void	makesort(Sort* s)					{ _sort = s;		}

		void	makesubname(const string& str, GlobalName*	g)	{ _subnames[str] = g;	}

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

		bool		isSubname(const string& str)	const { return _subnames.find(str) != _subnames.end();	}
		GlobalName*	subname(const string& str)		const;
};

#endif

