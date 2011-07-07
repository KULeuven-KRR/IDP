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
	int buffersize;
	std::stringstream buffer_;

	lua_State* state() const { return state_; }

public:
	LuaInteractivePrintMonitor(lua_State* state): state_(state){
	}

	virtual void print(const std::string& text){
		buffersize +=text.size();
		buffer_ <<text;

		if(buffersize>1000){
			lua_getglobal(state(),"print");
			lua_pushstring(state(),buffer_.str().c_str());
			lua_call(state(),1,0);
			buffersize = 0;
			buffer_.str("");
		}
	}

	virtual void printerror(const std::string& text){
		lua_pushstring(state(),text.c_str());
		lua_error(state());
	}

	//FiXME not the cleanest flush
	virtual std::string str() {
		lua_getglobal(state(),"print");
		lua_pushstring(state(),buffer_.str().c_str());
		lua_call(state(),1,0);
		buffersize = 0;
		buffer_.str("");
		return "";
	}
};

#endif /* LUAINTERACTIVEPRINTMONITOR_HPP_ */
