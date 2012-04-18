/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef PROPAGATORFACTORY_HPP_
#define PROPAGATORFACTORY_HPP_

#include <map>
#include "IncludeComponents.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

class Options;
class GenerateBDDAccordingToBounds;
class FOPropScheduler;
template<class InterpretationFactory, class PropDomain> class TypedFOPropagator;

enum InitBoundType {
	IBT_TWOVAL, IBT_BOTH, IBT_CT, IBT_CF, IBT_NONE
};

/**
 * Constraint propagator for first-order theories
 */
class FOPropagator: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
public:
	virtual ~FOPropagator() {
	}

	virtual void doPropagation() = 0; //!< Apply propagations until the propagation queue is empty

	// Inspectors
	virtual void applyPropagationToStructure(AbstractStructure* str) const = 0;
	//!< Obtain the resulting structure
	//!< (the given structure is used to evaluate BDDs in case of symbolic propagation)

	virtual GenerateBDDAccordingToBounds* symbolicstructure() const = 0;
	//!< Obtain the resulting structure (only works if the used domainfactory is a FOPropBDDDomainFactory)
};

/**
 * 	Factory class for creating a FOPropagator and initializing the scheduler
 * 	and domains for formulas in a theory. Initially schedules bottom-up propagation of domain
 * 	knowledge and top-down knowledge that sentence are true (if bool as is true)
 */
template<class InterpretationFactory, class PropDomain>
class FOPropagatorFactory: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
	typedef TypedFOPropagator<InterpretationFactory, PropDomain> Propagator;
private:
	int _verbosity;
	Propagator* _propagator;
	std::map<PFSymbol*, PredForm*> _leafconnectors;
	std::map<PFSymbol*, InitBoundType> _initbounds;
	bool _assertsentences;
	bool _multiplymaxsteps;

	void createleafconnector(PFSymbol*);
	void initFalse(const Formula*); //!< Set the ct- and cf-domains of the given formula to empty

protected:
	void visit(const Theory*);
	void visit(const PredForm*);
	void visit(const EqChainForm*);
	void visit(const EquivForm*);
	void visit(const BoolForm*);
	void visit(const QuantForm*);
	void visit(const AggForm*);
public:
	FOPropagatorFactory(InterpretationFactory*, FOPropScheduler*, bool as, const std::map<PFSymbol*, InitBoundType>&);

	Propagator* create(const AbstractTheory*);
};

// NOTE: structure can be NULL
FOPropagator* createPropagator(AbstractTheory* theory, AbstractStructure* s, const std::map<PFSymbol*, InitBoundType> mpi);
GenerateBDDAccordingToBounds* generateBounds(AbstractTheory* theory, AbstractStructure*& structure);

GenerateBDDAccordingToBounds* generateNaiveApproxBounds(AbstractTheory* theory, AbstractStructure* structure);


#endif /* PROPAGATORFACTORY_HPP_ */
