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

#ifndef FORMULAGROUNDERS_HPP_
#define FORMULAGROUNDERS_HPP_

#include "Grounder.hpp"

#include "IncludeComponents.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

class TermGrounder;
class InstChecker;
class InstGenerator;
class EnumSetGrounder;
class PredInter;
class Formula;
class SortTable;
class GroundTranslator;
class PFSymbol;

/*** Formula grounders ***/

bool recursive(const PFSymbol& symbol, const GroundingContext& context);

class FormulaGrounder: public Grounder {
protected:
	var2dommap _varmap; // Maps the (cloned) variables in the original formula to their instantiation

	Formula* _formula; 	// The formula represented by this grounder, if any. Currently not representing generators/checkers
						// This formula is a clone of the original and gets deleted together with the grounder.

	// Passes ownership!!!
	void setFormula(Formula* f);

public:
	FormulaGrounder(AbstractGroundTheory* grounding, const GroundingContext& ct);
	virtual ~FormulaGrounder();

	void printorig() const;

	virtual void put(std::ostream& stream) const;
	const var2dommap& getVarmapping() const {
		return _varmap;
	}

	bool hasFormula() const {
		return _formula != NULL;
	}
	Formula* getFormula() const {
		Assert(hasFormula());
		return _formula;
	}
};

class AtomGrounder: public FormulaGrounder {
private:
	std::vector<TermGrounder*> _subtermgrounders;
	PFSymbol* _symbol;
	SymbolOffset _symboloffset; // Stored for efficiency
	std::vector<SortTable*> _tables;
	SIGN _sign;
	GenType gentype;
	bool _recursive;

	mutable ElementTuple _args;
	mutable std::vector<GroundTerm> terms;

	Lit run() const;

protected:
	void internalRun(ConjOrDisj& literals, LazyGroundingRequest& request);

public:
	AtomGrounder(AbstractGroundTheory* grounding, SIGN sign, PFSymbol*, const std::vector<TermGrounder*>&,
			const std::vector<SortTable*>&, const GroundingContext&, const PredForm* orig = NULL);
	~AtomGrounder();
};

class ComparisonGrounder: public FormulaGrounder {
private:
	TermGrounder* _lefttermgrounder;
	TermGrounder* _righttermgrounder;
	CompType _comparator;

	Lit run() const;

protected:
	void internalRun(ConjOrDisj& literals, LazyGroundingRequest& request);

public:
	// NOTE: sign has been taken into account in comp, but is added for formula management!
	ComparisonGrounder(AbstractGroundTheory* grounding, TermGrounder* ltg, CompType comp, TermGrounder* rtg, const GroundingContext& gc, PFSymbol* symbol,
			SIGN sign);
	~ComparisonGrounder();
};

class DenotationGrounder: public FormulaGrounder {
private:
	SIGN sign;
	FuncTerm* term;
	std::vector<TermGrounder*> tgs;

	Lit run() const;

protected:
	void internalRun(ConjOrDisj& literals, LazyGroundingRequest& request);

public:
	DenotationGrounder(AbstractGroundTheory* grounding, SIGN sign, FuncTerm* term, const std::vector<TermGrounder*>& tgs, const GroundingContext& gc);
	~DenotationGrounder();
};

// Expresses bound comp aggterm!
class AggGrounder: public FormulaGrounder {
private:
	EnumSetGrounder* _setgrounder;
	TermGrounder* _boundgrounder;
	AggFunction _type;
	CompType _comp;
	SIGN _sign;
	bool _doublenegtseitin;
	Lit handleDoubleNegation(double boundvalue, SetId setnr, CompType comp) const;
	Lit finishCard(double truevalue, double boundvalue, SetId setnr) const;
	Lit finishSum(double truevalue, double boundvalue, SetId setnr) const;
	Lit finishProduct(double truevalue, double boundvalue, SetId setnr) const;
	Lit finishMaximum(double truevalue, double boundvalue, SetId setnr) const;
	Lit finishMinimum(double truevalue, double boundvalue, SetId setnr) const;
	Lit splitproducts(double boundvalue, double newboundvalue, double minpossvalue, double maxpossvalue, SetId setnr) const;
	Lit finish(double boundvalue, double newboundvalue, double maxpossvalue, double minpossvalue, SetId setnr) const;

	Lit run() const;

protected:
	void internalRun(ConjOrDisj& literals, LazyGroundingRequest& request);

public:
	AggGrounder(AbstractGroundTheory* grounding, GroundingContext gc, TermGrounder* bound, CompType comp, AggFunction tp, EnumSetGrounder* sg, SIGN sign);
	~AggGrounder();
};

enum class FormStat {
	UNKNOWN,
	DECIDED
};

class ClauseGrounder: public FormulaGrounder {
private:
	mutable ConjOrDisj _subformula;
	SIGN _sign;
	Conn _conn;

	bool negativeDefinedContext() const {
		return getTseitinType() == TsType::RULE && getContext()._monotone == Context::NEGATIVE;
	}
	Lit createTseitin(const ConjOrDisj& formula, TsType type) const;
	Lit getOneLiteralRepresenting(const ConjOrDisj& formula, TsType type) const;

public:
	ClauseGrounder(AbstractGroundTheory* grounding, SIGN sign, bool conj, const GroundingContext& ct);
	virtual ~ClauseGrounder() {
	}

	bool conjunctiveWithSign() const {
		return (_conn == Conn::CONJ && isPositive()) || (_conn == Conn::DISJ && isNegative());
	}

protected:
	void internalRun(ConjOrDisj& literals, LazyGroundingRequest& request);
	virtual void internalClauseRun(ConjOrDisj& literals, LazyGroundingRequest& request) = 0;

	TsType getTseitinType() const;

	Lit getReification(const ConjOrDisj& formula, TsType tseitintype) const;
	Lit getEquivalentReification(const ConjOrDisj& formula, TsType tseitintype) const; // NOTE: creates a tseitin EQUIVALENT with form, EVEN if the current tseitintype is IMPL or RIMPL

private:
	// NOTE: used by internalrun, which does not take SIGN into account!
	bool makesFormulaTrue(Lit l) const;
	// NOTE: used by internalrun, which does not take SIGN into account!
	bool makesFormulaFalse(Lit l) const;
	// NOTE: used by internalrun, which does not take SIGN into account!
	bool decidesFormula(Lit l) const;
	// NOTE: used by internalrun, which does not take SIGN into account!
	bool isRedundantInFormula(Lit l) const;
	// NOTE: used by internalrun, which does not take SIGN into account!
	Lit redundantLiteral() const;
	// NOTE: used by internalrun, which does not take SIGN into account!
	Lit getEmtyFormulaValue() const;

protected:

	Conn connective() const {
		return _conn;
	}

	bool isPositive() const {
		return isPos(_sign);
	}
	bool isNegative() const {
		return isNeg(_sign);
	}

	FormStat runSubGrounder(Grounder* subgrounder, bool conjFromRoot, bool considerAsConjunctiveWithSign, ConjOrDisj& formula, LazyGroundingRequest& request, bool lastsub = false) const;
};

class BoolGrounder: public ClauseGrounder {
private:
	std::vector<FormulaGrounder*> _subgrounders;
protected:
	virtual void internalClauseRun(ConjOrDisj& literals, LazyGroundingRequest& request);
public:
	BoolGrounder(AbstractGroundTheory* grounding, const std::vector<FormulaGrounder*>& sub, SIGN sign, bool conj, const GroundingContext& ct);
	~BoolGrounder();
	const std::vector<FormulaGrounder*>& getSubGrounders() const {
		return _subgrounders;
	}

	virtual void put(std::ostream& output) const;
};

class LazyGroundingManager;

class QuantGrounder: public ClauseGrounder {
protected:
	FormulaGrounder* _subgrounder;
	InstGenerator* _generator; // generates PF if univ, PT if exists => if generated, literal might decide formula (so otherwise irrelevant)
	InstChecker* _checker; // Checks CF if univ, CT if exists => if checks, certainly decides formula
	std::set<const DomElemContainer*> _generatescontainers;
	LazyGroundingManager* _manager;
	std::map<Variable*, SortTable*> map2delayedsorts; // Stored reduced tables during splitting

	bool splitallowed;
	AtomGrounder* replacementaftersplit;
protected:
	virtual void internalClauseRun(ConjOrDisj& literals, LazyGroundingRequest& request);
public:
	QuantGrounder(LazyGroundingManager* manager, AbstractGroundTheory* grounding, FormulaGrounder* sub, InstGenerator* gen, InstChecker* checker,
			const GroundingContext& ct, SIGN sign, QUANT quant, const std::set<const DomElemContainer*>& generates, const tablesize& quantsize);
	~QuantGrounder();
	FormulaGrounder* getSubGrounder() const {
		return _subgrounder;
	}
	const std::set<const DomElemContainer*>& getGeneratedContainers() const{
		return _generatescontainers;
	}
private:
	bool groundAfterGeneration(ConjOrDisj& formula, LazyGroundingRequest& request);

	bool split();

	LazyGroundingManager* getGroundingManager() const {
		return _manager;
	}
};

class EquivGrounder: public ClauseGrounder {
private:
	FormulaGrounder* _leftgrounder;
	FormulaGrounder* _rightgrounder;
protected:
	virtual void internalClauseRun(ConjOrDisj& literals, LazyGroundingRequest& request);
public:
	EquivGrounder(AbstractGroundTheory* grounding, FormulaGrounder* lg, FormulaGrounder* rg, SIGN sign, const GroundingContext& ct);
	~EquivGrounder();

	FormulaGrounder* getLeftGrounder() const {
		return _leftgrounder;
	}
	FormulaGrounder* getRightGrounder() const {
		return _rightgrounder;
	}
};

#endif /* FORMULAGROUNDERS_HPP_ */
