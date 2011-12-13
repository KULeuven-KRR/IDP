#ifndef MODELEXPANSION_HPP_
#define MODELEXPANSION_HPP_

#include <vector>
#include <string>
#include <iostream>

class AbstractStructure;
class AbstractTheory;
class Options;
class Theory;
class Definition;
class GroundTranslator;
class GroundTermTranslator;
class TraceMonitor;
namespace MinisatID{
	class Solution;
	class Model;
	class WrappedPCSolver;
}


class ModelExpansion {
public:
	static std::vector<AbstractStructure*> doModelExpansion(AbstractTheory* theory, AbstractStructure* structure, TraceMonitor* tracemonitor){
		ModelExpansion m(theory, structure, tracemonitor);
		return m.expand();
	}

private:
	AbstractTheory* theory;
	AbstractStructure* structure;
	TraceMonitor* tracemonitor;
	ModelExpansion(AbstractTheory* theory, AbstractStructure* structure, TraceMonitor* tracemonitor):
			theory(theory), structure(structure), tracemonitor(tracemonitor){

	}
	std::vector<AbstractStructure*> expand() const;
	bool calculateDefinition(Definition* definition) const;
	bool calculateKnownDefinitions(Theory* theory) const;
	MinisatID::WrappedPCSolver* createsolver() const;
	MinisatID::Solution* initsolution() const;

	void addLiterals(MinisatID::Model* model, GroundTranslator* translator, AbstractStructure* init) const;
	void addTerms(MinisatID::Model* model, GroundTermTranslator* termtranslator, AbstractStructure* init) const;
};

#endif
