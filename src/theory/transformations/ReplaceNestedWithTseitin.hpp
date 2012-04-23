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

#include "visitors/TheoryMutatingVisitor.hpp"
#include "structure/StructureComponents.hpp"

/**
 * Use:
 * 		first step towards real treatment of functions without grounding them:
 * 			reduce the grounding of definitions etc by replacing preds with functions by new smaller symbols
 * 			and using them everywhere.
 * 			Then adding an equivalence with the original pred, which is only necessary if it occurs anywhere else.
 */
struct ReducedPF {
	PredForm* origpf, *newpf;
	std::vector<Term*> arglist; // NULL if remaining, otherwise a varfree term
	std::vector<Term*> remainingargs;

	ReducedPF(PredForm* origpf) :
			origpf(origpf), newpf(NULL) {

	}
};

class ConstructNewReducedForm: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	bool cantransform, varfreeterm;
	ReducedPF* reduced;
public:
	ConstructNewReducedForm() :
			cantransform(false), reduced(NULL) {

	}
	void execute(PredForm* pf, const std::set<ReducedPF*>& list, Vocabulary* vocabulary) {
		reduced = new ReducedPF(pf);
		if (pf->symbol()->builtin()) { // TODO handle builtins?
			delete (reduced);
			reduced = NULL;
			return;
		}
		cantransform = true;
		pf->accept(this);
		if (not cantransform) {
			delete (reduced);
			reduced = NULL;
			return;
		}
		std::vector<Sort*> sorts;
		for (auto i : reduced->remainingargs) {
			sorts.push_back(i->sort());
		}
		bool identicalfound = false;
		auto manag = FOBDDManager();
		auto fact = FOBDDFactory(&manag, vocabulary);
		for (auto i : list) {
			if (i->origpf->symbol() != reduced->origpf->symbol()) {
				continue;
			}
			if (i->remainingargs.size() != reduced->remainingargs.size()) {
				continue;
			}
			auto ibdd = fact.turnIntoBdd(i->origpf);
			auto testbdd = fact.turnIntoBdd(reduced->origpf);
			if (ibdd == testbdd) {
				reduced = i;
				identicalfound = true;
			}
		}
		if (identicalfound) {
			return;
		}
		auto newsymbol = new Predicate(sorts);
		vocabulary->add(newsymbol);
		reduced->newpf = new PredForm(SIGN::POS, newsymbol, reduced->remainingargs, pf->pi());
	}

	bool isReducable() const {
		return reduced != NULL;
	}

	ReducedPF* getResult() {
		Assert(isReducable());
		return reduced;
	}

	virtual void visit(const PredForm* pf) {
		for (auto i : pf->args()) {
			varfreeterm = true;
			i->accept(this);
			if (not cantransform) {
				break;
			}
			if (varfreeterm) {
				reduced->arglist.push_back(i);
			} else {
				reduced->arglist.push_back(NULL);
				reduced->remainingargs.push_back(i);
				Assert(dynamic_cast<VarTerm*>(i) != NULL);
			}
		}
		Assert(reduced->arglist.size() == pf->args().size());
	}
	virtual void visit(const VarTerm*) {
		varfreeterm = false;
	}
	virtual void visit(const FuncTerm* f) {
		traverse(f);
		if (not varfreeterm) {
			cantransform = false; // TODO cannot transform functions which contain nested vars!
		}
	}
	virtual void visit(const DomainTerm*) {
	}
	virtual void visit(const AggTerm*) {
		cantransform = false;
	}
};

/**
 * Replaced an atom with function occurrences with a new propositional symbol and an equivalence.
 * Replace multiple occurrences with the same propositional symbol!
 */
class ReplaceNestedWithTseitinTerm: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	bool visiting;
	Vocabulary* vocabulary;
	std::map<PFSymbol*, std::vector<ReducedPF*> > symbol2reduction;

public:
	// FIXME clone voc and structure before call?
	// NOTE: changes vocabulary and structure
	template<typename T>
	T execute(T t, AbstractStructure* s) {
		visiting = false;
		Assert(s->vocabulary()==t->vocabulary());
		vocabulary = s->vocabulary();
		auto result = t->accept(this);
		s->changeVocabulary(vocabulary);
		return result;
	}
protected:
	Formula* visit(PredForm* pf) {
		if (not visiting) {
			return TheoryMutatingVisitor::visit(pf);
		}
		auto listit = symbol2reduction.find(pf->symbol());
		if (listit == symbol2reduction.cend()) {
			return pf;
		}
		Assert(listit->second.size() > 0);
		std::vector<Formula*> subforms;
		for (auto i : listit->second) {
			Assert(i->arglist.size() == pf->args().size());
			std::vector<Term*> arglist;
			std::vector<Formula*> equalities;
			for (uint j = 0; j < pf->args().size(); ++j) {
				if (i->arglist[j] == NULL) {
					arglist.push_back(pf->args()[j]);
				} else {
					equalities.push_back(new PredForm(SIGN::POS, vocabulary->pred("=/2"), { i->arglist[j], pf->args()[j] }, pf->pi()));
				}
			}
			equalities.push_back(new PredForm(pf->sign(), i->newpf->symbol(), arglist, pf->pi()));
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
		for (auto rule : d->rules()) {
			ConstructNewReducedForm t;
			t.execute(rule->head(), reducedlist, vocabulary);
			if (t.isReducable()) {
				reducedlist.insert(t.getResult());
				newheads.push_back(t.getResult()->newpf);
			} else {
				newheads.push_back(rule->head());
			}
		}

		for (auto i : reducedlist) {
			symbol2reduction[i->origpf->symbol()].push_back(i);
		}

		auto newdef = new Definition();
		int i = 0;
		for (auto rule : d->rules()) {
			visiting = true;
			auto newbody = rule->body()->accept(this);
			visiting = false;
			newdef->add(new Rule(rule->quantVars(), newheads[i], newbody, rule->pi()));
			i++;
		}
		auto result = newdef->clone();
		delete (d);
		// FIXME also add equivalences!!!
		return result;
	}
};

#endif /* REPLACENESTEDWITHTSEITIN_HPP_ */
