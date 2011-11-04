#ifndef FORMULAGROUNDERS_HPP_
#define FORMULAGROUNDERS_HPP_

#include "ground.hpp"
#include "grounders/Grounder.hpp"

/*** Formula grounders ***/

typedef std::map<Variable*, const DomElemContainer*> var2domelemmap;

class FormulaGrounder: public Grounder {
private:
	var2dommap _varmap; // Maps the effective variables in the current formula to their instantiation;

	int _verbosity;

	const Formula* _origform;
	var2dommap _origvarmap; // Maps the (cloned) variables in the original formula to their instantiation

protected:
	const var2dommap& varmap() const {
		return _varmap;
	}
	int verbosity() const {
		return _verbosity;
	}
	GroundTranslator* translator() const;

public:
	// FIXME verbosity should be passed in (or perhaps full option block?)
	FormulaGrounder(AbstractGroundTheory* grounding, const GroundingContext& ct);
	virtual ~FormulaGrounder() {
	}

	virtual bool conjunctive() const = 0;

	// TODO resolve this note?
	// NOTE: required for correctness because it creates the associated varmap!
	void setOrig(const Formula* f, const std::map<Variable*, const DomElemContainer*>& mvd, int);

	void printorig() const;
};

class AtomGrounder: public FormulaGrounder {
protected:
	std::vector<TermGrounder*> _subtermgrounders;
	InstChecker* _pchecker;
	InstChecker* _cchecker;
	size_t _symbol; // symbol's offset in translator's table.
	std::vector<SortTable*> _tables;
	SIGN _sign;
	GenType gentype;
	std::vector<const DomElemContainer*> _checkargs; // The variables representing the subterms of the atom. These are used in the generators and checkers
	PredInter* _inter;

	Lit run() const;
public:
	AtomGrounder(AbstractGroundTheory* grounding, SIGN sign, PFSymbol*, const std::vector<TermGrounder*>&, const std::vector<const DomElemContainer*>& checkargs,
			InstChecker*, InstChecker*, PredInter* inter, const std::vector<SortTable*>&, const GroundingContext&);
	void run(ConjOrDisj& formula) const;
	bool conjunctive() const {
		return true;
	}
};

//class CPAtomGrounder : public AtomGrounder {
//	private:
//		GroundTermTranslator*	_termtranslator;
//	public:
//		CPAtomGrounder(GroundTranslator*, GroundTermTranslator*, SIGN sign, Function*,
//					const std::vector<TermGrounder*>, InstanceChecker*, InstanceChecker*,
//					const std::vector<SortTable*>&, const GroundingContext&);
//		int	run() const;
//};

class ComparisonGrounder: public FormulaGrounder {
private:
	GroundTermTranslator* _termtranslator;
	TermGrounder* _lefttermgrounder;
	TermGrounder* _righttermgrounder;
	CompType _comparator;

	Lit run() const;
public:
	ComparisonGrounder(AbstractGroundTheory* grounding, GroundTermTranslator* tt, TermGrounder* ltg, CompType comp, TermGrounder* rtg,
			const GroundingContext& gc) :
			FormulaGrounder(grounding, gc), _termtranslator(tt), _lefttermgrounder(ltg), _righttermgrounder(rtg), _comparator(comp) {
	}
	void run(ConjOrDisj& formula) const;
	bool conjunctive() const {
		return true;
	}
};

enum AGG_COMP_TYPE {
	AGG_EQ, AGG_LT, AGG_GT
};

class AggGrounder: public FormulaGrounder {
private:
	SetGrounder* _setgrounder;
	TermGrounder* _boundgrounder;
	AggFunction _type;
	AGG_COMP_TYPE _comp;
	SIGN _sign;
	bool _doublenegtseitin;
	int handleDoubleNegation(double boundvalue, int setnr) const;
	int finishCard(double truevalue, double boundvalue, int setnr) const;
	int finishSum(double truevalue, double boundvalue, int setnr) const;
	int finishProduct(double truevalue, double boundvalue, int setnr) const;
	int finishMaximum(double truevalue, double boundvalue, int setnr) const;
	int finishMinimum(double truevalue, double boundvalue, int setnr) const;
	int finish(double boundvalue, double newboundvalue, double maxpossvalue, double minpossvalue, int setnr) const;

	int run() const;
public:
	AggGrounder(AbstractGroundTheory* grounding, GroundingContext gc, AggFunction tp, SetGrounder* sg, TermGrounder* bg, CompType comp, SIGN sign) :
			FormulaGrounder(grounding, gc), _setgrounder(sg), _boundgrounder(bg), _type(tp), _comp(AGG_EQ), _sign(SIGN::POS) {
		switch (comp) {
		case CompType::EQ:
			_comp = AGG_EQ;
			break;
		case CompType::NEQ:
			_comp = AGG_EQ;
			_sign = not _sign;
			break;
		case CompType::LT:
			_comp = AGG_LT;
			break;
		case CompType::LEQ:
			_comp = AGG_GT;
			_sign = not _sign;
			break;
		case CompType::GT:
			_comp = AGG_GT;
			break;
		case CompType::GEQ:
			_comp = AGG_LT;
			_sign = not _sign;
			break;
		}
		_doublenegtseitin = (gc._tseitin == TsType::RULE)
				&& ((gc._monotone == Context::POSITIVE && isPos(_sign)) || (gc._monotone == Context::NEGATIVE && isNeg(_sign)));
	}
	void run(ConjOrDisj& formula) const;
	bool conjunctive() const {
		return true;
	}
};

class ClauseGrounder: public FormulaGrounder {
protected:
	SIGN sign_;
	Conn conn_;

	TsType getTseitinType() const;
	bool negativeDefinedContext() const {
		return getTseitinType() == TsType::RULE && context()._monotone == Context::NEGATIVE;
	}
	Lit createTseitin(const ConjOrDisj& formula) const;
public:
	ClauseGrounder(AbstractGroundTheory* grounding, SIGN sign, bool conj, const GroundingContext& ct) :
			FormulaGrounder(grounding, ct), sign_(sign), conn_(conj ? Conn::CONJ : Conn::DISJ) {
	}

	void run(ConjOrDisj& formula) const;

protected:
	Lit getReification(const ConjOrDisj& formula) const;
	bool makesFormulaTrue(Lit l, bool negated) const;
	bool makesFormulaFalse(Lit l, bool negated) const;
	bool isRedundantInFormula(Lit l, bool negated) const;
	Lit getEmtyFormulaValue() const;
	bool conjunctive() const {
		return (conn_ == Conn::CONJ && isPositive()) || (conn_ == Conn::DISJ && isNegative());
	}

	bool isPositive() const {
		return isPos(sign_);
	}
	bool isNegative() const {
		return isNeg(sign_);
	}

	virtual void run(ConjOrDisj& formula, bool negatedformula) const = 0;
	void runSubGrounder(Grounder* subgrounder, bool conjFromRoot, ConjOrDisj& formula, bool negated) const;
};

class BoolGrounder: public ClauseGrounder {
private:
	std::vector<Grounder*> _subgrounders;

protected:
	virtual void run(ConjOrDisj& literals, bool negatedformula) const;

public:
	BoolGrounder(AbstractGroundTheory* grounding, const std::vector<Grounder*> sub, SIGN sign, bool conj, const GroundingContext& ct) :
			ClauseGrounder(grounding, sign, conj, ct), _subgrounders(sub) {
	}
};

class QuantGrounder: public ClauseGrounder {
protected:
	FormulaGrounder* _subgrounder;
	InstGenerator* _generator; // generates PF if univ, PT if exists => if generated, literal might decide formula (so otherwise irrelevant)
	InstChecker* _checker; // Checks CF if univ, CT if exists => if checks, certainly decides formula

protected:
	virtual void run(ConjOrDisj& literals, bool negatedformula) const;

public:
	QuantGrounder(AbstractGroundTheory* grounding, FormulaGrounder* sub, SIGN sign, QUANT quant, InstGenerator* gen, InstChecker* checker,
			const GroundingContext& ct) :
			ClauseGrounder(grounding, sign, quant == QUANT::UNIV, ct), _subgrounder(sub), _generator(gen), _checker(checker) {
	}
};

class EquivGrounder: public ClauseGrounder {
private:
	FormulaGrounder* _leftgrounder;
	FormulaGrounder* _rightgrounder;

protected:
	virtual void run(ConjOrDisj& literals, bool negatedformula) const;

public:
	EquivGrounder(AbstractGroundTheory* grounding, FormulaGrounder* lg, FormulaGrounder* rg, SIGN sign, const GroundingContext& ct) :
			ClauseGrounder(grounding, sign, true, ct), _leftgrounder(lg), _rightgrounder(rg) {
	}
};

#endif /* FORMULAGROUNDERS_HPP_ */
