/************************************
	ModelExpansion.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

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
		ModelExpansion m;
		return m.expand(theory, structure, tracemonitor);
	}

private:
	std::vector<AbstractStructure*> expand(AbstractTheory* theory, AbstractStructure* structure, TraceMonitor* tracemonitor) const;

	bool calculateDefinition(Definition* definition, AbstractStructure* structure) const;

	bool calculateKnownDefinitions(Theory* theory, AbstractStructure* structure) const;

	MinisatID::WrappedPCSolver* createsolver() const;

	MinisatID::Solution* initsolution() const;

	void addLiterals(MinisatID::Model* model, GroundTranslator* translator, AbstractStructure* init) const;

	void addTerms(MinisatID::Model* model, GroundTermTranslator* termtranslator, AbstractStructure* init) const;
};

#endif
