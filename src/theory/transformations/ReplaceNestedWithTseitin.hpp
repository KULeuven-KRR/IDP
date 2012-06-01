/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef REPLACENESTEDWITHTSEITIN_HPP_
#define REPLACENESTEDWITHTSEITIN_HPP_

#ifdef __APPLE__
#include <sys/types.h>
#endif
#include "visitors/TheoryMutatingVisitor.hpp"

/**
 * Use:
 * 		first step towards real treatment of functions without grounding them:
 * 			reduce the grounding of definitions etc by replacing preds with functions by new smaller symbols
 * 			and using them everywhere.
 * 			Then adding an equivalence with the original pred, which is only necessary if it occurs anywhere else.
 */
struct ReducedPF {
	PredForm *_origpf, *_newpf;
	std::vector<Term*> _arglist; // NULL if remaining, otherwise a varfree term
	std::vector<Term*> _remainingargs;

	ReducedPF(PredForm* origpf)
			: 	_origpf(origpf),
				_newpf(NULL) {

	}
};

class ConstructNewReducedForm: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	bool _cantransform, _varfreeterm;
	ReducedPF* _reduced;

public:
	ConstructNewReducedForm()
			: 	_cantransform(false),
				_reduced(NULL) {

	}
	void execute(PredForm* pf, const std::set<ReducedPF*>& list, Vocabulary* vocabulary) {
		_reduced = new ReducedPF(pf);
		if (pf->symbol()->builtin()) { // TODO handle builtins?
			delete (_reduced);
			_reduced = NULL;
			return;
		}
		_cantransform = true;
		pf->accept(this);
		if (not _cantransform) {
			delete (_reduced);
			_reduced = NULL;
			return;
		}
		std::vector<Sort*> sorts;
		for (auto i = _reduced->_remainingargs.cbegin(); i < _reduced->_remainingargs.cend(); ++i) {
			sorts.push_back((*i)->sort());
		}
		bool identicalfound = false;
		auto manag = FOBDDManager();
		auto fact = FOBDDFactory(&manag, vocabulary);
		for (auto i = list.cbegin(); i != list.cend(); ++i) {
			if ((*i)->_origpf->symbol() != _reduced->_origpf->symbol()) {
				continue;
			}
			if ((*i)->_remainingargs.size() != _reduced->_remainingargs.size()) {
				continue;
			}
			auto ibdd = fact.turnIntoBdd((*i)->_origpf);
			auto testbdd = fact.turnIntoBdd(_reduced->_origpf);
			if (ibdd == testbdd) {
				_reduced = *i;
				identicalfound = true;
			}
		}
		if (identicalfound) {
			return;
		}
		auto newsymbol = new Predicate(sorts);
		vocabulary->add(newsymbol);
		_reduced->_newpf = new PredForm(SIGN::POS, newsymbol, _reduced->_remainingargs, pf->pi());
	}

	bool isReducable() const {
		return _reduced != NULL;
	}

	ReducedPF* getResult() {
		Assert(isReducable());
		return _reduced;
	}

	virtual void visit(const PredForm* pf) {
		for (auto i = pf->args().cbegin(); i < pf->args().cend(); ++i) {
			_varfreeterm = true;
			(*i)->accept(this);
			if (not _cantransform) {
				break;
			}
			if (_varfreeterm) {
				_reduced->_arglist.push_back(*i);
			} else {
				_reduced->_arglist.push_back(NULL);
				_reduced->_remainingargs.push_back(*i);
				Assert(dynamic_cast<VarTerm*>(*i) != NULL);
			}
		}
		Assert(_reduced->_arglist.size() == pf->args().size());
	}
	virtual void visit(const VarTerm*) {
		_varfreeterm = false;
	}
	virtual void visit(const FuncTerm* f) {
		traverse(f);
		if (not _varfreeterm) {
			_cantransform = false; // TODO cannot transform functions which contain nested vars!
		}
	}
	virtual void visit(const DomainTerm*) {
	}
	virtual void visit(const AggTerm*) {
		_cantransform = false;
	}
};

/**
 * Replaced an atom with function occurrences with a new propositional symbol and an equivalence.
 * Replace multiple occurrences with the same propositional symbol!
 */
class ReplaceNestedWithTseitinTerm: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	bool _visiting;
	Vocabulary* _vocabulary;
	std::map<PFSymbol*, std::vector<ReducedPF*> > _symbol2reduction;

public:
	// FIXME clone voc and structure before call?
	// NOTE: changes vocabulary and structure
	template<typename T>
	T execute(T t, AbstractStructure* s) {
		_visiting = false;
		Assert(s->vocabulary()==t->vocabulary());
		_vocabulary = s->vocabulary();
		auto result = t->accept(this);
		s->changeVocabulary(_vocabulary);
		return result;
	}
protected:
	Formula* visit(PredForm* pf) {
		if (not _visiting) {
			return TheoryMutatingVisitor::visit(pf);
		}
		auto listit = _symbol2reduction.find(pf->symbol());
		if (listit == _symbol2reduction.cend()) {
			return pf;
		}
		Assert(listit->second.size() > 0);
		std::vector<Formula*> subforms;
		for (auto i = listit->second.cbegin(); i < listit->second.cend(); ++i) {
			Assert((*i)->_arglist.size() == pf->args().size());
			std::vector<Term*> arglist;
			std::vector<Formula*> equalities;
			for (uint j = 0; j < pf->args().size(); ++j) {
				if ((*i)->_arglist[j] == NULL) {
					arglist.push_back(pf->args()[j]);
				} else {
					equalities.push_back(new PredForm(SIGN::POS, get(STDPRED::EQ, pf->args()[j]->sort()), { (*i)->_arglist[j], pf->args()[j] }, pf->pi()));
				}
			}
			equalities.push_back(new PredForm(pf->sign(), (*i)->_newpf->symbol(), arglist, pf->pi()));
			subforms.push_back(new BoolForm(SIGN::POS, true, equalities, pf->pi()));
		}
		return new BoolForm(SIGN::POS, false, subforms, pf->pi());
	}

	Definition* visit(Definition* d) {
		/**
		 * Go over all heads, store in a map from symbol to heads
		 * Go over all literals P(x,y,z) in all bodies
		 * 		If P is defined and there is a head P(x2,y2,z2) in the map
		 * 			replace P(x,y,z) with P(x,y,z) | (x=x2 & y=y2 & z=z2 & P(x2, y2, z2))
		 * 				where any equality is dropped if it is still a var or a domain element
		 */
		std::set<ReducedPF*> reducedlist;
		std::vector<PredForm*> newheads; // NOTE: in order of rule iteration!
		for (auto i = d->rules().cbegin(); i < d->rules().cend(); ++i) {
			ConstructNewReducedForm t;
			t.execute((*i)->head(), reducedlist, _vocabulary);
			if (t.isReducable()) {
				reducedlist.insert(t.getResult());
				newheads.push_back(t.getResult()->_newpf);
			} else {
				newheads.push_back((*i)->head());
			}
		}

		for (auto i = reducedlist.cbegin(); i != reducedlist.cend(); ++i) {
			_symbol2reduction[(*i)->_origpf->symbol()].push_back(*i);
		}

		auto newdef = new Definition();
		int i = 0;
		for (auto j = d->rules().cbegin(); j < d->rules().cend(); ++j) {
			_visiting = true;
			auto newbody = (*j)->body()->accept(this);
			_visiting = false;
			newdef->add(new Rule((*j)->quantVars(), newheads[i], newbody, (*j)->pi()));
			i++;
		}
		auto result = newdef->clone();
		delete (d);
		// FIXME also add equivalences!!!
		return result;
	}
};

#endif /* REPLACENESTEDWITHTSEITIN_HPP_ */
