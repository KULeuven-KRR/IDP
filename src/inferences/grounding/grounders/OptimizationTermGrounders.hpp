/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef OPTIMTERMGROUNDERS_HPP_
#define OPTIMTERMGROUNDERS_HPP_

#include "IncludeComponents.hpp" // TODO too general
#include "inferences/grounding/grounders/Grounder.hpp"

class AbstractGroundTheory;
class SortTable;
class Term;
class Variable;
class GroundTerm;
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
	OptimizationGrounder(AbstractGroundTheory* g, const GroundingContext& context)
			: Grounder(g, context) {
	}
	virtual ~OptimizationGrounder();
	void setOrig(const Term* t);

	GroundTranslator* getTranslator() const;
};

class SetGrounder;

class AggregateOptimizationGrounder: public OptimizationGrounder {
private:
	AggFunction _type;
	SetGrounder* _setgrounder;
public:
	// NOTE: passes grounder ownership!
	AggregateOptimizationGrounder(AbstractGroundTheory* grounding, AggFunction tp, SetGrounder* gr, const GroundingContext& context)
			: OptimizationGrounder(grounding, context), _type(tp), _setgrounder(gr) {
	}
	~AggregateOptimizationGrounder();
	virtual void run(ConjOrDisj& formula) const;

	virtual void put(std::ostream&) const;
};

class TermGrounder;

class VariableOptimizationGrounder: public OptimizationGrounder {
private:
	TermGrounder* _termgrounder;
public:
	// NOTE: passes grounder ownership!
	VariableOptimizationGrounder(AbstractGroundTheory* grounding, TermGrounder* gr, const GroundingContext& context)
			: OptimizationGrounder(grounding, context), _termgrounder(gr) {
	}
	~VariableOptimizationGrounder();
	virtual void run(ConjOrDisj& formula) const;

	virtual void put(std::ostream&) const;
};

#endif /* OPTIMTERMGROUNDERS_HPP_ */
