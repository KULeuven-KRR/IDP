/************************************
	luatracemonitor.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef LUATRACEMONITOR_HPP_
#define LUATRACEMONITOR_HPP_

#include "monitors/tracemonitor.hpp"
#include "external/SearchMonitor.hpp"
#include "lua.hpp"

class LuaTraceMonitor: public TraceMonitor{
private:
	GroundTranslator*	_translator;
	lua_State*			_state;
	std::string*		_registryindex;
	static int			_tracenr;
	MinisatID::SearchMonitor*	solvermonitor_;

public:
	LuaTraceMonitor(lua_State* L) : _translator(NULL), _state(L) {
		++_tracenr;
		_registryindex = StringPointer(std::string("sat_trace_") + convertToString(_tracenr));
		lua_newtable(L);
		lua_setfield(L,LUA_REGISTRYINDEX,_registryindex->c_str());

		cb::Callback1<void, int> callbackback(this, &LuaTraceMonitor::backtrack);
		cb::Callback2<void, MinisatID::Literal, int> callbackprop(this, &LuaTraceMonitor::propagate);
		MinisatID::SearchMonitor* solvermonitor_ = new MinisatID::SearchMonitor();
		solvermonitor_->setBacktrackCB(callbackback);
		solvermonitor_->setPropagateCB(callbackprop);
	}

	void setTranslator(GroundTranslator* translator) { _translator = translator; }
	void setSolver(SATSolver* solver) { solver->addMonitor(solvermonitor_); }

	void backtrack(int dl){
		lua_getglobal(_state,"table");
		lua_getfield(_state,-1,"insert");
		lua_getfield(_state,LUA_REGISTRYINDEX,_registryindex->c_str());
		lua_newtable(_state);
		lua_pushstring(_state,"backtrack");
		lua_setfield(_state,-2,"type");
		lua_pushinteger(_state,dl);
		lua_setfield(_state,-2,"dl");
		lua_call(_state,2,0);
		lua_pop(_state,1);
	}

	void propagate(MinisatID::Literal lit, int dl){
		lua_getglobal(_state,"table");
		lua_getfield(_state,-1,"insert");
		lua_getfield(_state,LUA_REGISTRYINDEX,_registryindex->c_str());
		lua_newtable(_state);
		lua_pushstring(_state,"assign");
		lua_setfield(_state,-2,"type");
		lua_pushinteger(_state,dl);
		lua_setfield(_state,-2,"dl");
		lua_pushboolean(_state,!lit.hasSign());
		lua_setfield(_state,-2,"value");

		int atomnr = lit.getAtom().getValue();
		if(_translator->isInputAtom(atomnr)) {
			PFSymbol* s = _translator->getSymbol(atomnr);
			const ElementTuple& args = _translator->getArgs(lit.getAtom().getValue());
			const DomainAtom* atom = DomainAtomFactory::instance()->create(s,args);
			InternalArgument ia(atom);
			LuaConnection::convertToLua(_state,ia);
		}
		else {
			lua_pushstring(_state,_translator->printLit(lit.getAtom().getValue()).c_str());
		}
		lua_setfield(_state,-2,"atom");
		lua_call(_state,2,0);
		lua_pop(_state,1);
	}

	std::string* index() const { return _registryindex; }
};


#endif /* LUATRACEMONITOR_HPP_ */
