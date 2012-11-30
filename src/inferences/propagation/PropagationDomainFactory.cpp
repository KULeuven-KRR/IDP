/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/
#include "PropagationDomainFactory.hpp"

#include "IncludeComponents.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "structure/StructureComponents.hpp"
using namespace std;


FOPropBDDDomainFactory::FOPropBDDDomainFactory() {
	_manager = new FOBDDManager();
}

FOPropBDDDomainFactory::~FOPropBDDDomainFactory() {
	//do not delete manager: is passed to symbolicstructure (GenerateBDDAccordingToBounds)
}

ostream& FOPropBDDDomainFactory::put(ostream& output, FOPropBDDDomain* domain) const {
	pushtab();
	output << toString(domain->bdd());
	poptab();
	return output;
}

// Valid iff the free variables of the domain are a subset of the free variables of the formula
bool FOPropBDDDomainFactory::isValidAsDomainFor(const FOPropBDDDomain* d, const Formula* f) const {
	return d->isValidFor(f, _manager);
}

FOPropBDDDomain* FOPropBDDDomainFactory::trueDomain(const Formula* f) const {
	const FOBDD* bdd = _manager->truebdd();
	vector<Variable*> vv(f->freeVars().cbegin(), f->freeVars().cend());
	return new FOPropBDDDomain(bdd, vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::falseDomain(const Formula* f) const {
	const FOBDD* bdd = _manager->falsebdd();
	vector<Variable*> vv(f->freeVars().cbegin(), f->freeVars().cend());
	return new FOPropBDDDomain(bdd, vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::formuladomain(const Formula* f) const {
	FOBDDFactory bddfactory(_manager);
	vector<Variable*> vv(f->freeVars().cbegin(), f->freeVars().cend());
	return new FOPropBDDDomain(bddfactory.turnIntoBdd(f), vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::ctDomain(const PredForm* pf) const {
	vector<const FOBDDTerm*> args;
	FOBDDFactory bddfactory(_manager);
	for (auto it = pf->subterms().cbegin(); it != pf->subterms().cend(); ++it) {
		args.push_back(bddfactory.turnIntoBdd(*it));
	}
	const FOBDDKernel* k = _manager->getAtomKernel(pf->symbol(), AtomKernelType::AKT_CT, args);
	const FOBDD* bdd = _manager->ifthenelse(k, _manager->truebdd(), _manager->falsebdd());
	vector<Variable*> vv(pf->freeVars().cbegin(), pf->freeVars().cend());
	return new FOPropBDDDomain(bdd, vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::cfDomain(const PredForm* pf) const {
	vector<const FOBDDTerm*> args;
	FOBDDFactory bddfactory(_manager);
	for (auto it = pf->subterms().cbegin(); it != pf->subterms().cend(); ++it) {
		args.push_back(bddfactory.turnIntoBdd(*it));
	}
	const FOBDDKernel* k = _manager->getAtomKernel(pf->symbol(), AtomKernelType::AKT_CF, args);
	const FOBDD* bdd = _manager->ifthenelse(k, _manager->truebdd(), _manager->falsebdd());
	vector<Variable*> vv(pf->freeVars().cbegin(), pf->freeVars().cend());
	return new FOPropBDDDomain(bdd, vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::exists(FOPropBDDDomain* domain, const set<Variable*>& qvars) const {
	auto bddqvars = _manager->getVariables(qvars);
	const FOBDD* qbdd = _manager->existsquantify(bddqvars, domain->bdd());
	vector<Variable*> vv;
	for (auto it = domain->vars().cbegin(); it != domain->vars().cend(); ++it) {
		if (qvars.find(*it) == qvars.cend()) {
			vv.push_back(*it);
		}
	}
	return new FOPropBDDDomain(qbdd, vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::forall(FOPropBDDDomain* domain, const std::set<Variable*>& qvars) const {
	auto bddqvars = _manager->getVariables(qvars);
	const FOBDD* qbdd = _manager->univquantify(bddqvars, domain->bdd());
	vector<Variable*> vv;
	for (auto it = domain->vars().cbegin(); it != domain->vars().cend(); ++it) {
		if (qvars.find(*it) == qvars.cend()) {
			vv.push_back(*it);
		}
	}
	return new FOPropBDDDomain(qbdd, vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::conjunction(FOPropBDDDomain* domain1, FOPropBDDDomain* domain2) const {
	const FOBDD* conjbdd = _manager->conjunction(domain1->bdd(), domain2->bdd());
	set<Variable*> sv;
	sv.insert(domain1->vars().cbegin(), domain1->vars().cend());
	sv.insert(domain2->vars().cbegin(), domain2->vars().cend());
	vector<Variable*> vv(sv.cbegin(), sv.cend());
	return new FOPropBDDDomain(conjbdd, vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::disjunction(FOPropBDDDomain* domain1, FOPropBDDDomain* domain2) const {
	const FOBDD* disjbdd = _manager->disjunction(domain1->bdd(), domain2->bdd());
	set<Variable*> sv;
	sv.insert(domain1->vars().cbegin(), domain1->vars().cend());
	sv.insert(domain2->vars().cbegin(), domain2->vars().cend());
	vector<Variable*> vv(sv.cbegin(), sv.cend());
	return new FOPropBDDDomain(disjbdd, vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::substitute(FOPropBDDDomain* domain, const map<Variable*, Variable*>& mvv) const {
	map<const FOBDDVariable*, const FOBDDVariable*> newmvv;
	for (auto it = mvv.cbegin(); it != mvv.cend(); ++it) {
		const FOBDDVariable* oldvar = _manager->getVariable(it->first);
		const FOBDDVariable* newvar = _manager->getVariable(it->second);
		newmvv[oldvar] = newvar;
	}
	const FOBDD* substbdd = _manager->substitute(domain->bdd(), newmvv);
	vector<Variable*> vv = domain->vars();
	for (size_t n = 0; n < vv.size(); ++n) {
		auto it = mvv.find(vv[n]);
		if (it != mvv.cend()) {
			vv[n] = it->second;
		}
	}
	return new FOPropBDDDomain(substbdd, vv);
}

bool FOPropBDDDomainFactory::approxequals(FOPropBDDDomain* domain1, FOPropBDDDomain* domain2) const {
	return domain1->bdd() == domain2->bdd();
}

bool FOPropTableDomainFactory::approxequals(FOPropTableDomain* left, FOPropTableDomain* right) const {
	return left->table() == right->table();
}

PredInter* FOPropBDDDomainFactory::inter(const vector<Variable*>& vars, const ThreeValuedDomain<FOPropBDDDomain>& dom, AbstractStructure* str) const {
	// Construct the universe of the interpretation and two sets of new variables
	vector<SortTable*> vst;
	vector<Variable*> newctvars, newcfvars;
	map<const FOBDDVariable*, const FOBDDVariable*> ctmvv, cfmvv;
	bool twovalued = dom._twovalued;
	for (Variable* var : vars) {
		auto oldvar = _manager->getVariable(var);
		vst.push_back(str->inter(var->sort()));
		auto ctv = new Variable(var->sort());
		newctvars.push_back(ctv);
		auto newctvar = _manager->getVariable(ctv);
		ctmvv[oldvar] = newctvar;
		if (not twovalued) {
			auto cfv = new Variable(var->sort());
			newcfvars.push_back(cfv);
			auto newcfvar = _manager->getVariable(cfv);
			cfmvv[oldvar] = newcfvar;
		}
	}
	Universe univ(vst);
	// Construct the ct-table and cf-table
	auto newctbdd = _manager->substitute(dom._ctdomain->bdd(), ctmvv);
//	cerr <<"For vars " <<toString(newctvars) <<", the bdd is \n" <<toString(newctbdd) <<"\n";
	auto ct = new PredTable(new BDDInternalPredTable(newctbdd, _manager, newctvars, str), univ);
	if (twovalued) {
		return new PredInter(ct, true);
	} else {
		auto newcfbdd = _manager->substitute(dom._cfdomain->bdd(), cfmvv);
//		cerr <<"For vars " <<toString(newcfvars) <<", the bdd is \n" <<toString(newcfbdd) <<"\n";
		auto cf = new PredTable(new BDDInternalPredTable(newcfbdd, _manager, newcfvars, str), univ);
		return new PredInter(ct, cf, true, true);
	}
}


FOPropTableDomain* FOPropTableDomainFactory::exists(FOPropTableDomain* domain, const set<Variable*>& sv) const {
	vector<bool> keepcol;
	vector<Variable*> newvars;
	vector<SortTable*> newunivcols;
	for (unsigned int n = 0; n < domain->vars().size(); ++n) {
		Variable* v = domain->vars()[n];
		if (sv.find(v) == sv.cend()) {
			keepcol.push_back(true);
			newvars.push_back(v);
			newunivcols.push_back(domain->table()->universe().tables()[n]);
		} else {
			keepcol.push_back(false);
		}
	}

	if (not domain->table()->approxFinite()) {
		clog << "Probably entering an infinte loop when trying to project a possibly infinite table...\n";
	}
	PredTable* npt = new PredTable(new EnumeratedInternalPredTable(), Universe(newunivcols));
	for (TableIterator it = domain->table()->begin(); not it.isAtEnd(); ++it) {
		const ElementTuple& tuple = *it;
		ElementTuple newtuple;
		for (size_t n = 0; n < tuple.size(); ++n) {
			if (keepcol[n]) {
				newtuple.push_back(tuple[n]);
			}
		}
		npt->add(newtuple);
	}

	return new FOPropTableDomain(npt, newvars);
}

