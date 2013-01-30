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

void GroundPolicy::polAdd(DefId defnr, PCGroundRule* rule) {
	if (_definitions.find(defnr) == _definitions.end()) {
		_definitions.insert(std::pair<DefId, GroundDefinition*> { defnr, new GroundDefinition(defnr, _translator) });
	}
	_definitions.at(defnr)->addPCRule(rule->head(), rule->body(), rule->type() == RuleType::CONJ, rule->recursive());
}

void GroundPolicy::polAdd(DefId defnr, AggGroundRule* rule) {
	if (_definitions.find(defnr) == _definitions.end()) {
		_definitions.insert(std::pair<DefId, GroundDefinition*> { defnr, new GroundDefinition(defnr, _translator) });
	}
	_definitions.at(defnr)->addAggRule(rule->head(), rule->setnr(), rule->aggtype(), rule->lower(), rule->bound(), rule->recursive());
}

void GroundPolicy::polAddOptimization(AggFunction /*function*/, SetId /*setid*/) {
	throw notyetimplemented("Adding optimization to the grounding\n");
}

void GroundPolicy::polAddOptimization(VarId /*varid*/) {
	throw notyetimplemented("Adding optimization to the grounding\n");
}

void GroundPolicy::polAdd(const std::vector<std::map<Lit, Lit> >&){
	throw notyetimplemented("Adding symmetries to the grounding\n");
}

std::ostream& GroundPolicy::polPut(std::ostream& s, GroundTranslator* translator) const {
	std::clog << "Printing ground theory\n";
	std::clog << "Has " << _clauses.size() << " clauses." << "\n";
	for (auto i = _clauses.cbegin(); i < _clauses.cend(); ++i) {
		if ((*i).empty()) {
			s << "false";
			continue;
		}

		bool begin = true;
		for (auto j = (*i).cbegin(); j < (*i).cend(); ++j) {
			if (not begin) {
				s << " | ";
			}
			begin = false;
			s << translator->printLit((*j));
		}
		s << ".\n";
	}
	std::clog << "Has " << _definitions.size() << " definitions." << "\n";
	for (auto i = _definitions.cbegin(); i != _definitions.cend(); ++i) {
		s << print((*i).second);
	}
	std::clog << "Has " << _sets.size() << " sets." << "\n";
	for (auto i = _sets.cbegin(); i != _sets.cend(); ++i) {
		s << "Set nr. " << (*i)->setnr() << " = [ ";
		bool begin = true;
		for (size_t m = 0; m < (*i)->size(); ++m) {
			if (not begin) {
				s << "; ";
			}
			begin = false;
			s << "(" << translator->printLit((*i)->literal(m));
			s << " = " << (*i)->weight(m) << ")";
		}
		s << "].\n";
	}
	std::clog << "Has " << _aggregates.size() << " aggregates." << "\n";
	for (auto i = _aggregates.cbegin(); i != _aggregates.cend(); ++i) {
		auto agg = (*i);
		s << translator->printLit(agg->head()) << ' ';
		s << agg->arrow() << ' ';
		s << agg->bound();
		s << (agg->lower() ? " =< " : " >= ");
		s << agg->type() << '(' << agg->setnr() << ")." << "\n";
	}
	//TODO: repeat above for fixpoint definitions
	for (auto it = _cpreifications.begin(); it != _cpreifications.end(); ++it) {
		CPReification* cpr = *it;
		s << translator->printLit(cpr->_head) << ' ' << cpr->_body->type() << ' ';
		CPTerm* left = cpr->_body->left();
		if (isa<CPWSumTerm>(*left)) {
			CPWSumTerm* cpt = dynamic_cast<CPWSumTerm*>(left);
			s << "wsum[ ";
			bool begin = true;
			auto vit = cpt->varids().begin();
			auto wit = cpt->weights().begin();
			for (; vit != cpt->varids().end() && wit != cpt->weights().end(); ++vit, ++wit) {
				if (not begin) {
					s << "; ";
				}
				begin = false;
				s << '(' << translator->printTerm(*vit) << '=' << *wit << ')';
			}
			s << " ]";
		} else {
			Assert(isa<CPVarTerm>(*left));
			CPVarTerm* cpt = dynamic_cast<CPVarTerm*>(left);
			s << translator->printTerm(cpt->varid());
		}
		s << ' ' << print(cpr->_body->comp()) << ' ';
		CPBound right = cpr->_body->right();
		if (right._isvarid) {
			s << translator->printTerm(right._varid);
		} else {
			s << right._bound;
		}
		s << '.' << "\n";
	}
	return s;
}

std::string GroundPolicy::polToString(GroundTranslator* translator) const {
	std::stringstream s;
	polPut(s, translator);
	return s.str();
}
