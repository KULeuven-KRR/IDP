/************************************
 TermGrounders.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef TERMGROUNDERS_HPP_
#define TERMGROUNDERS_HPP_

#include "ground.hpp"

/*** Term grounders ***/

class TermGrounder {
	protected:
		AbstractGroundTheory*						_grounding;
		mutable SortTable*							_domain;
		const Term*									_origterm;
		std::map<Variable*,const DomainElement**>	_varmap;
		int											_verbosity;
		void printorig() const;
	public:
		TermGrounder() { }
		TermGrounder(AbstractGroundTheory* g, SortTable* dom) : _grounding(g), _domain(dom) { }
		virtual ~TermGrounder() { }
		virtual GroundTerm run() const = 0;
		void setorig(const Term* t, const std::map<Variable*, const DomainElement**>& mvd, int);
		SortTable* domain() const { return _domain; }
};

class DomTermGrounder : public TermGrounder {
	private:
		const DomainElement*	_value;
	public:
		DomTermGrounder(const DomainElement* val) : _value(val) { }
		GroundTerm run() const { return GroundTerm(_value);	}
};

class VarTermGrounder : public TermGrounder {
	private:
		const DomainElement**	_value;
	public:
		VarTermGrounder(const DomainElement** a) : _value(a) { }
		GroundTerm run() const;
};

class FuncTermGrounder : public TermGrounder {
	protected:
		GroundTermTranslator* 			_termtranslator;
		Function*						_function;
		FuncTable*						_functable;
		std::vector<TermGrounder*>		_subtermgrounders;
	public:
		FuncTermGrounder(GroundTermTranslator* tt, Function* func, FuncTable* ftable, SortTable* dom, const std::vector<TermGrounder*>& sub)
			: _termtranslator(tt), _function(func), _functable(ftable), _subtermgrounders(sub) { _domain = dom; }
		GroundTerm run() const;

		// TODO? Optimisation:
		//			Keep all values of the args + result of the previous call to run().
		//			If the values of the args did not change, return the result immediately instead of doing the
		//			table lookup
};

class SumTermGrounder : public TermGrounder {
	protected:
		GroundTermTranslator* 	_termtranslator;
		FuncTable*				_functable;
		TermGrounder*			_lefttermgrounder;
		TermGrounder*			_righttermgrounder;
	public:
		SumTermGrounder(AbstractGroundTheory* g, GroundTermTranslator* tt, FuncTable* ftable, SortTable* dom, TermGrounder* ltg, TermGrounder* rtg)
			: TermGrounder(g,dom), _termtranslator(tt), _functable(ftable), _lefttermgrounder(ltg), _righttermgrounder(rtg) { }
		GroundTerm run() const;
};

class SetGrounder;

class AggTermGrounder : public TermGrounder {
	private:
		GroundTranslator*	_translator;
		AggFunction			_type;
		SetGrounder*		_setgrounder;
	public:
		AggTermGrounder(GroundTranslator* gt, AggFunction tp, SetGrounder* gr)
			: _translator(gt), _type(tp), _setgrounder(gr) { }
		GroundTerm run() const;
};

#endif /* TERMGROUNDERS_HPP_ */
