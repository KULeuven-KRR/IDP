/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef INTERNALLUAARGUMENT_HPP_
#define INTERNALLUAARGUMENT_HPP_

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
#include "luaconnection.hpp"
#include "internalargument.hpp"

const char* getTypeField();
int& argProcNumber();

namespace LuaConnection {

int convertToLua(lua_State*,InternalArgument);

InternalArgument createArgument(int arg, lua_State* L);

class InternalProcedure {
	private:
		std::string				_name;		//!< the name of the procedure
		std::vector<ArgType>	_argtypes;	//!< types of the input arguments
		InternalArgument		(*_execute)(const std::vector<InternalArgument>&, lua_State*);

	protected:
		void add(ArgType type) { _argtypes.push_back(type); }

	public:
		InternalProcedure(const std::string& name): _name(name){ }
		int operator()(lua_State* L) const {
			std::vector<InternalArgument> args;
			for(int arg = 1; arg <= lua_gettop(L); ++arg) {
				args.push_back(createArgument(arg,L));
			}
			InternalArgument result = execute(args,L);
			return LuaConnection::convertToLua(L,result);
		}
		const std::string& name() const { return _name;	}
		virtual InternalArgument execute(const std::vector<InternalArgument>&, lua_State* ) const = 0;
};

} // LUA CONNECTION

#endif /* INTERNALLUAARGUMENT_HPP_ */
