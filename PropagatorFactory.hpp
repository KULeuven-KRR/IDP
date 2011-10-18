#ifndef PROPAGATORFACTORY_HPP_
#define PROPAGATORFACTORY_HPP_

/**
 * 	Factory class for creating a FOPropagator and initializing the scheduler
 * 	and domains for formulas in a theory.
 */
template<class InterpretationFactory, class PropDomain>
class FOPropagatorFactory : public TheoryVisitor {
	private:
		int									_verbosity;
		FOPropagator<PropDomain>*			_propagator;
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

		FOPropagator* create(const AbstractTheory*);
};

#endif /* PROPAGATORFACTORY_HPP_ */
