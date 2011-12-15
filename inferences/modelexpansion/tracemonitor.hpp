/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

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

	//NOTE: should be called BEFORE the grounding
	//(else, we don't keep track of propagations that occur immediately after adding a unit clause)
	virtual void setSolver(MinisatID::SATSolver* solver) = 0;
};


#endif /* TRACEMONITOR_HPP_ */
