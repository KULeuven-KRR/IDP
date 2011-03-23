/************************************
	namespace.hpp	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef NAMESPACE_HPP
#define NAMESPACE_HPP

#include <string>
#include <vector>
#include <map>
#include <set>

#include "parseinfo.hpp"
#include "lua.hpp"

class Vocabulary;
class AbstractStructure;
class AbstractTheory;
class Sort;
class Predicate;
class Function;
class LuaProcedure;
class OverloadedObject;
class InfOptions;
struct TypedInfArg;

/*****************
	Namespaces
*****************/

class Namespace {
	private:
		std::string									_name;			// The name of the namespace
		Namespace*									_superspace;	// The parent of the namespace in the namespace hierarchy
																	// Null-pointer if top of the hierarchy
		std::map<std::string,Namespace*>			_subspaces;		// Map a name to the corresponding subspace
		std::map<std::string,Vocabulary*>			_vocabularies;	// Map a name to the corresponding vocabulary
		std::map<std::string,AbstractStructure*>	_structures;	// Map a name to the corresponding structure
		std::map<std::string,AbstractTheory*>		_theories;		// Map a name to the corresponding theory
		std::map<std::string,InfOptions*>			_options;		// Map a name to the corresponding options
		std::map<std::string,LuaProcedure*>			_procedures;	// Map a name+arity to the corresponding procedure
		std::vector<Namespace*>						_subs;			// The children of the namespace in the namespace hierarchy
		std::vector<Vocabulary*>					_vocs;			// The vocabularies in the namespace
		std::vector<AbstractStructure*>				_structs;		// The structures in the namespace                   
		std::vector<AbstractTheory*>				_theos;			// The theories in the namespace                           
		std::vector<InfOptions*>					_opts;	
		std::vector<LuaProcedure*>					_procs;
		ParseInfo									_pi;			// the place where the namespace was parsed

		static Namespace*							_global;		// The global namespace

		std::set<std::string> doubleNames() const;

	public:
		// Constructors
		Namespace(std::string name, Namespace* super, const ParseInfo& pi) : _name(name), _superspace(super), _pi(pi)
			{ if(super) super->add(this); }

		// Destructor
		~Namespace();

		// Inspectors
		static Namespace*	global();	

		const std::string&			name()							const	{ return _name;				}
		Namespace*					super()							const	{ return _superspace;		}
		const ParseInfo&			pi()							const	{ return _pi;				}
		bool						isGlobal()						const	{ return this == _global;	}
		std::string					fullname()						const;	// return the full name of the namespace 
		std::vector<std::string>	fullnamevector()				const;
		bool						isSubspace(const std::string&)	const;
		bool						isVocab(const std::string&)		const;
		bool						isTheory(const std::string&)	const;
		bool						isStructure(const std::string&)	const;
		bool						isOption(const std::string&)	const;
		bool						isProc(const std::string&)		const;
		Namespace*					subspace(const std::string&)	const;
		Vocabulary*					vocabulary(const std::string&)	const;
		AbstractTheory*				theory(const std::string&)		const;
		AbstractStructure*			structure(const std::string&)	const;
		InfOptions*					option(const std::string&)		const;
		LuaProcedure*				procedure(const std::string&)	const;
		unsigned int				nrSubs()						const { return _subs.size();	}
		unsigned int				nrVocs()						const { return _vocs.size();	}
		unsigned int				nrStructs()						const { return _structs.size();	}
		unsigned int				nrTheos()						const { return _theos.size();	}
		unsigned int				nrOpts()						const { return _opts.size();	}
		unsigned int				nrProcs()						const { return _procs.size();	}
		Namespace*					subspace(unsigned int n)		const { return _subs[n];		}
		Vocabulary*					vocabulary(unsigned int n)		const { return _vocs[n];		}
		AbstractTheory*				theory(unsigned int n)			const { return _theos[n];		}
		AbstractStructure*			structure(unsigned int n)		const { return _structs[n];		}
		InfOptions*					options(unsigned int n)			const { return _opts[n];		}
		LuaProcedure*				procedure(unsigned int n)		const { return _procs[n];		}
		std::set<Sort*>				allSorts()						const;
		std::set<Predicate*>		allPreds()						const;
		std::set<Function*>			allFuncs()						const;

		// Mutators	
		void	add(Vocabulary* v);
		void	add(Namespace* n);
		void	add(AbstractStructure* s);
		void	add(AbstractTheory* t);
		void	add(InfOptions* o);
		void	add(LuaProcedure* l);

		// Lua communication
		void		toLuaGlobal(lua_State*);
		TypedInfArg	getObject(const std::string& str, lua_State*) const;

		// Visitors
		void	accept(Visitor*) const;
};

#endif
