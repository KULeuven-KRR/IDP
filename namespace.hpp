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
class Vocabulary;
class AbstractStructure;
class AbstractTheory;
class Options;
class UserProcedure;

#include "parseinfo.hpp"

/*****************
	Namespaces
*****************/

/**
 * Class to represent namespaces
 */
class Namespace {
	private:
		std::string									_name;			//!< The name of the namespace
		Namespace*									_superspace;	//!< The parent in the namespace hierarchy
																	//!< Null-pointer if top of the hierarchy
		std::map<std::string,Namespace*>			_subspaces;		//!< Map a name to the corresponding subspace
		std::map<std::string,Vocabulary*>			_vocabularies;	//!< Map a name to the corresponding vocabulary
		std::map<std::string,AbstractStructure*>	_structures;	//!< Map a name to the corresponding structure
		std::map<std::string,AbstractTheory*>		_theories;		//!< Map a name to the corresponding theory
		std::map<std::string,Options*>				_options;		//!< Map a name to the corresponding options
		std::map<std::string,UserProcedure*>		_procedures;	//!< Map a name+arity to the corresponding procedure

		ParseInfo									_pi;			//!< the place where the namespace was parsed

		static Namespace*							_global;		//!< The global namespace

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
		bool						isSubspace(const std::string&)	const;
		bool						isVocab(const std::string&)		const;
		bool						isTheory(const std::string&)	const;
		bool						isStructure(const std::string&)	const;
		bool						isOptions(const std::string&)	const;
		bool						isProc(const std::string&)		const;
		Namespace*					subspace(const std::string&)	const;
		Vocabulary*					vocabulary(const std::string&)	const;
		AbstractTheory*				theory(const std::string&)		const;
		AbstractStructure*			structure(const std::string&)	const;
		Options*					options(const std::string&)		const;
		UserProcedure*				procedure(const std::string&)	const;

		const std::map<std::string,UserProcedure*>&	procedures()	const { return _procedures;	}
		const std::map<std::string,Namespace*>&		subspaces()		const { return _subspaces;	}

		// Mutators	
		void	add(Vocabulary* v);
		void	add(Namespace* n);
		void	add(AbstractStructure* s);
		void	add(AbstractTheory* t);
		void	add(Options* o);
		void	add(UserProcedure* l);

		// Output
		std::ostream& putname(std::ostream&) const;
};

#endif
