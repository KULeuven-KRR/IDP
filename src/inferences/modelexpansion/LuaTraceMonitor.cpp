/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "IncludeComponents.hpp"
#include "LuaTraceMonitor.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "lua/luaconnection.hpp"
#include "creation/cppinterface.hpp"
#include "structure/StructureComponents.hpp"

LuaTraceMonitor::LuaTraceMonitor(lua_State* L)
		: _translator(NULL), _state(L) {
	++_tracenr;
	_registryindex = new std::string(std::string("sat_trace_") + convertToString(_tracenr));
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, _registryindex->c_str());
}

void LuaTraceMonitor::setSolver(PCSolver* solver) {
	auto solvermonitor_ = new SearchMonitor();
	solvermonitor_->setBacktrackCB([this](int dl){this->backtrack(dl);});
	solvermonitor_->setPropagateCB([this](MinisatID::Lit lit, int dl){this->propagate(lit, dl);});
	solver->addMonitor(solvermonitor_);
}

void LuaTraceMonitor::backtrack(int dl) {
	lua_getglobal(_state, "table");
	lua_getfield(_state, -1, "insert");
	lua_getfield(_state, LUA_REGISTRYINDEX, _registryindex->c_str());
	lua_newtable(_state);
	lua_pushstring(_state, "backtrack");
	lua_setfield(_state, -2, "type");
	lua_pushinteger(_state, dl);
	lua_setfield(_state, -2, "dl");
	lua_call(_state, 2, 0);
	lua_pop(_state, 1);
}

void LuaTraceMonitor::propagate(MinisatID::Lit lit, int dl) {
	lua_getglobal(_state, "table");
	lua_getfield(_state, -1, "insert");
	lua_getfield(_state, LUA_REGISTRYINDEX, _registryindex->c_str());
	lua_newtable(_state);
	lua_pushstring(_state, "assign");
	lua_setfield(_state, -2, "type");
	lua_pushinteger(_state, dl);
	lua_setfield(_state, -2, "dl");
	lua_pushboolean(_state, !lit.hasSign());
	lua_setfield(_state, -2, "value");

	int atomnr = var(lit);
	if (_translator->isInputAtom(atomnr)) {
		auto s = _translator->getSymbol(atomnr);
		auto args = _translator->getArgs(var(lit));
		InternalArgument ia(&Gen::atom(s, args));
		LuaConnection::convertToLua(_state, ia);
	} else {
		lua_pushstring(_state, _translator->printLit(var(lit)).c_str());
	}
	lua_setfield(_state, -2, "atom");
	lua_call(_state, 2, 0);
	lua_pop(_state, 1);
}
