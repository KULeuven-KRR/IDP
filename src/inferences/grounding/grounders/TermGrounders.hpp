/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef TERMGROUNDERS_HPP_
#define TERMGROUNDERS_HPP_

#include "IncludeComponents.hpp" // TODO too general
#include "GroundUtils.hpp"

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

class TermGrounder {
private:
	mutable SortTable* _domain;
	Term* _origterm;
	var2dommap _varmap;
public:
	// @parameter dom: the sort of the position the term occurs in
	TermGrounder(SortTable* dom = NULL)
			: _domain(dom) {
	}
	virtual ~TermGrounder();
	virtual GroundTerm run() const = 0;

	void setOrig(const Term* t, const var2dommap& mvd);
	void printOrig() const;

	SortTable* getDomain() const {
		return _domain;
	}
protected:
	void setDomain(SortTable* dom) const { // TODO ugly const setter!
		_domain = dom;
	}
};

class DomTermGrounder: public TermGrounder {
private:
	const DomainElement* _value;
public:
	DomTermGrounder(const DomainElement* val) :
			_value(val) {
	}
	GroundTerm run() const;
};

class VarTermGrounder: public TermGrounder {
private:
	const DomElemContainer* _value;
public:
	VarTermGrounder(const DomElemContainer* a) :
			_value(a) {
	}
	inline GroundTerm run() const {
		Assert(_value->get() != NULL);
		return GroundTerm(_value->get());
	}

	const DomElemContainer* getElement() const {
		return _value;
	}
};

class FuncTermGrounder: public TermGrounder {
protected:
	GroundTermTranslator* _termtranslator;
	Function* _function;
	FuncTable* _functable;
	std::vector<TermGrounder*> _subtermgrounders;
public:
	FuncTermGrounder(GroundTermTranslator* tt, Function* func, FuncTable* ftable, SortTable* dom, const std::vector<TermGrounder*>& sub) :
			TermGrounder(dom), _termtranslator(tt), _function(func), _functable(ftable), _subtermgrounders(sub) {
	}
	GroundTerm run() const;

	// TODO? Optimisation:
	//			Keep all values of the args + result of the previous call to run().
	//			If the values of the args did not change, return the result immediately instead of doing the
	//			table lookup
};

enum SumType {
	ST_PLUS, ST_MINUS
};

class SumTermGrounder: public TermGrounder {
protected:
	GroundTermTranslator* _termtranslator;
	FuncTable* _functable;
	TermGrounder* _lefttermgrounder;
	TermGrounder* _righttermgrounder;
	SumType _type;
public:
	SumTermGrounder(GroundTermTranslator* tt, FuncTable* ftable, SortTable* dom, TermGrounder* ltg, TermGrounder* rtg, SumType type = ST_PLUS)
			: TermGrounder(dom), _termtranslator(tt), _functable(ftable), _lefttermgrounder(ltg), _righttermgrounder(rtg), _type(type) {
	}
	GroundTerm run() const;
private:
	void computeDomain(GroundTerm& left, GroundTerm& right) const;
};

class SetGrounder;

CPTerm* createCPAggTerm(const AggFunction&, const varidlist&);

class AggTermGrounder: public TermGrounder {
private:
	GroundTranslator* _translator;
	GroundTermTranslator* _termtranslator;
	AggFunction _type;
	SetGrounder* _setgrounder;
public:
	AggTermGrounder(GroundTranslator* gt, GroundTermTranslator* tt, AggFunction tp, SetGrounder* gr) :
			_translator(gt), _termtranslator(tt), _type(tp), _setgrounder(gr) {
	}
	GroundTerm run() const;
};

#endif /* TERMGROUNDERS_HPP_ */
