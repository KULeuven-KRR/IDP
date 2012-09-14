/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/
#include "Propagator.hpp"

#include "PropagationDomainFactory.hpp"
#include "PropagationScheduler.hpp"
#include "IncludeComponents.hpp"
#include "fobdds/FoBddManager.hpp"
#include "GenerateBDDAccordingToBounds.hpp"
#include "utils/ListUtils.hpp"

using namespace std;

template<class Factory, class DomainType>
TypedFOPropagator<Factory, DomainType>::TypedFOPropagator(Factory* f, FOPropScheduler* s, Options* opts)
		: 	_factory(f),
			_scheduler(s),
			_theory(NULL) {
	_maxsteps = opts->getValue(IntType::NRPROPSTEPS);
	_options = opts;
}

template<class Factory, class DomainType>
TypedFOPropagator<Factory, DomainType>::~TypedFOPropagator() {
	delete (_scheduler);
	deleteList(_admissiblecheckers);
	deleteList(_leafconnectors);
	delete (_factory);
	if (_theory != NULL) {
		_theory->recursiveDelete();
	}
}

template<>
TypedFOPropagator<FOPropBDDDomainFactory, FOPropBDDDomain>::TypedFOPropagator(FOPropBDDDomainFactory* f, FOPropScheduler* s, Options* opts)
		: 	_factory(f),
			_scheduler(s),
			_theory(NULL) {
	_maxsteps = opts->getValue(IntType::NRPROPSTEPS);
	_options = opts;
	if (_options->getValue(IntType::LONGESTBRANCH) != 0) {
		_admissiblecheckers.push_back(new LongestBranchChecker(f->manager(), _options->getValue(IntType::LONGESTBRANCH)));
	}
}

template<class Factory, class DomainType>
void TypedFOPropagator<Factory, DomainType>::doPropagation() {
	if (getOption(IntType::VERBOSE_PROPAGATING) > 1) {
		clog << "=== Start propagation ===\n";
	}
	while (_scheduler->hasNext()) {
		FOPropagation* propagation = _scheduler->next();
		_direction = propagation->getDirection();
		_ct = propagation->isCT();
		_child = propagation->getChild();
		auto parent = propagation->getParent();
		if (getOption(IntType::VERBOSE_PROPAGATING) > 1) {
			clog << "  Propagate ";
			if (_direction == DOWN) {
				clog << "downward from " << (_ct ? "the ct-bound of " : "the cf-bound of ") <<toString(parent);
				if (_child) {
					clog << " to ";
					_child->put(clog);
				}
			} else {
				clog << "upward to " << ((_ct == isPos(parent->sign())) ? "the ct-bound of " : "the cf-bound of ") <<toString(parent);
				if (_child) {
					clog << " from ";
					_child->put(clog);
				}
			}
			clog << "\n";
		}
		parent->accept(this);
		delete (propagation);
	}
	if (getOption(IntType::VERBOSE_PROPAGATING) > 1) {
		clog << "=== End propagation ===\n";
	}
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::applyPropagationToStructure(AbstractStructure* structure) const {
	for (auto it = _leafconnectors.cbegin(); it != _leafconnectors.cend(); ++it) {
		auto connector = it->second;
		PFSymbol* symbol = connector->symbol();
		auto newinter = structure->inter(symbol);
		if (newinter->approxTwoValued()) {
			Assert(getDomain(connector)._twovalued);
			continue;
		}
		if (getOption(IntType::VERBOSE_PROPAGATING) > 1) {
			clog << "**   Applying propagation for " << toString(symbol);
			pushtab();
			clog << nt() << "Old interpretation was" << toString(newinter) << "\n";
		}
		vector<Variable*> vv;
		for (auto jt = connector->subterms().cbegin(); jt != connector->subterms().cend(); ++jt) {
			Assert((*jt)->freeVars().cbegin() != (*jt)->freeVars().cend());
			vv.push_back(*((*jt)->freeVars().cbegin()));
		}
		CHECKTERMINATION;
		Assert(_domains.find(connector) != _domains.cend());

		PredInter* bddinter = _factory->inter(vv, _domains.find(connector)->second, structure);
		if (getOption(IntType::VERBOSE_PROPAGATING) > 1) {
			clog << nt() << "Derived symbols: " << toString(bddinter) << "\n";
		}
		if (newinter->ct()->empty() && newinter->cf()->empty()) {
			bddinter->materialize();
			if (isa<Function>(*symbol)) {
				structure->changeInter(dynamic_cast<Function*>(symbol), new FuncInter(bddinter));
			} else {
				Assert(isa<Predicate>(*symbol));
				structure->changeInter(dynamic_cast<Predicate*>(symbol), bddinter);
			}
		} else {
			for (auto trueEl = bddinter->ct()->begin(); not trueEl.isAtEnd(); ++trueEl) {
				newinter->makeTrue(*trueEl);
			}
			for (auto falseEl = bddinter->cf()->begin(); not falseEl.isAtEnd(); ++falseEl) {
				newinter->makeFalse(*falseEl);
			}
		}
		if (getOption(IntType::VERBOSE_PROPAGATING) > 1) {
			clog << nt() << "Result: " << toString(structure->inter(symbol));
			poptab();
			clog << nt() << "**" << "\n";
		}
	}
}

template<class Factory, class Domain>
GenerateBDDAccordingToBounds* TypedFOPropagator<Factory, Domain>::symbolicstructure() const {
	map<PFSymbol*, vector<const FOBDDVariable*> > vars;
	map<PFSymbol*, const FOBDD*> ctbounds;
	map<PFSymbol*, const FOBDD*> cfbounds;
	Assert(typeid(*_factory) == typeid(FOPropBDDDomainFactory));
	auto manager = (dynamic_cast<FOPropBDDDomainFactory*>(_factory))->manager();
	for (auto it = _leafconnectors.cbegin(); it != _leafconnectors.cend(); ++it) {
		ThreeValuedDomain<Domain> tvd = (_domains.find(it->second))->second;
		ctbounds[it->first] = dynamic_cast<FOPropBDDDomain*>(tvd._ctdomain)->bdd();
		cfbounds[it->first] = dynamic_cast<FOPropBDDDomain*>(tvd._cfdomain)->bdd();
		vector<const FOBDDVariable*> bddvars;
		for (auto jt = it->second->subterms().cbegin(); jt != it->second->subterms().cend(); ++jt) {
			Assert(typeid(*(*jt)) == typeid(VarTerm));
			bddvars.push_back(manager->getVariable((dynamic_cast<VarTerm*>(*jt))->var()));
		}
		vars[it->first] = bddvars;
	}
	return new GenerateBDDAccordingToBounds(manager, ctbounds, cfbounds, vars);
}

template<class Factory, class Domain>
Domain* TypedFOPropagator<Factory, Domain>::addToConjunction(Domain* conjunction, Domain* newconjunct) {
	FOPropDomain* old = conjunction;
	conjunction = _factory->conjunction(conjunction, newconjunct);
	delete (old);
	return conjunction;
}

template<class Factory, class Domain>
Domain* TypedFOPropagator<Factory, Domain>::addToDisjunction(Domain* disjunction, Domain* newdisjunct) {
	FOPropDomain* temp = disjunction;
	disjunction = _factory->disjunction(disjunction, newdisjunct);
	delete (temp);
	return disjunction;
}

template<class Factory, class Domain>
Domain* TypedFOPropagator<Factory, Domain>::addToExists(Domain* exists, const set<Variable*>& qvars) {
	set<Variable*> newqvars;
	map<Variable*, Variable*> mvv;
	for (auto it = qvars.cbegin(); it != qvars.cend(); ++it) {
		Variable* v = new Variable((*it)->name(), (*it)->sort(), (*it)->pi());
		newqvars.insert(v);
		mvv[*it] = v;
	}
	Domain* cl = _factory->substitute(exists, mvv);
	delete (exists);
	Domain* q = _factory->exists(cl, newqvars);
	delete (cl);
	return q;
}

template<class Factory, class Domain>
Domain* TypedFOPropagator<Factory, Domain>::addToForall(Domain* forall, const set<Variable*>& qvars) {
	set<Variable*> newqvars;
	map<Variable*, Variable*> mvv;
	for (auto it = qvars.cbegin(); it != qvars.cend(); ++it) {
		Variable* v = new Variable((*it)->name(), (*it)->sort(), (*it)->pi());
		newqvars.insert(v);
		mvv[*it] = v;
	}
	Domain* cl = _factory->substitute(forall, mvv);
	delete (forall);
	Domain* q = _factory->forall(cl, newqvars);
	delete (cl);
	return q;
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::schedule(const Formula* p, FOPropDirection dir, bool ct, const Formula* c) {
	if (getMaxSteps() <= 0) {
		return;
	}
	_maxsteps--;
	_scheduler->add(new FOPropagation(p, dir, ct, c));
	if (getOption(IntType::VERBOSE_PROPAGATING) > 1) {
		clog << "  Schedule ";
		if (dir == DOWN) {
			clog << "downward propagation from " << (ct ? "the ct-bound of " : "the cf-bound of ") << *p;
			if (c) {
				clog << " towards " << *c;
			}
		} else {
			clog << "upward propagation to " << ((ct == isPos(p->sign())) ? "the ct-bound of " : "the cf-bound of ") << *p;
			if (c) {
				clog << ". Propagation comes from " << *c;
			}
		}
		clog << "\n";
	}
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::setDomain(const Formula* key, const ThreeValuedDomain<Domain>& value) {
	_domains.insert(std::pair<const Formula*, const ThreeValuedDomain<Domain> >(key, value));
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::updateDomain(const Formula* f, FOPropDirection dir, bool ct, Domain* newdomain, const Formula* child) {
	Assert(newdomain!=NULL && f!=NULL && hasDomain(f));
	if (getOption(IntType::VERBOSE_PROPAGATING) > 2) {
		clog << "    Derived the following " << (ct ? "ct " : "cf ") << "domain for " << *f << ":" << nt();
		_factory->put(clog, newdomain);
		clog << nt();
	}
	Domain* olddom = ct ? getDomain(f)._ctdomain : getDomain(f)._cfdomain;
	Domain* newdom = _factory->disjunction(olddom, newdomain);
	//disjunction -> Make the domains larger (thus the unknown part smaller)

	if ((not _factory->approxequals(olddom, newdom)) && admissible(newdom, olddom)) {
		ct ? setCTOfDomain(f, newdom) : setCFOfDomain(f, newdom);
		if (dir == DOWN) {
			for (auto it = f->subformulas().cbegin(); it != f->subformulas().cend(); ++it) {
				//Propagate the newly found domain further down.
				schedule(f, DOWN, ct, *it);
			}
			if (isa<PredForm>(*f)) {
				const PredForm* pf = dynamic_cast<const PredForm*>(f);
				auto it = _leafupward.find(pf);
				if (it != _leafupward.cend()) {
					for (auto jt = it->second.cbegin(); jt != it->second.cend(); ++jt) {
						if (*jt != child) {
							schedule(*jt, UP, ct, f);
						}
					}
				} else if (not pf->symbol()->builtin()) { //TODO: I (Bart) added this condition, is it right?
					Assert(_leafconnectdata.find(pf) != _leafconnectdata.cend());
					schedule(pf, DOWN, ct, 0);
				}
			}
		} else {
			Assert(dir == UP);
			auto it = _upward.find(f);
			if (it != _upward.cend()) {
				schedule(it->second, UP, ct, f);
			}
		}
	} else {
		delete (newdom);
	}

	if (child != NULL && dir == UP) {
		for (auto it = f->subformulas().cbegin(); it != f->subformulas().cend(); ++it) {
			if (*it != child) {
				schedule(f, DOWN, !ct, *it);
			}
		}
	}

	delete (newdomain);
}

template<class Factory, class Domain>
bool TypedFOPropagator<Factory, Domain>::admissible(Domain* newd, Domain* oldd) const {
	for (auto it = _admissiblecheckers.cbegin(); it != _admissiblecheckers.cend(); ++it) {
		if (not ((*it)->check(newd, oldd)))
			return false;
	}
	return true;
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const PredForm* pf) {
	Assert(_leafconnectdata.find(pf) != _leafconnectdata.cend());
	Assert(pf!=NULL);
	auto lcd = _leafconnectdata[pf];
	PredForm* connector = lcd._connector;
	Assert(connector!=NULL);

	Domain* deriveddomain;
	Domain* temp;
	if (_direction == DOWN) {
		if (getDomain(connector)._twovalued) {
			return;
		}
		deriveddomain = _ct ? getDomain(pf)._ctdomain->clone() : getDomain(pf)._cfdomain->clone();
		deriveddomain = _factory->conjunction(deriveddomain, lcd._equalities);
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain, lcd._leaftoconnector);
		delete (temp);
		//TODO?
		/*set<Variable*> freevars;
		 map<Variable*,Variable*> mvv;
		 for(auto it = pf->freeVars().cbegin(); it != pf->freeVars().cend(); ++it) {
		 Variable* clone = new Variable((*it)->sort());
		 freevars.insert(clone);
		 mvv[(*it)] = clone;
		 }
		 temp = deriveddomain;
		 deriveddomain = _factory->substitute(deriveddomain,mvv);
		 delete(temp);
		 deriveddomain = addToExists(deriveddomain,freevars);*/
		updateDomain(connector, DOWN, (_ct == isPos(pf->sign())), deriveddomain, pf);

	} else {
		Assert(_direction == UP);
		if (getDomain(pf)._twovalued) {
			return;
		}
		Assert(_domains.find(connector) != _domains.cend());
		deriveddomain = _ct ? getDomain(connector)._ctdomain->clone() : getDomain(connector)._cfdomain->clone();
		// deriveddomain = _factory->conjunction(deriveddomain,lcd->_equalities);
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain, lcd._connectortoleaf);
		delete (temp);
		/*set<Variable*> freevars;
		 map<Variable*,Variable*> mvv;
		 for(auto it = connector->freeVars().cbegin(); it != connector->freeVars().cend(); ++it) {
		 Variable* clone = new Variable((*it)->sort());
		 freevars.insert(clone);
		 mvv[(*it)] = clone;
		 }
		 temp = deriveddomain;
		 deriveddomain = _factory->substitute(deriveddomain,mvv);
		 delete(temp);
		 deriveddomain = addToExists(deriveddomain,freevars);*/
		updateDomain(pf, UP, (_ct == isPos(pf->sign())), deriveddomain);
	}
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const EqChainForm*) {
	throw notyetimplemented("Creating a propagator for comparison chains");
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const EquivForm* ef) {
	Assert(ef!=NULL && ef->left()!=NULL && ef->right()!=NULL);
//TODO improve: ad hoc method for solving the _child==NULL case. Smarter things can be done.
	if (_child == NULL) {
		_child = ef->left();
		visit(ef);
		_child = ef->right();
		visit(ef);
	}

	Formula* otherchild = (_child == ef->left() ? ef->right() : ef->left());
	const ThreeValuedDomain<Domain>& tvd = getDomain(otherchild);
	switch (_direction) {
	case DOWN: {
		Domain* parentdomain = _ct ? getDomain(ef)._ctdomain : getDomain(ef)._cfdomain;
		Domain* domain1 = _factory->conjunction(parentdomain, tvd._ctdomain);
		Domain* domain2 = _factory->conjunction(parentdomain, tvd._cfdomain);
		const set<Variable*>& qvars = _quantvars[_child];
		domain1 = addToExists(domain1, qvars);
		domain2 = addToExists(domain2, qvars);
		updateDomain(_child, DOWN, (_ct == isPos(ef->sign())), domain1);
		updateDomain(_child, DOWN, (_ct != isPos(ef->sign())), domain2);
		break;
	}
	case UP: {
		Domain* childdomain = _ct ? getDomain(_child)._ctdomain : getDomain(_child)._cfdomain;
		Domain* domain1 = _factory->conjunction(childdomain, tvd._ctdomain);
		Domain* domain2 = _factory->conjunction(childdomain, tvd._cfdomain);
		updateDomain(ef, UP, (_ct == isPos(ef->sign())), domain1, _child);
		updateDomain(ef, UP, (_ct != isPos(ef->sign())), domain2, _child);
		break;
	}
	}
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const BoolForm* bf) {
	Assert(bf!=NULL);
	switch (_direction) {
	case DOWN: {
		Domain* bfdomain = _ct ? getDomain(bf)._ctdomain : getDomain(bf)._cfdomain;
		Assert(bfdomain!=NULL);
		vector<Domain*> subdomains;
		bool longnewdomain = (_ct != (bf->conj() == isPos(bf->sign())));
		if (longnewdomain) {
			for (size_t n = 0; n < bf->subformulas().size(); ++n) {
				const ThreeValuedDomain<Domain>& subfdomain = getDomain(bf->subformulas()[n]);
				subdomains.push_back(_ct == isPos(bf->sign()) ? subfdomain._cfdomain : subfdomain._ctdomain);
			}
		}
		for (size_t n = 0; n < bf->subformulas().size(); ++n) {
			Formula* subform = bf->subformulas()[n];
			Assert(subform!=NULL);
			if (_child == NULL || subform == _child) {
				const ThreeValuedDomain<Domain>& subfdomain = getDomain(subform);
				if (not subfdomain._twovalued) {
					Domain* deriveddomain;
					const set<Variable*>& qvars = _quantvars[subform];
					if (longnewdomain) {
						deriveddomain = bfdomain->clone();
						for (size_t m = 0; m < subdomains.size(); ++m) {
							if (n != m) {
								deriveddomain = addToConjunction(deriveddomain, subdomains[m]);
							}
						}
						deriveddomain = addToExists(deriveddomain, qvars);
					} else {
						deriveddomain = _factory->exists(bfdomain, qvars);
					}
					updateDomain(subform, DOWN, (_ct == isPos(bf->sign())), deriveddomain);
				}
			}
		}
		break;
	}
	case UP: {
		// FIXME fishy code?
		Domain* deriveddomain = NULL;
		if (_ct == bf->conj()) {
			deriveddomain = _factory->trueDomain(bf);
			for (size_t n = 0; n < bf->subformulas().size(); ++n) {
				const ThreeValuedDomain<Domain>& tvd = getDomain(bf->subformulas()[n]);
				deriveddomain = addToConjunction(deriveddomain, (_ct ? tvd._ctdomain : tvd._cfdomain));
			}
		} else {
			// FIXME child can be NULL?
			const ThreeValuedDomain<Domain>& tvd = getDomain(_child);
			deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		}
		updateDomain(bf, UP, (_ct == isPos(bf->sign())), deriveddomain, _child);
		break;
	}
	}
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const QuantForm* qf) {
	Assert(qf!=NULL && qf->subformula()!=NULL);
	switch (_direction) {
	case DOWN: {
		Assert(_domains.find(qf) != _domains.cend());
		const ThreeValuedDomain<Domain>& tvd = getDomain(qf);
		Domain* deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		if (_ct != qf->isUnivWithSign()) {
			set<Variable*> newvars;
			map<Variable*, Variable*> mvv;
			Domain* conjdomain = _factory->trueDomain(qf->subformula());
			vector<CompType> comps(1, CompType::EQ);
			for (auto it = qf->quantVars().cbegin(); it != qf->quantVars().cend(); ++it) {
				Variable* newvar = new Variable((*it)->sort());
				newvars.insert(newvar);
				mvv[*it] = newvar;
				vector<Term*> terms(2);
				terms[0] = new VarTerm((*it), TermParseInfo());
				terms[1] = new VarTerm(newvar, TermParseInfo());
				EqChainForm* ef = new EqChainForm(SIGN::POS, true, terms, comps, FormulaParseInfo());
				Domain* equaldomain = _factory->formuladomain(ef);
				ef->recursiveDelete();
				conjdomain = addToConjunction(conjdomain, equaldomain);
				delete (equaldomain);
			}
			Domain* substdomain = _factory->substitute((_ct ? tvd._cfdomain : tvd._ctdomain), mvv);
			Domain* disjdomain = addToDisjunction(conjdomain, substdomain);
			delete (substdomain);
			Domain* univdomain = addToForall(disjdomain, newvars);
			deriveddomain = addToConjunction(deriveddomain, univdomain);
			delete (univdomain);
		}
		updateDomain(qf->subformula(), DOWN, (_ct == isPos(qf->sign())), deriveddomain);
		break;
	}
	case UP: {
		const ThreeValuedDomain<Domain>& tvd = getDomain(qf->subformula());
		Domain* deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		if (_ct == qf->isUniv()) {
			deriveddomain = addToForall(deriveddomain, qf->quantVars());
		} else {
			deriveddomain = addToExists(deriveddomain, qf->quantVars());
		}
		updateDomain(qf, UP, (_ct == isPos(qf->sign())), deriveddomain);
		break;
	}
	}
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const AggForm*) {
// TODO
}

//The second domain is the old domain. Since the only check is on the length of the longest branch. No need to check this.
bool LongestBranchChecker::check(FOPropBDDDomain* newdomain, FOPropBDDDomain*) const {
	return (_treshhold > _manager->longestbranch(newdomain->bdd()));
}

template class TypedFOPropagator<FOPropBDDDomainFactory, FOPropBDDDomain> ;
