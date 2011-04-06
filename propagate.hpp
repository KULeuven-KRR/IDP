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

#include "visitor.hpp"

class Variable;
class PFSymbol;
class FOBDD;
class FOPropagation;
class FOPropDomain;
class FOBDDManager;
class AbstractStructure;

/**
 * \file propagate.hpp
 * DESCRIPTION
 * 	File contains classes for propagation on first-order level.
 */

enum FOPropDirection { UP, DOWN };

/**
 * DESCRIPTION
 * 	Class for scheduling propagation steps.	
 */
class FOPropScheduler {
	private:
		std::queue<FOPropagation*>	_queue;
	public:
		// Mutators
		void			add(FOPropagation*);
		FOPropagation*	next();
	
		// Inspectors
		bool 			hasNext() const;
};

/**
 * DESCRIPTION
 * 	Class representing a single propagation step.
 */
class FOPropagation {
	private:
		Formula* 		_parent;
		FOPropDirection	_direction;
		bool			_ct;
		Formula*		_child;
	public:
		FOPropagation(Formula* p, FOPropDirection dir, bool ct, Formula* c = 0):
			_parent(p), _direction(dir), _ct(ct), _child(c) { }
	friend class FOPropagator;
};

/**
 * DESCRIPTION
 * 	Class representing domains for formulas.
 */
class FOPropDomain {
	public:
		virtual FOPropDomain* clone() const = 0;
};

class FOPropBDDDomain : public FOPropDomain {
	private:
		FOBDD* _bdd;
	public:
		FOPropBDDDomain(FOBDD* bdd): _bdd(bdd) { }
		FOPropBDDDomain* clone() const { return new FOPropBDDDomain(_bdd);	}
		FOBDD*	bdd() const { return _bdd;	}
};


class FOPropDomainFactory {
	public:
		virtual FOPropDomain* trueDomain()					const = 0;
		virtual FOPropDomain* falseDomain()					const = 0;
		virtual FOPropDomain* ctDomain(const PredForm*)		const = 0;
		virtual FOPropDomain* cfDomain(const PredForm*)		const = 0;
		virtual FOPropDomain* domain(const PredForm*)		const = 0;
		virtual FOPropDomain* domain(const EqChainForm*) 	const = 0;
		virtual FOPropDomain* exists(FOPropDomain*, const std::vector<Variable*>&) const = 0;
		virtual FOPropDomain* forall(FOPropDomain*, const std::vector<Variable*>&) const = 0;
		virtual FOPropDomain* conjunction(FOPropDomain*,FOPropDomain*) const = 0;
		virtual FOPropDomain* disjunction(FOPropDomain*,FOPropDomain*) const = 0;
		virtual FOPropDomain* substitute(FOPropDomain*,const std::map<Variable*,Variable*>&) const = 0;
};

class FOPropBDDDomainFactory : public FOPropDomainFactory {
	private:
		FOBDDManager* _manager;
	public:
		FOPropBDDDomain* trueDomain()					const;
		FOPropBDDDomain* falseDomain()					const;
		FOPropBDDDomain* ctDomain(const PredForm*)		const;
		FOPropBDDDomain* cfDomain(const PredForm*)		const;
		FOPropBDDDomain* domain(const PredForm*)		const;
		FOPropBDDDomain* domain(const EqChainForm*) 	const;
		FOPropBDDDomain* exists(FOPropDomain*, const std::vector<Variable*>&) const;
		FOPropBDDDomain* forall(FOPropDomain*, const std::vector<Variable*>&) const;
		FOPropBDDDomain* conjunction(FOPropDomain*,FOPropDomain*) const;
		FOPropBDDDomain* disjunction(FOPropDomain*,FOPropDomain*) const;
		FOPropBDDDomain* substitute(FOPropDomain*,const std::map<Variable*,Variable*>&) const;
};



/**
 * DESCRIPTION
 * 	A domain is split in a certainly true and a certainly false part.
 */
struct ThreeValuedDomain {
	FOPropDomain* 	_ctdomain;
	FOPropDomain* 	_cfdomain;
	bool			_twovalued;
	ThreeValuedDomain() : _ctdomain(0), _cfdomain(0), _twovalued(false) { assert(false);	}
	ThreeValuedDomain(const FOPropDomainFactory*, bool ctdom, bool cfdom);
	ThreeValuedDomain(const FOPropDomainFactory*, PredForm*, bool twovalued);
};



struct LeafConnectData {
	PredForm*						_connector;
	FOPropDomain*					_equalities;
	std::map<Variable*,Variable*>	_leaftoconnector;
	std::map<Variable*,Variable*>	_connectortoleaf;
};

/**
 * DESCRIPTION
 * 	TODO
 */
class FOPropagator : public Visitor {
	private:
		FOPropDomainFactory*								_factory;
		FOPropScheduler*									_scheduler;
		std::map<const Formula*,ThreeValuedDomain>			_domains;
		std::map<const Formula*,std::vector<Variable*> >	_quantvars;
		std::map<const PredForm*,LeafConnectData*>			_leafconnectdata;
		FOPropDirection										_direction;
		bool												_ct;
		Formula*											_child;

		FOPropDomain* addToConjunction(FOPropDomain* conjunction, FOPropDomain* newconjunct);
		FOPropDomain* addToDisjunction(FOPropDomain* disjunction, FOPropDomain* newdisjunct);
		FOPropDomain* addToExists(FOPropDomain* exists, const std::vector<Variable*>&);
		FOPropDomain* addToForall(FOPropDomain* forall, const std::vector<Variable*>&);

		void updateDomain(const Formula* tobeupdated,FOPropDirection,bool ct,FOPropDomain* newdomain,const Formula* child = 0);
	public:
		// Constructor
		FOPropagator(FOPropDomainFactory* f, FOPropScheduler* s): _factory(f), _scheduler(s) { }; 

		// Execution
		void run();

		// Visitor
		void visit(const PredForm*);
		void visit(const EqChainForm*);
		void visit(const EquivForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);
		void visit(const AggForm*);

	friend class FOPropagatorFactory;
};

/**
 * DESCRIPTION
 * 	Factory class for creating a FOPropagator and initializing the scheduler
 * 	and domains for formulas in a theory.
 */
class FOPropagatorFactory : public Visitor {
	private:
		const AbstractStructure*		_structure;
		FOPropagator*					_propagator;
		std::map<PFSymbol*,PredForm*>	_leafconnectors;
		bool							_assertsentences;
		
	public:
		// Constructors
		FOPropagatorFactory(FOPropDomainFactory* f, FOPropScheduler*, const AbstractStructure* s = 0);

		// Factory methods
		FOPropagator* create(const AbstractTheory* t);
	
		// Mutators
		void initFalse(const Formula*);

		// Visitor	
		void visit(const Theory*);
		void visit(const PredForm*);
		void visit(const EqChainForm*);
		void visit(const EquivForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);
};

#endif
