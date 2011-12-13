/************************************
	ModelExpansion.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef MODELEXPANSION_HPP_
#define MODELEXPANSION_HPP_

#include <vector>

class AbstractStructure;
class AbstractTheory;
class TraceMonitor;



class ModelExpansion {
public:
	static std::vector<AbstractStructure*> doModelExpansion(AbstractTheory* theory, AbstractStructure* structure, TraceMonitor* tracemonitor){
		ModelExpansion m;
		return m.expand(theory, structure, tracemonitor);
	}

private:
	std::vector<AbstractStructure*> expand(AbstractTheory* theory, AbstractStructure* structure, TraceMonitor* tracemonitor) const;
};

#endif
