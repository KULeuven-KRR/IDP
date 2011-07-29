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

#include "ecnf.hpp"
#include "commontypes.hpp"

#include "ground.hpp"
#include <sstream>

#include <assert.h>

class GroundPolicy {
private:
	std::vector<GroundClause>		_clauses;
	std::vector<GroundDefinition*>	_definitions;
	std::vector<GroundFixpDef*>		_fixpdefs;
	std::vector<GroundSet*>			_sets;
	std::vector<GroundAggregate*>	_aggregates;
	std::vector<CPReification*>		_cpreifications;

public:
	// Inspectors
	unsigned int		nrClauses()						const { return _clauses.size();							}
	unsigned int		nrDefinitions()					const { return _definitions.size();						}
	unsigned int		nrFixpDefs()					const { return _fixpdefs.size();						}
	unsigned int		nrSets()						const { return _sets.size();							}
	unsigned int		nrAggregates()					const { return _aggregates.size();						}
	unsigned int 		nrCPReifications()				const { return _cpreifications.size();					}
	GroundClause		clause(unsigned int n)			const { return _clauses[n];								}
	GroundDefinition*	definition(unsigned int n)		const { return _definitions[n];							}
	GroundFixpDef*		fixpdef(unsigned int n)			const { return _fixpdefs[n];							}
	GroundSet*			set(unsigned int n)				const { return _sets[n];								}
	GroundAggregate*	aggregate(unsigned int n)		const { return _aggregates[n];							}
	CPReification*		cpreification(unsigned int n)	const { return _cpreifications[n];						}

	void polRecursiveDelete() {
		for(auto defit = _definitions.begin(); defit != _definitions.end(); ++defit) {
			(*defit)->recursiveDelete();
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

	void polAddClause(GroundClause& cl) {
		_clauses.push_back(cl);
	}

	void polAddAggregate(int head, AggTsBody* body) {
		_aggregates.push_back(new GroundAggregate(body->aggtype(),body->lower(),body->type(),head,body->setnr(),body->bound()));
	}

	void polAddCPReification(int tseitin, CPTsBody* body) {
		//TODO also add variables (in a separate container?)
		_cpreifications.push_back(new CPReification(tseitin,body));
	}

	void polAddSet(const TsSet& tsset, int setnr, bool weighted) {
		_sets.push_back(new GroundSet(setnr,tsset.literals(),tsset.weights()));
	}

	void polAddPCRule(int defnr, int tseitin, PCTsBody* body, bool recursive) {
		assert(_definitions[defnr]->rule(tseitin) == _definitions[defnr]->end());
		_definitions[defnr]->addPCRule(tseitin,body->body(),body->conj(),recursive);
	}

	void polAddAggRule(int defnr, int tseitin, AggTsBody* body, bool recursive) {
		assert(_definitions[defnr]->rule(tseitin) == _definitions[defnr]->end());
		_definitions[defnr]->addAggRule(tseitin,body->setnr(),body->aggtype(),body->lower(),body->bound(),recursive);
	}

	std::ostream& polPut(std::ostream& s, GroundTranslator* translator, GroundTermTranslator* termtranslator) const {
		for(unsigned int n = 0; n < _clauses.size(); ++n) {
			if(_clauses[n].empty()) {
				s << "false";
			}
			else {
				for(unsigned int m = 0; m < _clauses[n].size(); ++m) {
					if(_clauses[n][m] < 0) s << '~';
					s << translator->printAtom(_clauses[n][m]);
					if(m < _clauses[n].size()-1) s << " | ";
				}
			}
			s << ". // ";
			for(unsigned int m = 0; m < _clauses[n].size(); ++m) {
				s << _clauses[n][m] << " ";
			}
			s << "0\n";
		}
		for(unsigned int n = 0; n < _definitions.size(); ++n) {
			s << _definitions[n]->to_string();
		}
		for(unsigned int n = 0; n < _sets.size(); ++n) {
			s << "Set nr. " << _sets[n]->setnr() << " = [ ";
			for(unsigned int m = 0; m < _sets[n]->size(); ++m) {
				s << "(" << translator->printAtom(_sets[n]->literal(m));
				s << " = " << _sets[n]->weight(m) << ")";
				if(m < _sets[n]->size()-1) s << "; ";
			}
			s << "].\n";
		}
		for(unsigned int n = 0; n < _aggregates.size(); ++n) {
			const GroundAggregate* agg = _aggregates[n];
			s << translator->printAtom(agg->head()) << ' ' << agg->arrow() << ' ' << agg->bound();
			s << (agg->lower() ? " =< " : " >= ");
			s << agg->type() << '(' << agg->setnr() << ")." << "\n";
		}
		//TODO: repeat above for fixpoint definitions
		for(std::vector<CPReification*>::const_iterator it = _cpreifications.begin(); it != _cpreifications.end(); ++it) {
			CPReification* cpr = *it;
			s << translator->printAtom(cpr->_head) << ' ' << cpr->_body->type() << ' ';
			CPTerm* left = cpr->_body->left();
			if(typeid(*left) == typeid(CPSumTerm)) {
				CPSumTerm* cpt = dynamic_cast<CPSumTerm*>(left);
				s << "sum[ ";
				for(std::vector<unsigned int>::const_iterator vit = cpt->_varids.begin(); vit != cpt->_varids.end(); ++vit) {
					s << termtranslator->printTerm(*vit);
					if(vit != cpt->_varids.end()-1) s << "; ";
				}
				s << " ]";
			}
			else if(typeid(*left) == typeid(CPWSumTerm)) {
				CPWSumTerm* cpt = dynamic_cast<CPWSumTerm*>(left);
				std::vector<unsigned int>::const_iterator vit;
				std::vector<int>::const_iterator wit;
				s << "wsum[ ";
				for(vit = cpt->_varids.begin(), wit = cpt->_weights.begin(); vit != cpt->_varids.end() && wit != cpt->_weights.end(); ++vit, ++wit) {
					s << '(' << termtranslator->printTerm(*vit) << '=' << *wit << ')';
					if(vit != cpt->_varids.end()-1) s << "; ";
				}
				s << " ]";
			}
			else {
				assert(typeid(*left) == typeid(CPVarTerm));
				CPVarTerm* cpt = dynamic_cast<CPVarTerm*>(left);
				s << termtranslator->printTerm(cpt->_varid);
			}
			s << ' ' << cpr->_body->comp() << ' ';
			CPBound right = cpr->_body->right();
			if(right._isvarid) s << termtranslator->printTerm(right._varid);
			else s << right._bound;
			s << '.' << "\n";
		}
		return s;
	}

	std::string polTo_string(GroundTranslator* translator, GroundTermTranslator* termtranslator) const {
		std::stringstream s;
		polPut(s, translator, termtranslator);
		return s.str();
	}
};


#endif /* GROUNDTHEORY_HPP_ */
