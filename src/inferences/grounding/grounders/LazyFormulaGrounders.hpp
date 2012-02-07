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

class LazyGrounder: public ClauseGrounder{
private:
	const std::set<Variable*> freevars; // The freevariables according to which we have to ground
	LazyGroundingManager lazyManager;
public:
	LazyGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, SIGN sign, bool conj, const GroundingContext& ct);
	virtual ~LazyGrounder(){}
	bool groundMore(ResidualAndFreeInst* instance) const;

protected:
	virtual void internalRun(ConjOrDisj& formula) const;
	virtual bool grounderIsEmpty() const = 0;
	virtual void initializeInst(ResidualAndFreeInst* inst) const = 0;
	virtual Grounder* getLazySubGrounder(ResidualAndFreeInst* instance) const = 0;
	virtual void increment(ResidualAndFreeInst* instance) const = 0;
	virtual bool isAtEnd(ResidualAndFreeInst* instance) const = 0;
	virtual void initializeGroundMore(ResidualAndFreeInst*) const {}
};

class LazyQuantGrounder: public LazyGrounder {
private:
	FormulaGrounder* _subgrounder;
	InstGenerator* _generator;
public:
	LazyQuantGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, FormulaGrounder* sub, SIGN sign, QUANT q, InstGenerator* gen, const GroundingContext& ct);

protected:
	virtual bool grounderIsEmpty() const;
	virtual void initializeInst(ResidualAndFreeInst* inst) const;
	virtual Grounder* getLazySubGrounder(ResidualAndFreeInst* instance) const;
	virtual void increment(ResidualAndFreeInst* instance) const;
	virtual bool isAtEnd(ResidualAndFreeInst* instance) const;
	virtual void initializeGroundMore(ResidualAndFreeInst* instance) const;

	FormulaGrounder* getSubGrounder() const{
		return _subgrounder;
	}
};

class LazyBoolGrounder: public LazyGrounder {
private:
	std::vector<Grounder*> _subgrounders;
public:
	LazyBoolGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, std::vector<Grounder*> sub, SIGN sign, bool conj, const GroundingContext& ct);

protected:
	virtual bool grounderIsEmpty() const;
	virtual void initializeInst(ResidualAndFreeInst* inst) const;
	virtual Grounder* getLazySubGrounder(ResidualAndFreeInst* instance) const;
	virtual void increment(ResidualAndFreeInst* instance) const;
	virtual bool isAtEnd(ResidualAndFreeInst* instance) const;

	const std::vector<Grounder*>& getSubGrounders() const {
		return _subgrounders;
	}
};

#endif /* LAZYQUANTGROUNDER_HPP_ */
