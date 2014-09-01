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

#include "IncludeComponents.hpp"
#include "utils/UniqueNames.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "inferences/querying/Query.hpp"

/* TOOO issue with model:
 *	subtypes cannot be extended during mx (so splitdefs is not yet possible)
 *  Possible solutions:
 *  	allow unary predicates as subtypes, replacing them with parent types during parsing
 *  		probably not much work!
 *  		but have to specify sub/supertypes over predicate symbols then
 *  			or express it in the theory every time
 */

inline const DomainElement* toDom(PFSymbol* symbol, UniqueStringNames<PFSymbol*>& symbols) {
	return createDomElem(symbols.getUnique(symbol));
}
inline const DomainElement* toDom(Sort* sort, UniqueStringNames<PFSymbol*>& symbols) {
	return createDomElem(symbols.getUnique(sort->pred()));
}

struct MetaInters {
	SortTable *preds, *sorts, *funcs, *vars;
	PredInter *partial, *subtypeof, *constrFrom;
	FuncInter *name, *arities, *argsorts, *outsort, *varsort;

	SortTable *forms, *negations, *quants, *boolforms, *atoms, *equivs, *defs;
	SortTable *functerms, *domterms, *varterms, *aggterms, *quantsets, *enumsets;
	FuncInter *singleform, *qtype, *btype, *eqleft, *eqright, *asymbol, *aterms, *termsort, *domelem, *fsymbol, *fterms, *varelem, *aggfunc, *aggset, *rhead,
			*rbody, *setform, *setterm;
	PredInter *qvars, *rvars, *bsubforms, *ruleof, *setof, *setvars, *sentences;

	const DomainElement *conj, *disj, *forall, *exists;
	const DomainElement *sum, *min, *max, *prod, *card;

	//Takes as input the name of a nilary constructor. Outputs its value, i.e. the compound name()
	const DomainElement* getConstrElem(std::string name, const Structure* str) {
		auto func=*str->vocabulary()->func_no_arity(name).begin();
		Assert(func->arity() == 0);
		return str->inter(func)->value({});
	}

	MetaInters(Structure* str) {
		auto voc = str->vocabulary();

		str->changeInter(voc->sort("Index"), TableUtils::createSortTable(0, 10));
		// TODO make max index an option

		preds = str->inter(voc->sort("Pred"));
		sorts = str->inter(voc->sort("Sort"));
		funcs = str->inter(voc->sort("Func"));
		vars = str->inter(voc->sort("Var"));
		arities = str->inter(voc->func("arity/1"));
		name = str->inter(voc->func("name/1"));
		partial = str->inter(voc->pred("isPartial/1"));
		constrFrom = str->inter(voc->pred("constructedFrom/2"));
		argsorts = str->inter(voc->func("sort/2"));
		outsort = str->inter(voc->func("outputSort/1"));
		subtypeof = str->inter(voc->pred("subtypeOf/2"));
		varsort = str->inter(voc->func("varSort/1"));

		forms = str->inter(voc->sort("Form"));
		negations = str->inter(voc->sort("Negation"));
		quants = str->inter(voc->sort("Quant"));
		boolforms = str->inter(voc->sort("BoolForm"));
		equivs = str->inter(voc->sort("Equiv"));
		atoms = str->inter(voc->sort("Atom"));
		defs = str->inter(voc->sort("Definition"));
		functerms = str->inter(voc->sort("FuncT"));
		domterms = str->inter(voc->sort("DomT"));
		varterms = str->inter(voc->sort("VarT"));
		aggterms = str->inter(voc->sort("AggT"));
		quantsets = str->inter(voc->sort("QuantSet"));
		enumsets = str->inter(voc->sort("EnumSet"));

		singleform = str->inter(voc->func("subForm/1"));

		qtype = str->inter(voc->func("quantType/1"));
		qvars = str->inter(voc->pred("quantVars/2"));

		bsubforms = str->inter(voc->pred("subForms/2"));
		btype = str->inter(voc->func("boolType/1"));

		eqleft = str->inter(voc->func("left/1"));
		eqright = str->inter(voc->func("right/1"));

		asymbol = str->inter(voc->func("aSymbol/1"));
		aterms = str->inter(voc->func("aTerms/2"));

		termsort = str->inter(voc->func("termSort/1"));

		domelem = str->inter(voc->func("dom/1"));

		fsymbol = str->inter(voc->func("fSymbol/1"));
		fterms = str->inter(voc->func("fTerms/2"));

		varelem = str->inter(voc->func("var/1"));

		aggfunc = str->inter(voc->func("aggFunc/1"));
		aggset = str->inter(voc->func("aggSet/1"));

		rhead = str->inter(voc->func("head/1"));
		rbody = str->inter(voc->func("body/1"));
		ruleof = str->inter(voc->pred("ruleOf/2"));
		rvars = str->inter(voc->pred("ruleVars/2"));

		setform = str->inter(voc->func("setForm/1"));
		setvars = str->inter(voc->pred("setVars/2"));
		setterm = str->inter(voc->func("setTerm/1"));
		setof = str->inter(voc->pred("setOf/2"));

		sentences = str->inter(voc->pred("sentence/1"));

		conj = getConstrElem("conj", str);
		disj = getConstrElem("disj", str);
		forall = getConstrElem("forall", str);
		exists = getConstrElem("exists", str);

		card = getConstrElem("card", str);
		prod = getConstrElem("prod", str);
		sum = getConstrElem("sum", str);
		max = getConstrElem("max", str);
		min = getConstrElem("min", str);
	}
};

void addSymbol(PFSymbol* s, MetaInters& m, UniqueStringNames<PFSymbol*>& symbols);

class Metafier: public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	MetaInters& m;
	int maxid;
	std::vector<const DomainElement*> ids;

	UniqueStringNames<PFSymbol*>& symbols;
	UniqueStringNames<Variable*> vars;

public:
	Metafier(Theory* f, MetaInters& meta, UniqueStringNames<PFSymbol*>& _symbols)
			: 	m(meta),
			  	maxid(0),
				symbols(_symbols) {
		f->accept(this);
		for (auto id : ids) {
			m.sentences->makeTrueExactly( { id }, true);
		}
	}

	const DomainElement* getAgg(AggFunction f);

	bool handleNegation(const Formula* f);

	void visit(const FuncTerm*);
	void visit(const DomainTerm*);
	void visit(const AggTerm*);
	void visit(const VarTerm*);

	void visit(const PredForm*);
	void visit(const AggForm*);
	void visit(const BoolForm*);
	void visit(const EquivForm*);
	void visit(const EqChainForm*);
	void visit(const Rule*);
	void visit(const Definition*);
	void visit(const QuantForm*);

	void visit(const QuantSetExpr*);
	void visit(const EnumSetExpr*);
};

class DeMetafier {
private:
	Vocabulary* voc;
	Structure* str;
	MetaInters m;
	int maxid;
	std::vector<const DomainElement*> ids;

	UniqueStringNames<PFSymbol*>& symbols;
	UniqueStringNames<Variable*> vars;

	std::map<const DomainElement*, varset> f2vars;
	std::map<const DomainElement*, std::vector<const DomainElement*>> f2forms;
	std::map<const DomainElement*, Variable*> d2var;
	std::map<const DomainElement*, std::vector<const DomainElement*>> es2qsets;
	std::map<const DomainElement*, std::vector<const DomainElement*>> def2rules;

public:
	/**
	 * Preconditions to structure
	 * 		form- and termtypes are disjoint
	 */
	DeMetafier(Vocabulary* origvoc, Structure* str, UniqueStringNames<PFSymbol*>& _symbols)
			: 	voc(origvoc),
				str(str),
				m(str),
				maxid(0),
				symbols(_symbols) {
		for (auto i = m.vars->begin(); not i.isAtEnd(); ++i) {
			d2var[(*i)[0]] = new Variable(symbols.getOriginal(*m.varsort->value( { (*i)[0] })->value()._string)->sorts()[0]);
		}
		for (auto i = m.qvars->ct()->begin(); not i.isAtEnd(); ++i) {
			f2vars[(*i)[0]].insert(d2var[(*i)[1]]);
		}
		for (auto i = m.setvars->ct()->begin(); not i.isAtEnd(); ++i) {
			f2vars[(*i)[0]].insert(d2var[(*i)[1]]);
		}
		for (auto i = m.rvars->ct()->begin(); not i.isAtEnd(); ++i) {
			f2vars[(*i)[0]].insert(d2var[(*i)[1]]);
		}
		for (auto i = m.bsubforms->ct()->begin(); not i.isAtEnd(); ++i) {
			f2forms[(*i)[0]].push_back((*i)[1]);
		}
		for (auto i = m.setof->ct()->begin(); not i.isAtEnd(); ++i) {
			es2qsets[(*i)[1]].push_back((*i)[0]);
		}
		for (auto i = m.ruleof->ct()->begin(); not i.isAtEnd(); ++i) {
			def2rules[(*i)[1]].push_back((*i)[0]);
		}
	}

	Theory* translate(Predicate* setOfSentences) {
		std::stringstream ss;
		ss <<"demetafied_theory_" <<getGlobal()->getNewID();
		auto t = new Theory(ss.str(), voc, { });
		for (auto i = str->inter(setOfSentences)->ct()->begin(); not i.isAtEnd(); ++i) {
			auto e = (*i).front();
			if (m.defs->contains(e)) {
				t->add(getDef(e));
			} else {
				t->add(getForm(e));
			}
		}
		return t;
	}

	Definition* getDef(const DomainElement* d) {
		auto def = new Definition();
		for (auto r : def2rules[d]) {
			def->add(getRule(r));
		}
		return def;
	}

	Rule* getRule(const DomainElement* d) {
		return new Rule(f2vars[d], getPredForm(m.rhead->value( { d })), getForm(m.rbody->value( { d })), { });
	}

	PredForm* getPredForm(const DomainElement* d) {
		auto symd = m.asymbol->value( { d });
		auto symbol = symbols.getOriginal(*symd->value()._string);
		std::vector<Term*> terms;
		auto ar = m.arities->value( { symd })->value()._int;
		if (symbol->isFunction()) {
			ar++;
		}
		for (auto i = 0; i < ar; i++) {
			terms.push_back(getTerm(m.aterms->value( { d, createDomElem(i) })));
		}
		return new PredForm(SIGN::POS, symbol, terms, { });
	}

	// TODO for now, guaranteed to not be a definition
	Formula* getForm(const DomainElement* d) {
		if (m.negations->contains(d)) {
			auto subf = getForm(m.singleform->value( { d }));
			subf->negate();
			return subf;
		} else if (m.quants->contains(d)) {
			auto qt = m.qtype->value( { d }) == m.forall ? QUANT::UNIV : QUANT::EXIST;
			auto subf = getForm(m.singleform->value( { d }));
			return new QuantForm(SIGN::POS, qt, f2vars[d], subf, { });
		} else if (m.boolforms->contains(d)) {
			auto conj = m.btype->value( { d }) == m.conj;
			std::vector<Formula*> forms;
			for (auto f : f2forms[d]) {
				forms.push_back(getForm(f));
			}
			return new BoolForm(SIGN::POS, conj, forms, { });
		} else if (m.atoms->contains(d)) {
			return getPredForm(d);
		} else {
			Assert(m.equivs->contains(d));
			auto left = getForm(m.eqleft->value( { d }));
			auto right = getForm(m.eqright->value( { d }));
			return new EquivForm(SIGN::POS, left, right, { });
		}
	}

	AggFunction getAggFunc(const DomainElement* d) {
		if (d == m.card) {
			return AggFunction::CARD;
		} else if (d == m.sum) {
			return AggFunction::SUM;
		} else if (d == m.prod) {
			return AggFunction::PROD;
		} else if (d == m.min) {
			return AggFunction::MIN;
		} else if (d == m.max) {
			return AggFunction::MAX;
		} else {
			throw IdpException("Invalid code path: unexpected aggregate function encoding in meta-representation");
		}
	}

	Term* getTerm(const DomainElement* d) {
		auto sort = symbols.getOriginal(*m.termsort->value( { d })->value()._string)->sorts()[0];
		if (m.functerms->contains(d)) {
			auto symd = m.fsymbol->value( { d });
			auto func = dynamic_cast<Function*>(symbols.getOriginal(*symd->value()._string));
			std::vector<Term*> terms;
			for (auto i = 0; i < m.arities->value( { symd })->value()._int; i++) {
				terms.push_back(getTerm(m.fterms->value( { d, createDomElem(i) })));
			}
			return new FuncTerm(func, terms, { });
		} else if (m.domterms->contains(d)) {
			auto dom = m.domelem->value( { d });
			return new DomainTerm(sort, dom, { });
		} else if (m.varterms->contains(d)) {
			return new VarTerm(d2var[m.varelem->value({d})], { });
		} else {
			Assert(m.aggterms->contains(d));
			return new AggTerm(getEnumSet(m.aggset->value( { d })), getAggFunc(m.aggfunc->value( { d })), { });
		}
	}

	EnumSetExpr* getEnumSet(const DomainElement* d) {
		std::vector<QuantSetExpr*> sets;
		for (auto s : es2qsets[d]) {
			sets.push_back(getQuantSet(s));
		}
		return new EnumSetExpr(sets, { });
	}

	QuantSetExpr* getQuantSet(const DomainElement* d) {
		return new QuantSetExpr(f2vars[d], getForm(m.setform->value( { d })), getTerm(m.setterm->value( { d })), { });
	}
};
