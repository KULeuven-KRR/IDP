#ifndef PROPAGATORFACTORY_HPP_
#define PROPAGATORFACTORY_HPP_

#include <map>
#include "theory.hpp"
class PFSymbol;
class PredForm;
class EqChainForm;
class EquivForm;
class BoolForm;
class AggForm;
class QuantForm;
class FOPropScheduler;
template<class InterpretationFactory, class PropDomain> class TypedFOPropagator;

enum InitBoundType { IBT_TWOVAL, IBT_BOTH, IBT_CT, IBT_CF, IBT_NONE };

/**
 * Constraint propagator for first-order theories
 */
class FOPropagator : public TheoryVisitor {
protected:
	FOPropagator();
public:
	// Execution
	virtual void run();		//!< Apply propagations until the propagation queue is empty

	// Visitor
	virtual void visit(const PredForm*);
	virtual void visit(const EqChainForm*);
	virtual void visit(const EquivForm*);
	virtual void visit(const BoolForm*);
	virtual void visit(const QuantForm*);
	virtual void visit(const AggForm*);

	// Inspectors
	virtual AbstractStructure*	currstructure(AbstractStructure* str) const;
		//!< Obtain the resulting structure
		//!< (the given structure is used to evaluate BDDs in case of symbolic propagation)
	virtual SymbolicStructure*	symbolicstructure()		const;
		//!< Obtain the resulting structure (only works if the used domainfactory is a FOPropBDDDomainFactory)
	virtual FuncInter*	interpretation(Function* f)		const;	//!< Returns the current interpretation of function symbol f
	virtual PredInter*	interpretation(Predicate* p)	const;	//!< Returns the current interpretation of predicate symbol p
};

/**
 * 	Factory class for creating a FOPropagator and initializing the scheduler
 * 	and domains for formulas in a theory.
 */
template<class InterpretationFactory, class PropDomain>
class FOPropagatorFactory : public TheoryVisitor {
	typedef TypedFOPropagator<InterpretationFactory, PropDomain> Propagator;
	private:
		int									_verbosity;
		Propagator*							_propagator;
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
		FOPropagatorFactory(InterpretationFactory*, FOPropScheduler*, bool as, const std::map<PFSymbol*,InitBoundType>&, Options*);

		Propagator* create(const AbstractTheory*);
};

FOPropagator* createPropagator(AbstractTheory* theory, const std::map<PFSymbol*,InitBoundType> mpi, Options* options);

#endif /* PROPAGATORFACTORY_HPP_ */
