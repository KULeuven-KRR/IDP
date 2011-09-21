/************************************
	propagate.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PROPAGATE_HPP
#define PROPAGATE_HPP

#include <vector>
#include <queue>
#include <map>
#include <cassert>
#include "theory.hpp"
#include "options.hpp"
#include "symbolicstructure.hpp"

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

enum InitBoundType { IBT_TWOVAL, IBT_BOTH, IBT_CT, IBT_CF, IBT_NONE };

/**
 * 	Class for scheduling propagation steps.	
 */
class FOPropScheduler {
	private:
		std::queue<FOPropagation*>	_queue;		//!< The queue of scheduled propagations
	public:
		// Mutators
		void			add(FOPropagation*);	//!< Push a propagation on the queue
		FOPropagation*	next();					//!< Pop the first propagation from the queue and return it
	
		// Inspectors
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
										//!< then the ct-domain of the parent is propagated the child, etc.
		const Formula*	_child;			//!< The subformula
	public:
		FOPropagation(const Formula* p, FOPropDirection dir, bool ct, const Formula* c = 0):
			_parent(p), _direction(dir), _ct(ct), _child(c) { }
	friend class FOPropagator;
};

/**
 * 	Class representing domains for formulas.
 */
class FOPropDomain {
	protected:
		std::vector<Variable*>	_vars;
	public:
		FOPropDomain(const std::vector<Variable*>& vars) : _vars(vars) { }
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

struct ThreeValuedDomain;

/**
 * Class to create and managed domains for formulas
 */
class FOPropDomainFactory {
	public:
		virtual FOPropDomain*	trueDomain(const Formula*)		const = 0;	//!< Create a domain containing all tuples
		virtual FOPropDomain* 	falseDomain(const Formula*)		const = 0;	//!< Create an empty domain 
		virtual FOPropDomain* 	formuladomain(const Formula*)	const = 0;
		virtual FOPropDomain*	ctDomain(const PredForm*)		const = 0;
		virtual FOPropDomain*	cfDomain(const PredForm*)		const = 0;
		virtual FOPropDomain* 	exists(FOPropDomain*, const std::set<Variable*>&) const = 0;
		virtual FOPropDomain* 	forall(FOPropDomain*, const std::set<Variable*>&) const = 0;
		virtual FOPropDomain* 	conjunction(FOPropDomain*,FOPropDomain*) const = 0;
		virtual FOPropDomain*	disjunction(FOPropDomain*,FOPropDomain*) const = 0;
		virtual FOPropDomain*	substitute(FOPropDomain*,const std::map<Variable*,Variable*>&) const = 0;
		virtual bool			approxequals(FOPropDomain*,FOPropDomain*) const = 0;	//!< Checks if two domains are equal
		virtual PredInter*		inter(const std::vector<Variable*>&, const ThreeValuedDomain&, AbstractStructure*) const = 0;
		virtual std::ostream&	put(std::ostream&,FOPropDomain*) const = 0;
};

class FOPropBDDDomainFactory : public FOPropDomainFactory {
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
		FOPropBDDDomain*	exists(FOPropDomain*, const std::set<Variable*>&) const;
		FOPropBDDDomain*	forall(FOPropDomain*, const std::set<Variable*>&) const;
		FOPropBDDDomain*	conjunction(FOPropDomain*,FOPropDomain*) const;
		FOPropBDDDomain*	disjunction(FOPropDomain*,FOPropDomain*) const;
		FOPropBDDDomain*	substitute(FOPropDomain*,const std::map<Variable*,Variable*>&) const;
		bool				approxequals(FOPropDomain*,FOPropDomain*)	const;
		PredInter*			inter(const std::vector<Variable*>&, const ThreeValuedDomain&, AbstractStructure*) const;
		std::ostream&		put(std::ostream&,FOPropDomain*) const;
};

class FOPropTableDomainFactory : public FOPropDomainFactory {
	private:
		AbstractStructure*	_structure;
	public:
		FOPropTableDomainFactory(AbstractStructure*);
		FOPropTableDomain*	trueDomain(const Formula*)		const;
		FOPropTableDomain*	falseDomain(const Formula*)		const;
		FOPropTableDomain*	formuladomain(const Formula*)	const;
		FOPropTableDomain*	ctDomain(const PredForm*)		const;
		FOPropTableDomain*	cfDomain(const PredForm*)		const;
		FOPropTableDomain*	exists(FOPropDomain*, const std::set<Variable*>&) const;
		FOPropTableDomain*	forall(FOPropDomain*, const std::set<Variable*>&) const;
		FOPropTableDomain*	conjunction(FOPropDomain*,FOPropDomain*) const;
		FOPropTableDomain*	disjunction(FOPropDomain*,FOPropDomain*) const;
		FOPropTableDomain*	substitute(FOPropDomain*,const std::map<Variable*,Variable*>&) const;
		bool				equals(FOPropDomain*,FOPropDomain*)	const;
		PredInter*			inter(const std::vector<Variable*>&, const ThreeValuedDomain&, AbstractStructure*) const;
		std::ostream&		put(std::ostream&,FOPropDomain*) const;
};



/**
 * 	A domain is split in a certainly true and a certainly false part.
 */
struct ThreeValuedDomain {
	FOPropDomain* 	_ctdomain;
	FOPropDomain* 	_cfdomain;
	bool			_twovalued;
	ThreeValuedDomain() : _ctdomain(0), _cfdomain(0), _twovalued(false) { }
	ThreeValuedDomain(const FOPropDomainFactory*, bool ctdom, bool cfdom, const Formula*);
	ThreeValuedDomain(const FOPropDomainFactory*, const Formula*);
	ThreeValuedDomain(const FOPropDomainFactory*, const PredForm*, InitBoundType);
};

/**
 * Struct to represent the connection between atomic formulas over the same symbol
 */
struct LeafConnectData {
	PredForm*						_connector;
	FOPropDomain*					_equalities;
	std::map<Variable*,Variable*>	_leaftoconnector;
	std::map<Variable*,Variable*>	_connectortoleaf;
};

class AdmissibleBoundChecker {
	public:
		virtual bool check(FOPropDomain*,FOPropDomain*) const = 0;
};

class LongestBranchChecker : public AdmissibleBoundChecker {
	private:
		int				_treshhold;
		FOBDDManager*	_manager;
	public:
		LongestBranchChecker(FOBDDManager* m, int th) : _treshhold(th), _manager(m) { }
		bool check(FOPropDomain*,FOPropDomain*) const;
};

/**
 * Constraint propagator for first-order theories
 */
class FOPropagator : public TheoryVisitor {
	private:
		Options*												_options;
		int														_verbosity;
		int														_maxsteps;		//!< Maximum number of propagations
		FOPropDomainFactory*									_factory;		//!< Manages and creates domains for formulas
		FOPropScheduler*										_scheduler;		//!< Schedules propagations
		std::map<const Formula*,ThreeValuedDomain>				_domains;		//!< Map each formula to its current domain
		std::map<const Formula*,std::set<Variable*> >			_quantvars;
		std::map<PFSymbol*,PredForm*>							_leafconnectors;
		std::map<const PredForm*,LeafConnectData>				_leafconnectdata;
		std::map<const Formula*,const Formula*>					_upward;
		std::map<const PredForm*,std::set<const PredForm*> >	_leafupward;
		std::vector<AdmissibleBoundChecker*>					_admissiblecheckers;

		// Variables to temporarily store a propagation
		FOPropDirection		_direction;
		bool				_ct;
		const Formula*		_child;

		FOPropDomain* addToConjunction(FOPropDomain* conjunction, FOPropDomain* newconjunct);
		FOPropDomain* addToDisjunction(FOPropDomain* disjunction, FOPropDomain* newdisjunct);
		FOPropDomain* addToExists(FOPropDomain* exists, const std::set<Variable*>&);
		FOPropDomain* addToForall(FOPropDomain* forall, const std::set<Variable*>&);

		void updateDomain(const Formula* tobeupdated,FOPropDirection,bool ct,FOPropDomain* newdomain,const Formula* child = 0);
		void schedule(const Formula* par, FOPropDirection, bool, const Formula* child);
		bool admissible(FOPropDomain*, FOPropDomain*) const;	//!< Returns true iff the first domain is an allowed
																//!< replacement of the second domain

	public:
		// Constructor
		FOPropagator(FOPropDomainFactory* f, FOPropScheduler* s, Options* opts); 

		// Execution
		void run();		//!< Apply propagations until the propagation queue is empty

		// Visitor
		void visit(const PredForm*);
		void visit(const EqChainForm*);
		void visit(const EquivForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);
		void visit(const AggForm*);

		// Inspectors
		AbstractStructure*	currstructure(AbstractStructure* str) const;	
			//!< Obtain the resulting structure 
			//!< (the given structure is used to evaluate BDDs in case of symbolic propagation)
		SymbolicStructure*	symbolicstructure()		const;
			//!< Obtain the resulting structure (only works if the used domainfactory is a FOPropBDDDomainFactory)
		FuncInter*	interpretation(Function* f)		const;	//!< Returns the current interpretation of function symbol f
		PredInter*	interpretation(Predicate* p)	const;	//!< Returns the current interpretation of predicate symbol p

	friend class FOPropagatorFactory;
};


/**
 * 	Factory class for creating a FOPropagator and initializing the scheduler
 * 	and domains for formulas in a theory.
 */
class FOPropagatorFactory : public TheoryVisitor {
	private:
		FOPropagator*						_propagator;
		std::map<PFSymbol*,PredForm*>		_leafconnectors;
		std::map<PFSymbol*,InitBoundType>	_initbounds;
		bool								_assertsentences;
		bool								_multiplymaxsteps;
		
		void createleafconnector(PFSymbol*);
		void initFalse(const Formula*);		//!< Set the ct- and cf-domains of the given formula to empty

		void visit(const Theory*);
		void visit(const PredForm*);
		void visit(const EqChainForm*);
		void visit(const EquivForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);
		void visit(const AggForm*);
	public:
		// Constructors
		FOPropagatorFactory(FOPropDomainFactory* f, FOPropScheduler*, bool as, const std::map<PFSymbol*,InitBoundType>& in, Options* opts);

		// Factory methods
		FOPropagator* create(const AbstractTheory* t);
	
};

#endif
