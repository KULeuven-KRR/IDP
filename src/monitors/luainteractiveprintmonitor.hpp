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

#ifndef LUAINTERACTIVEPRINTMONITOR_HPP_
#define LUAINTERACTIVEPRINTMONITOR_HPP_

#include "monitors/interactiveprintmonitor.hpp"
#include "lua/luaconnection.hpp"
#include "lua.hpp"

class LuaInteractivePrintMonitor: public InteractivePrintMonitor {
private:
	lua_State* state_;
	//int buffersize;
	std::stringstream buffer_;

	lua_State* state() const {
		return state_;
	}

public:
	LuaInteractivePrintMonitor(lua_State* state)
			: state_(state) /*, buffersize(0)*/ {
	}

	virtual void print(const std::string& text) {
		std::cout << text;
		/*buffersize +=text.size();
		 buffer_ <<text;

		 if(buffersize>1000){
		 flush();
		 }*/
	}

	virtual void printerror(const std::string& text) {
		lua_pushstring(state(), text.c_str());
		lua_error(state());
	}

	virtual void flush() {
		/*lua_getglobal(state(), "io");
		 lua_getfield(state(), -1, "write");
		 lua_pushstring(state(),buffer_.str().c_str());
		 lua_call(state(),1,0);
		 lua_pop(state(), 1);
		 buffersize = 0;
		 buffer_.str("");*/
		std::cout << buffer_.str();
		buffer_.str("");
	}
};

#endif /* LUAINTERACTIVEPRINTMONITOR_HPP_ */
