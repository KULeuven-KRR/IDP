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

#pragma once

#include "inferences/grounding/grounders/FormulaGrounders.hpp"

class LazyDisjunctiveGrounder: public ClauseGrounder {
private:
	mutable tablesize alreadyground; // Statistics
	bool useExplicitTseitins;
	LazyInstantiation* _instance;

public:
	LazyDisjunctiveGrounder(AbstractGroundTheory* groundtheory, SIGN sign, bool conj, const GroundingContext& ct, bool explicitTseitins);
	virtual ~LazyDisjunctiveGrounder();

	void notifyGroundingRequested(int ID, bool groundall, LazyInstantiation* instance, bool& stilldelayed) const;
	void notifyTheoryOccurrence(LazyInstantiation* instance, TsType type) const;

protected:
	virtual litlist groundMore(bool groundall, LazyInstantiation * instance, bool& stilldelayed) const;

	virtual void internalClauseRun(ConjOrDisj& formula, LazyGroundingRequest& request);

	virtual void initializeInst(LazyInstantiation* inst) const = 0;
	virtual FormulaGrounder* getLazySubGrounder(LazyInstantiation* instance) const = 0;
	virtual bool incrementAndCheckDecided(LazyInstantiation* instance) const = 0;
	virtual bool isAtEnd(LazyInstantiation* instance) const = 0;
	virtual void prepareToGroundForVarInstance(LazyInstantiation*) const = 0;

	bool isRedundant(Lit l) const;
	bool decidesFormula(Lit l) const;
};

class LazyExistsGrounder: public LazyDisjunctiveGrounder {
private:
	FormulaGrounder* _subgrounder;
	InstGenerator* _generator;
	InstChecker* _checker;
public:
	LazyExistsGrounder(AbstractGroundTheory* groundtheory, FormulaGrounder* sub, InstGenerator* gen, InstChecker* checker, const GroundingContext& ct,
			bool explicitTseitins, SIGN sign, QUANT quant, const std::set<const DomElemContainer*>& generates, const tablesize& quantsize);
	~LazyExistsGrounder();

	InstChecker* getChecker() const {
		return _checker;
	}

protected:
	virtual void initializeInst(LazyInstantiation* inst) const;
	virtual FormulaGrounder* getLazySubGrounder(LazyInstantiation* instance) const;
	virtual bool incrementAndCheckDecided(LazyInstantiation* instance) const;
	virtual bool isAtEnd(LazyInstantiation* instance) const;
	virtual void prepareToGroundForVarInstance(LazyInstantiation* instance) const;

	FormulaGrounder* getSubGrounder() const {
		return _subgrounder;
	}
};

class LazyDisjGrounder: public LazyDisjunctiveGrounder {
private:
	std::vector<FormulaGrounder*> _subgrounders;
public:
	LazyDisjGrounder(AbstractGroundTheory* groundtheory, std::vector<FormulaGrounder*> sub, SIGN sign, bool conj, const GroundingContext& ct,
			bool explicitTseitins);
	~LazyDisjGrounder();

protected:
	virtual void initializeInst(LazyInstantiation* inst) const;
	virtual FormulaGrounder* getLazySubGrounder(LazyInstantiation* instance) const;
	virtual bool incrementAndCheckDecided(LazyInstantiation* instance) const;
	virtual bool isAtEnd(LazyInstantiation* instance) const;
	virtual void prepareToGroundForVarInstance(LazyInstantiation*) const {
	}

	const std::vector<FormulaGrounder*>& getSubGrounders() const {
		return _subgrounders;
	}
};
