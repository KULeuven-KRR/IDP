/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef ESTIMATECOSTS_HPP_
#define ESTIMATECOSTS_HPP_

#include <vector>
#include <set>
#include "commandinterface.hpp"
#include "query.hpp"
#include "structure.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"

class EstimateBDDCostInference: public Inference {
public:
	EstimateBDDCostInference() :
			Inference("estimatecost") {
		add(AT_QUERY);
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		auto q = args[0]._value._query;
		auto structure = args[1].structure();
		FOBDDManager manager;
		FOBDDFactory m(&manager);
		std::set<Variable*> sv(q->variables().cbegin(), q->variables().cend());
		auto svbdd = manager.getVariables(sv);
		auto bdd = m.turnIntoBdd(q->query());
		InternalArgument ia;
		ia._type = AT_DOUBLE;
		manager.optimizequery(bdd, svbdd, { }, structure);
		ia._value._double = manager.estimatedCostAll(bdd, svbdd, { }, structure);
		return ia;
	}
};

class EstimateNumberOfAnswersInference: public Inference {
public:
	EstimateNumberOfAnswersInference() :
			Inference("estimatenrofanswers") {
		add(AT_QUERY);
		add(AT_STRUCTURE);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Query* q = args[0]._value._query;
		AbstractStructure* structure = args[1].structure();
		FOBDDManager manager;
		FOBDDFactory m(&manager);
		std::set<Variable*> sv(q->variables().cbegin(), q->variables().cend());
		std::set<const FOBDDVariable*> svbdd = manager.getVariables(sv);
		std::set<const FOBDDDeBruijnIndex*> si;
		const FOBDD* bdd = m.turnIntoBdd(q->query());
		InternalArgument ia;
		ia._type = AT_DOUBLE;
		ia._value._double = manager.estimatedNrAnswers(bdd, svbdd, si, structure);
		return ia;
	}
};

#endif /* ESTIMATECOSTS_HPP_ */
