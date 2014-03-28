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

#include "Propagator.hpp"
#include "PropagationDomainFactory.hpp"
#include "PropagationScheduler.hpp"
#include "IncludeComponents.hpp"
#include "structure/StructureComponents.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBdd.hpp"
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
	for(auto l: _leafconnectors){
		l.second->recursiveDelete();
	}
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
		CHECKTERMINATION;
		auto propagation = _scheduler->next();
		_direction = propagation->getDirection();
		_ct = propagation->isCT();
		_child = propagation->getChild();
		auto parent = propagation->getParent();
		if (getOption(IntType::VERBOSE_PROPAGATING) > 1) {
			clog << "  Propagate ";
			if (_direction == DOWN) {
				clog << "downward from " << (_ct ? "the ct-bound of " : "the cf-bound of ") <<print(parent);
				if (_child) {
					clog << " to ";
					_child->put(clog);
				}
			} else {
				clog << "upward to " << ((_ct == isPos(parent->sign())) ? "the ct-bound of " : "the cf-bound of ") <<print(parent);
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
void TypedFOPropagator<Factory, Domain>::applyPropagationToStructure(Structure* structure, const Vocabulary& outputvoc) const {
	for (auto it = _leafconnectors.cbegin(); it != _leafconnectors.cend(); ++it) {
		auto connector = it->second;
		auto symbol = connector->symbol();

		if(not outputvoc.contains(symbol)){
			continue;
		}

		auto newinter = structure->inter(symbol);
		if (newinter->approxTwoValued()) {
			Assert(getDomain(connector)._twovalued);
			continue;
		}
		if (getOption(IntType::VERBOSE_PROPAGATING) > 1) {
			clog << "**   Applying propagation for " << print(symbol);
			pushtab();
			clog << nt() << "Old interpretation was" << print(newinter) << "\n";
		}

		vector<Variable*> vv;
		Assert(connector->subterms().size()==symbol->nrSorts());
		for (auto term : connector->subterms()) {
			Assert(term->type()!=TermType::AGG && term->type()!=TermType::FUNC && term->type()!=TermType::DOM);
			Assert(not term->freeVars().empty());
			vv.push_back(*(term->freeVars().cbegin()));
		}
		CHECKTERMINATION;
		Assert(_domains.find(connector) != _domains.cend());

		PredInter* bddinter = _factory->inter(vv, _domains.find(connector)->second, structure);
		if (getOption(IntType::VERBOSE_PROPAGATING) > 1) {
			if (getOption(IntType::VERBOSE_PROPAGATING) > 3) {
				clog << nt() << "The used BDDs are:";
				clog << nt() << "CT: ";
				auto cttable = dynamic_cast<BDDInternalPredTable*>(bddinter->ct()->internTable());
				if(cttable!=NULL){
					auto ctbdd = cttable->bdd();
					clog << nt() << print(ctbdd);
				}
				clog << nt() << "CF: ";
				auto cftable = dynamic_cast<BDDInternalPredTable*>(bddinter->cf()->internTable());
				if(cftable!=NULL){
					auto cfbdd = cftable->bdd();
					clog << nt() << print(cfbdd);
				}
			}
			clog << nt() << "Derived symbols: " << print(bddinter) << "\n";
		}
		if (newinter->ct()->empty() && newinter->cf()->empty()) {
			bddinter->materialize();
			if (isa<Function>(*symbol)) {
				structure->changeInter(dynamic_cast<Function*>(symbol), new FuncInter(bddinter));
			} else {
				Assert(isa<Predicate>(*symbol));
				structure->changeInter(dynamic_cast<Predicate*>(symbol), bddinter);
			}
			// both reuse the original bddinter, so should not delete it!
		} else {
			for (auto trueEl = bddinter->ct()->begin(); not trueEl.isAtEnd(); ++trueEl) {
				newinter->makeTrueAtLeast(*trueEl);
			}
			for (auto falseEl = bddinter->cf()->begin(); not falseEl.isAtEnd(); ++falseEl) {
				newinter->makeFalseAtLeast(*falseEl);
			}
			delete(bddinter);
		}
		if (getOption(IntType::VERBOSE_PROPAGATING) > 1) {
			clog << nt() << "Result: " << print(structure->inter(symbol));
			poptab();
			clog << nt() << "**" << "\n";
		}
	}
}

template<class Factory, class Domain>
std::shared_ptr<GenerateBDDAccordingToBounds> TypedFOPropagator<Factory, Domain>::symbolicstructure(Vocabulary* symbolsThatCannotBeReplacedByBDDs) const {
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
	return make_shared<GenerateBDDAccordingToBounds>(manager, ctbounds, cfbounds, vars,symbolsThatCannotBeReplacedByBDDs);
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
Domain* TypedFOPropagator<Factory, Domain>::addToExists(Domain* exists, Variable* var) {
	Variable* v = new Variable(var->name(), var->sort(), var->pi());
	map<Variable*, Variable*> mvv;
	mvv[var] = v;

	auto cl = _factory->substitute(exists, mvv);
	delete (exists);
	auto q = _factory->exists(cl, {v});
	delete (cl);
	return q;
}

template<class Factory, class Domain>
Domain* TypedFOPropagator<Factory, Domain>::addToForall(Domain* forall, Variable* var) {
	Variable* v = new Variable(var->name(), var->sort(), var->pi());
	map<Variable*, Variable*> mvv;
	mvv[var] = v;
	auto cl = _factory->substitute(forall, mvv);
	delete (forall);
	auto q = _factory->forall(cl, {v});
	delete (cl);
	return q;
}

/**
 * Returns a false domain with as vars the free variables of  \forall qvars: orig
 */
template<class Factory, class Domain>
Domain* falseDomain(Domain* orig, const varset& qvars, Factory* factory) {
	auto origdomainvars = orig->vars();
	vector<Variable*> domainvars;
	for (auto domainvar : origdomainvars) {
		if (not contains(qvars, domainvar)) {
			domainvars.push_back(domainvar);
		}
	}
	delete orig;
	return factory->falseDomain(domainvars);
}

/**
 * Returns a domain that is either
 * * Equal to the domain \exists qvars: orig
 * * Or, in case that the previous domain is inadmissible, a false domain with as vars the free variables of  \exists qvars: orig
 */
template<class Factory, class Domain>
Domain* TypedFOPropagator<Factory, Domain>::addToExists(Domain* orig, const varset& qvars) {
	//We quantify the variables one by one. As soon as the domain becomes unadmissible, we return the falsedomain (i.e. the domain that derives nothing)
	for (auto var : qvars) {
		if (not admissible(orig, NULL)) {
			return falseDomain(orig, qvars, _factory);
		}
		orig = addToExists(orig, var);
	}
	return orig;
}

/**
 * Returns a domain that is either
 * * Equal to the domain \forall qvars: orig
 * * Or, in case that the previous domain is inadmissible, a false domain with as vars the free variables of  \forall qvars: orig
 */
template<class Factory, class Domain>
Domain* TypedFOPropagator<Factory, Domain>::addToForall(Domain* orig, const varset& qvars) {
	//We quantify the variables one by one. As soon as the domain becomes unadmissible, we return the falsedomain (i.e. the domain that derives nothing)
	for (auto var : qvars) {
		if (not admissible(orig, NULL)) {
			return falseDomain(orig, qvars, _factory);
		}
		orig = addToForall(orig, var);
	}
	return orig;
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
	if(_domains.find(key) != _domains.cend()){
		//The erase is needed since the "insert" method does not update. If the key is already present, insert does nothing!
		//Using the operator[] is not possible since it requires a default constructor.
		_domains.erase(key);
	}
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

	// If we were propagating UP, this means that child has changed, and hence extra information can be
	// propagated from parent to the other children.
	if (child != NULL && dir == UP) {
		for (auto subf : f->subformulas()) {
			if (subf != child) {
				schedule(f, DOWN, not ct, subf);
			}
		}
	}

	//In case actual propagation took place (the domain changed),
	//extra propagation in the same direction can be done.
	if (_factory->approxequals(olddom, newdom) || not admissible(newdom, olddom)) {
		delete (newdom);
		delete (newdomain);
		return;
	}
	ct ? setCTOfDomain(f, newdom) : setCFOfDomain(f, newdom);
	switch (dir) {
	case DOWN: {
		for (auto subf : f->subformulas()) {
			//Propagate the newly found domain further down.
			schedule(f, DOWN, ct, subf);
		}
		if (isa<PredForm>(*f)) {
			auto pf = dynamic_cast<const PredForm*>(f);
			auto it = _leafupward.find(pf);
			if (it != _leafupward.cend()) {
				for (auto formula : it->second) {
					if (formula != child) {
						schedule(formula, UP, ct, f);
					}
				}
			} else if (not pf->symbol()->builtin()) { //TODO: I (Bart) added this condition, is it right?
				Assert(_leafconnectdata.find(pf) != _leafconnectdata.cend());
				schedule(pf, DOWN, ct, 0);
			}
		}
		break;
	}
	case UP: {
		auto it = _upward.find(f);
		if (it != _upward.cend()) {
			schedule(it->second, UP, ct, f);
		}
		break;
	}
	}

	delete (newdomain);
}

template<class Factory, class Domain>
bool TypedFOPropagator<Factory, Domain>::admissible(Domain* newd, Domain* oldd) const {
	for (auto it = _admissiblecheckers.cbegin(); it != _admissiblecheckers.cend(); ++it) {
		if (not ((*it)->check(newd, oldd))){
			return false;
		}
	}
	return true;
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const PredForm* pf) {
	if(_leafconnectdata.find(pf) == _leafconnectdata.cend()){
		//Normally, this does not happen since we don't schedule predform-level up/down propagation for builtins.
		//However, in case the builtin is a topformula, we will once schedule down-propagation from this.
		Assert(pf->symbol()->builtin());
		return;
	}
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
		updateDomain(connector, DOWN, (_ct == isPos(pf->sign())), deriveddomain, pf);

	} else {
		Assert(_direction == UP);
		if (getDomain(pf)._twovalued) {
			return;
		}
		Assert(_domains.find(connector) != _domains.cend());
		deriveddomain = _ct ? getDomain(connector)._ctdomain->clone() : getDomain(connector)._cfdomain->clone();
		deriveddomain = _factory->substitute(deriveddomain, lcd._connectortoleaf);
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
		const auto& qvars = _quantvars[_child];
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
					const auto& qvars = _quantvars[subform];
					if (longnewdomain) {
						deriveddomain = bfdomain->clone();
						for (size_t m = 0; m < subdomains.size(); ++m) {
							if (n != m) {
								deriveddomain = addToConjunction(deriveddomain, subdomains[m]);
							}
						}
						deriveddomain = addToExists(deriveddomain, qvars);
					} else {
						//Clone because addToExists deletes old domains and the superformula still needs its domain.
						deriveddomain = addToExists(bfdomain->clone(), qvars);
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
			// if exists and ct: if !xxx: (PARENT(xxx) & (!xxx': xxx\neq xxx' => ~CHILD(xxx'))) => CHILD(xxx)
			// Note: this turned out to be much to expensive, so disabled by default
			if(not getOption(EXISTS_ONLYONELEFT_APPROX)){
				break;
			}
			varset newvars;
			map<Variable*, Variable*> mvv;
			Domain* conjdomain = _factory->trueDomain(qf->subformula());
			for (auto var : qf->quantVars()) {
				auto newvar = new Variable(var->sort());
				newvars.insert(newvar);
				mvv[var] = newvar;
				auto ef = new EqChainForm(SIGN::POS, true, {new VarTerm((var), TermParseInfo()), new VarTerm(newvar, TermParseInfo())}, {CompType::EQ}, FormulaParseInfo());
				Domain* equaldomain = _factory->formuladomain(ef);
				ef->recursiveDelete();
				conjdomain = addToConjunction(conjdomain, equaldomain);
				delete (equaldomain);
			}
			auto substdomain = _factory->substitute((_ct ? tvd._cfdomain : tvd._ctdomain), mvv);
			auto univdomain = addToForall(addToDisjunction(conjdomain, substdomain), newvars);
			deriveddomain = addToConjunction(deriveddomain, univdomain);

			delete (substdomain);
			delete (univdomain);
			for(auto v:qf->quantVars()){
				//If subformula does not depend on one of those variables, it suffices that the derived domain is t
				//true for one of them.
				if(not contains(qf->subformula()->freeVars(),v)){
					deriveddomain = addToExists(deriveddomain,v);
				}
			}
		}
		updateDomain(qf->subformula(), DOWN, (_ct == isPos(qf->sign())), deriveddomain);
		break;
	}
	case UP: {
		const auto& tvd = getDomain(qf->subformula());
		auto deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
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
