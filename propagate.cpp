/************************************
	propagate.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <typeinfo>
#include <iostream>
#include "fobdd.hpp"
#include "vocabulary.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "structure.hpp"
#include "propagate.hpp"

using namespace std;

/****************************
	Propagation scheduler
****************************/

void FOPropScheduler::add(FOPropagation* propagation) {
	// TODO: don't schedule a propagation that is already on the queue
	_queue.push(propagation);
}

bool FOPropScheduler::hasNext() const {
	return (not _queue.empty());
}

FOPropagation* FOPropScheduler::next() {
	FOPropagation* propagation = _queue.front();
	_queue.pop();
	return propagation;
}

/**************************
	Propagation domains
**************************/

FOPropBDDDomainFactory::FOPropBDDDomainFactory() {
	_manager = new FOBDDManager();
}

ostream& FOPropBDDDomainFactory::put(ostream& output, FOPropBDDDomain* domain) const {
	_manager->put(output,domain->bdd(),6);
	return output;
}

FOPropBDDDomain* FOPropBDDDomainFactory::trueDomain(const Formula* f) const {
	const FOBDD* bdd = _manager->truebdd();
	vector<Variable*> vv(f->freeVars().begin(),f->freeVars().end());
	return new FOPropBDDDomain(bdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::falseDomain(const Formula* f) const {
	const FOBDD* bdd = _manager->falsebdd();
	vector<Variable*> vv(f->freeVars().begin(),f->freeVars().end());
	return new FOPropBDDDomain(bdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::formuladomain(const Formula* f) const {
	FOBDDFactory bddfactory(_manager);
	vector<Variable*> vv(f->freeVars().begin(),f->freeVars().end());
	return new FOPropBDDDomain(bddfactory.run(f),vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::ctDomain(const PredForm* pf) const {
	vector<const FOBDDArgument*> args;
	FOBDDFactory bddfactory(_manager);
	for(auto it = pf->subterms().begin(); it != pf->subterms().end(); ++it) {
		args.push_back(bddfactory.run(*it));
	}
	const FOBDDKernel* k = _manager->getAtomKernel(pf->symbol(),AKT_CT,args);
	const FOBDD* bdd = _manager->getBDD(k,_manager->truebdd(),_manager->falsebdd());
	vector<Variable*> vv(pf->freeVars().begin(),pf->freeVars().end());
	return new FOPropBDDDomain(bdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::cfDomain(const PredForm* pf) const {
	vector<const FOBDDArgument*> args;
	FOBDDFactory bddfactory(_manager);
	for(auto it = pf->subterms().begin(); it != pf->subterms().end(); ++it) {
		args.push_back(bddfactory.run(*it));
	}
	const FOBDDKernel* k = _manager->getAtomKernel(pf->symbol(),AKT_CF,args);
	const FOBDD* bdd = _manager->getBDD(k,_manager->truebdd(),_manager->falsebdd());
	vector<Variable*> vv(pf->freeVars().begin(),pf->freeVars().end());
	return new FOPropBDDDomain(bdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::exists(FOPropBDDDomain* domain, const set<Variable*>& qvars) const {
	set<const FOBDDVariable*> bddqvars = _manager->getVariables(qvars);
	const FOBDD* qbdd = _manager->existsquantify(bddqvars,domain->bdd());
	vector<Variable*> vv;
	for(auto it = domain->vars().begin(); it != domain->vars().end(); ++it) {
		if(qvars.find(*it) == qvars.end()) {
			vv.push_back(*it);
		}
	}
	return new FOPropBDDDomain(qbdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::forall(FOPropBDDDomain* domain, const std::set<Variable*>& qvars) const {
	set<const FOBDDVariable*> bddqvars = _manager->getVariables(qvars);
	const FOBDD* qbdd = _manager->univquantify(bddqvars,domain->bdd());
	vector<Variable*> vv;
	for(auto it = domain->vars().begin(); it != domain->vars().end(); ++it) {
		if(qvars.find(*it) == qvars.end()) {
			vv.push_back(*it);
		}
	}
	return new FOPropBDDDomain(qbdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::conjunction(FOPropBDDDomain* domain1,FOPropBDDDomain* domain2) const {
	const FOBDD* conjbdd = _manager->conjunction(domain1->bdd(),domain2->bdd());
	set<Variable*> sv; 
	sv.insert(domain1->vars().begin(),domain1->vars().end());
	sv.insert(domain2->vars().begin(),domain2->vars().end());
	vector<Variable*> vv(sv.begin(),sv.end());
	return new FOPropBDDDomain(conjbdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::disjunction(FOPropBDDDomain* domain1,FOPropBDDDomain* domain2) const {
	const FOBDD* disjbdd = _manager->disjunction(domain1->bdd(),domain2->bdd());
	set<Variable*> sv; 
	sv.insert(domain1->vars().begin(),domain1->vars().end());
	sv.insert(domain2->vars().begin(),domain2->vars().end());
	vector<Variable*> vv(sv.begin(),sv.end());
	return new FOPropBDDDomain(disjbdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::substitute(FOPropBDDDomain* domain,const map<Variable*,Variable*>& mvv) const {
	map<const FOBDDVariable*,const FOBDDVariable*> newmvv;
	for(auto it = mvv.begin(); it != mvv.end(); ++it) {
		const FOBDDVariable* oldvar = _manager->getVariable(it->first);
		const FOBDDVariable* newvar = _manager->getVariable(it->second);
		newmvv[oldvar] = newvar;
	}
	const FOBDD* substbdd = _manager->substitute(domain->bdd(),newmvv);
	vector<Variable*> vv = domain->vars();
	for(size_t n = 0; n < vv.size(); ++n) {
		auto it = mvv.find(vv[n]);
		if(it != mvv.end()) {
			vv[n] = it->second;
		}
	}
	return new FOPropBDDDomain(substbdd,vv);
}

bool FOPropBDDDomainFactory::approxequals(FOPropBDDDomain* domain1, FOPropBDDDomain* domain2) const {
	return domain1->bdd() == domain2->bdd();
}

PredInter* FOPropBDDDomainFactory::inter(const vector<Variable*>& vars, const ThreeValuedDomain<FOPropBDDDomain>& dom, AbstractStructure* str) const {
	// Construct the universe of the interpretation and two sets of new variables
	vector<SortTable*> vst;
	vector<Variable*> newctvars;
	vector<Variable*> newcfvars;
	map<const FOBDDVariable*,const FOBDDVariable*> ctmvv;
	map<const FOBDDVariable*,const FOBDDVariable*> cfmvv;
	bool twovalued = dom._twovalued;	
	for(auto it = vars.begin(); it != vars.end(); ++it) {
		const FOBDDVariable* oldvar = _manager->getVariable(*it);
		vst.push_back(str->inter((*it)->sort()));
		Variable* ctv = new Variable((*it)->sort());
		newctvars.push_back(ctv);
		const FOBDDVariable* newctvar = _manager->getVariable(ctv);
		ctmvv[oldvar] = newctvar;
		if(not twovalued) {
			Variable* cfv = new Variable((*it)->sort());
			newcfvars.push_back(cfv);
			const FOBDDVariable* newcfvar = _manager->getVariable(cfv);
			cfmvv[oldvar] = newcfvar;
		}
	}
	Universe univ(vst);

	// Construct the ct-table and cf-table
	const FOBDD* newctbdd = _manager->substitute(dom._ctdomain->bdd(),ctmvv);
	PredTable* ct = new PredTable(new BDDInternalPredTable(newctbdd,_manager,newctvars,str),univ);
	if(twovalued) {
		return new PredInter(ct,true);
	} else {
		const FOBDD* newcfbdd = _manager->substitute(dom._cfdomain->bdd(),cfmvv);
		PredTable* cf = new PredTable(new BDDInternalPredTable(newcfbdd,_manager,newcfvars,str),univ);
		return new PredInter(ct,cf,true,true);
	}
}

FOPropTableDomain* FOPropTableDomain::clone() const {
	return new FOPropTableDomain(new PredTable(_table->internTable(),_table->universe()),_vars);
}

FOPropTableDomain* FOPropTableDomainFactory::exists(FOPropTableDomain* domain, const set<Variable*>& sv) const {
	vector<bool> keepcol;
	vector<Variable*> newvars;
	vector<SortTable*> newunivcols;
	for(unsigned int n = 0; n < domain->vars().size(); ++n) {
		Variable* v= domain->vars()[n];
		if(sv.find(v) == sv.end()) {
			keepcol.push_back(true);
			newvars.push_back(v);
			newunivcols.push_back(domain->table()->universe().tables()[n]);
		} else {
			keepcol.push_back(false);
		}
	}

	if(not domain->table()->approxFinite()) {
		cerr << "Probably entering an infinte loop when trying to project a possibly infinite table...\n";
	}
	PredTable* npt = new PredTable(new EnumeratedInternalPredTable(),Universe(newunivcols));
	for(TableIterator it = domain->table()->begin(); it.hasNext(); ++it) {
		const ElementTuple& tuple = *it;
		ElementTuple newtuple;
		for(size_t n = 0; n < tuple.size(); ++n) {
			if(keepcol[n]) {
				newtuple.push_back(tuple[n]);
			}
		}
		npt->add(newtuple);
	}

	return new FOPropTableDomain(npt,newvars);
}

/*****************
	Propagator
*****************/

template<class Factory, class DomainType>
TypedFOPropagator<Factory, DomainType>::TypedFOPropagator(Factory* f, FOPropScheduler* s, Options* opts)
		: _verbosity(opts->getValue(IntType::PROPAGATEVERBOSITY)), _factory(f), _scheduler(s) {
	_maxsteps = opts->getValue(IntType::NRPROPSTEPS);
	_options = opts;
}
template<>
TypedFOPropagator<FOPropBDDDomainFactory, FOPropBDDDomain>::TypedFOPropagator(FOPropBDDDomainFactory* f, FOPropScheduler* s, Options* opts)
		: _verbosity(opts->getValue(IntType::PROPAGATEVERBOSITY)), _factory(f), _scheduler(s) {
	_maxsteps = opts->getValue(IntType::NRPROPSTEPS);
	_options = opts;
	if(_options->getValue(IntType::LONGESTBRANCH)!=0) {
		_admissiblecheckers.push_back(new LongestBranchChecker(f->manager(),_options->getValue(IntType::LONGESTBRANCH)));
	}
} 

template<class Factory, class DomainType>
void TypedFOPropagator<Factory, DomainType>::run() {
	if(_verbosity > 1) { cerr << "=== Start propagation ===\n"; }
	while(_scheduler->hasNext()) {
		FOPropagation* propagation = _scheduler->next();
		_direction = propagation->getDirection();
		_ct = propagation->isCT();
		_child = propagation->getChild();
		if(_verbosity > 1) { 
			const Formula* p = propagation->getParent();
			cerr << "  Propagate ";
			if(_direction == DOWN) {
				cerr << "downward from " << (_ct ? "the ct-bound of " : "the cf-bound of "); 
				p->put(cerr,_options->getValue(BoolType::LONGNAMES));
				if(_child) { cerr << " to "; _child->put(cerr,_options->getValue(BoolType::LONGNAMES));	}
			} else {
				cerr << "upward to " << ((_ct == isPos(p->sign())) ? "the ct-bound of " : "the cf-bound of ");
				p->put(cerr,_options->getValue(BoolType::LONGNAMES));
				if(_child) { cerr << " from "; _child->put(cerr,_options->getValue(BoolType::LONGNAMES));	}
			}
			cerr << "\n";
		}
		propagation->getParent()->accept(this);
		delete(propagation);
	}
	if(_verbosity > 1) { cerr << "=== End propagation ===\n"; }
}

template<class Factory, class Domain>
AbstractStructure* TypedFOPropagator<Factory, Domain>::currstructure(AbstractStructure* structure) const {
	Vocabulary* vocabulary = new Vocabulary("");
	for(auto it = _leafupward.begin(); it != _leafupward.end(); ++it) {
		PFSymbol* symbol = it->first->symbol();
		if(typeid(*symbol) == typeid(Predicate)) { vocabulary->addPred(dynamic_cast<Predicate*>(symbol)); }
		else { vocabulary->addFunc(dynamic_cast<Function*>(symbol)); }
	}
	AbstractStructure* res = structure->clone();
	res->vocabulary(vocabulary);

	for(auto it = _leafupward.begin(); it != _leafupward.end(); ++it) {
		const PredForm* connector = it->first;
		PFSymbol* symbol = connector->symbol();
		vector<Variable*> vv;
		for(auto jt = connector->subterms().begin(); jt != connector->subterms().end(); ++jt) {
			vv.push_back(*((*jt)->freeVars().begin()));
		}
		PredInter* pinter = _factory->inter(vv,_domains.find(connector)->second,structure);
		if(typeid(*symbol) == typeid(Predicate)) { res->inter(dynamic_cast<Predicate*>(symbol),pinter); }
		else {
			FuncInter* finter = new FuncInter(pinter);
			res->inter(dynamic_cast<Function*>(symbol),finter);
		}
	}
	return res;
}

template<class Factory, class Domain>
SymbolicStructure* TypedFOPropagator<Factory, Domain>::symbolicstructure() const {
	map<PFSymbol*,vector<const FOBDDVariable*> > vars;
	map<PFSymbol*,const FOBDD*> ctbounds;
	map<PFSymbol*,const FOBDD*> cfbounds;
	assert(typeid(*_factory) == typeid(FOPropBDDDomainFactory));
	FOBDDManager* manager = (dynamic_cast<FOPropBDDDomainFactory*>(_factory))->manager();
	for(auto it = _leafconnectors.begin(); it != _leafconnectors.end(); ++it) {
		ThreeValuedDomain<Domain> tvd = (_domains.find(it->second))->second;
		ctbounds[it->first] = dynamic_cast<FOPropBDDDomain*>(tvd._ctdomain)->bdd();
		cfbounds[it->first] = dynamic_cast<FOPropBDDDomain*>(tvd._cfdomain)->bdd();
		vector<const FOBDDVariable*> bddvars;
		for(auto jt = it->second->subterms().begin(); jt != it->second->subterms().end(); ++jt) {
			assert(typeid(*(*jt)) == typeid(VarTerm));
			bddvars.push_back(manager->getVariable((dynamic_cast<VarTerm*>(*jt))->var()));
		}
		vars[it->first] = bddvars;
	}
	return new SymbolicStructure(manager,ctbounds,cfbounds,vars);
}

template<class Factory, class Domain>
FuncInter* TypedFOPropagator<Factory, Domain>::interpretation(Function* ) const {
	assert(false);
	// TODO
	return 0;
}

template<class Factory, class Domain>
PredInter* TypedFOPropagator<Factory, Domain>::interpretation(Predicate* ) const {
	assert(false);
	// TODO
	return 0;
}

template<class Factory, class Domain>
Domain* TypedFOPropagator<Factory, Domain>::addToConjunction(Domain* conjunction, Domain* newconjunct) {
	FOPropDomain* temp = conjunction;
	conjunction = _factory->conjunction(conjunction,newconjunct);
	delete(temp);
	return conjunction;
}

template<class Factory, class Domain>
Domain* TypedFOPropagator<Factory, Domain>::addToDisjunction(Domain* disjunction, Domain* newdisjunct) {
	FOPropDomain* temp = disjunction;
	disjunction = _factory->disjunction(disjunction,newdisjunct);
	delete(temp);
	return disjunction;
}

template<class Factory, class Domain>
Domain* TypedFOPropagator<Factory, Domain>::addToExists(Domain* exists, const set<Variable*>& qvars) {
	set<Variable*> newqvars;
	map<Variable*,Variable*> mvv;
	for(auto it = qvars.begin(); it != qvars.end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),(*it)->pi());
		newqvars.insert(v);
		mvv[*it] = v;
	}
	Domain* cl = _factory->substitute(exists,mvv);
	delete(exists);
	Domain* q = _factory->exists(cl,newqvars);
	delete(cl);
	return q;
}

template<class Factory, class Domain>
Domain* TypedFOPropagator<Factory, Domain>::addToForall(Domain* forall, const set<Variable*>& qvars) {
	set<Variable*> newqvars;
	map<Variable*,Variable*> mvv;
	for(auto it = qvars.begin(); it != qvars.end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),(*it)->pi());
		newqvars.insert(v);
		mvv[*it] = v;
	}
	Domain* cl = _factory->substitute(forall,mvv);
	delete(forall);
	Domain* q = _factory->forall(cl,newqvars);
	delete(cl);
	return q;
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::schedule(const Formula* p, FOPropDirection dir, bool ct, const Formula* c) {
	if(_maxsteps > 0) {
		--_maxsteps;
		_scheduler->add(new FOPropagation(p,dir,ct,c));
		if(_verbosity > 1) {
			cerr << "  Schedule ";
			if(dir == DOWN) {
				cerr << "downward propagation from " << (ct ? "the ct-bound of " : "the cf-bound of ") << *p;
				if(c) { cerr << " towards " << *c; }
			}
			else {
				cerr << "upward propagation to " << ((ct == isPos(p->sign())) ? "the ct-bound of " : "the cf-bound of ") << *p;
				if(c){
					cerr << ". Propagation comes from " << *c;
				}
			}
			cerr << "\n";
		}
	}
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::updateDomain(const Formula* f, FOPropDirection dir, bool ct, Domain* newdomain, const Formula* child) {
	assert(newdomain!=NULL && f!=NULL && hasDomain(f));
	if(_verbosity > 2) {
		cerr << "    Derived the following " << (ct ? "ct " : "cf ") << "domain for " << *f << ":\n";
		_factory->put(cerr,newdomain);
	}

	Domain* olddom = ct ? getDomain(f)._ctdomain : getDomain(f)._cfdomain;
	Domain* newdom = _factory->disjunction(olddom,newdomain);

	if((not _factory->approxequals(olddom,newdom)) && admissible(newdom,olddom)) {
		ct ? setCTOfDomain(f, newdom) : setCFOfDomain(f, newdom);
		if(dir == DOWN) {
			for(auto it = f->subformulas().begin(); it != f->subformulas().end(); ++it) {
				schedule(f,DOWN,ct,*it);
			}
			if(typeid(*f) == typeid(PredForm)) {
				const PredForm* pf = dynamic_cast<const PredForm*>(f);
				auto it = _leafupward.find(pf);
				if(it != _leafupward.end()) {
					for(auto jt = it->second.begin(); jt != it->second.end(); ++jt) {
						if(*jt != child) { schedule(*jt,UP,ct,f); }
					}
				} else {
					assert(_leafconnectdata.find(pf) != _leafconnectdata.end());
					schedule(pf,DOWN,ct,0);
				}
			}
		} else {
			assert(dir == UP);
			auto it = _upward.find(f);
			if(it != _upward.end()) { schedule(it->second,UP,ct,f); }
		}
	} else {
		delete(newdom);
	}

	if(child!=NULL && dir == UP) {
		for(auto it = f->subformulas().begin(); it != f->subformulas().end(); ++it) {
			if(*it != child) { schedule(f,DOWN,!ct,*it); }
		}
	}

	delete(newdomain);
}

template<class Factory, class Domain>
bool TypedFOPropagator<Factory, Domain>::admissible(Domain* newd, Domain* oldd) const {
	for(auto it = _admissiblecheckers.begin(); it != _admissiblecheckers.end(); ++it) {
		if(!((*it)->check(newd,oldd))) return false;
	}
	return true;
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const PredForm* pf) {
	assert(_leafconnectdata.find(pf) != _leafconnectdata.end());
	assert(pf!=NULL);
	auto lcd = _leafconnectdata[pf];
	PredForm* connector = lcd._connector;
	assert(connector!=NULL);
	Domain* deriveddomain;
	Domain* temp;
	if(_direction == DOWN) {
		deriveddomain = _ct ? getDomain(pf)._ctdomain->clone() : getDomain(pf)._cfdomain->clone();
		deriveddomain = _factory->conjunction(deriveddomain,lcd._equalities);
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,lcd._leaftoconnector);
		delete(temp);
		//TODO?
		/*set<Variable*> freevars;
		map<Variable*,Variable*> mvv;
		for(auto it = pf->freeVars().begin(); it != pf->freeVars().end(); ++it) {
			Variable* clone = new Variable((*it)->sort());
			freevars.insert(clone);
			mvv[(*it)] = clone;
		}
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,mvv);
		delete(temp);
		deriveddomain = addToExists(deriveddomain,freevars);*/
		updateDomain(connector,DOWN,(_ct == isPos(pf->sign())),deriveddomain,pf);
	}
	else {
		assert(_direction == UP);
		assert(_domains.find(connector) != _domains.end());
		deriveddomain = _ct ? getDomain(connector)._ctdomain->clone() : getDomain(connector)._cfdomain->clone();
		// deriveddomain = _factory->conjunction(deriveddomain,lcd->_equalities);
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,lcd._connectortoleaf);
		delete(temp);
		/*set<Variable*> freevars;
		map<Variable*,Variable*> mvv;
		for(auto it = connector->freeVars().begin(); it != connector->freeVars().end(); ++it) {
			Variable* clone = new Variable((*it)->sort());
			freevars.insert(clone);
			mvv[(*it)] = clone;
		}
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,mvv);
		delete(temp);
		deriveddomain = addToExists(deriveddomain,freevars);*/
		updateDomain(pf,UP,(_ct == isPos(pf->sign())),deriveddomain);
	}
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const EqChainForm*) {
	assert(false);
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const EquivForm* ef) {
	assert(ef!=NULL && ef->left()!=NULL && ef->right()!=NULL);
	// FIXME child can be NULL, code should handle this?
	assert(_child!=NULL);

	Formula* otherchild = (_child == ef->left() ? ef->right() : ef->left());
	const ThreeValuedDomain<Domain>& tvd = getDomain(otherchild);
	switch(_direction){
	case DOWN:{
		Domain* parentdomain = _ct ? getDomain(ef)._ctdomain : getDomain(ef)._cfdomain;
		Domain* domain1 = _factory->conjunction(parentdomain,tvd._ctdomain);
		Domain* domain2 = _factory->conjunction(parentdomain,tvd._cfdomain);
		const set<Variable*>& qvars = _quantvars[_child];
		domain1 = addToExists(domain1,qvars);
		domain2 = addToExists(domain2,qvars);
		updateDomain(_child,DOWN,(_ct == isPos(ef->sign())),domain1);
		updateDomain(_child,DOWN,(_ct != isPos(ef->sign())),domain2);
		break; }
	case UP:{
		Domain* childdomain = _ct ? getDomain(_child)._ctdomain : getDomain(_child)._cfdomain;
		Domain* domain1 = _factory->conjunction(childdomain,tvd._ctdomain);
		Domain* domain2 = _factory->conjunction(childdomain,tvd._cfdomain);
		updateDomain(ef,UP,(_ct == isPos(ef->sign())),domain1,_child);
		updateDomain(ef,UP,(_ct != isPos(ef->sign())),domain2,_child);
		break;}
	}
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const BoolForm* bf) {
	assert(bf!=NULL);
	switch(_direction){
	case DOWN:{
		Domain* bfdomain = _ct ? getDomain(bf)._ctdomain : getDomain(bf)._cfdomain;
		assert(bfdomain!=NULL);
		vector<Domain*> subdomains;
		bool longnewdomain = (_ct != (bf->conj() == isPos(bf->sign())));
		if(longnewdomain) {
			for(size_t n = 0; n < bf->subformulas().size(); ++n) {
				const ThreeValuedDomain<Domain>& subfdomain = getDomain(bf->subformulas()[n]);
				subdomains.push_back(_ct == isPos(bf->sign()) ? subfdomain._cfdomain : subfdomain._ctdomain);
			}
		}
		for(size_t n = 0; n < bf->subformulas().size(); ++n) {
			Formula* subform = bf->subformulas()[n];
			assert(subform!=NULL);
			if(_child==NULL || subform == _child) {
				const ThreeValuedDomain<Domain>& subfdomain = getDomain(subform);
				if(not subfdomain._twovalued) {
					Domain* deriveddomain;
					const set<Variable*>& qvars = _quantvars[subform];
					if(longnewdomain) {
						deriveddomain = bfdomain->clone();
						for(size_t m = 0; m < subdomains.size(); ++m) {
							if(n != m) { deriveddomain = addToConjunction(deriveddomain,subdomains[m]); }
						}
						deriveddomain = addToExists(deriveddomain,qvars);
					}else{
						deriveddomain = _factory->exists(bfdomain,qvars);
					}
					updateDomain(subform,DOWN,(_ct == isPos(bf->sign())),deriveddomain);
				}
			}
		}
		break;}
	case UP:{
		// FIXME fishy code?
		Domain* deriveddomain = NULL;
		if(_ct == bf->conj()) {
			deriveddomain = _factory->trueDomain(bf);
			for(size_t n = 0; n < bf->subformulas().size(); ++n) {
				const ThreeValuedDomain<Domain>& tvd = getDomain(bf->subformulas()[n]);
				deriveddomain = addToConjunction(deriveddomain,(_ct ? tvd._ctdomain : tvd._cfdomain));
			}
		} else {
			// FIXME child can be NULL?
			const ThreeValuedDomain<Domain>& tvd = getDomain(_child);
			deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		}
		updateDomain(bf,UP,(_ct == isPos(bf->sign())),deriveddomain,_child);
		break;}
	}
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const QuantForm* qf) {
	assert(qf!=NULL && qf->subformula()!=NULL);
	switch(_direction){
	case DOWN:{
		assert(_domains.find(qf) != _domains.end());
		const ThreeValuedDomain<Domain>& tvd = getDomain(qf);
		Domain* deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		if(_ct != qf->isUnivWithSign()) {
			set<Variable*> newvars;
			map<Variable*,Variable*> mvv;
			Domain* conjdomain = _factory->trueDomain(qf->subformula());
			vector<CompType> comps(1,CompType::EQ);
			for(auto it = qf->quantVars().begin(); it != qf->quantVars().end(); ++it) {
				Variable* newvar = new Variable((*it)->sort());
				newvars.insert(newvar);
				mvv[*it] = newvar;
				vector<Term*> terms(2); 
				terms[0] = new VarTerm((*it),TermParseInfo()); 
				terms[1] = new VarTerm(newvar,TermParseInfo());
				EqChainForm* ef = new EqChainForm(SIGN::POS,true,terms,comps,FormulaParseInfo());
				Domain* equaldomain = _factory->formuladomain(ef); ef->recursiveDelete();
				conjdomain = addToConjunction(conjdomain,equaldomain); delete(equaldomain);
			}
			Domain* substdomain = _factory->substitute((_ct ? tvd._cfdomain : tvd._ctdomain),mvv);
			Domain* disjdomain = addToDisjunction(conjdomain,substdomain); delete(substdomain);
			Domain* univdomain = addToForall(disjdomain,newvars);
			deriveddomain = addToConjunction(deriveddomain,univdomain); delete(univdomain);
		}
		updateDomain(qf->subformula(),DOWN,(_ct == isPos(qf->sign())),deriveddomain);
		break;}
	case UP:{
		const ThreeValuedDomain<Domain>& tvd = getDomain(qf->subformula());
		Domain* deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		if(_ct == qf->isUniv()){
			deriveddomain = addToForall(deriveddomain,qf->quantVars());
		}else{
			deriveddomain = addToExists(deriveddomain,qf->quantVars());
		}
		updateDomain(qf,UP,(_ct == isPos(qf->sign())),deriveddomain);
		break;}
	}
}

template<class Factory, class Domain>
void TypedFOPropagator<Factory, Domain>::visit(const AggForm*) {
	// TODO
}

bool LongestBranchChecker::check(FOPropBDDDomain* newdomain, FOPropBDDDomain*) const { // FIXME second domain?
	return (_treshhold > _manager->longestbranch(newdomain->bdd()));
}
