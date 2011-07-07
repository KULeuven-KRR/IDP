/************************************
	luainteractiveprintmonitor.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef LUAINTERACTIVEPRINTMONITOR_HPP_
#define LUAINTERACTIVEPRINTMONITOR_HPP_

#include "monitors/interactiveprintmonitor.hpp"
#include "luaconnection.hpp"

class LuaInteractivePrintMonitor: public InteractivePrintMonitor{
private:
	lua_State* state_;

	lua_State* state() const { return state_; }

public:
	LuaInteractivePrintMonitor(lua_State* state): state_(state){
	}

	virtual void print(const std::string& text){
		lua_getglobal(state(),"print");
		lua_pushstring(state(),text.c_str());
		lua_call(state(),1,0);
	}
};

#endif /* LUAINTERACTIVEPRINTMONITOR_HPP_ */
