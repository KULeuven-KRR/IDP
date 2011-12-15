/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "GroundPolicy.hpp"
#include "ecnf.hpp"
#include "common.hpp"

#include <sstream>

#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/GroundTermTranslator.hpp"

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
	//for(std::vector<GroundFixpDef*>::iterator fdefit = _fixpdefs.begin(); fdefit != _fixpdefs.end(); ++fdefit) {
	//	(*defit)->recursiveDelete();
	//	delete(*defit);
	//}
	for (auto cprit = _cpreifications.begin(); cprit != _cpreifications.end(); ++cprit) {
		delete (*cprit);
	}
}

void GroundPolicy::polAdd(const GroundClause& cl) {
	_clauses.push_back(cl);
}

void GroundPolicy::polAdd(int head, AggTsBody* body) {
	_aggregates.push_back(new GroundAggregate(body->aggtype(), body->lower(), body->type(), head, body->setnr(), body->bound()));
}

void GroundPolicy::polAdd(int tseitin, CPTsBody* body) {
	//TODO also add variables (in a separate container?)
	_cpreifications.push_back(new CPReification(tseitin, body));
}

void GroundPolicy::polAdd(const TsSet& tsset, int setnr, bool) {
	_sets.push_back(new GroundSet(setnr, tsset.literals(), tsset.weights()));
}

void GroundPolicy::polAdd(int defnr, PCGroundRule* rule) {
	if (_definitions.find(defnr) == _definitions.end()) {
		_definitions.insert(std::pair<int, GroundDefinition*>(defnr, new GroundDefinition(defnr, _translator)));
	}
	_definitions.at(defnr)->addPCRule(rule->head(), rule->body(), rule->type() == RT_CONJ, rule->recursive());
}

void GroundPolicy::polAdd(int defnr, AggGroundRule* rule) {
	if (_definitions.find(defnr) == _definitions.end()) {
		_definitions.insert(std::pair<int, GroundDefinition*>(defnr, new GroundDefinition(defnr, _translator)));
	}
	_definitions.at(defnr)->addAggRule(rule->head(), rule->setnr(), rule->aggtype(), rule->lower(), rule->bound(), rule->recursive());
}

std::ostream& GroundPolicy::polPut(std::ostream& s, GroundTranslator* translator, GroundTermTranslator* termtranslator) const {
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
		s << toString((*i).second);
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
		if (typeid(*left) == typeid(CPSumTerm)) {
			CPSumTerm* cpt = dynamic_cast<CPSumTerm*>(left);
			s << "sum[ ";
			bool begin = true;
			for (auto vit = cpt->varids().begin(); vit != cpt->varids().end(); ++vit) {
				if (not begin) {
					s << "; ";
				}
				begin = false;
				s << termtranslator->printTerm(*vit);
			}
			s << " ]";
		} else if (typeid(*left) == typeid(CPWSumTerm)) {
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
				s << '(' << termtranslator->printTerm(*vit) << '=' << *wit << ')';
			}
			s << " ]";
		} else {
			Assert(typeid(*left) == typeid(CPVarTerm));
			CPVarTerm* cpt = dynamic_cast<CPVarTerm*>(left);
			s << termtranslator->printTerm(cpt->varid());
		}
		s << ' ' << cpr->_body->comp() << ' ';
		CPBound right = cpr->_body->right();
		if (right._isvarid) {
			s << termtranslator->printTerm(right._varid);
		} else {
			s << right._bound;
		}
		s << '.' << "\n";
	}
	return s;
}

std::string GroundPolicy::polToString(GroundTranslator* translator, GroundTermTranslator* termtranslator) const {
	std::stringstream s;
	polPut(s, translator, termtranslator);
	return s.str();
}
