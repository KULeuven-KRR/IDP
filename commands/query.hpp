/************************************
	query.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef QUERY_HPP_
#define QUERY_HPP_

#include <vector>
#include <set>
#include "commandinterface.hpp"
#include "structure.hpp"
#include "query.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include "generators/InstGenerator.hpp"

class QueryInference: public Inference {
public:
	QueryInference(): Inference("query") {
		add(AT_QUERY);
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Query* q = args[0]._value._query;
		AbstractStructure* structure = args[1].structure();

		// translate the formula to a bdd
		FOBDDManager manager;
		FOBDDFactory factory(&manager);
		std::set<Variable*> vars(q->variables().cbegin(),q->variables().cend());
		std::set<const FOBDDVariable*> bddvars = manager.getVariables(vars);
		std::set<const FOBDDDeBruijnIndex*> bddindices;
		const FOBDD* bdd = factory.run(q->query());

		// optimize the query
		manager.optimizequery(bdd,bddvars,bddindices,structure);

		// create a generator
		std::vector<const DomElemContainer*> genvars;
		std::vector<const FOBDDVariable*> vbddvars;
		std::vector<Pattern> pattern;
		std::vector<SortTable*> tables;
		for(auto it = q->variables().cbegin(); it != q->variables().cend(); ++it) {
			pattern.push_back(Pattern::OUTPUT);
			genvars.push_back(new const DomElemContainer());
			vbddvars.push_back(manager.getVariable(*it));
            tables.push_back(structure->inter((*it)->sort()));
		}
		BDDToGenerator btg(&manager);
		InstGenerator* generator = btg.create(bdd,pattern,genvars,vbddvars,structure,Universe(tables));

		// Create an empty table
		EnumeratedInternalPredTable* interntable = new EnumeratedInternalPredTable();
		std::vector<SortTable*> vst;
		for(auto it = q->variables().cbegin(); it != q->variables().cend(); ++it) {
			vst.push_back(structure->inter((*it)->sort()));
		}
		Universe univ(vst);
		PredTable* result = new PredTable(interntable,univ);

		// execute the query
		ElementTuple currtuple(q->variables().size());
		for(generator->begin(); not generator->isAtEnd(); generator->operator ++()){
			for(unsigned int n = 0; n < q->variables().size(); ++n) {
				currtuple[n] = genvars[n]->get();
			}
			result->add(currtuple);
		}

		// return the result
		InternalArgument arg;
		arg._type = AT_PREDTABLE;
		arg._value._predtable = result;
		return arg;
	}
};

#endif /* QUERY_HPP_ */
