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

class Vocabulary;
class AbstractStructure;
class AbstractTheory;
class DomainElement;
class InternalArgument;
class Inference;

namespace LuaConnection {

lua_State* getState();

void makeLuaConnection();
void closeLuaConnection();
int convertToLua(lua_State*, InternalArgument);

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

InternalArgument* call(const std::vector<std::string>&, const std::vector<std::vector<std::string>>&, const ParseInfo&);
const DomainElement* funccall(std::string*, const std::vector<const DomainElement*>& tuple);
bool predcall(std::string*, const std::vector<const DomainElement*>& tuple);

std::string* getProcedure(const std::vector<std::string>&, const ParseInfo&);

const DomainElement* execute(const std::string& chunk);
void compile(UserProcedure*);

InternalArgument createArgument(int arg, lua_State* L);

class InternalProcedure {
private:
	Inference* inference_; // NOTE: does not have pointer authority

public:
	InternalProcedure(Inference* inference)
			: inference_(inference) {
	}
	~InternalProcedure() {
	}

	int operator()(lua_State* L) const;

	const std::string& getName() const;
	const std::vector<ArgType>& getArgumentTypes() const;
	const std::string& getNameSpace() const;
};

LuaTraceMonitor* getLuaTraceMonitor();

}

#endif /* LUACONNECTION_HPP_ */