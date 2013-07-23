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

#ifndef FINDUNKNBOUND_HPP_
#define FINDUNKNBOUND_HPP_

#include "IncludeComponents.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

/**
 *	Given a formula F, find a literal L such that,
 *		for each interpretation I in which L^I=unknown,
 *			then either I[L=T] \models F or I[L=F] \models F.
 */
class FindUnknownBoundLiteral: public TheoryVisitor {
	VISITORFRIENDS()
private:
	bool _start;
	bool _allquantvars;
	const GroundTranslator* _translator;
	const Structure* _structure;

	const PredForm* _resultingliteral;
	varset _quantvars, _containingquantvars;
	Context _context, _resultingContext;

public:
	template<typename T>
	const PredForm* execute(T t, const Structure* structure, const GroundTranslator* trans, Context& context) {
		_start = true;
		_resultingliteral = NULL;
		_translator = trans;
		_structure = structure;
		_context = Context::POSITIVE;
		t->accept(this);
		if(_resultingliteral!=NULL){
			context = _resultingContext;
		}
		return _resultingliteral;
	}

protected:
	virtual void traverse(const Formula* f) {
		auto tempcontext = _context;
		if(f->sign()==SIGN::NEG){
			_context = not _context;
		}

		for (size_t n = 0; n < f->subterms().size(); ++n) {
			f->subterms()[n]->accept(this);
		}
		for (size_t n = 0; n < f->subformulas().size() && _resultingliteral==NULL; ++n) {
			f->subformulas()[n]->accept(this);
		}

		_context = tempcontext;
	}
	virtual void visit(const PredForm* pf){
		auto context = _context;
		if(isNeg(pf->sign())){
			context = not context;
		}
		if(not _translator->canBeDelayedOn(pf->symbol(), context, -1)){
			return;
		}
		if(_structure!=NULL && (not _structure->inter(pf->symbol())->cf()->empty() || not _structure->inter(pf->symbol())->ct()->empty())){
			// TODO checks here whether NO tuples are known about the predicate. This can obvious be done better, by checking if it is only a small number AND adding those to the grounding explicitly!!!
			return;
		}
		_allquantvars = true;
		_containingquantvars.clear();
		for(auto i=pf->args().cbegin(); i<pf->args().cend(); ++i) {
			 (*i)->accept(this);
		}
		if(_allquantvars && _containingquantvars.size()==_quantvars.size()){
			_resultingliteral = pf;
			_resultingContext = context;
		}
		return;
	}
	virtual void visit(const BoolForm* formula){
		if(not formula->conj()){
			traverse(formula);
		}
		return;
	}
	virtual void visit(const EquivForm* formula){
		_context = Context::BOTH;
		_start = false;
		formula->left()->accept(this);
		return;
	}
	virtual void visit(const QuantForm* formula){
		if(_start){
			_quantvars = formula->quantVars();
		}else{
			return;
		}
		_start = false;
		traverse(formula);
		return;
	}

	virtual void visit(const VarTerm* term){
		_start = false;
		if(_quantvars.find(term->var())==_quantvars.cend()){
			_allquantvars = false;
		}else{
			_containingquantvars.insert(term->var());
		}
		return;
	}
	virtual void visit(const FuncTerm*){
		_allquantvars = false;
		_start = false;
		return;
	}
	virtual void visit(const DomainTerm*){
		_allquantvars = false;
		_start = false;
		return;
	}
	virtual void visit(const AggTerm*){
		_allquantvars = false;
		_start = false;
		return;
	}

	// NOTE: DEFAULTS: just return
	virtual void visit(const EqChainForm*){
		_start = false;
		return;
	}
	virtual void visit(const Theory*){
		_start = false;
		return;
	}
	virtual void visit(const AbstractGroundTheory*){
		_start = false;
		return;
	}
	virtual void visit(const GroundTheory<GroundPolicy>*){
		_start = false;
		return;
	}
	virtual void visit(const AggForm*){
		_start = false;
		return;
	}
	virtual void visit(const GroundDefinition*){
		_start = false;
		return;
	}
	virtual void visit(const PCGroundRule*){
		_start = false;
		_start = false;
		return;
	}
	virtual void visit(const AggGroundRule*){
		_start = false;
		return;
	}
	virtual void visit(const GroundSet*){
		_start = false;
		return;
	}
	virtual void visit(const GroundAggregate*){
		_start = false;
		return;
	}
	virtual void visit(const CPReification*){
		_start = false;
		return;
	}
	virtual void visit(const Rule*){
		_start = false;
		return;
	}
	virtual void visit(const Definition*){
		_start = false;
		return;
	}
	virtual void visit(const FixpDef*){
		_start = false;
		return;
	}
	virtual void visit(const CPVarTerm*){
		_start = false;
		return;
	}
	virtual void visit(const CPWSumTerm*){
		_start = false;
		return;
	}
	virtual void visit(const CPWProdTerm*){
		_start = false;
		return;
	}
	virtual void visit(const EnumSetExpr*){
		_start = false;
		return;
	}
	virtual void visit(const QuantSetExpr*){
		_start = false;
		return;
	}
};

#endif /* FINDUNKNBOUND_HPP_ */
