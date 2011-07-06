/************************************
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef LUACONNECTION_HPP_
#define LUACONNECTION_HPP_

#include "execute.hpp"
#include "lua.hpp"

class Vocabulary;
class AbstractStructure;
class AbstractTheory;
class DomainElement;
class InternalArgument;

namespace LuaConnection {
	void makeLuaConnection();
	void closeLuaConnection();
	int convertToLua(lua_State*,InternalArgument);

	lua_State* getState();

	template<typename Object>
	void addGlobal(const std::string& name, Object v){
		convertToLua(getState(),InternalArgument(v));
		lua_setglobal(getState(),name.c_str());
	}

	template<typename Object>
	void addGlobal(Object v){
		addGlobal(v->name(), v);
	}

	template<> void addGlobal(UserProcedure* p);

	Vocabulary*			vocabulary(InternalArgument*);
	AbstractTheory*		theory(InternalArgument*);
	AbstractStructure*	structure(InternalArgument*);

	InternalArgument*		call(const std::vector<std::string>&, const std::vector<std::vector<std::string> >&, const ParseInfo&);
	const DomainElement*	funccall(std::string*, const std::vector<const DomainElement*>& tuple);
	bool					predcall(std::string*, const std::vector<const DomainElement*>& tuple);

	std::string*		getProcedure(const std::vector<std::string>&, const ParseInfo&);

	void				execute(std::stringstream* chunk);
	void				compile(UserProcedure*);
}

#endif /* LUACONNECTION_HPP_ */