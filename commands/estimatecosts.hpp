/************************************
	estimatecosts.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef ESTIMATECOSTS_HPP_
#define ESTIMATECOSTS_HPP_

#include <vector>
#include <set>
#include "commandinterface.hpp"
#include "query.hpp"
#include "structure.hpp"

class EstimateBDDCostInference: public Inference {
public:
	EstimateBDDCostInference(): Inference("estimateCost") {
		add(AT_QUERY);
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Query* q = args[0]._value._query;
		AbstractStructure* structure = args[1].structure();
		FOBDDManager manager;
		FOBDDFactory m(&manager);
		std::set<Variable*> sv(q->variables().begin(),q->variables().end());
		std::set<const FOBDDVariable*> svbdd = manager.getVariables(sv);
		std::set<const FOBDDDeBruijnIndex*> si;
		const FOBDD* bdd = m.run(q->query());
		InternalArgument ia; ia._type = AT_DOUBLE;
		manager.optimizequery(bdd,svbdd,si,structure);
		ia._value._double = manager.estimatedCostAll(bdd,svbdd,si,structure);
		return ia;
	}
};

class EstimateNumberOfAnswersInference: public Inference {
public:
	EstimateNumberOfAnswersInference(): Inference("estimateNrOfAnswers") {
		add(AT_QUERY);
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Query* q = args[0]._value._query;
		AbstractStructure* structure = args[1].structure();
		FOBDDManager manager;
		FOBDDFactory m(&manager);
		std::set<Variable*> sv(q->variables().begin(),q->variables().end());
		std::set<const FOBDDVariable*> svbdd = manager.getVariables(sv);
		std::set<const FOBDDDeBruijnIndex*> si;
		const FOBDD* bdd = m.run(q->query());
		InternalArgument ia; ia._type = AT_DOUBLE;
		ia._value._double = manager.estimatedNrAnswers(bdd,svbdd,si,structure);
		return ia;
	}
};

#endif /* ESTIMATECOSTS_HPP_ */
