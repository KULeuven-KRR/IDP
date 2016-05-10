#pragma once

#include <memory>
#include <inferences/grounding/GroundTranslator.hpp>
#include "common.hpp"
#include "../modelexpansion/ModelExpansion.hpp"
#include "inferences/SolverInclude.hpp"

class Definition;
class Structure;
class AbstractTheory;
class AbstractGroundTheory;
class StructureExtender;
class Theory;
class TraceMonitor;
class Term;
class Vocabulary;
class PredForm;
class Predicate;
class PFSymbol;
class DomainElement;
typedef std::vector<const DomainElement*> ElementTuple;
class ModelIterator;

std::shared_ptr<ModelIterator> createIterator(AbstractTheory*, Structure*, Vocabulary*,
		TraceMonitor*, const MXAssumptions& = MXAssumptions());

class ModelIterator {
public:
	ModelIterator(Structure*, Theory*, Vocabulary*, TraceMonitor*, const MXAssumptions&);
	~ModelIterator();
	void init();
	MXResult calculate();
  MXResult calculateMonitor();
	void addAssumption(const Lit);
	void removeAssumption(const Lit);
  void addClause(const std::vector<Lit>& lits);
	GroundTranslator* translator();

private:
	std::vector<Definition*> preprocess(Theory*);
	void ground(Theory*);
	void prepareSolver();
	MXResult getStructure(MXResult, clock_t, std::shared_ptr<MinisatID::Model>);

	Structure* _structure;
	Theory* _theory;
	TraceMonitor* _tracemonitor;
	std::vector<Definition*> postprocessdefs;

	Vocabulary* _outputvoc; // if not NULL, mx is allowed to return models which are only two-valued on the outputvoc.
	Vocabulary* _currentVoc;
	MXAssumptions _assumeFalse;


	PCSolver* _data;
	AbstractGroundTheory* _grounding;
	StructureExtender* _extender;
	litlist* _assumptions;

	MinisatID::ModelIterationTask* _mx;
};
