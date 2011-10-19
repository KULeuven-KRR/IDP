/************************************
 FormulaGrounders.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef FORMULAGROUNDERS_HPP_
#define FORMULAGROUNDERS_HPP_

#include "ground.hpp"

/*** Formula grounders ***/

typedef std::map<Variable*,const DomElemContainer*> var2domelemmap;

class FormulaGrounder {
	private:
		GroundTranslator*	_translator;
		GroundingContext	_context;

		var2dommap			_varmap; // Maps the effective variables in the current formula to their instantiation;

		int					_verbosity;

		const Formula*		_origform;
		var2dommap			_origvarmap; // Maps the (cloned) variables in the original formula to their instantiation

	protected:
		const var2dommap& 	varmap	()	const { return _varmap; }
		int					verbosity	()	const { return _verbosity; }
		GroundTranslator*	translator	() 	const { return _translator; }
		GroundingContext	context		()	const { return _context; }

	public:
		// FIXME verbosity should be passed in (or perhaps full option block?)
		FormulaGrounder(GroundTranslator* gt, const GroundingContext& ct): _translator(gt), _context(ct), _verbosity(0) { }
		virtual ~FormulaGrounder() { }
		virtual Lit		run()			const = 0;
		virtual void	run(litlist&)	const = 0;
		virtual bool	conjunctive()	const = 0;

		// NOTE: required for correctness because it creates the associated varmap!
		void setOrig(const Formula* f, const std::map<Variable*, const DomElemContainer*>& mvd, int);

		void printorig() const;
};

class AtomGrounder : public FormulaGrounder {
	protected:
		std::vector<TermGrounder*>		_subtermgrounders;
		InstGenerator*					_pchecker;
		InstGenerator*					_cchecker;
		size_t							_symbol; // symbol's offset in translator's table.
		std::vector<SortTable*>			_tables;
		SIGN							_sign;
		int								_certainvalue;
		std::vector<const DomElemContainer*>	_checkargs;
		PredInter*							_inter;
	public:
		AtomGrounder(
				GroundTranslator*,
				SIGN sign,
				PFSymbol*,
				const std::vector<TermGrounder*>&,
				const std::vector<const DomElemContainer*>& checkargs,
				InstGenerator*,
				InstGenerator*,
				PredInter* inter,
				const std::vector<SortTable*>&,
				const GroundingContext&);
		int		run() const;
		void	run(litlist&)	const;
		bool	conjunctive() const { return true;	}
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

class ComparisonGrounder : public FormulaGrounder {
	private:
		GroundTermTranslator* 	_termtranslator;
		TermGrounder*			_lefttermgrounder;
		TermGrounder*			_righttermgrounder;
		CompType				_comparator;
	public:
		ComparisonGrounder(
			GroundTranslator* gt,
			GroundTermTranslator* tt,
			TermGrounder* ltg,
			CompType comp,
			TermGrounder* rtg,
			const GroundingContext& gc)
			: FormulaGrounder(gt,gc), _termtranslator(tt), _lefttermgrounder(ltg), _righttermgrounder(rtg), _comparator(comp) { }
		int		run() const;
		void	run(litlist&)	const;
		bool	conjunctive() const { return true;	}
};

enum AGG_COMP_TYPE { AGG_EQ, AGG_LT, AGG_GT};

enum CONN{ CONJ, DISJ};

class AggGrounder : public FormulaGrounder {
	private:
		SetGrounder*	_setgrounder;
		TermGrounder*	_boundgrounder;
		AggFunction		_type;
		AGG_COMP_TYPE	_comp;
		SIGN			_sign;
		bool			_doublenegtseitin;
		int	handleDoubleNegation(double boundvalue,int setnr) const;
		int	finishCard(double truevalue,double boundvalue,int setnr) 	const;
		int	finishSum(double truevalue,double boundvalue,int setnr)		const;
		int	finishProduct(double truevalue,double boundvalue,int setnr)	const;
		int	finishMaximum(double truevalue,double boundvalue,int setnr)	const;
		int	finishMinimum(double truevalue,double boundvalue,int setnr)	const;
		int finish(double boundvalue,double newboundvalue,double maxpossvalue,double minpossvalue,int setnr) const;
	public:
		AggGrounder(GroundTranslator* tr, GroundingContext gc, AggFunction tp, SetGrounder* sg, TermGrounder* bg, CompType comp, SIGN sign)
			: FormulaGrounder(tr,gc), _setgrounder(sg), _boundgrounder(bg), _type(tp), _comp(AGG_EQ), _sign(SIGN::POS) {
				switch(comp) {
					case CompType::EQ: _comp = AGG_EQ; break;
					case CompType::NEQ: _comp = AGG_EQ; _sign = not _sign; break;
					case CompType::LT: _comp = AGG_LT; break;
					case CompType::LEQ: _comp = AGG_GT; _sign = not _sign; break;
					case CompType::GT: _comp = AGG_GT; break;
					case CompType::GEQ: _comp = AGG_LT; _sign = not _sign; break;
				}
				_doublenegtseitin = (gc._tseitin == TsType::RULE) && ((gc._monotone == Context::POSITIVE && isPos(_sign)) || (gc._monotone == Context::NEGATIVE && isNeg(_sign)));
			}
		int		run()								const;
		void	run(std::vector<int>&)				const;
		bool	conjunctive() 						const { return true;	}
};

class ClauseGrounder : public FormulaGrounder {
	protected:
		SIGN	sign_;
		CONN	conn_;

		TsType getTseitinType() const;
		bool negativeDefinedContext() const { return getTseitinType()==TsType::RULE && context()._monotone == Context::NEGATIVE; }
		Lit createTseitin(const litlist& clause) const;
	public:
		ClauseGrounder(GroundTranslator* gt, SIGN sign, bool conj, const GroundingContext& ct) :
			FormulaGrounder(gt,ct),
			sign_(sign),
			conn_(conj?CONJ:DISJ){}
	protected:
		Lit 	getReification(litlist& clause) const;
		bool 	makesFormulaTrue(Lit l, bool negated) const;
		bool 	makesFormulaFalse(Lit l, bool negated) const;
		bool 	isRedundantInFormula(Lit l, bool negated) const;
		Lit 	getEmtyFormulaValue() const;
		bool	conjunctive() const { return (conn_==CONJ && isPositive()) || (conn_==DISJ && isNegative());	}

		bool 	isPositive() const { return isPos(sign_); }
		bool 	isNegative() const { return isNeg(sign_); }

		Lit		run() const;
		void	run(litlist&)	const;
		virtual void	run(litlist&, bool negatedclause = true)	const = 0;
};

class BoolGrounder : public ClauseGrounder {
	private:
		std::vector<FormulaGrounder*>	_subgrounders;

	protected:
		virtual void	run(litlist&, bool negatedclause = true)	const;

	public:
		BoolGrounder(GroundTranslator* gt, const std::vector<FormulaGrounder*> sub, SIGN sign, bool conj, const GroundingContext& ct):
			ClauseGrounder(gt,sign,conj,ct), _subgrounders(sub) { }
};

class QuantGrounder : public ClauseGrounder {
protected:
	FormulaGrounder*	_subgrounder;
	InstGenerator*		_generator;
	InstGenerator*		_checker;

protected:
	virtual void	run(litlist&, bool negatedclause = true)	const;

public:
	QuantGrounder(
			GroundTranslator* gt,
			FormulaGrounder* sub,
			SIGN sign,
			QUANT quant,
			InstGenerator* gen,
			InstGenerator* checker,
			const GroundingContext& ct):
		ClauseGrounder(gt,sign,quant==QUANT::UNIV,ct), _subgrounder(sub), _generator(gen), _checker(checker) { }
};

class EquivGrounder : public FormulaGrounder {
	private:
		FormulaGrounder*	_leftgrounder;
		FormulaGrounder*	_rightgrounder;
		SIGN				sign_;

	protected:
		bool 	isPositive() const { return sign_==SIGN::POS; }
		bool 	isNegative() const { return sign_==SIGN::NEG; }

	public:
		EquivGrounder(GroundTranslator* gt, FormulaGrounder* lg, FormulaGrounder* rg, SIGN sign, const GroundingContext& ct):
			FormulaGrounder(gt,ct), _leftgrounder(lg), _rightgrounder(rg), sign_(sign) { }
		bool	conjunctive() const { return true;	}

		Lit		run() const;
		void	run(litlist&)	const;
};

#endif /* FORMULAGROUNDERS_HPP_ */
