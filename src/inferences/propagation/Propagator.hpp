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

#ifndef PROPAGATE_HPP
#define PROPAGATE_HPP

#include "PropagationCommon.hpp"
#include "IncludeComponents.hpp"
#include "visitors/TheoryVisitor.hpp"

template<class DomainType>
struct ThreeValuedDomain;
class FOPropScheduler;


/**
 * Constraint propagator for first-order theories
 */
class FOPropagator: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
public:
	virtual ~FOPropagator() {
	}

	virtual void doPropagation() = 0; //!< Apply propagations until the propagation queue is empty

	/*
	 * Obtain the resulting structure
	 * (the given structure is used to evaluate BDDs in case of symbolic propagation)
	 */
	virtual void applyPropagationToStructure(Structure* str,  const Vocabulary& outputvoc) const = 0;

	virtual std::shared_ptr<GenerateBDDAccordingToBounds> symbolicstructure(Vocabulary* allreadyPropagatedSymbols) const = 0;
	//!< Obtain the resulting structure (only works if the used domainfactory is a FOPropBDDDomainFactory)
};


template<class InterpretationFactory, class Domain>
class TypedFOPropagator: public FOPropagator {
	VISITORFRIENDS()
private:
	Options* _options; //TODO: remove options, verbosity and maxsteps. They belong to the globaldata
	int _maxsteps; //!< Maximum number of propagations
	InterpretationFactory* _factory; //!< Manages and creates domains for formulas
	FOPropScheduler* _scheduler; //!< Schedules propagations
	std::map<const Formula*, ThreeValuedDomain<Domain> > _domains; //!< Map each formula to its current domain
	std::map<const Formula*, varset> _quantvars; //What is this?
	//Every symbol has exactly one "connector". A "prototype" of an atom by this symbol
	//If we want the interpretation of an other PredForm over the same symbol, we should
	//first replace all it's subterms by the ones given in leafconnectdata
	std::map<PFSymbol*, PredForm*> _leafconnectors;
	std::map<const PredForm*, LeafConnectData<Domain> > _leafconnectdata;
	std::map<const Formula*, const Formula*> _upward; //!<mapping from a formula to it's parent
	std::map<const PredForm*, std::set<const PredForm*> > _leafupward;
	std::vector<AdmissibleBoundChecker<Domain>*> _admissiblecheckers;
	AbstractTheory* _theory;

	// Variables to temporarily store a propagation
	FOPropDirection _direction;
	bool _ct;
	const Formula* _child;

	Domain* addToConjunction(Domain* conjunction, Domain* newconjunct);
	Domain* addToDisjunction(Domain* disjunction, Domain* newdisjunct);
	Domain* addToExists(Domain* exists, Variable*);
	Domain* addToForall(Domain* forall, Variable*);
	Domain* addToExists(Domain* exists, const varset&);
	Domain* addToForall(Domain* forall, const varset&);

	void updateDomain(const Formula* tobeupdated, FOPropDirection, bool ct, Domain* newdomain, const Formula* child = 0);
	bool admissible(Domain*, Domain*) const; //!< Returns true iff the first domain is an allowed
											 //!< replacement of the second domain

protected:
	void visit(const PredForm*);
	void visit(const EqChainForm*);
	void visit(const EquivForm*);
	void visit(const BoolForm*);
	void visit(const QuantForm*);
	void visit(const AggForm*);

public:
	//Elements from the theory are used during propagation.
	//NOTE theory is responsibility of the propagator. Hence clone before giving one!
	TypedFOPropagator(InterpretationFactory*, FOPropScheduler*, Options*);

	~TypedFOPropagator();

	void doPropagation(); //!< Apply propagations until the propagation queue is empty

	/**
	 * Obtain the resulting structure
	 * 	(the given structure is used to evaluate BDDs in case of symbolic propagation)
	 */
	void applyPropagationToStructure(Structure* str, const Vocabulary& outputvoc) const;

	std::shared_ptr<GenerateBDDAccordingToBounds> symbolicstructure(Vocabulary* symbolsThatCannotBeReplacedByBDDs) const;
	//!< Obtain the resulting structure (only works if the used domainfactory is a FOPropBDDDomainFactory)

	void schedule(const Formula* par, FOPropDirection, bool, const Formula* child);

	// TODO check that domains can never contain nullpointers!

	int getMaxSteps() const {
		return _maxsteps;
	}
	void setMaxSteps(int steps) {
		_maxsteps = steps;
	}
	const Options* getOptions() const {
		return _options;
	}

	const std::map<const Formula*, const Formula*>& getUpward() const {
		return _upward;
	}
	const std::map<PFSymbol*, PredForm*>& getLeafConnectors() const {
		return _leafconnectors;
	}

	void setDomain(const Formula* key, const ThreeValuedDomain<Domain>& value);
	void setTheory(AbstractTheory* t) {
		if(_theory != NULL){
			_theory->recursiveDelete();
		}
		_theory = t;
	}
	void setQuantVar(const Formula* key, const varset& value) {
		_quantvars[key] = value;
	}
	void setUpward(const Formula* key, const Formula* value) {
		_upward[key] = value;
	}
	void setLeafConnector(PFSymbol* key, PredForm* value) {
		_leafconnectors[key] = value;
	}
	void setLeafConnectData(const PredForm* key, const LeafConnectData<Domain>& value) {
		_leafconnectdata[key] = value;
	}

	void addToLeafUpward(PredForm* index, const PredForm* pf) {
		_leafupward[index].insert(pf);
	}

	bool hasDomain(const Formula* f) const {
		return _domains.find(f) != _domains.cend();
	}
	const ThreeValuedDomain<Domain>& getDomain(const Formula* f) const {
		Assert(hasDomain(f));
		return _domains.at(f);
	}
	void setCFOfDomain(const Formula* f, Domain* d) {
		Assert(getFactory()->isValidAsDomainFor(d,f));
		Assert(admissible(d,NULL));
		_domains.at(f)._cfdomain = d;
	}
	void setCTOfDomain(const Formula* f, Domain* d) {
		Assert(getFactory()->isValidAsDomainFor(d,f));
		Assert(admissible(d,NULL));
		_domains.at(f)._ctdomain = d;
	}

	InterpretationFactory* getFactory() const {
		return _factory;
	}
};

#endif
