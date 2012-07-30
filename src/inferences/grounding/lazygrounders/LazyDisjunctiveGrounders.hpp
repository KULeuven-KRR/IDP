/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef LAZYDISJUNCTIVEGROUNDERS_HPP_
#define LAZYDISJUNCTIVEGROUNDERS_HPP_

#include "inferences/grounding/grounders/FormulaGrounders.hpp"

class LazyDisjunctiveGrounder: public ClauseGrounder {
private:
	const std::set<Variable*> freevars; // The freevariables according to which we have to ground
	mutable tablesize alreadyground; // Statistics
	bool useExplicitTseitins;

public:
	LazyDisjunctiveGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, SIGN sign, bool conj, const GroundingContext& ct, bool explicitTseitins);
	virtual ~LazyDisjunctiveGrounder() {
	}

	void notifyGroundingRequested(int ID, bool groundall, LazyInstantiation* instance, bool& stilldelayed) const;
	void notifyTheoryOccurrence(LazyInstantiation* instance, TsType type) const;

protected:
	virtual litlist groundMore(bool groundall, LazyInstantiation * instance, bool& stilldelayed) const;

	virtual void internalRun(ConjOrDisj& formula) const;

	virtual void initializeInst(LazyInstantiation* inst) const = 0;
	virtual Grounder* getLazySubGrounder(LazyInstantiation* instance) const = 0;
	virtual void increment(LazyInstantiation* instance) const = 0;
	virtual bool isAtEnd(LazyInstantiation* instance) const = 0;
	virtual void prepareToGroundForVarInstance(LazyInstantiation*) const = 0;
};

class LazyExistsGrounder: public LazyDisjunctiveGrounder {
private:
	FormulaGrounder* _subgrounder;
	InstGenerator* _generator;
	InstChecker* _checker;
public:
	LazyExistsGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, FormulaGrounder* sub, SIGN sign, QUANT q, InstGenerator* gen,
			InstChecker* checker, const GroundingContext& ct, bool explicitTseitins);
	~LazyExistsGrounder();

protected:
	virtual void initializeInst(LazyInstantiation* inst) const;
	virtual Grounder* getLazySubGrounder(LazyInstantiation* instance) const;
	virtual void increment(LazyInstantiation* instance) const;
	virtual bool isAtEnd(LazyInstantiation* instance) const;
	virtual void prepareToGroundForVarInstance(LazyInstantiation* instance) const;

	FormulaGrounder* getSubGrounder() const {
		return _subgrounder;
	}
};

class LazyDisjGrounder: public LazyDisjunctiveGrounder {
private:
	std::vector<Grounder*> _subgrounders;
public:
	LazyDisjGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, std::vector<Grounder*> sub, SIGN sign, bool conj,
			const GroundingContext& ct, bool explicitTseitins);
	~LazyDisjGrounder();

protected:
	virtual void initializeInst(LazyInstantiation* inst) const;
	virtual Grounder* getLazySubGrounder(LazyInstantiation* instance) const;
	virtual void increment(LazyInstantiation* instance) const;
	virtual bool isAtEnd(LazyInstantiation* instance) const;
	virtual void prepareToGroundForVarInstance(LazyInstantiation*) const{}

	const std::vector<Grounder*>& getSubGrounders() const {
		return _subgrounders;
	}
};

#endif /* LAZYDISJUNCTIVEGROUNDERS_HPP_ */
