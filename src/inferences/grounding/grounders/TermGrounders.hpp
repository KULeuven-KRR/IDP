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

#include "IncludeComponents.hpp" // TODO too general
#include "inferences/grounding/GroundUtils.hpp"

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

class TermGrounder {
private:
	mutable SortTable* _domain;
	Term* _term;

protected:
	GroundTranslator* _translator;
	GroundTranslator* translator() const{return _translator;}

	var2dommap _varmap;

	void setTerm(Term* t){
		_term = t;
	}

	// @parameter dom: the sort of the position the term occurs in
	TermGrounder(SortTable* dom = NULL, GroundTranslator* translator = NULL)
			: _domain(dom), _term(NULL), _translator(translator) {
	}

public:
	virtual ~TermGrounder();
	virtual GroundTerm run() const = 0;

	bool hasTerm() const {
		return _term!=NULL;
	}
	virtual Term* getTerm() const {
		Assert(hasTerm());
		return _term;
	}
	virtual const var2dommap& getVarmapping() const {
		return _varmap;
	}

	void printOrig() const;

	SortTable* getDomain() const {
		return _domain;
	}

	virtual SortTable* getLatestDomain() const {
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
	DomTermGrounder(Sort* sort, const DomainElement* val)
			: _value(val) {
		setTerm(new DomainTerm(sort, _value, {}));
	}
	GroundTerm run() const;
	const DomainElement* getDomainElement() const {
		return _value;
	}
};

class VarTermGrounder: public TermGrounder {
private:
	const DomElemContainer* _value;
public:
	VarTermGrounder(GroundTranslator* tt, SortTable* domain, Variable* var, const DomElemContainer* a)
			:TermGrounder(domain, tt),
			_value(a) {
		_varmap.insert({var, _value});
		setTerm(new VarTerm(var, {}));
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
	FuncTermGrounder(GroundTranslator* tt, Function* func, FuncTable* ftable, SortTable* dom, const std::vector<SortTable*>& tables,
			const std::vector<TermGrounder*>& sub);
	virtual GroundTerm run() const;

	// TODO? Optimisation:
	//			Keep all values of the args + result of the previous call to run().
	//			If the values of the args did not change, return the result immediately instead of doing the
	//			table lookup
};

enum class TwinTT{
	PLUS, MIN, PROD
};

class TwinTermGrounder: public TermGrounder {
protected:
	TwinTT _type;
	FuncTable* _functable;
	TermGrounder* _lefttermgrounder;
	TermGrounder* _righttermgrounder;

	mutable SortTable* _latestdomain;
public:
	TwinTermGrounder(GroundTranslator* tt, Function* func, TwinTT type, FuncTable* ftable, SortTable* dom, TermGrounder* ltg, TermGrounder* rtg);
	GroundTerm run() const;

	virtual SortTable* getLatestDomain() const {
		return _latestdomain;
	}
private:
	SortTable* computeDomain(const GroundTerm& left, const GroundTerm& right) const;
};

class EnumSetGrounder;

class AggTermGrounder: public TermGrounder {
private:
	AggFunction _type;
	EnumSetGrounder* _setgrounder;
	mutable std::map<std::pair<uint,AggFunction>, VarId> aggterm2cpterm; // TODO memory management and move to translator!

public:
	AggTermGrounder(GroundTranslator* gt, AggFunction tp, SortTable* dom, EnumSetGrounder* gr);
	GroundTerm run() const;
};
