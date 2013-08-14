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

#ifndef OPTIMTERMGROUNDERS_HPP_
#define OPTIMTERMGROUNDERS_HPP_

#include "IncludeComponents.hpp" // TODO too general
#include "inferences/grounding/grounders/Grounder.hpp"

class AbstractGroundTheory;
class SortTable;
class Term;
class Variable;
struct GroundTerm;
class DomainElement;
class DomElemContainer;
class FuncTable;
class Function;

class GroundTranslator;

class OptimizationGrounder: public Grounder {
private:
	Term* _origterm;
protected:
	void printOrig() const;
public:
	OptimizationGrounder(AbstractGroundTheory* g, const GroundingContext& context, Term const*const origterm);
	virtual ~OptimizationGrounder();

	GroundTranslator* getTranslator() const;
};

class SetGrounder;

class AggregateOptimizationGrounder: public OptimizationGrounder {
private:
	AggFunction _type;
	SetGrounder* _setgrounder;

protected:
	virtual void internalRun(ConjOrDisj& formula, LazyGroundingRequest& request);

public:
	// NOTE: passes grounder ownership!
	AggregateOptimizationGrounder(AbstractGroundTheory* grounding, AggFunction tp, SetGrounder* gr, const GroundingContext& context, Term const*const  origterm)
			: OptimizationGrounder(grounding, context, origterm), _type(tp), _setgrounder(gr) {
	}
	~AggregateOptimizationGrounder();

	virtual void put(std::ostream&) const;
};

class TermGrounder;

class VariableOptimizationGrounder: public OptimizationGrounder {
private:
	TermGrounder* _termgrounder;

protected:
	virtual void internalRun(ConjOrDisj& formula, LazyGroundingRequest& request);

public:
	// NOTE: passes grounder ownership!
	VariableOptimizationGrounder(AbstractGroundTheory* grounding, TermGrounder* gr, const GroundingContext& context, Term const*const  origterm)
			: OptimizationGrounder(grounding, context, origterm), _termgrounder(gr) {
	}
	~VariableOptimizationGrounder();

	virtual void put(std::ostream&) const;
};

#endif /* OPTIMTERMGROUNDERS_HPP_ */
