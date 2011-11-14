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
	static std::vector<AbstractStructure*> doModelExpansion(AbstractTheory* theory, AbstractStructure* structure, Options* options, TraceMonitor* tracemonitor){
		ModelExpansion m;
		return m.expand(theory, structure, options, tracemonitor);
	}

private:
	std::vector<AbstractStructure*> expand(AbstractTheory* theory, AbstractStructure* structure, Options* options, TraceMonitor* tracemonitor) const;

	bool calculateDefinition(Definition* definition, AbstractStructure* structure, Options* options) const;

	bool calculateKnownDefinitions(Theory* theory, AbstractStructure* structure, Options* options) const;

	MinisatID::WrappedPCSolver* createsolver(Options* options) const;

	MinisatID::Solution* initsolution(Options* options) const;

	void addLiterals(MinisatID::Model* model, GroundTranslator* translator, AbstractStructure* init) const;

	void addTerms(MinisatID::Model* model, GroundTermTranslator* termtranslator, AbstractStructure* init) const;
};

#endif
