#ifndef TRACEMONITOR_HPP_
#define TRACEMONITOR_HPP_

#include <string>

class GroundTranslator;

namespace MinisatID{
	class Literal;
	class WrappedPCSolver;
	typedef WrappedPCSolver SATSolver;
}

class TraceMonitor{
public:
	virtual ~TraceMonitor(){}
	virtual void backtrack(int dl) = 0;
	virtual void propagate(MinisatID::Literal lit, int dl) = 0;
	virtual void setTranslator(GroundTranslator* translator) = 0;
	virtual void setSolver(MinisatID::SATSolver* solver) = 0;
};


#endif /* TRACEMONITOR_HPP_ */
