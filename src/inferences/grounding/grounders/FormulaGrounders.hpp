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
#include "inferences/grounding/GroundTranslator.hpp" // TODO Only for symboloffset

class TermGrounder;
class InstChecker;
class InstGenerator;
class SetGrounder;
class PredInter;
class Formula;
class SortTable;
class GroundTranslator;
class PFSymbol;

/*** Formula grounders ***/

bool recursive(const PFSymbol& symbol, const GroundingContext& context);

class FormulaGrounder: public Grounder {
private:
	var2dommap _varmap; // Maps the effective variables in the current formula to their instantiation;
	var2dommap _origvarmap; // Maps the (cloned) variables in the original formula to their instantiation
protected:
	const var2dommap& varmap() const {
		return _varmap;
	}
	Formula* _origform;
public:
	// FIXME verbosity should be passed in (or perhaps full option block?)
	FormulaGrounder(AbstractGroundTheory* grounding, const GroundingContext& ct);
	virtual ~FormulaGrounder();

	// TODO resolve this note?
	// NOTE: required for correctness because it creates the associated varmap!
	void setOrig(const Formula* f, const std::map<Variable*, const DomElemContainer*>& mvd);

	void printorig() const;
	virtual void put(std::ostream& stream) const;
};

class AtomGrounder: public FormulaGrounder {
protected:
	std::vector<TermGrounder*> _subtermgrounders;
	PFSymbol* _symbol;
	SymbolOffset _symboloffset; // Stored for efficiency
	std::vector<SortTable*> _tables;
	SIGN _sign;
	GenType gentype;
	std::vector<const DomElemContainer*> _checkargs; // The variables representing the subterms of the atom. These are used in the generators and checkers
	bool _recursive;

	mutable ElementTuple _args;
	mutable std::vector<GroundTerm> terms;

	Lit run() const;

public:
	AtomGrounder(AbstractGroundTheory* grounding, SIGN sign, PFSymbol*, const std::vector<TermGrounder*>&,
			const std::vector<const DomElemContainer*>& checkargs, const std::vector<SortTable*>&,
			const GroundingContext&);
	~AtomGrounder();
	void run(ConjOrDisj& formula) const;
};

class ComparisonGrounder: public FormulaGrounder {
private:
	TermGrounder* _lefttermgrounder;
	TermGrounder* _righttermgrounder;
	CompType _comparator;

	Lit run() const;

public:
	ComparisonGrounder(AbstractGroundTheory* grounding, TermGrounder* ltg, CompType comp, TermGrounder* rtg, const GroundingContext& gc)
			: FormulaGrounder(grounding, gc), _lefttermgrounder(ltg), _righttermgrounder(rtg), _comparator(comp) {
		Assert(context()._tseitin!=TsType::RULE);
		setMaxGroundSize(tablesize(TableSizeType::TST_EXACT, 1));
	}
	~ComparisonGrounder();
	void run(ConjOrDisj& formula) const;
};

// Expresses bound comp aggterm!
class AggGrounder: public FormulaGrounder {
private:
	SetGrounder* _setgrounder;
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

public:
	AggGrounder(AbstractGroundTheory* grounding, GroundingContext gc, TermGrounder* bound, CompType comp, AggFunction tp, SetGrounder* sg, SIGN sign);
	~AggGrounder();
	void run(ConjOrDisj& formula) const;
};

enum class FormStat {
	UNKNOWN, DECIDED
};

class ClauseGrounder: public FormulaGrounder {
private:
	mutable ConjOrDisj _subformula;
	SIGN _sign;
	Conn _conn;

	bool negativeDefinedContext() const {
		return getTseitinType() == TsType::RULE && context()._monotone == Context::NEGATIVE;
	}
	Lit createTseitin(const ConjOrDisj& formula, TsType type) const;
	Lit getOneLiteralRepresenting(const ConjOrDisj& formula, TsType type) const;

public:
	ClauseGrounder(AbstractGroundTheory* grounding, SIGN sign, bool conj, const GroundingContext& ct)
			: FormulaGrounder(grounding, ct), _sign(sign), _conn(conj ? Conn::CONJ : Conn::DISJ) {
	}
	virtual ~ClauseGrounder() {
	}
	void run(ConjOrDisj& formula) const;

	bool conjunctiveWithSign() const {
		return (_conn == Conn::CONJ && isPositive()) || (_conn == Conn::DISJ && isNegative());
	}

protected:
	TsType getTseitinType() const;

	Lit getReification(const ConjOrDisj& formula, TsType tseitintype) const;
	Lit getEquivalentReification(const ConjOrDisj& formula, TsType tseitintype) const; // NOTE: creates a tseitin EQUIVALENT with form, EVEN if the current tseitintype is IMPL or RIMPL

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

	Conn connective() const {
		return _conn;
	}

	bool isPositive() const {
		return isPos(_sign);
	}
	bool isNegative() const {
		return isNeg(_sign);
	}

	virtual void internalRun(ConjOrDisj& formula) const = 0;

	FormStat runSubGrounder(Grounder* subgrounder, bool conjFromRoot, bool considerAsConjunctiveWithSign, ConjOrDisj& formula) const;
};

class BoolGrounder: public ClauseGrounder {
private:
	std::vector<Grounder*> _subgrounders;
protected:
	virtual void internalRun(ConjOrDisj& literals) const;
public:
	BoolGrounder(AbstractGroundTheory* grounding, const std::vector<Grounder*>& sub, SIGN sign, bool conj, const GroundingContext& ct);
	~BoolGrounder();
	const std::vector<Grounder*>& getSubGrounders() const {
		return _subgrounders;
	}

	virtual void put(std::ostream& output) const;
};

class QuantGrounder: public ClauseGrounder {
protected:
	FormulaGrounder* _subgrounder;
	InstGenerator* _generator; // generates PF if univ, PT if exists => if generated, literal might decide formula (so otherwise irrelevant)
	InstChecker* _checker; // Checks CF if univ, CT if exists => if checks, certainly decides formula
protected:
	virtual void internalRun(ConjOrDisj& literals) const;
public:
	QuantGrounder(AbstractGroundTheory* grounding, FormulaGrounder* sub, SIGN sign, QUANT quant, InstGenerator* gen, InstChecker* checker,
			const GroundingContext& ct);
	~QuantGrounder();
	FormulaGrounder* getSubGrounder() const {
		return _subgrounder;
	}
};

class EquivGrounder: public ClauseGrounder {
private:
	FormulaGrounder* _leftgrounder;
	FormulaGrounder* _rightgrounder;
protected:
	virtual void internalRun(ConjOrDisj& literals) const;
public:
	EquivGrounder(AbstractGroundTheory* grounding, FormulaGrounder* lg, FormulaGrounder* rg, SIGN sign, const GroundingContext& ct);
	~EquivGrounder();
};

#endif /* FORMULAGROUNDERS_HPP_ */
