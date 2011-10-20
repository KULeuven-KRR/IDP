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
class FuncInter;
class PredInter;
class Options;
class SymbolicStructure;
class FOPropScheduler;
class Predicate;
class Function;
class PredTable;
template<class InterpretationFactory, class PropDomain> class TypedFOPropagator;

enum InitBoundType { IBT_TWOVAL, IBT_BOTH, IBT_CT, IBT_CF, IBT_NONE };

/**
 * Constraint propagator for first-order theories
 */
class FOPropagator : public TheoryVisitor {
public:
	virtual ~FOPropagator() {}

	virtual void run() = 0;		//!< Apply propagations until the propagation queue is empty

	// Inspectors
	virtual AbstractStructure*	currstructure(AbstractStructure* str) const = 0;
		//!< Obtain the resulting structure
		//!< (the given structure is used to evaluate BDDs in case of symbolic propagation)
	virtual SymbolicStructure*	symbolicstructure()		const = 0;
		//!< Obtain the resulting structure (only works if the used domainfactory is a FOPropBDDDomainFactory)
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
