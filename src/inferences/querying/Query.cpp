#include "Query.hpp"

#include "IncludeComponents.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include "generators/InstGenerator.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"

PredTable* Querying::solveQuery(Query* q, AbstractStructure* structure) const {
	// translate the formula to a bdd
	FOBDDManager manager;
	FOBDDFactory factory(&manager);
	std::set<Variable*> vars(q->variables().cbegin(), q->variables().cend());
	std::set<const FOBDDVariable*> bddvars = manager.getVariables(vars);
	std::set<const FOBDDDeBruijnIndex*> bddindices;
	const FOBDD* bdd = factory.turnIntoBdd(q->query());
	Assert(bdd != NULL);
	// optimize the query
	manager.optimizeQuery(bdd, bddvars, bddindices, structure);

	// create a generator
	BddGeneratorData data;
	data.bdd = bdd;
	data.structure = structure;
	for (auto it = q->variables().cbegin(); it != q->variables().cend(); ++it) {
		data.pattern.push_back(Pattern::OUTPUT);
		data.vars.push_back(new const DomElemContainer());
		data.bddvars.push_back(manager.getVariable(*it));
		data.universe.addTable(structure->inter((*it)->sort()));
	}
	BDDToGenerator btg(&manager);

	InstGenerator* generator = btg.create(data);

	// Create an empty table
	EnumeratedInternalPredTable* interntable = new EnumeratedInternalPredTable();
	std::vector<SortTable*> vst;
	for (auto it = q->variables().cbegin(); it != q->variables().cend(); ++it) {
		vst.push_back(structure->inter((*it)->sort()));
	}
	Universe univ(vst);
	PredTable* result = new PredTable(interntable, univ);

	// execute the query
	ElementTuple currtuple(q->variables().size());
	for (generator->begin(); not generator->isAtEnd(); generator->operator ++()) {
		for (unsigned int n = 0; n < q->variables().size(); ++n) {
			currtuple[n] = data.vars[n]->get();
		}
		result->add(currtuple);
	}
	return result;
}
