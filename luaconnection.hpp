/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef LUACONNECTION_HPP_
#define LUACONNECTION_HPP_

#include <string>
#include <sstream>
#include "internalargument.hpp"
#include "lua.hpp"
#include "commands/commandinterface.hpp"

class Vocabulary;
class AbstractStructure;
class AbstractTheory;
class DomainElement;
class InternalArgument;

namespace LuaConnection {

void makeLuaConnection();
void closeLuaConnection();
int convertToLua(lua_State*, InternalArgument);

lua_State* getState();

template<typename Object>
void addGlobal(const std::string& name, Object v) {
	convertToLua(getState(), InternalArgument(v));
	lua_setglobal(getState(), name.c_str());
}

template<typename Object>
void addGlobal(Object v) {
	addGlobal(v->name(), v);
}

template<> void addGlobal(UserProcedure* p);

Vocabulary* vocabulary(InternalArgument*);
AbstractTheory* theory(InternalArgument*);
AbstractStructure* structure(InternalArgument*);

InternalArgument* call(const std::vector<std::string>&, const std::vector<std::vector<std::string> >&, const ParseInfo&);
const DomainElement* funccall(std::string*, const std::vector<const DomainElement*>& tuple);
bool predcall(std::string*, const std::vector<const DomainElement*>& tuple);

std::string* getProcedure(const std::vector<std::string>&, const ParseInfo&);

const DomainElement* execute(const std::string& chunk);
void compile(UserProcedure*);

InternalArgument createArgument(int arg, lua_State* L);

class InternalProcedure {
private:
	Inference* inference_;

public:
	InternalProcedure(Inference* inference) :
			inference_(inference) {
	}
	~InternalProcedure() {
		delete (inference_);
	}

	int operator()(lua_State* L) const;

	const std::string& getName() const {
		return inference_->getName();
	}
	const std::vector<ArgType>& getArgumentTypes() const {
		return inference_->getArgumentTypes();
	}
};

}

#endif /* LUACONNECTION_HPP_ */
