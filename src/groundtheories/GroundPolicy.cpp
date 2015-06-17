/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include "GroundPolicy.hpp"
#include "theory/ecnf.hpp"
#include "common.hpp"

#include <sstream>

#include "inferences/grounding/GroundTranslator.hpp"

void GroundPolicy::polStartTheory(GroundTranslator* translator) {
	_translator = translator;
}
void GroundPolicy::polEndTheory() {
}

void GroundPolicy::polRecursiveDelete() {
	for (auto defit = _definitions.begin(); defit != _definitions.end(); ++defit) {
		(*defit).second->recursiveDelete();
	}
	for (auto aggit = _aggregates.begin(); aggit != _aggregates.end(); ++aggit) {
		delete (*aggit);
	}
	for (auto setit = _sets.begin(); setit != _sets.end(); ++setit) {
		delete (*setit);
	}
	for (auto fdefit = _fixpdefs.begin(); fdefit != _fixpdefs.end(); ++fdefit) {
		(*fdefit)->recursiveDelete();
	}
	for (auto cprit = _cpreifications.begin(); cprit != _cpreifications.end(); ++cprit) {
		delete (*cprit);
	}
}

void GroundPolicy::polAdd(const GroundClause& cl) {
	_clauses.push_back(cl);
}

void GroundPolicy::polAdd(Lit tseitin, AggTsBody* body) {
	_aggregates.push_back(new GroundAggregate(body->aggtype(), body->lower(), body->type(), tseitin, body->setnr(), body->bound()));
}

void GroundPolicy::polAdd(Lit tseitin, CPTsBody* body) {
	//TODO also add variables (in a separate container?)
	_cpreifications.push_back(new CPReification(tseitin, body));
}

void GroundPolicy::polAdd(const TsSet& tsset, SetId setnr, bool) {
	_sets.push_back(new GroundSet(setnr, tsset.literals(), tsset.weights()));
}

void GroundPolicy::polAdd(DefId defnr, const PCGroundRule& rule) {
	if (_definitions.find(defnr) == _definitions.end()) {
		_definitions.insert(std::pair<DefId, GroundDefinition*> { defnr, new GroundDefinition(defnr, _translator) });
	}
	_definitions.at(defnr)->addPCRule(rule.head(), rule.body(), rule.type() == RuleType::CONJ, rule.recursive());
}

void GroundPolicy::polAdd(DefId defnr, AggGroundRule* rule) {
	if (_definitions.find(defnr) == _definitions.end()) {
		_definitions.insert(std::pair<DefId, GroundDefinition*> { defnr, new GroundDefinition(defnr, _translator) });
	}
	_definitions.at(defnr)->addAggRule(rule->head(), rule->setnr(), rule->aggtype(), rule->lower(), rule->bound(), rule->recursive());
}

void GroundPolicy::polAdd(const GroundEquivalence& geq){
	// no support for equalities yet, so we convert to clauses
	std::vector<GroundClause> clauses;
	geq.getClauses(clauses);
	for(auto cl: clauses){
		polAdd(cl);
	}
}

void GroundPolicy::polAddOptimization(AggFunction /*function*/, SetId /*setid*/) {
	throw notyetimplemented("Adding optimization to the grounding\n");
}

void GroundPolicy::polAddOptimization(VarId /*varid*/) {
	throw notyetimplemented("Adding optimization to the grounding\n");
}
