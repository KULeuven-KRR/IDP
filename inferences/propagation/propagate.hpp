#ifndef PROPAGATE_HPP
#define PROPAGATE_HPP

#include "common.hpp"
#include "theory.hpp"
#include "options.hpp"
#include "PropagatorFactory.hpp"

class Variable;
class PFSymbol;
class FOBDD;
class FOPropagation;
class FOPropDomain;
class FOBDDManager;
class AbstractStructure;

/**
 * \file propagate.hpp
 * 
 * 	File contains classes for propagation on first-order level.
 */

/**
 * Direction of propagation
 *	UP = from a subformula to its parents
 *	DOWN = from a formula to its subformulas
 */
enum FOPropDirection { UP, DOWN };

/**
 * 	Class for scheduling propagation steps.	
 */
class FOPropScheduler {
private:
	std::queue<FOPropagation*>	_queue;		//!< The queue of scheduled propagations
public:
	void			add(FOPropagation*);	//!< Push a propagation on the queue
	FOPropagation*	next();					//!< Pop the first propagation from the queue and return it

	bool 			hasNext() const;		//!< True iff the queue is non-empty
};

/**
 * 	Class representing a single propagation step.
 */
class FOPropagation {
private:
	const Formula* 	_parent;		//!< The parent formula
	FOPropDirection	_direction;		//!< Direction of propagation (from parent to child or vice versa)
	bool			_ct;			//!< Indicates which domain is propagated
									//!< If _direction == DOWN and _ct == true,
									//!< then the ct-domain of the parent is propagated to the child, etc.
	const Formula*	_child;			//!< The subformula // NOTE child can be NULL
public:
	FOPropagation(const Formula* p, FOPropDirection dir, bool ct, const Formula* c)
			:_parent(p), _direction(dir), _ct(ct), _child(c) {
	}

	const Formula* getChild() const { return _child; }
	FOPropDirection getDirection() const { return _direction; }
	const Formula* getParent() const { return _parent; }
	bool isCT() const { return _ct; }
};

/**
 * 	Class representing domains for formulas.
 */
class FOPropDomain {
protected:
	std::vector<Variable*>	_vars;
public:
	FOPropDomain(const std::vector<Variable*>& vars) : _vars(vars) { }
	virtual ~FOPropDomain(){}
	const std::vector<Variable*>&	vars()	const { return _vars;	}
	virtual FOPropDomain* clone() const = 0;	//!< Take a deep copy of the domain
};

/**
 * A domain represented by a first-order BDD
 */
class FOPropBDDDomain : public FOPropDomain {
private:
	const FOBDD* _bdd;
public:
	FOPropBDDDomain(const FOBDD* bdd, const std::vector<Variable*>& vars): FOPropDomain(vars), _bdd(bdd) { }
	FOPropBDDDomain* clone() const { return new FOPropBDDDomain(_bdd,_vars);	}
	const FOBDD*	bdd() const { return _bdd;	}
};

/**
 * A domain represented by a predicate table
 */
class FOPropTableDomain : public FOPropDomain {
private:
	PredTable*				_table;
public:
	FOPropTableDomain(PredTable* t, const std::vector<Variable*>& v) : FOPropDomain(v), _table(t) { }
	FOPropTableDomain* clone() const;
	PredTable* table() const { return _table;	}
};

template<class DomainType> struct ThreeValuedDomain;

/**
 * Class to create and managed domains for formulas
 */
template<class PropagatorDomain>
class FOPropDomainFactory {
	public:
		virtual ~FOPropDomainFactory(){}
		virtual PropagatorDomain*	trueDomain(const Formula*)		const = 0;	//!< Create a domain containing all tuples
		virtual PropagatorDomain* 	falseDomain(const Formula*)		const = 0;	//!< Create an empty domain
		virtual PropagatorDomain* 	formuladomain(const Formula*)	const = 0;
		virtual PropagatorDomain*	ctDomain(const PredForm*)		const = 0;
		virtual PropagatorDomain*	cfDomain(const PredForm*)		const = 0;
		virtual PropagatorDomain* 	exists(PropagatorDomain*, const std::set<Variable*>&) const = 0;
		virtual PropagatorDomain* 	forall(PropagatorDomain*, const std::set<Variable*>&) const = 0;
		virtual PropagatorDomain* 	conjunction(PropagatorDomain*,PropagatorDomain*) const = 0;
		virtual PropagatorDomain*	disjunction(PropagatorDomain*,PropagatorDomain*) const = 0;
		virtual PropagatorDomain*	substitute(PropagatorDomain*,const std::map<Variable*,Variable*>&) const = 0;
		virtual bool			approxequals(PropagatorDomain*,PropagatorDomain*) const = 0;	//!< Checks if two domains are equal
		virtual PredInter*		inter(const std::vector<Variable*>&, const ThreeValuedDomain<PropagatorDomain>&, AbstractStructure*) const = 0;
		virtual std::ostream&	put(std::ostream&,PropagatorDomain*) const = 0;
};

class FOPropBDDDomainFactory : public FOPropDomainFactory<FOPropBDDDomain> {
	private:
		FOBDDManager* _manager;
	public:
		FOPropBDDDomainFactory();
		FOBDDManager*		manager()	const { return _manager;	}
		FOPropBDDDomain*	trueDomain(const Formula*)		const;
		FOPropBDDDomain*	falseDomain(const Formula*)		const;
		FOPropBDDDomain*	formuladomain(const Formula*)	const;
		FOPropBDDDomain*	ctDomain(const PredForm*)		const;
		FOPropBDDDomain*	cfDomain(const PredForm*)		const;
		FOPropBDDDomain*	exists(FOPropBDDDomain*, const std::set<Variable*>&) const;
		FOPropBDDDomain*	forall(FOPropBDDDomain*, const std::set<Variable*>&) const;
		FOPropBDDDomain*	conjunction(FOPropBDDDomain*,FOPropBDDDomain*) const;
		FOPropBDDDomain*	disjunction(FOPropBDDDomain*,FOPropBDDDomain*) const;
		FOPropBDDDomain*	substitute(FOPropBDDDomain*,const std::map<Variable*,Variable*>&) const;
		bool				approxequals(FOPropBDDDomain*,FOPropBDDDomain*)	const;
		PredInter*			inter(const std::vector<Variable*>&, const ThreeValuedDomain<FOPropBDDDomain>&, AbstractStructure*) const;
		std::ostream&		put(std::ostream&,FOPropBDDDomain*) const;
};

class FOPropTableDomainFactory : public FOPropDomainFactory<FOPropTableDomain> {
	private:
		AbstractStructure*	_structure;
	public:
		FOPropTableDomainFactory(AbstractStructure*);
		FOPropTableDomain*	trueDomain(const Formula*)		const;
		FOPropTableDomain*	falseDomain(const Formula*)		const;
		FOPropTableDomain*	formuladomain(const Formula*)	const;
		FOPropTableDomain*	ctDomain(const PredForm*)		const;
		FOPropTableDomain*	cfDomain(const PredForm*)		const;
		FOPropTableDomain*	exists(FOPropTableDomain*, const std::set<Variable*>&) const;
		FOPropTableDomain*	forall(FOPropTableDomain*, const std::set<Variable*>&) const;
		FOPropTableDomain*	conjunction(FOPropTableDomain*,FOPropTableDomain*) const;
		FOPropTableDomain*	disjunction(FOPropTableDomain*,FOPropTableDomain*) const;
		FOPropTableDomain*	substitute(FOPropTableDomain*,const std::map<Variable*,Variable*>&) const;
		bool				equals(FOPropTableDomain*,FOPropTableDomain*)	const;
		PredInter*			inter(const std::vector<Variable*>&, const ThreeValuedDomain<FOPropTableDomain>&, AbstractStructure*) const;
		std::ostream&		put(std::ostream&,FOPropTableDomain*) const;
};

/**
 * 	A domain is split in a certainly true and a certainly false part.
 */
template<class DomainType>
struct ThreeValuedDomain {
	DomainType* 	_ctdomain;
	DomainType* 	_cfdomain;
	bool			_twovalued;
	//ThreeValuedDomain() : _ctdomain(NULL), _cfdomain(NULL), _twovalued(false) { }

	ThreeValuedDomain(const FOPropDomainFactory<DomainType>* factory, bool ctdom, bool cfdom, const Formula* f) {
		_ctdomain = ctdom ? factory->trueDomain(f) : factory->falseDomain(f);
		_cfdomain = cfdom ? factory->trueDomain(f) : factory->falseDomain(f);
		_twovalued = ctdom || cfdom;
		Assert(_ctdomain!=NULL);
		Assert(_cfdomain!=NULL);
	}

	ThreeValuedDomain(const FOPropDomainFactory<DomainType>* factory, const Formula* f) {
		_ctdomain = factory->formuladomain(f);
		Formula* negf = f->clone(); negf->negate();
		_cfdomain = factory->formuladomain(negf);
		_twovalued = true;
		Assert(_ctdomain!=NULL);
		Assert(_cfdomain!=NULL);
	}

	ThreeValuedDomain(const FOPropDomainFactory<DomainType>* factory, const PredForm* pf, InitBoundType ibt) {
		_twovalued = false;
		switch(ibt) {
		case IBT_CT:
			_ctdomain = factory->ctDomain(pf);
			_cfdomain = factory->falseDomain(pf);
			break;
		case IBT_CF:
			_ctdomain = factory->falseDomain(pf);
			_cfdomain = factory->cfDomain(pf);
			break;
		case IBT_BOTH:
			_ctdomain = factory->ctDomain(pf);
			_cfdomain = factory->cfDomain(pf);
			break;
		}
		Assert(_ctdomain!=NULL);
		Assert(_cfdomain!=NULL);
	}
};

/**
 * Struct to represent the connection between atomic formulas over the same symbol
 */
template<class DomainType>
struct LeafConnectData {
	PredForm*						_connector;
	DomainType*						_equalities;
	std::map<Variable*,Variable*>	_leaftoconnector;
	std::map<Variable*,Variable*>	_connectortoleaf;
};

template<class DomainType>
class AdmissibleBoundChecker {
public:
	virtual ~AdmissibleBoundChecker(){}
	virtual bool check(DomainType*,DomainType*) const = 0;
};

class LongestBranchChecker : public AdmissibleBoundChecker<FOPropBDDDomain> {
private:
	int				_treshhold;
	FOBDDManager*	_manager;
public:
	LongestBranchChecker(FOBDDManager* m, int th) : _treshhold(th), _manager(m) { }
	bool check(FOPropBDDDomain*,FOPropBDDDomain*) const;
};

template<class InterpretationFactory, class Domain>
class TypedFOPropagator : public FOPropagator {
	VISITORFRIENDS()
private:
	Options*												_options;
	int														_verbosity;
	int														_maxsteps;		//!< Maximum number of propagations
	InterpretationFactory*									_factory;		//!< Manages and creates domains for formulas
	FOPropScheduler*										_scheduler;		//!< Schedules propagations
	std::map<const Formula*,ThreeValuedDomain<Domain> >		_domains;		//!< Map each formula to its current domain
	std::map<const Formula*,std::set<Variable*> >			_quantvars;
	std::map<PFSymbol*,PredForm*>							_leafconnectors;
	std::map<const PredForm*,LeafConnectData<Domain> >		_leafconnectdata;
	std::map<const Formula*,const Formula*>					_upward;
	std::map<const PredForm*,std::set<const PredForm*> >	_leafupward;
	std::vector<AdmissibleBoundChecker<Domain>*>			_admissiblecheckers;

	// Variables to temporarily store a propagation
	FOPropDirection		_direction;
	bool				_ct;
	const Formula*		_child;

	Domain* addToConjunction(Domain* conjunction, Domain* newconjunct);
	Domain* addToDisjunction(Domain* disjunction, Domain* newdisjunct);
	Domain* addToExists(Domain* exists, const std::set<Variable*>&);
	Domain* addToForall(Domain* forall, const std::set<Variable*>&);

	void updateDomain(const Formula* tobeupdated,FOPropDirection,bool ct,Domain* newdomain,const Formula* child = 0);
	bool admissible(Domain*, Domain*) const;	//!< Returns true iff the first domain is an allowed
															//!< replacement of the second domain

protected:
	void visit(const PredForm*);
	void visit(const EqChainForm*);
	void visit(const EquivForm*);
	void visit(const BoolForm*);
	void visit(const QuantForm*);
	void visit(const AggForm*);

public:
	TypedFOPropagator(InterpretationFactory*, FOPropScheduler*, Options*);

	void doPropagation();		//!< Apply propagations until the propagation queue is empty

	AbstractStructure*	currstructure(AbstractStructure* str) const;
		//!< Obtain the resulting structure
		//!< (the given structure is used to evaluate BDDs in case of symbolic propagation)
	GenerateBDDAccordingToBounds*	symbolicstructure()		const;
		//!< Obtain the resulting structure (only works if the used domainfactory is a FOPropBDDDomainFactory)

	void schedule(const Formula* par, FOPropDirection, bool, const Formula* child);

	// TODO check that domains can never contain nullpointers!

	int getVerbosity() const { return _verbosity; }
	int getMaxSteps() const { return _maxsteps; }
	void setMaxSteps(int steps) { _maxsteps = steps; }
	const Options* getOptions() const { return _options; }

	const std::map<const Formula*,const Formula*>& getUpward() const { return _upward; }
	const std::map<PFSymbol*,PredForm*>& getLeafConnectors() const { return _leafconnectors; }

	void setDomain(const Formula* key, const ThreeValuedDomain<Domain>& value ) { _domains.insert(std::pair<const Formula*, const ThreeValuedDomain<Domain> >(key, value)); }
	void setQuantVar(const Formula* key, const std::set<Variable*>& value) { _quantvars[key] = value;}
	void setUpward(const Formula* key, const Formula* value) { _upward[key] = value;}
	void setLeafConnector(PFSymbol* key,PredForm* value) { _leafconnectors[key] = value; }
	void setLeafConnectData(const PredForm* key, const LeafConnectData<Domain>& value) { _leafconnectdata[key] = value;}

	void addToLeafUpward(PredForm* index, const PredForm* pf) { _leafupward[index].insert(pf); }

	bool hasDomain(const Formula* f) const { return _domains.find(f)!=_domains.cend(); }
	const ThreeValuedDomain<Domain>& getDomain(const Formula* f) const { Assert(hasDomain(f)); return _domains.at(f); }
	void setCFOfDomain(const Formula* f, Domain* d) { _domains.at(f)._cfdomain = d; }
	void setCTOfDomain(const Formula* f, Domain* d) { _domains.at(f)._ctdomain = d; }

	InterpretationFactory* getFactory() const { return _factory; }
};

#endif
