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

#ifndef PROPAGATORFACTORY_HPP_
#define PROPAGATORFACTORY_HPP_

#include <map>
#include "IncludeComponents.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "PropagationCommon.hpp"


class Options;
class GenerateBDDAccordingToBounds;
class FOPropScheduler;
template<class InterpretationFactory, class PropDomain> class TypedFOPropagator;
class FOPropagator;




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
	Propagator* _propagator;
	std::map<PFSymbol*, PredForm*> _leafconnectors; //_propagator is responsible for deleting this
	std::map<PFSymbol*, InitBoundType> _initbounds;
	bool _assertsentences;
	bool _multiplymaxsteps;

	void createleafconnector(PFSymbol*);
	void initUnknown(const Formula*); //!< Set the ct- and cf-domains of the given formula to empty
	void initTwoVal(const Formula*); //!< Set domain of this formula to be itself

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
	~FOPropagatorFactory();

	Propagator* create(const AbstractTheory*, const Structure*);
};

// NOTE: structure can be NULL
FOPropagator* createPropagator(const AbstractTheory* theory, const Structure* structure, const std::map<PFSymbol*, InitBoundType> mpi);
std::shared_ptr<GenerateBDDAccordingToBounds> generateNonLiftedBounds(AbstractTheory* theory, Structure const * const structure);

/** Generates bounds for the given theory-structure combination
 *
 * @param doSymbolicPropagation: If false, it means that the bounds will be trivial (no propagation!!)
 * @param applyPropagationToStructure: decides whether or not we will apply the results of the propagation to the structure
 * @param outputvoc: We will evaluate all symbols in this vocabulary in the BDDs and modify the structure accordingly
 *
 * NOTE: if applyPropagationToStructure == false, then outputvoc is useless and can savely be NULL
 * NOTE: if outputvoc == NULL, we will propagate for EVERY symbol in structure
 */
std::shared_ptr<GenerateBDDAccordingToBounds> generateBounds(AbstractTheory* theory, Structure* structure, bool doSymbolicPropagation, bool applyPropagationToStructure, Vocabulary* outputvoc = NULL);

//GenerateBDDAccordingToBounds* generateNaiveApproxBounds(AbstractTheory* theory, Structure* structure);

/** Collect symbolic propagation vocabulary **/
std::map<PFSymbol*, InitBoundType> propagateVocabulary(AbstractTheory* theory, Structure const * const structure);


#endif /* PROPAGATORFACTORY_HPP_ */
