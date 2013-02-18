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

#ifndef TERMGROUNDERS_HPP_
#define TERMGROUNDERS_HPP_

#include "IncludeComponents.hpp" // TODO too general
#include "inferences/grounding/GroundUtils.hpp"

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

class TermGrounder {
private:
	mutable SortTable* _domain;
	Term* _origterm;
	var2dommap _varmap;
protected:
	GroundTranslator* _translator;

public:
	// @parameter dom: the sort of the position the term occurs in
	TermGrounder(SortTable* dom = NULL, GroundTranslator* translator = NULL)
			: _domain(dom), _origterm(NULL), _translator(translator) {
	}
	virtual ~TermGrounder();
	virtual GroundTerm run() const = 0;

	void setOrig(const Term* t, const var2dommap& mvd);
	void printOrig() const;

	SortTable* getDomain() const {
		return _domain;
	}

	int verbosity() const;
protected:
	void setDomain(SortTable* dom) const { // TODO ugly const setter!
		Assert(dom!=NULL);
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
	VarTermGrounder(GroundTranslator* tt, SortTable* domain, const DomElemContainer* a) : TermGrounder(domain, tt),
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
	Function* _function;
	FuncTable* _functable;
	std::vector<SortTable*> _tables; // Function argument sorts
	std::vector<TermGrounder*> _subtermgrounders;
public:
	FuncTermGrounder(GroundTranslator* tt, Function* func, FuncTable* ftable, SortTable* dom, const std::vector<SortTable*>& tables, const std::vector<TermGrounder*>& sub) :
			TermGrounder(dom, tt), _function(func), _functable(ftable), _tables(tables), _subtermgrounders(sub) {
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
	FuncTable* _functable;
	TermGrounder* _lefttermgrounder;
	TermGrounder* _righttermgrounder;
	SumType _type;
public:
	SumTermGrounder(GroundTranslator* tt, FuncTable* ftable, SortTable* dom, TermGrounder* ltg, TermGrounder* rtg, SumType type)
			: TermGrounder(dom, tt), _functable(ftable), _lefttermgrounder(ltg), _righttermgrounder(rtg), _type(type) {
	}
	GroundTerm run() const;
private:
	void computeDomain(const GroundTerm& left, const GroundTerm& right) const;
};

class ProdTermGrounder: public TermGrounder {
protected:
	FuncTable* _functable;
	TermGrounder* _lefttermgrounder;
	TermGrounder* _righttermgrounder;
public:
	ProdTermGrounder(GroundTranslator* tt, FuncTable* ftable, SortTable* dom, TermGrounder* ltg, TermGrounder* rtg)
			: TermGrounder(dom, tt), _functable(ftable), _lefttermgrounder(ltg), _righttermgrounder(rtg) {
	}
	GroundTerm run() const;
private:
	void computeDomain(const GroundTerm& left, const GroundTerm& right) const;
};

class TermWithFactorGrounder: public TermGrounder {
protected:
	FuncTable* _functable;
	TermGrounder* _factortermgrounder;
	TermGrounder* _subtermgrounder;
public:
	TermWithFactorGrounder(GroundTranslator* tt, FuncTable* ftable, SortTable* dom, TermGrounder* fg, TermGrounder* tg)
			: TermGrounder(dom, tt), _functable(ftable), _factortermgrounder(fg), _subtermgrounder(tg) {
	}
	GroundTerm run() const;
private:
	void computeDomain(const DomainElement* factor, const GroundTerm& term) const;
};


class SetGrounder;

CPTerm* createCPAggTerm(const AggFunction&, const varidlist&);

Weight getNeutralElement(AggFunction type);
varidlist rewriteCpTermsIntoVars(AggFunction type, AbstractGroundTheory* grounding, const litlist& conditions, const termlist& cpterms);

class AggTermGrounder: public TermGrounder {
private:
	AggFunction _type;
	SetGrounder* _setgrounder;
	AbstractGroundTheory* grounding;

public:
	AggTermGrounder(AbstractGroundTheory* grounding, GroundTranslator* gt, AggFunction tp, SortTable* dom, SetGrounder* gr);
	GroundTerm run() const;
};

#endif /* TERMGROUNDERS_HPP_ */
