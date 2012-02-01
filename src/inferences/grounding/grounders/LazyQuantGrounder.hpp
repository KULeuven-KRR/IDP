/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef LAZYQUANTGROUNDER_HPP_
#define LAZYQUANTGROUNDER_HPP_

#include "FormulaGrounders.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"

typedef GroundTheory<SolverPolicy> SolverTheory;

class LazyGroundingManager{
private:
	mutable bool currentlyGrounding; // If true, groundMore is currently already on the stack, so do not call it again!
	mutable std::queue<ResidualAndFreeInst *> queuedtseitinstoground; // Stack of what we still have to ground

public:
	LazyGroundingManager():currentlyGrounding(false){}
	void groundMore() const;

	// TODO for some reason, the callback framework does not compile when using the const method groundmore directly.
	void notifyBoundSatisfied(ResidualAndFreeInst* instance);
	void notifyBoundSatisfiedInternal(ResidualAndFreeInst* instance) const;
};

class LazyGrounder{
public:
	virtual ~LazyGrounder(){}
	virtual void groundMore(ResidualAndFreeInst* instance) const = 0;
};

class LazyQuantGrounder: public QuantGrounder, public LazyGrounder {
private:
	const std::set<Variable*> freevars; // The freevariables according to which we have to ground
	LazyGroundingManager lazyManager;

public:
	LazyQuantGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, FormulaGrounder* sub, SIGN sign, QUANT q, InstGenerator* gen,
			InstChecker* checker, const GroundingContext& ct);

	void groundMore(ResidualAndFreeInst* instance) const;

protected:
	virtual void internalRun(ConjOrDisj& literals) const;
};

class LazyBoolGrounder: public BoolGrounder, public LazyGrounder {
private:
	const std::set<Variable*> freevars; // The freevariables according to which we have to ground
	LazyGroundingManager lazyManager;

public:
	LazyBoolGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, std::vector<Grounder*> sub, SIGN sign, bool conj, const GroundingContext& ct);

	void groundMore(ResidualAndFreeInst* instance) const;

protected:
	virtual void internalRun(ConjOrDisj& literals) const;
};

#endif /* LAZYQUANTGROUNDER_HPP_ */
