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
#include <string>

using namespace std;

template<class T, class C>
set<T,C> difference(const set<T,C>& v1, const set<T,C>& v2) {
	auto temp = v1;
	for (auto i = v2.cbegin(); i != v2.cend(); ++i) {
		temp.erase(*i);
	}
	return temp;
}

template<class RuleList>
void add(RuleList& list, PredForm* head, Formula* body, ApproxData* data) {
	if (not head->symbol()->builtin() && // don't define built-in symbols
			data->actions.find(head->symbol()) == data->actions.cend()) {
		list.push_back(new Rule(head->freeVars(), head, body, ParseInfo()));
	}
}

void handlePredForm(const PredForm* pf, ApproxData* data, vector<Rule*>* rules) {
	if(data->_baseformulas_already_added ||
					data->_pred2predCt.find(pf->symbol()) == data->_pred2predCt.end()) {
		return;
	}
	if (data->_predCt2InputPredCt.find(data->_pred2predCt[pf->symbol()]) == data->_predCt2InputPredCt.cend()) {
		// predform symbol hasn't already been handled before
		auto ctpred = new Predicate((data->_pred2predCt[pf->symbol()]->nameNoArity() + "_input_ct"),pf->symbol()->sorts());
		auto cfpred = new Predicate((data->_pred2predCf[pf->symbol()]->nameNoArity() + "_input_cf"),pf->symbol()->sorts());

		data->_predCt2InputPredCt.insert( std::pair<PFSymbol*,PFSymbol*>(data->_pred2predCt[pf->symbol()],ctpred) );
		data->_predCf2InputPredCf.insert( std::pair<PFSymbol*,PFSymbol*>(data->_pred2predCf[pf->symbol()],cfpred) );

		if(pf->sign() == SIGN::NEG) {
			std::swap(ctpred,cfpred);
		}
		PredForm* ctformula = new PredForm(SIGN::POS, ctpred, pf->subterms(), FormulaParseInfo());
		PredForm* cfformula = new PredForm(SIGN::POS, cfpred, pf->subterms(), FormulaParseInfo());

		add(*rules, data->formula2ct[pf], ctformula, data);
		add(*rules, data->formula2cf[pf], cfformula, data);
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

		std::vector<Formula*> vareq_forms;
		varset vars;
		std::vector<Term*> terms;
		for(auto i=qf->quantVars().cbegin(); i!=qf->quantVars().cend(); ++i){
			auto newvar = new Variable((*i)->sort());
			auto newterm = new VarTerm(newvar, TermParseInfo());
			vars.insert(newvar);
			terms.push_back(newterm);
			vareq_forms.push_back(new PredForm(SIGN::POS, get(STDPRED::EQ, (*i)->sort()), {newterm, new VarTerm(*i, TermParseInfo())}, FormulaParseInfo()));
		}
		auto vareqs = &Gen::conj(vareq_forms);
		std::vector<Formula*> disj_forms;
		disj_forms.push_back(vareqs);
		disj_forms.push_back(new PredForm(SIGN::POS, (((*first)[qf->subformula()])->symbol()),terms,FormulaParseInfo()));
		auto& quant = Gen::forall(vars, Gen::disj(disj_forms));

		add(topdownrules, (*second)[qf->subformula()], &Gen::conj({&quant, (*second)[qf]}), data);
		traverse(qf);
	}

	void visit(const PredForm* pf) {
		handlePredForm(pf, data, &topdownrules);
	}
	void visit(const AggForm*) {
//		throw IdpException("Generating an approximating definition does not work for aggregate formulas.");
	}
	void visit(const EqChainForm*) {
//		throw IdpException("Generating an approximating definition does not work for comparison chains.");
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
	virtual void visit(const CPSetTerm*) {
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
	 * 		Pct(x) <- ?y: Lct(x, y)
	 * 		Pcf(x) <- !y: Lcf(x, y)
	 *
	 * UNIV:
	 * 		Pct(x) <- !y: Lct(x, y)
	 * 		Pcf(x) <- ?y: Lcf(x, y)
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

	void visit(const PredForm* pf) {
		handlePredForm(pf, data, &bottomuprules);
	}
	void visit(const AggForm*) {
//		throw IdpException("Generating an approximating definition does not work for aggregate formulas.");
	}
	void visit(const EqChainForm*) {
//		throw IdpException("Generating an approximating definition does not work for comparison chains.");
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
	virtual void visit(const CPSetTerm*) {
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
	for (auto i = _sentences.cbegin(); i < _sentences.cend(); ++i) {
		auto tr = new BoolForm(SIGN::POS, true, { }, FormulaParseInfo());
		auto ts = data->formula2ct[*i];
		if(not ts->symbol()->builtin()) {
			d->add(new Rule(ts->freeVars(), ts, tr, ParseInfo()));
		}
	}
	if (dir != Direction::UP) {
		d->add(getallDownRules());
		data->_baseformulas_already_added=true;
	}
	if (dir != Direction::DOWN) {
		d->add(getallUpRules());
		data->_baseformulas_already_added=true;
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

void GenerateApproximatingDefinition::setFormula2PredFormMap(Formula* f, const AbstractStructure* s) {
	auto ctcfpair = std::pair<PredForm*,PredForm*>();
	auto swapIfNegated = true;
	if(isa<PredForm>(*f)) {
		auto fPredForm = dynamic_cast<PredForm*>(f);
		if (data->_pred2predCt.find(fPredForm->symbol()) == data->_pred2predCt.cend()) {
			if (fPredForm->symbol()->builtin()) {
				ctcfpair.first = fPredForm;
				auto clone = fPredForm->clone();
				clone->negate();
				ctcfpair.second = clone;
				swapIfNegated = false;
			}else if (s->inter(fPredForm->symbol())->approxTwoValued()) {
				ctcfpair.first = fPredForm;
				auto clone = fPredForm->clone();
				clone->negate();
				ctcfpair.second = clone;
				swapIfNegated = false;
				data->actions.insert(fPredForm->symbol());
			} else {
				Predicate* ctpred = new Predicate((fPredForm->symbol()->nameNoArity() + "_ct"),fPredForm->symbol()->sorts());
				Predicate* cfpred = new Predicate((fPredForm->symbol()->nameNoArity() + "_cf"),fPredForm->symbol()->sorts());
				ctcfpair.first = new PredForm(SIGN::POS, ctpred, fPredForm->subterms(), FormulaParseInfo());
				ctcfpair.second = new PredForm(SIGN::POS, cfpred, fPredForm->subterms(), FormulaParseInfo());

				data->_pred2predCt.insert( std::pair<PFSymbol*,PFSymbol*>(fPredForm->symbol(),ctcfpair.first->symbol()) );
				data->_pred2predCf.insert( std::pair<PFSymbol*,PFSymbol*>(fPredForm->symbol(),ctcfpair.second->symbol()) );
			}
		} else {
			ctcfpair.first = new PredForm(SIGN::POS, data->_pred2predCt[fPredForm->symbol()], fPredForm->subterms(), FormulaParseInfo());
			ctcfpair.second = new PredForm(SIGN::POS, data->_pred2predCf[fPredForm->symbol()], fPredForm->subterms(), FormulaParseInfo());
		}
	} else {
		ctcfpair = createGeneralPredForm(f);
	}

	if(f->sign() == SIGN::NEG  && swapIfNegated) { // If the formula is negative, the _ct and _cf maps need to be swapped
		std::swap(ctcfpair.first,ctcfpair.second);
	}

	if (getOption(IntType::VERBOSE_APPROXDEF) >= 2) {
		clog << "In the approximating definitions, " << toString(ctcfpair.first) << " represents formula " << toString(f) << " being true\n";
		clog << "In the approximating definitions, " << toString(ctcfpair.second) << " represents formula " << toString(f) << " being false\n";
	}
	data->formula2ct.insert( std::pair<Formula*,PredForm*>(f,ctcfpair.first) );
	data->formula2cf.insert( std::pair<Formula*,PredForm*>(f,ctcfpair.second) );

	for (auto subf : f->subformulas()) {
		setFormula2PredFormMap(subf,s);
	}
}

std::pair<PredForm*,PredForm*> GenerateApproximatingDefinition::createGeneralPredForm(Formula* f) {

	auto subterms = std::vector<Term*>();
	for(auto fv : f->freeVars()) {
		subterms.push_back(new VarTerm(fv, TermParseInfo()));
	}
	auto formulaID = getGlobal()->getNewID();
	std::vector<Sort*> sorts;
	for(auto var : f->freeVars()) {
		sorts.push_back(var->sort());
	}
	auto ctpred = new Predicate(("T" + toString(formulaID) + "_ct"),sorts);
	auto cfpred = new Predicate(("T" + toString(formulaID) + "_cf"),sorts);

	auto newct = new PredForm(SIGN::POS, ctpred, subterms, FormulaParseInfo());
	auto newcf = new PredForm(SIGN::POS, cfpred, subterms, FormulaParseInfo());
	return std::pair<PredForm*,PredForm*>(newct, newcf);
}

std::vector<Formula*>* GenerateApproximatingDefinition::performTransformations(const std::vector<Formula*>& sentences, AbstractStructure* s) {
	std::vector<Formula*>* ret = new vector<Formula*>();
	for(auto sentence : sentences) {
		auto copyToWorkOn = sentence->clone();
		auto context = Context::POSITIVE;
		if (copyToWorkOn->sign() == SIGN::NEG) {
			context = Context::NEGATIVE;
		}
		auto sentence2 = FormulaUtils::unnestFuncsAndAggs(copyToWorkOn,s,context);
		auto sentence3 = FormulaUtils::graphFuncsAndAggs(sentence2,s,true,false,context);
		auto sentence4 = FormulaUtils::removeEquivalences(sentence3);
		auto sentence5 = FormulaUtils::pushNegations(sentence4);
		ret->push_back(sentence5);
	}
	return ret;
}

Theory* GenerateApproximatingDefinition::constructTheory(Definition* def) {
	auto ret = new Theory("approxdef_theory", ParseInfo());
	ret->add(def->clone());
	return ret;
}
Vocabulary* GenerateApproximatingDefinition::constructVocabulary(AbstractStructure* s, Definition* d) {
	auto ret = new Vocabulary(s->vocabulary()->name());

	for(auto ctf : data->formula2ct) {
		ret->add(ctf.second->symbol());
	}
	for(auto ctf : data->_predCt2InputPredCt) {
		ret->add(ctf.second);
	}

	for(auto cff : data->formula2cf) {
		ret->add(cff.second->symbol());
	}
	for(auto cff : data->_predCf2InputPredCf) {
		ret->add(cff.second);
	}

	for (auto sort : s->vocabulary()->getSorts()) {
		ret->add(sort.second);
	}
	for (auto opensymbol: DefinitionUtils::opens(d)) {
		ret->add(opensymbol);
	}

	return ret;
}

AbstractStructure* GenerateApproximatingDefinition::constructStructure(AbstractStructure* s, Theory* t, Vocabulary* v) {
	auto ret = new Structure("approxdef_struct", v, ParseInfo());

	for(auto sortinter : s->getSortInters()) {
		auto interToChange = ret->inter(sortinter.first);
		ret->inter(sortinter.first)->internTable(sortinter.second->internTable());
	}

	for(auto ctf : data->_pred2predCt) {
		auto newinter = new PredInter(s->inter(ctf.first)->ct(),true);
		auto interToChange = ret->inter(data->_predCt2InputPredCt[ctf.second]);
		interToChange->ctpt(newinter->ct());
	}
	for(auto cff : data->_pred2predCf) {
		auto newinter = new PredInter(s->inter(cff.first)->cf(),true);
		auto interToChange = ret->inter(data->_predCf2InputPredCf[cff.second]);
		interToChange->ctpt(newinter->ct());
	}
	// Only one definition in the theory
	auto definition = *(t->definitions().begin());
	auto opens = DefinitionUtils::opens(definition);
	for (auto opensymbol : opens) {
		if (s->vocabulary()->contains(opensymbol) &&
				s->inter(opensymbol)->approxTwoValued()) {
			ret->inter(opensymbol)->ctpt(s->inter(opensymbol)->ct());
		}
	}

	return ret;
}

void GenerateApproximatingDefinition::updateStructure(AbstractStructure* s, AbstractStructure* approxdef_struct) {
	for(auto ctf : data->_pred2predCt) {
		s->inter(ctf.first)->ct(approxdef_struct->inter(ctf.second)->ct());
	}
	for(auto cff : data->_pred2predCf) {
		s->inter(cff.first)->cf(approxdef_struct->inter(cff.second)->ct());
	}
}

bool GenerateApproximatingDefinition::isConsistent(AbstractStructure* s) {
	for (auto i = _sentences.cbegin(); i < _sentences.cend(); ++i) {
		auto sentence_cf = data->formula2cf[*i];
		// The sentences cannot be calculated to be certainly false

		if(s->vocabulary()->contains(sentence_cf->symbol()) && not s->inter(sentence_cf->symbol())->ct()->empty()){
			stringstream ss;
			ss << "The approximating definition detected formula " << toString(*i) << " to be certainly false.\n";
			Warning::warning(ss.str());
			return false;
		}
	}
	return true;
}
