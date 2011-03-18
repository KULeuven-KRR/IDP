/************************************
	propagate.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "propagate.hpp"

/****************************
	Propagation scheduler
****************************/

void FOPropScheduler::add(FOPropagation* propagation) {
	_queue.push(propagation);
}

bool FOPropScheduler::hasNext() {
	return !_queue.empty();
}

bool FOPropScheduler::next() {
	FOPropagation* propagation = _queue.front();
	_queue.pop();
	return propagation;
}

/**************************
	Propagation domains
**************************/

FOPropBDDDomain* FOPropBDDDomainFactory::trueDomain() const {
	FOBDD* bdd = _manager->truebdd();
	return new FOPropBDDDomain(bdd);
}

FOPropBDDDomain* FOPropBDDDomainFactory::falseDomain() const {
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
	return bddfactory.bdd();
}

/*****************
	Propagator
*****************/

ThreeValuedDomain::ThreeValuedDomain(const FOPropDomainFactory* factory, bool ctdom, bool cfdom) {
	_ctdomain = ctdom ? factory->trueDomain() : factory->falseDomain();
	_cfdomain = cfdom ? factory->trueDomain() : factory->falseDomain();
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
		propagation->_formula->accept(this);
		delete(propagation);
	}
}

void FOPropagator::visit(const Form*) {
	//TODO
}

void FOPropagator::visit(const Form*) {
	//TODO
}

void FOPropagator::visit(const Form*) {
	//TODO
}

void FOPropagator::visit(const BoolForm* bf) {
	if(_direction == DOWN) {
		FOPropDomain* bfdomain = (bf->sign() == _ct) ? _domains[bf]._ctdomain : _domains[bf]._cfdomain;
		for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
			Formula* subform = bf->subform(n);
			ThreeValuedDomain* subfdomain = _domains[subform];
			if(!domain._twovalued) {
				FOPropDomain* deriveddomain;
				const vector<Variable*>& qvars = _quantvars[subform];
				if(_ct) {
					if(bf->conj() == bf->sign()) {
						deriveddomain = _factory->exists(bfdomain,qvars);
					}
					else {
						assert(bf->conj() != bf->sign());
						//TODO HIER BEZIG!!!
					}
				}
				else {
					assert(!_ct);
					if(bf->conj() == bf->sign()) {
						//TODO
					}
					else {
						assert(bf->conj() != bf->sign());
						deriveddomain = _factory->exists(bfdomain,qvars);
					}
				}
				//TODO
			}
		}
	}
	else {
		assert(_direction == UP);
		//TODO
	}
}

void FOPropagator::visit(const Form*) {
	//TODO
}


FOPropagatorFactory(FOPropDomainFactory* factory, FOPropScheduler* scheduler, const AbstractStructure* s = 0): _structure(s) {
	_propagator = new FOPropagator(factory, scheduler);
}

FOPropagator* FOPropagatorFactory::create(const AbstractTheory* theory) {
	_assertsentences = true;
	theory->accept(this);
	return _propagator;
}

void FOPropagatorFactory::visit(const Theory* theory) {
	for(unsigned int n = 0; n < theory->nrSentences(); ++n) {
		Formula* sentence = theory->sentence(n);
		if(_assertsentences) {
			_propagator->_scheduler.add(new FOPropagation(sentence,DOWN,true));
			_propagator->_domains[sentence] = ThreeValuedDomain(_propagator->_factory,true,false);
		}
		sentence->accept(this);
	}
}

void FOPropagatorFactory::initFalse(const Formula* f) {
	if(_propagator->_domains.find(f) == _propagator->_domains.end())
		_propagator->_domains[f] = ThreeValuedDomain(_propagator->_factory,false,false);
}

void FOPropagatorFactory::visit(const PredForm* pf) {
	initFalse(pf);
	PFSymbol* symbol = pf->symb();
	if(_leafconnectors.find(symbol) == _leafconnectors.end()) {
		vector<Variable*> vars = VarUtils::makeNewVariables(symbol->sorts());
		vector<Term*> args = TermUtils::makeNewVarTerms(vars);
		PredForm* leafconnector = new PredForm(true,symbol,args,FormParseInfo());
		_leafconnectors[symbol] = leafconnector;
		if(symbol->builtin() || (_structure && _structure->inter(symbol)->fasttwovalued())) {
			_propagator->_domains[leafconnector] = ThreeValuedDomain(_propagator->_factory,leafconnector,true);
		}
		else {
			initFalse(leafconnector);
			//FIXME replace previous line by following:
			//_propagator->_domains[leafconnector] = ThreeValuedDomain(_propagator->_factory,leafconnector,false);
		}
		_propagator->_scheduler.add(new FOPropagation(leafconnector,UP,true));
		_propagator->_scheduler.add(new FOPropagation(leafconnector,UP,false));
	}
	traverse(pf);
}

void FOPropagatorFactory::visit(const EqChainForm* ecf) {
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
