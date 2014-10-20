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

#include "Metafier.hpp"

#include "theory/TheoryUtils.hpp"

// TODO can we get more compile safety here instead of passing in domain elements into inters?

#define INIT\
	auto id = createDomElem(++maxid);\
	auto stored = ids;
#define CLOSE\
	ids = stored;\
	ids.push_back(id);

void addSymbol(PFSymbol* s, MetaInters& m, UniqueStringNames<PFSymbol*>& symbols) {
	auto symbol = toDom(s, symbols);

	Assert(s->arity() >= 0); // Cast is done to int, but the relation "arities" has a nat type for the second argument
	m.arities->add( { symbol, createDomElem((int) s->arity()) }, true);

	m.name->add( { symbol, createDomElem(s->nameNoArity()) }, true);

	for (uint i = 0; i < s->arity(); ++i) {
		m.argsorts->add( { symbol, createDomElem((int)i), toDom(s->sorts()[i], symbols) }, true);
	}

	if (s->isFunction()) {
		auto func = dynamic_cast<Function*>(s);

		if (func->partial()) {
			m.partial->makeTrueExactly( { symbol }, true);
		}

		m.outsort->add( { symbol, toDom(func->outsort(), symbols) }, true);
	} else {
		if (s->nrSorts() == 1 && s->sorts()[0]->pred() == s) {
			// here we are sure it's a sort
			auto sort = s->sorts()[0];
			for (auto p : sort->parents()) {
				m.subtypeof->makeTrueExactly( { symbol, toDom(p, symbols) }, true);
			}

			if (sort->isConstructed()) {
				for (auto f : sort->getConstructors()) {
					m.constrFrom->makeTrueExactly( { symbol, toDom(f, symbols) }, true);
					addSymbol(f, m, symbols);
				}
			}
		}
	}
}

void Metafier::visit(const FuncTerm* ft) {
	INIT
	m.termsort->add( { id, toDom(ft->sort(), symbols) }, true);
	m.fsymbol->add( { id, toDom(ft->function(), symbols) }, true);
	addSymbol(ft->function(), m, symbols);
	for (uint i = 0; i < ft->subterms().size(); ++i) {
		ids.clear();
		ft->subterms()[i]->accept(this);
		Assert(ids.size() == 1);
		m.fterms->add( { id, createDomElem((int) i), ids.front() }, true);
	}
	CLOSE
}

void Metafier::visit(const VarTerm* ft) {
	INIT
	m.termsort->add( { id, toDom(ft->sort(), symbols) }, true);
	auto vid = createDomElem(vars.getUnique(ft->var()));
	m.varelem->add( { id, vid }, true);
	m.varsort->add( { vid, toDom(ft->var()->sort(), symbols) }, true);
	CLOSE
}

void Metafier::visit(const DomainTerm* ft) {
	INIT
	m.termsort->add( { id, toDom(ft->sort(), symbols) }, true);
	m.domelem->add( { id, ft->value() }, true);
	CLOSE
}

// true if was negative and handled
bool Metafier::handleNegation(const Formula* f) {
	if (f->sign() == SIGN::POS) {
		return false;
	}
	INIT
	const_cast<Formula*>(f)->negate();
	f->accept(this);
	auto idsub = ids.back();
	const_cast<Formula*>(f)->negate();
	m.negations->add(id);
	m.singleform->add( { id, idsub }, true);
	CLOSE
	return true;
}

#define HANDLENEGATION(form)\
		if(handleNegation(form)){\
			return;\
		}

void Metafier::visit(const PredForm* pf) {
	HANDLENEGATION(pf)
	INIT
	m.asymbol->add( { id, toDom(pf->symbol(), symbols) }, true);
	addSymbol(pf->symbol(), m, symbols);
	for (uint i = 0; i < pf->subterms().size(); ++i) {
		ids.clear();
		pf->subterms()[i]->accept(this);
		Assert(ids.size() == 1);
		m.aterms->add( { id, createDomElem((int) i), ids.front() }, true);
	}
	CLOSE
}

void Metafier::visit(const BoolForm* bf) {
	HANDLENEGATION(bf)
	INIT
	m.btype->add( { id, bf->conj() ? m.conj : m.disj }, true);
	for (uint i = 0; i < bf->subformulas().size(); ++i) {
		ids.clear();
		bf->subformulas()[i]->accept(this);
		Assert(ids.size() == 1);
		m.bsubforms->makeTrueExactly( { id, ids.front() }, true);
	}
	CLOSE
}
void Metafier::visit(const EquivForm* eqf) {
	HANDLENEGATION(eqf)
	INIT
	ids.clear();
	eqf->left()->accept(this);
	Assert(ids.size() == 1);
	m.eqleft->add( { id, ids.front() }, true);
	ids.clear();
	eqf->right()->accept(this);
	Assert(ids.size() == 1);
	m.eqright->add( { id, ids.front() }, true);
	CLOSE
}
void Metafier::visit(const QuantForm* qf) {
	HANDLENEGATION(qf)
	INIT
	m.qtype->add( { id, qf->quant() == QUANT::UNIV ? m.forall : m.exists }, true);

	for (auto v : qf->quantVars()) {
		auto vid = createDomElem(vars.getUnique(v));
		m.qvars->makeTrueExactly( { id, vid }, true);
		m.varsort->add( { vid, toDom(v->sort(), symbols) }, true);
	}

	ids.clear();
	qf->subformula()->accept(this);
	Assert(ids.size() == 1);
	m.singleform->add( { id, ids.front() }, true);
	CLOSE
}

void Metafier::visit(const Rule* r) {
	INIT
	for (auto v : r->quantVars()) {
		auto vid = createDomElem(vars.getUnique(v));
		m.rvars->makeTrueExactly( { id, vid }, true);
		m.varsort->add( { vid, toDom(v->sort(), symbols) }, true);
	}

	ids.clear();
	r->head()->accept(this);
	Assert(ids.size() == 1);
	m.rhead->add( { id, ids.front() }, true);

	ids.clear();
	r->body()->accept(this);
	Assert(ids.size() == 1);
	m.rbody->add( { id, ids.front() }, true);
	CLOSE
}
void Metafier::visit(const Definition* d) {
	INIT
	for (auto r : d->rules()) {
		ids.clear();
		r->accept(this);
		Assert(ids.size() == 1);
		m.ruleof->makeTrueExactly( { ids.front(), id }, true);
	}
	CLOSE
}

void Metafier::visit(const EqChainForm* eq) {
	Formula* f = eq->clone();
	auto cloned = FormulaUtils::splitComparisonChains(f, NULL);
	cloned->accept(this);
	cloned->recursiveDelete();
}

void Metafier::visit(const AggForm* af) {
	auto ecf = new EqChainForm(af->sign(), true, { af->getBound(), af->getAggTerm() }, { af->comp() }, { });
	ecf->accept(this);
	delete ecf;
}

const DomainElement* Metafier::getAgg(AggFunction f) {
	switch (f) {
	case AggFunction::CARD:
		return m.card;
	case AggFunction::MAX:
		return m.max;
	case AggFunction::MIN:
		return m.min;
	case AggFunction::PROD:
		return m.prod;
	case AggFunction::SUM:
		return m.sum;
	default:
		throw IdpException("Invalid code path: unexpected aggregate function");
	}
}

void Metafier::visit(const AggTerm* at) {
	INIT
	m.aggfunc->add( { id, getAgg(at->function()) }, true);
	m.termsort->add( { id, toDom(at->sort(), symbols) }, true);
	ids.clear();
	at->set()->accept(this);
	Assert(ids.size() == 1);
	m.aggset->add( { id, ids.front() }, true);
	CLOSE
}

void Metafier::visit(const EnumSetExpr* es) {
	INIT
	for (auto s : es->getSubSets()) {
		ids.clear();
		s->accept(this);
		Assert(ids.size() == 1);
		m.setof->makeTrueExactly( { ids.front(), id }, true);
	}
	CLOSE
}
void Metafier::visit(const QuantSetExpr* qs) {
	INIT
	ids.clear();
	qs->getTerm()->accept(this);
	Assert(ids.size() == 1);
	m.setterm->add( { id, ids.front() }, true);
	ids.clear();
	qs->getCondition()->accept(this);
	m.setform->add( { id, ids.front() }, true);
	for (auto v : qs->quantVars()) {
		auto vid = createDomElem(vars.getUnique(v));
		m.setvars->makeTrueExactly( { id, vid }, true);
		m.varsort->add( { vid, toDom(v->sort(), symbols) }, true);
	}
	CLOSE
}
