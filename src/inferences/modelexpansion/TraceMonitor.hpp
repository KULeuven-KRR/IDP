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

#ifndef TRACEMONITOR_HPP_
#define TRACEMONITOR_HPP_

#include <string>
#include "inferences/SolverInclude.hpp"

class GroundTranslator;

class TraceMonitor {
public:
	virtual ~TraceMonitor() {
	}
	virtual void backtrack(int dl) = 0;
	virtual void propagate(MinisatID::Lit lit, int dl) = 0;
	virtual void setTranslator(GroundTranslator* translator) = 0;

	//NOTE: should be called BEFORE the grounding
	//(else, we don't keep track of propagations that occur immediately after adding a unit clause)
	virtual void setSolver(PCSolver* solver) = 0;
};

#endif /* TRACEMONITOR_HPP_ */
