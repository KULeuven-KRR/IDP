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
#include <cstdlib>

class AbstractStructure;
class AbstractTheory;
class Theory;
class TraceMonitor;
class Term;

class ModelExpansion {
public:
	static std::vector<AbstractStructure*> doModelExpansion(AbstractTheory* theory, AbstractStructure* structure, TraceMonitor* tracemonitor);
	static std::vector<AbstractStructure*> doOptimization(AbstractTheory* theory, AbstractStructure* structure, Term* term, TraceMonitor* tracemonitor);

private:
	Theory* _theory;
	AbstractStructure* _structure;
	TraceMonitor* _tracemonitor;
	Term* _minimizeterm; // if NULL, no optimization is done

	ModelExpansion(Theory* theory, AbstractStructure* structure, Term* minimize, TraceMonitor* tracemonitor)
			: _theory(theory), _structure(structure), _tracemonitor(tracemonitor), _minimizeterm(minimize) {
	}
	std::vector<AbstractStructure*> expand() const;
};
#endif //MODELEXPANSION_HPP_
