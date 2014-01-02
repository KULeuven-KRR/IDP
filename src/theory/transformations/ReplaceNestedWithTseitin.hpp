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

#pragma once

#include "visitors/TheoryMutatingVisitor.hpp"

/**
 * Use:
 * First step towards real treatment of functions without grounding them:
 * 	reduce the grounding of definitions etc by replacing preds with functions by new smaller symbols
 * 	and using them everywhere.
 * 	Then adding an equivalence with the original pred, which can be watched on whether it really occurs.
 */
struct ReducedPF {
	PredForm *_origpf, *_newpf;
	std::vector<Term*> _arglist; // NULL if remaining, otherwise a varfree term
	std::vector<Term*> _remainingargs;
	varset _quantvars;

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
	ConstructNewReducedForm(PredForm* pf, const std::set<ReducedPF*>& list, Vocabulary* vocabulary)
			: 	_cantransform(false),
				_reduced(NULL) {
		execute(pf, list, vocabulary);
	}

	bool isReducible() const {
		cerr <<"Can be reduced?" <<(_reduced != NULL?"yes":"no") <<"\n";
		return _reduced != NULL;
	}

	ReducedPF* getResult() {
		Assert(isReducible());
		return _reduced;
	}

private:
	void execute(PredForm* pf, const std::set<ReducedPF*>& list, Vocabulary* vocabulary) {
		cerr <<"Reducing " <<print(pf) <<" to ";
		_reduced = NULL;

		if (pf->symbol()->builtin()) { // TODO handle builtins?
			cerr <<"itself.\n";
			return;
		}

		_reduced = new ReducedPF(pf);
		_cantransform = true;

		pf->accept(this);

		if (not _cantransform || _reduced->_remainingargs.size()==pf->args().size()) {
			delete (_reduced);
			_reduced = NULL;
			cerr <<"itself.\n";
			return;
		}

		std::vector<Sort*> sorts;
		for (auto i = _reduced->_remainingargs.cbegin(); i < _reduced->_remainingargs.cend(); ++i) {
			sorts.push_back((*i)->sort());
		}
		bool identicalfound = false;
		auto manag = FOBDDManager::createManager();
		auto fact = FOBDDFactory(manag, vocabulary);
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
			cerr <<print(_reduced->_newpf) <<".\n";
			return;
		}

		auto newsymbol = new Predicate(sorts);
		vocabulary->add(newsymbol);
		_reduced->_newpf = new PredForm(SIGN::POS, newsymbol, _reduced->_remainingargs, pf->pi());
		cerr <<print(_reduced->_newpf) <<".\n";
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
	virtual void visit(const VarTerm* vt) {
		_reduced->_quantvars.insert(vt->var());
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
	Vocabulary* _vocabulary;
	std::map<PFSymbol*, std::set<ReducedPF*> > _symbol2reduction;
	bool _indefbody;
	bool firstpass;

public:
	// NOTE: changes vocabulary and structure
	Theory* execute(Theory* theory) {
		throw IdpException("Invalid code path"); // Code is bugged
		_vocabulary = theory->vocabulary();
		firstpass = true;
		auto t = theory->accept(this);
		firstpass = false;
		auto result = t->accept(this);
		for (auto i = _symbol2reduction.cbegin(); i != _symbol2reduction.cend(); ++i) {
			for (auto j = i->second.cbegin(); j != i->second.cend(); ++j) {
				// FIXME are they all UNIV quants?
				auto newform = new QuantForm(SIGN::POS, QUANT::UNIV, (*j)->_quantvars,
						new EquivForm(SIGN::POS, (*j)->_newpf, (*j)->_origpf, FormulaParseInfo()), FormulaParseInfo());
				result->add(newform);
			}
		}
		return result;
	}
protected:
	Formula* visit(PredForm* pf) {
		if(firstpass){
			auto& reducedlist = _symbol2reduction[pf->symbol()];
			ConstructNewReducedForm t(pf, reducedlist, _vocabulary);
			if (t.isReducible()) {
				reducedlist.insert(t.getResult());
				return t.getResult()->_newpf;
			} else {
				return pf;
			}
		}else{
			auto listit = _symbol2reduction[pf->symbol()];
			if (listit.size()==0) {
				return pf;
			}
			//Assert(listit->second.size() > 0);
			std::vector<Formula*> subforms;
			for (auto i = listit.cbegin(); i != listit.cend(); ++i) {
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
			if(not _indefbody){
				subforms.push_back(pf);
			}
			return new BoolForm(SIGN::POS, false, subforms, pf->pi());
		}
	}

	Definition* visit(Definition* d) {
		auto saved = _symbol2reduction;
		_symbol2reduction.clear();

		/**
		 * Go over all heads, store in a map from symbol to heads
		 * Go over all literals P(x,y,z) in all bodies
		 *    If P is defined and there is a head P(x2,y2,z2) in the map
		 *    replace P(x,y,z) with P(x,y,z) | (x=x2 & y=y2 & z=z2 & P(x2, y2, z2))
		 *    where any equality is dropped if it is still a var or a domain element
		 */
		for (auto rule : d->rules()) {
			rule->head()->accept(this);
		}

		_indefbody = true;
		for (auto rule : d->rules()) {
			rule->body()->accept(this);
		}
		_indefbody = false;

		_symbol2reduction.insert(saved.cbegin(), saved.cend());

		return d;
	}
};
