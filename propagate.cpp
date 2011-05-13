/************************************
	propagate.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "propagate.hpp"
#include "fobdd.hpp"
#include "vocabulary.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "structure.hpp"

using namespace std;

/****************************
	Propagation scheduler
****************************/

void FOPropScheduler::add(FOPropagation* propagation) {
	_queue.push(propagation);
}

bool FOPropScheduler::hasNext() const {
	return !_queue.empty();
}

FOPropagation* FOPropScheduler::next() {
	FOPropagation* propagation = _queue.front();
	_queue.pop();
	return propagation;
}

/**************************
	Propagation domains
**************************/


FOPropBDDDomain* FOPropBDDDomainFactory::trueDomain(const Formula* ) const {
	FOBDD* bdd = _manager->truebdd();
	return new FOPropBDDDomain(bdd);
}

FOPropBDDDomain* FOPropBDDDomainFactory::falseDomain(const Formula* ) const {
	FOBDD* bdd = _manager->falsebdd();
	return new FOPropBDDDomain(bdd);
}

FOPropBDDDomain* FOPropBDDDomainFactory::ctDomain(const PredForm*) const {
	//TODO
	assert(false);
}

FOPropBDDDomain* FOPropBDDDomainFactory::cfDomain(const PredForm*) const {
	//TODO
	assert(false);
}

FOPropBDDDomain* FOPropBDDDomainFactory::domain(const PredForm* pf) const {
	FOBDDFactory bddfactory(_manager);
	bddfactory.visit(pf);
	return new FOPropBDDDomain(bddfactory.bdd());
}

FOPropBDDDomain* FOPropBDDDomainFactory::domain(const EqChainForm* ef) const {
	FOBDDFactory bddfactory(_manager);
	bddfactory.visit(ef);
	return new FOPropBDDDomain(bddfactory.bdd());
}

FOPropBDDDomain* FOPropBDDDomainFactory::exists(FOPropDomain* domain, const set<Variable*>& qvars) const {
	FOPropBDDDomain* bdddomain = dynamic_cast<FOPropBDDDomain*>(domain);
	set<FOBDDVariable*> bddqvars = _manager->getVariables(qvars);
	FOBDD* qbdd = _manager->existsquantify(bddqvars,bdddomain->bdd());
	return new FOPropBDDDomain(qbdd);
}

FOPropBDDDomain* FOPropBDDDomainFactory::forall(FOPropDomain* domain, const std::set<Variable*>& qvars) const {
	FOPropBDDDomain* bdddomain = dynamic_cast<FOPropBDDDomain*>(domain);
	set<FOBDDVariable*> bddqvars = _manager->getVariables(qvars);
	FOBDD* qbdd = _manager->univquantify(bddqvars,bdddomain->bdd());
	return new FOPropBDDDomain(qbdd);
}

FOPropBDDDomain* FOPropBDDDomainFactory::conjunction(FOPropDomain* dom1,FOPropDomain* dom2) const {
	FOPropBDDDomain* bdddomain1 = dynamic_cast<FOPropBDDDomain*>(dom1);
	FOPropBDDDomain* bdddomain2 = dynamic_cast<FOPropBDDDomain*>(dom2);
	FOBDD* conjbdd = _manager->conjunction(bdddomain1->bdd(),bdddomain2->bdd());
	return new FOPropBDDDomain(conjbdd);
}

FOPropBDDDomain* FOPropBDDDomainFactory::disjunction(FOPropDomain* dom1,FOPropDomain* dom2) const {
	FOPropBDDDomain* bdddomain1 = dynamic_cast<FOPropBDDDomain*>(dom1);
	FOPropBDDDomain* bdddomain2 = dynamic_cast<FOPropBDDDomain*>(dom2);
	FOBDD* disjbdd = _manager->disjunction(bdddomain1->bdd(),bdddomain2->bdd());
	return new FOPropBDDDomain(disjbdd);
}

FOPropBDDDomain* FOPropBDDDomainFactory::substitute(FOPropDomain* domain,const map<Variable*,Variable*>& mvv) const {
	FOPropBDDDomain* bdddomain = dynamic_cast<FOPropBDDDomain*>(domain);
	map<FOBDDVariable*,FOBDDVariable*> newmvv;
	for(map<Variable*,Variable*>::const_iterator it = mvv.begin(); it != mvv.end(); ++it) {
		FOBDDVariable* oldvar = _manager->getVariable(it->first);
		FOBDDVariable* newvar = _manager->getVariable(it->second);
		newmvv[oldvar] = newvar;
	}
	FOBDD* substbdd = _manager->substitute(bdddomain->bdd(),newmvv);
	return new FOPropBDDDomain(substbdd);
}

bool FOPropBDDDomainFactory::equals(FOPropBDDDomain* dom1, FOPropBDDDomain* dom2) const {
	FOPropBDDDomain* bdddomain1 = dynamic_cast<FOPropBDDDomain*>(dom1);
	FOPropBDDDomain* bdddomain2 = dynamic_cast<FOPropBDDDomain*>(dom2);
	return bdddomain1->bdd() == bdddomain2->bdd();
}

/*****************
	Propagator
*****************/

ThreeValuedDomain::ThreeValuedDomain(const FOPropDomainFactory* factory, bool ctdom, bool cfdom, const Formula* f) {
	_ctdomain = ctdom ? factory->trueDomain(f) : factory->falseDomain(f);
	_cfdomain = cfdom ? factory->trueDomain(f) : factory->falseDomain(f);
	_twovalued = ctdom || cfdom;
}

ThreeValuedDomain::ThreeValuedDomain(const FOPropDomainFactory* factory, PredForm* pf, bool twovalued) {
	if(twovalued) {
		_ctdomain = factory->domain(pf);
		PredForm* negpf = pf->clone(); negpf->swapsign();
		_cfdomain = factory->domain(negpf);
	}
	else {
		_ctdomain = factory->ctDomain(pf);
		_cfdomain = factory->cfDomain(pf);
	}
	_twovalued = twovalued;
}

void FOPropagator::run() {
	while(_scheduler->hasNext()) {
		FOPropagation* propagation = _scheduler->next();
		_direction = propagation->_direction;
		_ct = propagation->_ct;
		_child = propagation->_child;
		propagation->_parent->accept(this);
		delete(propagation);
	}
}

FOPropDomain* FOPropagator::addToConjunction(FOPropDomain* conjunction, FOPropDomain* newconjunct) {
	FOPropDomain* temp = conjunction;
	conjunction = _factory->conjunction(conjunction,newconjunct);
	delete(temp);
	return conjunction;
}

FOPropDomain* FOPropagator::addToDisjunction(FOPropDomain* disjunction, FOPropDomain* newdisjunct) {
	FOPropDomain* temp = disjunction;
	disjunction = _factory->disjunction(disjunction,newdisjunct);
	delete(temp);
	return disjunction;
}

FOPropDomain* FOPropagator::addToExists(FOPropDomain* exists, const set<Variable*>& qvars) {
	FOPropDomain* temp = exists;
	exists = _factory->exists(exists,qvars);
	delete(temp);
	return exists;
}

FOPropDomain* FOPropagator::addToForall(FOPropDomain* forall, const set<Variable*>& qvars) {
	FOPropDomain* temp = forall;
	forall = _factory->forall(forall,qvars);
	delete(temp);
	return forall;
}

void FOPropagator::updateDomain(const Formula* f, FOPropDirection dir, bool ct,FOPropDomain* newdomain, const Formula* child) {
	FOPropDomain* olddom = ct ? _domains[f]._ctdomain : _domains[f]._cfdomain;
	FOPropDomain* newdom = _factory->disjunction(olddom,newdomain);
	if((!_factory->equals(olddom,newdom)) && admissible(newdom,olddom)) {
		ct ? _domains[f]._ctdomain = newdom : _domains[f]._cfdomain = newdom;
		if(dir == DOWN) {
			for(vector<Formula*>::const_iterator it = f->subformulas().begin(); it != f->subformulas().end(); ++it) {
				schedule(f,DOWN,ct,*it);
			}
			if(typeid(*f) == typeid(PredForm)) {
				// TODO propagation to leafconnectors...
			}
		}
		else {
			// TODO schedule propagations
		}
	}
	else delete(newdom);
	if(child) {
		assert(dir == UP);
		for(vector<Formula*>::const_iterator it = f->subformulas().begin(); it != f->subformulas().end(); ++it) {
			if(*it != child) schedule(f,DOWN,ct,*it);
		}
	}
	delete(newdomain);
}

bool FOPropagator::admissible(FOPropDomain*, FOPropDomain* ) const {
	// TODO
	return true;
}

void FOPropagator::visit(const PredForm* pf) {
	LeafConnectData* lcd = _leafconnectdata[pf];
	PredForm* connector = lcd->_connector;
	FOPropDomain* deriveddomain;
	FOPropDomain* temp;
	if(_direction == DOWN) {
		deriveddomain = _ct ? _domains[pf]._ctdomain : _domains[pf]._cfdomain;
		deriveddomain = _factory->conjunction(deriveddomain,lcd->_equalities);
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,lcd->_leaftoconnector);
		delete(temp);
		set<Variable*> freevars;
		map<Variable*,Variable*> mvv;
		for(set<Variable*>::const_iterator it = pf->freevars().begin(); it != pf->freevars().end(); ++it) {
			Variable* clone = new Variable((*it)->sort());
			freevars.insert(clone);
			mvv[(*it)] = clone;
		}
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,mvv);
		delete(temp);
		deriveddomain = addToExists(deriveddomain,freevars);
		updateDomain(connector,DOWN,(_ct == pf->sign()),deriveddomain,pf);
	}
	else {
		assert(_direction == UP);
		deriveddomain = _ct ? _domains[connector]._ctdomain : _domains[connector]._cfdomain;
		deriveddomain = _factory->conjunction(deriveddomain,lcd->_equalities);
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,lcd->_connectortoleaf);
		delete(temp);
		set<Variable*> freevars;
		map<Variable*,Variable*> mvv;
		for(set<Variable*>::const_iterator it = connector->freevars().begin(); it != connector->freevars().end(); ++it) {
			Variable* clone = new Variable((*it)->sort());
			freevars.insert(clone);
			mvv[(*it)] = clone;
		}
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,mvv);
		delete(temp);
		deriveddomain = addToExists(deriveddomain,freevars);
		updateDomain(pf,UP,(_ct == pf->sign()),deriveddomain);
	}
}

void FOPropagator::visit(const EqChainForm*) {
	if(_direction == DOWN) {
		//TODO
	}
	else {
		assert(_direction == UP);
		//TODO
	}
}

void FOPropagator::visit(const EquivForm* ef) {
	Formula* otherchild = _child == ef->left() ? ef->right() : ef->left();
	if(_direction == DOWN) {
		const ThreeValuedDomain& tvd = _domains[_child];
		FOPropDomain* parentdomain = _ct ? _domains[ef]._ctdomain : _domains[ef]._cfdomain;
		FOPropDomain* domain1 = _factory->conjunction(parentdomain,tvd._ctdomain);
		FOPropDomain* domain2 = _factory->conjunction(parentdomain,tvd._cfdomain);
		const set<Variable*>& qvars = _quantvars[_child];
		domain1 = addToExists(domain1,qvars);
		domain2 = addToExists(domain2,qvars);
		updateDomain(otherchild,DOWN,(_ct == ef->sign()),domain1);
		updateDomain(otherchild,DOWN,(_ct != ef->sign()),domain2);
	}
	else {
		assert(_direction == UP);
		const ThreeValuedDomain& tvd = _domains[otherchild];
		FOPropDomain* childdomain = _ct ? _domains[_child]._ctdomain : _domains[_child]._cfdomain;
		FOPropDomain* domain1 = _factory->conjunction(childdomain,tvd._ctdomain);
		FOPropDomain* domain2 = _factory->conjunction(childdomain,tvd._cfdomain);
		updateDomain(ef,UP,(_ct == ef->sign()),domain1,_child);
		updateDomain(ef,UP,(_ct != ef->sign()),domain2,_child);
	}
}

void FOPropagator::visit(const BoolForm* bf) {
	if(_direction == DOWN) {
		FOPropDomain* bfdomain = _ct ? _domains[bf]._ctdomain : _domains[bf]._cfdomain;
		vector<FOPropDomain*> subdomains;
		bool longnewdomain = (_ct != (bf->conj() == bf->sign()));
		if(longnewdomain) {
			for(unsigned int n = 0; n < bf->subformulas().size(); ++n) {
				const ThreeValuedDomain& subfdomain = _domains[bf->subformulas()[n]];
				subdomains.push_back(_ct == bf->sign() ? subfdomain._cfdomain : subfdomain._ctdomain);
			}
		}
		for(unsigned int n = 0; n < bf->subformulas().size(); ++n) {
			Formula* subform = bf->subformulas()[n];
			if(subform != _child) {
				const ThreeValuedDomain& subfdomain = _domains[subform];
				if(!subfdomain._twovalued) {
					FOPropDomain* deriveddomain;
					const set<Variable*>& qvars = _quantvars[subform];
					if(longnewdomain) {
						deriveddomain = bfdomain->clone();
						for(unsigned int m = 0; n < subdomains.size(); ++m) {
							if(n != m) deriveddomain = addToConjunction(deriveddomain,subdomains[m]);
						}
						deriveddomain = addToExists(deriveddomain,qvars);
					}
					else deriveddomain = _factory->exists(bfdomain,qvars);
					updateDomain(subform,DOWN,(_ct == bf->sign()),deriveddomain);
				}
			}
		}
	}
	else {
		assert(_direction == UP);
		FOPropDomain* deriveddomain;
		if(_ct == bf->conj()) {
			deriveddomain = _factory->trueDomain(bf);
			for(unsigned int n = 0; n < bf->subformulas().size(); ++n) {
				const ThreeValuedDomain& tvd = _domains[bf->subformulas()[n]];
				deriveddomain = addToConjunction(deriveddomain,(_ct ? tvd._ctdomain : tvd._cfdomain));
			}
		}
		else {
			const ThreeValuedDomain& tvd = _domains[_child];
			deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		}
		updateDomain(bf,UP,(_ct == bf->sign()),deriveddomain,_child);
	}
}

void FOPropagator::visit(const QuantForm* qf) {
	if(_direction == DOWN) {
		const ThreeValuedDomain& tvd = _domains[qf];
		FOPropDomain* deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		if(_ct != (qf->univ() == qf->sign())) {
			set<Variable*> newvars;
			map<Variable*,Variable*> mvv;
			FOPropDomain* conjdomain = _factory->trueDomain(qf->subf());
			vector<CompType> comps(1,CT_EQ);
			for(set<Variable*>::const_iterator it = qf->quantvars().begin(); it != qf->quantvars().end(); ++it) {
				Variable* newvar = new Variable((*it)->sort());
				newvars.insert(newvar);
				mvv[*it] = newvar;
				vector<Term*> terms(2); 
				terms[0] = new VarTerm((*it),TermParseInfo()); 
				terms[1] = new VarTerm(newvar,TermParseInfo());
				EqChainForm* ef = new EqChainForm(true,true,terms,comps,FormulaParseInfo());
				FOPropDomain* equaldomain = _factory->domain(ef); ef->recursiveDelete();
				conjdomain = addToConjunction(conjdomain,equaldomain); delete(equaldomain);
			}
			FOPropDomain* substdomain = _factory->substitute((_ct ? tvd._cfdomain : tvd._ctdomain),mvv);
			FOPropDomain* disjdomain = addToDisjunction(conjdomain,substdomain); delete(substdomain);
			FOPropDomain* univdomain = addToForall(disjdomain,newvars);
			deriveddomain = addToConjunction(deriveddomain,univdomain); delete(univdomain);
		}
		updateDomain(qf->subf(),DOWN,(_ct == qf->sign()),deriveddomain);
	}
	else {
		assert(_direction == UP);
		const ThreeValuedDomain& tvd = _domains[qf->subf()];
		FOPropDomain* deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		if(_ct == qf->univ()) deriveddomain = addToForall(deriveddomain,qf->quantvars());
		else deriveddomain = addToExists(deriveddomain,qf->quantvars());
		updateDomain(qf,UP,(_ct == qf->sign()),deriveddomain);
	}
}

void FOPropagator::visit(const AggForm*) {
	//TODO
}

/**************************
	FOPropagatorFactory
**************************/

FOPropagatorFactory::FOPropagatorFactory(FOPropDomainFactory* factory, FOPropScheduler* scheduler, const AbstractStructure* s): _structure(s) {
	_propagator = new FOPropagator(factory, scheduler);
}

FOPropagator* FOPropagatorFactory::create(const AbstractTheory* theory) {
	_assertsentences = true;
	theory->accept(this);
	return _propagator;
}

void FOPropagatorFactory::visit(const Theory* theory) {
	for(unsigned int n = 0; n < theory->sentences().size(); ++n) {
		Formula* sentence = theory->sentences()[n];
		if(_assertsentences) {
			_propagator->_scheduler->add(new FOPropagation(sentence,DOWN,true));
			_propagator->_domains[sentence] = ThreeValuedDomain(_propagator->_factory,true,false,sentence);
		}
		sentence->accept(this);
	}
}

void FOPropagatorFactory::initFalse(const Formula* f) {
	if(_propagator->_domains.find(f) == _propagator->_domains.end())
		_propagator->_domains[f] = ThreeValuedDomain(_propagator->_factory,false,false,f);
}

void FOPropagatorFactory::visit(const PredForm* pf) {
	initFalse(pf);
	PFSymbol* symbol = pf->symbol();
	if(_leafconnectors.find(symbol) == _leafconnectors.end()) {
		vector<Variable*> vars = VarUtils::makeNewVariables(symbol->sorts());
		vector<Term*> args = TermUtils::makeNewVarTerms(vars);
		PredForm* leafconnector = new PredForm(true,symbol,args,FormulaParseInfo());
		_leafconnectors[symbol] = leafconnector;
		if(symbol->builtin() || (_structure && _structure->inter(symbol)->approxtwovalued())) {
			_propagator->_domains[leafconnector] = ThreeValuedDomain(_propagator->_factory,leafconnector,true,leafconnector);
		}
		else {
			initFalse(leafconnector);
			//FIXME replace previous line by following:
			//_propagator->_domains[leafconnector] = ThreeValuedDomain(_propagator->_factory,leafconnector,false);
		}
		_propagator->_scheduler->add(new FOPropagation(leafconnector,UP,true));	// FIXME: schedule parents!
		_propagator->_scheduler->add(new FOPropagation(leafconnector,UP,false));	// FIXME: schedule parents!
	}
	traverse(pf);
}

void FOPropagatorFactory::visit(const AggForm* af) {
	initFalse(af);
	traverse(af);
}

void FOPropagatorFactory::visit(const EqChainForm* ecf) {
	// TODO? Change this???
	initFalse(ecf);
	traverse(ecf);
}

void FOPropagatorFactory::visit(const EquivForm* ef) {
	initFalse(ef);
	traverse(ef);
}

void FOPropagatorFactory::visit(const BoolForm* bf) {
	initFalse(bf);
	traverse(bf);
}

void FOPropagatorFactory::visit(const QuantForm* qf) {
	initFalse(qf);
	traverse(qf);
}
