/************************************
	propagate.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef PROPAGATE_HPP
#define PROPAGATE_HPP

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
		queue<FOPropagation*>	_queue;
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
		Formula* 		_formula;
		FOPropDirection	_direction;
		bool			_ct;
	public:
		FOPropagation(Formula* f, FOPropDirection dir, bool ct): _formula(f), _direction(dir), _ct(ct) { }
	friend class FOPropagator;
};

/**
 * DESCRIPTION
 * 	Class representing domains for formulas.
 */
class FOPropDomain {
};

class FOPropBDDDomain : public FOPropDomain {
	private:
		FOBDD* _bdd;
	public:
		FOPropBDDDomain(bdd): _bdd(bdd) { }
};


class FOPropDomainFactory {
	public:
		virtual FOPropDomain* trueDomain() 				const = 0;
		virtual FOPropDomain* falseDomain() 			const = 0;
		virtual FOPropDomain* ctDomain(const PredForm*)	const = 0;
		virtual FOPropDomain* cfDomain(const PredForm*)	const = 0;
		virtual FOPropDomain* domain(const PredForm*) 	const = 0;
		virtual FOPropDomain* exists(FOPropDomain*, const vector<Variable*>&) const = 0;
};

class FOPropBDDDomainFactory : public FOPropDomainFactory {
	private:
		FOBDDManager* _manager;
	public:
		FOPropBDDDomain* trueDomain() 				const;
		FOPropBDDDomain* falseDomain() 				const;
		FOPropBDDDomain* ctDomain(const PredForm*)	const;
		FOPropBDDDomain* cfDomain(const PredForm*)	const;
		FOPropBDDDomain* domain(const PredForm*) 	const;
		FOPropBDDDomain* exists(FOPropDomain*, const vector<Variable*>&) const;
};



/**
 * DESCRIPTION
 * 	A domain is split in a certainly true and a certainly false part.
 */
struct ThreeValuedDomain {
	FOPropDomain* 	_ctdomain;
	FOPropDomain* 	_cfdomain;
	bool			_twovalued;
	ThreeValuedDomain(const FOPropDomainFactory*, bool ctdom, bool cfdom);
	ThreeValuedDomain(const FOPropDomainFactory*, PredForm*, bool twovalued);
};

/**
 * DESCRIPTION
 * 	TODO
 */
class FOPropagator : public Visitor {
	private:
		FOPropDomainFactory*				_factory;
		FOPropScheduler* 					_scheduler;
		map<Formula*,ThreeValuedDomain>		_domains;
		map<Formula*,vector<Variable*> >	_quantvars;
		FOPropDirection						_direction;
		bool								_ct;
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

	friend class FOPropagatorFactory;
};

/**
 * DESCRIPTION
 * 	Factory class for creating a FOPropagator and initializing the scheduler
 * 	and domains for formulas in a theory.
 */
class FOPropagatorFactory : public Visitor {
	private:
		AbstractStructure* 			_structure;
		FOPropagator* 				_propagator;
		map<PFSymbol*,PredForm*>	_leafconnectors;
		bool						_assertsentences;
		
	public:
		// Constructors
		FOPropagatorFactory(FOPropagatorFactory* f, const AbstractStructure* s = 0);

		// Factory methods
		FOPropagator* create(const AbstractTheory* t);
	
		// Mutators
		initFalse(const Formula*);

		// Visitor	
		void visit(const Theory*);
		void visit(const PredForm*);
		void visit(const EqChainForm*);
		void visit(const EquivForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);
};

#endif
