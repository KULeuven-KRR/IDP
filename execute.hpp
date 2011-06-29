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
			_name(name), _pi(pi), _registryindex(""), _description(des ? des->str() : "(undocumented)\n") { }

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
class Query;

namespace LuaConnection {
	void makeLuaConnection();
	void closeLuaConnection();
	int convertToLua(lua_State*,InternalArgument);

	void addGlobal(Vocabulary*);
	void addGlobal(AbstractStructure*);
	void addGlobal(AbstractTheory*);
	void addGlobal(Options*);
	void addGlobal(UserProcedure*);
	void addGlobal(Namespace*);
	void addGlobal(const std::string& name, Formula*);
	void addGlobal(const std::string& name, Query*);

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
