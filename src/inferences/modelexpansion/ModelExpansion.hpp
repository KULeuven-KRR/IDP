/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef MODELEXPANSION_HPP_
#define MODELEXPANSION_HPP_

#include <vector>

class AbstractStructure;
class AbstractTheory;
class TraceMonitor;

class ModelExpansion {
public:
	static std::vector<AbstractStructure*> doModelExpansion(AbstractTheory* theory, AbstractStructure* structure, TraceMonitor* tracemonitor) {
		ModelExpansion m(theory, structure, tracemonitor);
		return m.expand();
	}

private:
	AbstractTheory* theory;
	AbstractStructure* structure;
	TraceMonitor* tracemonitor;
	ModelExpansion(AbstractTheory* theory, AbstractStructure* structure, TraceMonitor* tracemonitor)
			: theory(theory), structure(structure), tracemonitor(tracemonitor) {
	}
	std::vector<AbstractStructure*> expand() const;

};
#endif //MODELEXPANSION_HPP_
