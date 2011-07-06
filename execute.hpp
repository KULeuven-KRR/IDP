/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EXECUTE_HPP
#define EXECUTE_HPP

#include <string>
#include <vector>
#include <sstream>
#include "parseinfo.hpp"

class UserProcedure;
class lua_State;

namespace LuaConnection{
	void compile(UserProcedure* procedure, lua_State* state);
}

#endif
