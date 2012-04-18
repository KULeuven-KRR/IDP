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
class GroundTermTranslator;

class OptimizationGrounder {
private:
	AbstractGroundTheory* _grounding;
	Term* _origterm;
protected:
	void printOrig() const;
public:
	OptimizationGrounder(AbstractGroundTheory* g)
			: _grounding(g) {
	}
	virtual ~OptimizationGrounder();
	virtual void run() const = 0;
	void setOrig(const Term* t);

	AbstractGroundTheory* getGrounding() const { return _grounding; }
	GroundTranslator* getTranslator() const;
	GroundTermTranslator* getTermTranslator() const;
};

class SetGrounder;

class AggregateOptimizationGrounder: public OptimizationGrounder {
private:
	AggFunction _type;
	SetGrounder* _setgrounder;
public:
	AggregateOptimizationGrounder(AbstractGroundTheory* grounding, AggFunction tp, SetGrounder* gr)
			: OptimizationGrounder(grounding), _type(tp), _setgrounder(gr) {
	}
	void run() const;
};

#endif /* OPTIMTERMGROUNDERS_HPP_ */
