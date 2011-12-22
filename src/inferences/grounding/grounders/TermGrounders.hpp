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

#include "common.hpp"

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
	protected:
		AbstractGroundTheory*						_grounding;
		mutable SortTable*							_domain;
		const Term*									_origterm;
		std::map<Variable*,const DomElemContainer*>	_varmap;
		int											_verbosity;
		void printOrig() const;
	public:
		TermGrounder() { }
		TermGrounder(AbstractGroundTheory* g, SortTable* dom) : _grounding(g), _domain(dom) { }
		virtual ~TermGrounder() { }
		virtual GroundTerm run() const = 0;
		void setOrig(const Term* t, const std::map<Variable*, const DomElemContainer*>& mvd, int);
		SortTable* 	getDomain() const 			{ return _domain; }
		void		setDomain(SortTable* dom) 	{ _domain = dom; }
};

class DomTermGrounder : public TermGrounder {
	private:
		const DomainElement*	_value;
	public:
		DomTermGrounder(const DomainElement* val) : _value(val) { }
		GroundTerm run() const;
};

class VarTermGrounder : public TermGrounder {
	private:
		const DomElemContainer*	_value;
	public:
		VarTermGrounder(const DomElemContainer* a) : _value(a) { }
		GroundTerm run() const;

		const DomElemContainer* getElement() const { return _value; }
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

enum SumType { ST_PLUS, ST_MINUS };

class SumTermGrounder : public TermGrounder {
	protected:
		GroundTermTranslator* 	_termtranslator;
		FuncTable*				_functable;
		TermGrounder*			_lefttermgrounder;
		TermGrounder*			_righttermgrounder;
		SumType					_type;
	public:
		SumTermGrounder(AbstractGroundTheory* g, GroundTermTranslator* tt, FuncTable* ftable
			, SortTable* dom, TermGrounder* ltg, TermGrounder* rtg, SumType type = ST_PLUS)
			: TermGrounder(g,dom), _termtranslator(tt), _functable(ftable), _lefttermgrounder(ltg), _righttermgrounder(rtg), _type(type) { }
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
