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

#ifndef LUATRACEMONITOR_HPP_
#define LUATRACEMONITOR_HPP_

#include "TraceMonitor.hpp"
#include "lua.hpp"

//for i,v in pairs(trace) do print(v["atom"]) print(v["value"]) end
class LuaTraceMonitor: public TraceMonitor {
private:
	GroundTranslator* _translator;
	lua_State* _state;
	std::string* _registryindex;
	static int _tracenr;

public:
	LuaTraceMonitor(lua_State* L);

	void setTranslator(GroundTranslator* translator) {
		_translator = translator;
	}
	void setSolver(PCSolver* solver);

	void backtrack(int dl);
	void propagate(MinisatID::Lit lit, int dl);

	std::string* index() const {
		return _registryindex;
	}
};

#endif /* LUATRACEMONITOR_HPP_ */
