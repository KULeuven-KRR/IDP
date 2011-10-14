/************************************
	GroundTheory.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GROUNDTHEORY_HPP_
#define GROUNDTHEORY_HPP_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <ostream>
#include <iostream>

#include "ecnf.hpp"
#include "common.hpp"

#include "ground.hpp"
#include <sstream>

#include <assert.h>

class GroundPolicy {
private:
	std::vector<GroundClause>		_clauses;
	std::map<int,GroundDefinition*>	_definitions;
	std::vector<GroundFixpDef*>		_fixpdefs;
	std::vector<GroundSet*>			_sets;
	std::vector<GroundAggregate*>	_aggregates;
	std::vector<CPReification*>		_cpreifications;

	GroundTranslator*				_translator;
	GroundTranslator* polTranslator() const { return _translator; }

public:
	// Inspectors
	unsigned int		nrClauses()						const { return _clauses.size();							}
	unsigned int		nrDefinitions()					const { return _definitions.size();						}
	unsigned int		nrFixpDefs()					const { return _fixpdefs.size();						}
	unsigned int		nrSets()						const { return _sets.size();							}
	unsigned int		nrAggregates()					const { return _aggregates.size();						}
	unsigned int 		nrCPReifications()				const { return _cpreifications.size();					}
	GroundClause		clause(unsigned int n)			const { return _clauses[n];								}
	GroundDefinition*	definition(unsigned int n)		const { return _definitions.at(n);						}
	GroundFixpDef*		fixpdef(unsigned int n)			const { return _fixpdefs[n];							}
	GroundSet*			set(unsigned int n)				const { return _sets[n];								}
	GroundAggregate*	aggregate(unsigned int n)		const { return _aggregates[n];							}
	CPReification*		cpreification(unsigned int n)	const { return _cpreifications[n];						}

	void polStartTheory(GroundTranslator* translator){
		_translator = translator;
	}
	void polEndTheory(){}

	void polRecursiveDelete() {
		for(auto defit = _definitions.begin(); defit != _definitions.end(); ++defit) {
			(*defit).second->recursiveDelete();
		}
		for(auto aggit = _aggregates.begin(); aggit != _aggregates.end(); ++aggit) {
			delete(*aggit);
		}
		for(auto setit = _sets.begin(); setit != _sets.end(); ++setit) {
			delete(*setit);
		}
		//for(std::vector<GroundFixpDef*>::iterator fdefit = _fixpdefs.begin(); fdefit != _fixpdefs.end(); ++fdefit) {
		//	(*defit)->recursiveDelete();
		//	delete(*defit);
		//}
		for(auto cprit = _cpreifications.begin(); cprit != _cpreifications.end(); ++cprit) {
			delete(*cprit);
		}
	}

	void polAdd(const GroundClause& cl) {
		_clauses.push_back(cl);
	}

	void polAdd(int head, AggTsBody* body) {
		_aggregates.push_back(new GroundAggregate(body->aggtype(),body->lower(),body->type(),head,body->setnr(),body->bound()));
	}

	void polAdd(int tseitin, CPTsBody* body) {
		//TODO also add variables (in a separate container?)
		_cpreifications.push_back(new CPReification(tseitin,body));
	}

	void polAdd(const TsSet& tsset, int setnr, bool weighted) {
		_sets.push_back(new GroundSet(setnr,tsset.literals(),tsset.weights()));
	}

	void polAdd(GroundDefinition* d){
		_definitions.insert(std::pair<int, GroundDefinition*>(d->id(), d));
	}

	void polAdd(int defnr, PCGroundRule* rule) {
		if(_definitions.find(defnr)==_definitions.end()){
			_definitions.insert(std::pair<int, GroundDefinition*>(defnr, new GroundDefinition(defnr, _translator)));
		}
		_definitions.at(defnr)->addPCRule(rule->head(), rule->body(), rule->type()==RT_CONJ, rule->recursive());
	}

	void polAdd(int defnr, AggGroundRule* rule) {
		if(_definitions.find(defnr)==_definitions.end()){
			_definitions.insert(std::pair<int, GroundDefinition*>(defnr, new GroundDefinition(defnr, _translator)));
		}
		_definitions.at(defnr)->addAggRule(rule->head(), rule->setnr(), rule->aggtype(), rule->lower(), rule->bound(), rule->recursive());
	}

	std::ostream& polPut(std::ostream& s, GroundTranslator* translator, GroundTermTranslator* termtranslator, bool longnames) const {
		std::cerr << "Printing ground theory\n";
		std::cerr << "Has " << _clauses.size() << " clauses." << "\n";
		for(size_t n = 0; n < _clauses.size(); ++n) {
			if(_clauses[n].empty()) { s << "false"; }
			else {
				for(size_t m = 0; m < _clauses[n].size(); ++m) {
					if(_clauses[n][m] < 0) { s << '~'; }
					s << translator->printAtom(_clauses[n][m],longnames);
					if(m < _clauses[n].size()-1) { s << " | "; }
				}
			}
			s << ". // ";
			for(size_t m = 0; m < _clauses[n].size(); ++m) {
				s << _clauses[n][m] << " ";
			}
			s << "0\n";
		}
		for(size_t n = 0; n < _definitions.size(); ++n) {
			s << _definitions.at(n)->toString(longnames);
		}
		for(size_t n = 0; n < _sets.size(); ++n) {
			s << "Set nr. " << _sets[n]->setnr() << " = [ ";
			for(size_t m = 0; m < _sets[n]->size(); ++m) {
				s << "(" << translator->printAtom(_sets[n]->literal(m),longnames);
				s << " = " << _sets[n]->weight(m) << ")";
				if(m < _sets[n]->size()-1) { s << "; "; }
			}
			s << "].\n";
		}
		for(size_t n = 0; n < _aggregates.size(); ++n) {
			const GroundAggregate* agg = _aggregates[n];
			s << translator->printAtom(agg->head(), longnames) << ' ';
			s << agg->arrow() << ' ';
			s << agg->bound();
			s << (agg->lower() ? " =< " : " >= ");
			s << agg->type() << '(' << agg->setnr() << ")." << "\n";
		}
		//TODO: repeat above for fixpoint definitions
		for(auto it = _cpreifications.begin(); it != _cpreifications.end(); ++it) {
			CPReification* cpr = *it;
			s << translator->printAtom(cpr->_head,longnames) << ' ' << cpr->_body->type() << ' ';
			CPTerm* left = cpr->_body->left();
			if(typeid(*left) == typeid(CPSumTerm)) {
				CPSumTerm* cpt = dynamic_cast<CPSumTerm*>(left);
				s << "sum[ ";
				bool begin = true;
				for(auto vit = cpt->varids().begin(); vit != cpt->varids().end(); ++vit) {
					if(not begin){ s << "; "; }	begin = false;
					s << termtranslator->printTerm(*vit, longnames);
				}
				s << " ]";
			}
			else if(typeid(*left) == typeid(CPWSumTerm)) {
				CPWSumTerm* cpt = dynamic_cast<CPWSumTerm*>(left);
				s << "wsum[ ";
				bool begin = true;
				for(auto vit = cpt->varids().begin(), wit = cpt->weights().begin(); vit != cpt->varids().end() && wit != cpt->weights().end(); ++vit, ++wit) {
					if(not begin){ s << "; "; }	begin = false;
					s << '(' << termtranslator->printTerm(*vit, longnames) << '=' << *wit << ')';
				}
				s << " ]";
			}
			else {
				assert(typeid(*left) == typeid(CPVarTerm));
				CPVarTerm* cpt = dynamic_cast<CPVarTerm*>(left);
				s << termtranslator->printTerm(cpt->varid(), longnames);
			}
			s << ' ' << cpr->_body->comp() << ' ';
			CPBound right = cpr->_body->right();
			if(right._isvarid) { s << termtranslator->printTerm(right._varid, longnames); }
			else { s << right._bound; }
			s << '.' << "\n";
		}
		return s;
	}

	std::string polToString(GroundTranslator* translator, GroundTermTranslator* termtranslator, bool longnames) const {
		std::stringstream s;
		polPut(s, translator, termtranslator, longnames);
		return s.str();
	}
};

#endif /* GROUNDTHEORY_HPP_ */
