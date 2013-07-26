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

#pragma once

#include <iostream>
#include "TraceMonitor.hpp"

class PropagateMonitor: public TraceMonitor {
private:
	std::vector<MinisatID::Lit> _partialmodel;
	SearchMonitor* _solvermonitor;
public:
	PropagateMonitor() {
		_solvermonitor = new SearchMonitor();
		_solvermonitor->setPropagateCB([this](MinisatID::Lit lit, int dl){this->propagate(lit,dl);});
	}
	virtual ~PropagateMonitor() {
		delete (_solvermonitor);
	}

	void backtrack(int) {
		// TODO implement (currently only used to do unit propagation!)
	}
	void propagate(MinisatID::Lit lit, int) {
		_partialmodel.push_back(lit);
	}

	virtual void setSolver(PCSolver* solver) {
		solver->addMonitor(_solvermonitor);
	}
	const std::vector<MinisatID::Lit>& model() {
		return _partialmodel;
	}
	void setTranslator(GroundTranslator*) {
		// Note: no-op as we return the original literals // TODO should we do this?
	}
};
