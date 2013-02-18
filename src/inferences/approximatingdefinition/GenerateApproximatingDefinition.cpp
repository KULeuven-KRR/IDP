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

#include "GenerateApproximatingDefinition.hpp"

#include "common.hpp"
#include "theory/theory.hpp"
#include "vocabulary/vocabulary.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "creation/cppinterface.hpp"
#include "theory/term.hpp"
#include "utils/ListUtils.hpp"

using namespace std;

template<class Elem>
set<Elem> difference(const set<Elem>& v1, const set<Elem>& v2) {
	auto temp = v1;
	for (auto i = v2.cbegin(); i != v2.cend(); ++i) {
		temp.erase(*i);
	}
	return temp;
}

template<class RuleList>
void add(RuleList& list, PredForm* head, Formula* body, ApproxData* data) {
	if (data->actions.find(head->symbol()) == data->actions.cend()) {
		list.push_back(new Rule(head->freeVars(), head, body, ParseInfo()));
	}
}

// TODO guarantee equivalences have been removed and negations have been pushed!
// TODO handling negations! (pushing them is not the best solution
class TopDownApproximatingDefinition: public TheoryVisitor {
private:
	ApproxData* data;
	vector<Rule*> topdownrules;
public:
	template<typename T>
	const std::vector<Rule*>& execute(T f, ApproxData * approxdata) {
		topdownrules.clear();
		data = approxdata;
		f->accept(this);
		return topdownrules;
	}

	/**
	 * !x y z: P(x, y, z) <=> L1(x, y) | ... | Ln(y, z)
	 *
	 * DISJ:
	 * Lict(x, y) <- ?z: Pct(x, y, z) & Ljcf (!j: j~=i)
	 * Licf(x, y) <- ?z: Pcf(x, y, z)
	 *
	 * CONJ:
	 * Lict(x, y) <- ?z: Pct(x, y, z)
	 * Licf(x, y) <- ?z: Pcf(x, y, z) & Ljct (!j: j~=i)
	 */
	void visit(const BoolForm* bf) {
		Assert(bf->sign()==SIGN::POS);
		auto first = &data->formula2ct, second = &data->formula2cf;
		if (not bf->conj()) {
			first = &data->formula2cf;
			second = &data->formula2ct;
		}
		for (auto i = bf->subformulas().cbegin(); i < bf->subformulas().cend(); ++i) {
			auto v = difference(bf->freeVars(), (*i)->freeVars());
			Formula* f = (*first)[bf];
			if (not v.empty() != 0) {
				f = new QuantForm(SIGN::POS, QUANT::EXIST, v, f, FormulaParseInfo());
			}
			add(topdownrules, (*first)[*i], f, data); // DISJ: Licf(x, y) <- !z: Pcf(x, y, z)

			vector<Formula*> forms;
			forms.push_back((*second)[bf]);
			for (auto j = bf->subformulas().cbegin(); j < bf->subformulas().cend(); ++j) {
				if (i != j) {
					forms.push_back((*first)[*j]);
				}
			}
			f = new BoolForm(SIGN::POS, true, forms, FormulaParseInfo());
			if (not v.empty()) {
				f = new QuantForm(SIGN::POS, QUANT::EXIST, v, f, FormulaParseInfo());
			}
			add(topdownrules, (*second)[*i], f, data); // DISJ: Lict(x, y) <- ?z: Pct(x, y, z) & Ljcf (!j: j~=i)
		}
		traverse(bf);
	}

	/**
	 * !x: P(x) <=> ?y: L(x, y)
	 *
	 * EXISTS:
	 * 		Lcf(x, y) <- Pcf(x)
	 * 		Lct(x, y) <- Pct(x) & !y': y~=y' => Lcf(x, y')
	 */
	void visit(const QuantForm* qf) {
		Assert(qf->sign()==SIGN::POS);
		auto first = &data->formula2cf, second = &data->formula2ct;
		if (qf->isUniv()) {
			first = &data->formula2ct;
			second = &data->formula2cf;
		}

		add(topdownrules, (*first)[qf->subformula()], (*first)[qf], data);

		std::vector<Formula*> forms;
		std::set<Variable*> vars;
		for(auto i=qf->quantVars().cbegin(); i!=qf->quantVars().cend(); ++i){
			auto newvar = new Variable((*i)->sort());
			vars.insert(newvar);
			forms.push_back(new PredForm(SIGN::POS, get(STDPRED::EQ, (*i)->sort()), {new VarTerm(newvar, TermParseInfo()), new VarTerm(*i, TermParseInfo())}, FormulaParseInfo()));
		}
		forms.push_back((*first)[qf->subformula()]);
		auto& quant = Gen::forall(vars, Gen::disj(forms));

		add(topdownrules, (*second)[qf->subformula()], &Gen::conj({&quant, (*first)[qf]}), data);
		traverse(qf);
	}

	void visit(const PredForm*) {
	}
	void visit(const AggForm*) {
		throw IdpException("Generating an approximating definition does not work for aggregate formulas.");
	}
	void visit(const EqChainForm*) {
		throw IdpException("Generating an approximating definition does not work for comparison chains.");
	}
	virtual void visit(const Theory* t) {
		traverse(t);
	}

	// NOTE: should have been transformed away
	void visit(const EquivForm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const AbstractGroundTheory*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const GroundTheory<GroundPolicy>*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const GroundDefinition*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const PCGroundRule*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const AggGroundRule*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const GroundSet*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const GroundAggregate*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const CPReification*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const Rule*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const Definition*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const FixpDef*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const VarTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const FuncTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const DomainTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const AggTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const CPVarTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const CPWSumTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const CPWProdTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const EnumSetExpr*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const QuantSetExpr*) {
		throw IdpException("Illegal execution path, aborting.");
	}
};

class BottomUpApproximatingDefinition: public TheoryVisitor {
private:
	ApproxData* data;
	vector<Rule*> bottomuprules;
public:
	template<class T>
	const std::vector<Rule*>& execute(T* f, ApproxData * approxdata) {
		bottomuprules.clear();
		data = approxdata;
		f->accept(this);
		return bottomuprules;
	}

	/**
	 * !x y z: P(x, y, z) <=> L1(x, y) | ... | Ln(y, z)
	 *
	 * DISJ:
	 * Pct(x, y, z) <- Lict(x, y)
	 * Pcf(x, y, z) <- L1cf & ... & Lncf
	 *
	 * CONJ:
	 * Pct(x, y, z) <- L1ct & ... & L1ct
	 * Pcf(x, y, z) <- Licf(x, y)
	 */
	void visit(const BoolForm* bf) {
		Assert(bf->sign()==SIGN::POS);
		auto first = &data->formula2ct, second = &data->formula2cf;
		if (bf->conj()) {
			first = &data->formula2cf;
			second = &data->formula2ct;
		}
		std::vector<Formula*> forms;
		for (auto i = bf->subformulas().cbegin(); i < bf->subformulas().cend(); ++i) {
			add(bottomuprules, (*first)[bf], (*first)[*i], data); // DISJ: Pct(x, y, z) <- Lict(x, y)

			forms.push_back((*second)[*i]);
		}
		add(bottomuprules, (*second)[bf], &Gen::conj(forms), data); // DISj: Pcf(x, y, z) <- L1cf & ... & Lncf
		traverse(bf);
	}

	/**
	 * !x: P(x) <=> ?y: L(x, y)
	 *
	 * EXISTS:
	 * 		Pct(x, y) <- ?y: Lct(x, y)
	 * 		Pcf(x, y) <- !y: Lcf(x, y)
	 *
	 * UNIV:
	 * 		Pct(x, y) <- !y: Lct(x, y)
	 * 		Pcf(x, y) <- ?y: Lcf(x, y)
	 */
	void visit(const QuantForm* qf) {
		Assert(qf->sign()==SIGN::POS);
		auto exists = &data->formula2ct, univ = &data->formula2cf;
		if (qf->isUniv()) {
			exists = &data->formula2cf;
			univ = &data->formula2ct;
		}
		add(bottomuprules, (*exists)[qf], &Gen::exists(qf->quantVars(), *(*exists)[qf->subformula()]), data);
		add(bottomuprules, (*univ)[qf], &Gen::forall(qf->quantVars(), *(*univ)[qf->subformula()]), data);
		traverse(qf);
	}

	void visit(const PredForm*) {
	}
	void visit(const AggForm*) {
		throw IdpException("Generating an approximating definition does not work for aggregate formulas.");
	}
	void visit(const EqChainForm*) {
		throw IdpException("Generating an approximating definition does not work for comparison chains.");
	}
	virtual void visit(const Theory* t) {
		traverse(t);
	}

	void visit(const EquivForm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const AbstractGroundTheory*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const GroundTheory<GroundPolicy>*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const GroundDefinition*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const PCGroundRule*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const AggGroundRule*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const GroundSet*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const GroundAggregate*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const CPReification*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const Rule*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const Definition*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const FixpDef*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const VarTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const FuncTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const DomainTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const AggTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const CPVarTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const CPWSumTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const CPWProdTerm*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const EnumSetExpr*) {
		throw IdpException("Illegal execution path, aborting.");
	}
	virtual void visit(const QuantSetExpr*) {
		throw IdpException("Illegal execution path, aborting.");
	}
};

Definition* GenerateApproximatingDefinition::getallRules(Direction dir) {
	auto d = new Definition();
	if (dir != Direction::DOWN) {
		d->add(getallUpRules());
	}
	if (dir != Direction::UP) {
		d->add(getallDownRules());
	}
	for (auto i = _sentences.cbegin(); i < _sentences.cend(); ++i) {
		auto tr = new BoolForm(SIGN::POS, true, { }, FormulaParseInfo());
		auto ts = data->formula2ct[*i];
		d->add(new Rule(ts->freeVars(), ts, tr, ParseInfo()));
	}
	return d;
}
std::vector<Rule*> GenerateApproximatingDefinition::getallDownRules() {
	std::vector<Rule*> result;
	for (auto i = _sentences.cbegin(); i < _sentences.cend(); ++i) {
		auto rules = TopDownApproximatingDefinition().execute(*i, data);
		insertAtEnd(result, rules);
	}
	return result;
}

std::vector<Rule*> GenerateApproximatingDefinition::getallUpRules() {
	std::vector<Rule*> result;
	for (auto i = _sentences.cbegin(); i < _sentences.cend(); ++i) {
		auto rules = BottomUpApproximatingDefinition().execute(*i, data);
		insertAtEnd(result, rules);
	}
	return result;
}
