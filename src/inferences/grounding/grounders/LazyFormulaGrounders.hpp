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

class LazyUnknBoundGrounder{
private:
	unsigned int _id;

	bool _isGrounding;
	std::queue<std::pair<Lit, ElementTuple>> _stilltoground;

	AbstractGroundTheory* _grounding;

public:
	// @precondition: two IDs HAVE to be different if referring to an instance of a symbol in a DIFFERENT definition (if it is a head)
	//		it HAS to be -1 if it is not a head occurrence
	// 		in all other cases, they should preferably be equal
	LazyUnknBoundGrounder(PFSymbol* symbol, unsigned int id, AbstractGroundTheory* gt);
	virtual ~LazyUnknBoundGrounder(){}
	void ground(const Lit& boundlit, const ElementTuple& args);
	void notify(const Lit& boundlit, const ElementTuple& args, const std::vector<LazyUnknBoundGrounder*>& grounders);

	unsigned int getID() const { return _id; }

protected:
	AbstractGroundTheory* getGrounding() const { return _grounding; }

	void doGrounding();
	virtual void doGround(const Lit& boundlit, const ElementTuple& args) = 0;
};

class LazyUnknUnivGrounder: public FormulaGrounder, public LazyUnknBoundGrounder {
private:
	bool _isGrounding;
	std::queue<std::pair<Lit, ElementTuple>> _stilltoground;

	std::vector<const DomElemContainer*> _quantvars;

	FormulaGrounder* _subgrounder;
public:
	LazyUnknUnivGrounder(PFSymbol* symbol, const std::vector<const DomElemContainer*>& quantvars, AbstractGroundTheory* groundtheory, FormulaGrounder* sub, const GroundingContext& ct);

	virtual void run(ConjOrDisj& formula) const;

protected:
	FormulaGrounder* getSubGrounder() const{
		return _subgrounder;
	}

	dominstlist createInst(const ElementTuple& args);
	void doGround(const Lit& boundlit, const ElementTuple& args);
};

#endif /* LAZYQUANTGROUNDER_HPP_ */
