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
void add(RuleList& list, PredForm* head, Formula* body, ApproxDefGeneratorData* data) {
	if (not head->symbol()->builtin() && // don't define built-in symbols
			data->_freesymbols.find(head->symbol()) == data->_freesymbols.cend()) {
		list.push_back(new Rule(head->freeVars(), head, body, ParseInfo()));
	}
}

void handlePredForm(const PredForm* pf, ApproxDefGeneratorData* data, vector<Rule*>* rules) {
	if(data->_baseformulas_already_added ||
			data->_mappings->_pred2predCt.find(pf->symbol()) == data->_mappings->_pred2predCt.end()) {
		return;
	}
	if (data->_mappings->_predCt2InputPredCt.find(data->_mappings->_pred2predCt[pf->symbol()]) == data->_mappings->_predCt2InputPredCt.cend()) {
		// predform symbol hasn't already been handled before
		auto ctpred = new Predicate((data->_mappings->_pred2predCt[pf->symbol()]->nameNoArity() + "_input_ct"),pf->symbol()->sorts());
		auto cfpred = new Predicate((data->_mappings->_pred2predCf[pf->symbol()]->nameNoArity() + "_input_cf"),pf->symbol()->sorts());

		data->_mappings->_predCt2InputPredCt.insert( std::pair<PFSymbol*,PFSymbol*>(data->_mappings->_pred2predCt[pf->symbol()],ctpred) );
		data->_mappings->_predCf2InputPredCf.insert( std::pair<PFSymbol*,PFSymbol*>(data->_mappings->_pred2predCf[pf->symbol()],cfpred) );

		if(pf->sign() == SIGN::NEG) {
			std::swap(ctpred,cfpred);
		}
		std::vector<Term*> newSubTerms = pf->args();
		for(unsigned int it = 0; it < pf->args().size(); it++) {
			auto newvar = new Variable(pf->args()[it]->sort());
			auto newterm = new VarTerm(newvar, TermParseInfo());
			newSubTerms[it] = newterm;
		}

		PredForm* ct_head = new PredForm(SIGN::POS, data->_mappings->_formula2ct[pf]->symbol(), newSubTerms, FormulaParseInfo());
		PredForm* cf_head= new PredForm(SIGN::POS, data->_mappings->_formula2cf[pf]->symbol(), newSubTerms, FormulaParseInfo());
		PredForm* ctformula = new PredForm(SIGN::POS, ctpred, newSubTerms, FormulaParseInfo());
		PredForm* cfformula = new PredForm(SIGN::POS, cfpred, newSubTerms, FormulaParseInfo());

		add(*rules, ct_head, ctformula, data);
		add(*rules, cf_head, cfformula, data);
	}
}

// TODO guarantee equivalences have been removed and negations have been pushed!
// TODO handling negations! (pushing them is not the best solution
class TopDownApproximatingDefinition: public TheoryVisitor {
private:
	ApproxDefGeneratorData* _data;
	vector<Rule*> _topdownrules;

	void generateTopDownApproximation(const BoolForm* bf,
			std::map<const Formula*, PredForm*> map1,
			std::map<const Formula*, PredForm*> map2) {
		if (!bf->conj()) {
			// This is a "cheap rule"
			if (_data->_rule_types.find(ApproximatingDefinition::RuleType::CHEAP)
					!= _data->_rule_types.end()) {
				for (auto i = bf->subformulas().cbegin(); i < bf->subformulas().cend(); ++i) {
					auto v = difference(bf->freeVars(), (*i)->freeVars());
					Formula* f = map1[bf];
					vector<Formula*> forms;
					forms.push_back(map2[bf]);
					for (auto j = bf->subformulas().cbegin(); j < bf->subformulas().cend(); ++j) {
						if (i != j) {
							forms.push_back(map1[*j]);
						}
					}
					f = new BoolForm(SIGN::POS, true, forms, FormulaParseInfo());
					if (!v.empty()) {
						f = new QuantForm(SIGN::POS, QUANT::EXIST, v, f, FormulaParseInfo());
					}
					add(_topdownrules, map2[*i], f, _data); // DISJ: Lict(x, y) <- ?z: Pct(x, y, z) & Ljcf (!j: j~=i)
				}
			}
		} else {
			// This is a "cheap rule"
			if (_data->_rule_types.find(ApproximatingDefinition::RuleType::CHEAP)
					!= _data->_rule_types.end()) {
				for (auto i = bf->subformulas().cbegin(); i < bf->subformulas().cend(); ++i) {
					auto v = difference(bf->freeVars(), (*i)->freeVars());
					Formula* f = map2[bf];
					if (!v.empty() != 0) {
						f = new QuantForm(SIGN::POS, QUANT::EXIST, v, f, FormulaParseInfo());
					}
					add(_topdownrules, map2[*i], f, _data); // DISJ: Licf(x, y) <- !z: Pcf(x, y, z)
				}
			}
		}
	}

	void generateTopDownApproximation(const QuantForm* qf,
			std::map<const Formula*, PredForm*> map1,
			std::map<const Formula*, PredForm*> map2) {
		if (qf->isUniv()) {
			// This is a "cheap rule"
			if (_data->_rule_types.find(ApproximatingDefinition::RuleType::CHEAP)
					!= _data->_rule_types.end()) {
				add(_topdownrules, map2[qf->subformula()], map2[qf], _data);
			}

		} else {
			// This is a "forall" rule
			if (_data->_rule_types.find(ApproximatingDefinition::RuleType::FORALL)
					!= _data->_rule_types.end()) {
				std::vector<Formula*> vareq_forms;
				// subterms for the call to QF's subformula - some of these will be newly created varterms
				std::vector<Term*> newSubTerms = map2[qf->subformula()]->subterms();
				varset vars;
				for (auto i = qf->quantVars().cbegin(); i != qf->quantVars().cend(); ++i) {
					auto newvar = new Variable((*i)->sort());
					auto newterm = new VarTerm(newvar, TermParseInfo());
					vars.insert(newvar);
					// Replace the old term with the new varterm that should be used in the call to QF's subformula
					for (unsigned int it = 0; it < map2[qf->subformula()]->subterms().size(); it++) {
						if (map2[qf->subformula()]->subterms()[it]->contains(*i)) {
							newSubTerms[it] = newterm;
						}
					}
					vareq_forms.push_back(new PredForm(SIGN::POS, get(STDPRED::EQ, (*i)->sort()),{ newterm, new VarTerm(*i, TermParseInfo()) },FormulaParseInfo()));
				}
				auto vareqs = &Gen::conj(vareq_forms);
				std::vector<Formula*> disj_forms;
				disj_forms.push_back(vareqs);
				disj_forms.push_back(new PredForm(SIGN::POS,((map1[qf->subformula()])->symbol()), newSubTerms,FormulaParseInfo()));
				auto& quant = Gen::forall(vars, Gen::disj(disj_forms));
				add(_topdownrules, map2[qf->subformula()], &Gen::conj( { &quant,map2[qf] }), _data);
			}
		}
	}

public:
	template<typename T>
	const std::vector<Rule*>& execute(T f, ApproxDefGeneratorData* data) {
		_data = data;
		_topdownrules.clear();
		f->accept(this);
		return _topdownrules;
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
		traverse(bf);
		if(_data->_derivations->hasDerivation(
				ApproximatingDefinition::TruthPropagation::FALSE,
				ApproximatingDefinition::Direction::DOWN)) {
			generateTopDownApproximation(bf,_data->_mappings->_formula2ct,_data->_mappings->_formula2cf);
		}
		if(_data->_derivations->hasDerivation(
				ApproximatingDefinition::TruthPropagation::TRUE,
				ApproximatingDefinition::Direction::DOWN)) {
			generateTopDownApproximation(bf,_data->_mappings->_formula2cf,_data->_mappings->_formula2ct);
		}
	}

	/**
	 * !x: P(x) <=> ?y: L(x, y)
	 *
	 * EXISTS:
	 * 		Lcf(x, y) <- Pcf(x)
	 * 		Lct(x, y) <- Pct(x) & !y': y~=y' => Lcf(x, y')  // This expensive rule filtered out (added by other visitor)
	 */
	void visit(const QuantForm* qf) {
		Assert(qf->sign()==SIGN::POS);
		traverse(qf);
		if(_data->_derivations->hasDerivation(
				ApproximatingDefinition::TruthPropagation::FALSE,
				ApproximatingDefinition::Direction::DOWN)) {
			generateTopDownApproximation(qf,_data->_mappings->_formula2ct,_data->_mappings->_formula2cf);
		}
		if(_data->_derivations->hasDerivation(
				ApproximatingDefinition::TruthPropagation::TRUE,
				ApproximatingDefinition::Direction::DOWN)) {
			generateTopDownApproximation(qf,_data->_mappings->_formula2cf,_data->_mappings->_formula2ct);
		}

	}

	void visit(const PredForm* pf) {
		handlePredForm(pf, _data, &_topdownrules);
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
	ApproxDefGeneratorData* _data;
	vector<Rule*> _bottomuprules;

	void generateBottomUpApproximation(const BoolForm* bf,
			std::map<const Formula*, PredForm*> map) {
		if (bf->conj()) {
			// This is a "cheap rule"
			if (_data->_rule_types.find(ApproximatingDefinition::RuleType::CHEAP)
					!= _data->_rule_types.end()) {
				for (auto i = bf->subformulas().cbegin(); i < bf->subformulas().cend(); ++i) {
					add(_bottomuprules, map[bf], map[*i], _data); // DISJ: Pct(x, y, z) <- Lict(x, y)
				}
			}
		}
		else {
			// This is a "cheap rule"
			if (_data->_rule_types.find(ApproximatingDefinition::RuleType::CHEAP)
					!= _data->_rule_types.end()) {
				std::vector<Formula*> forms;
				for (auto i = bf->subformulas().cbegin(); i < bf->subformulas().cend(); ++i) {
					forms.push_back(map[*i]);
				}
				add(_bottomuprules, map[bf], &Gen::conj(forms), _data); // DISj: Pcf(x, y, z) <- L1cf & ... & Lncf
			}
		}
	}

	void generateBottomUpApproximation(const QuantForm* qf,
			std::map<const Formula*, PredForm*> map) {
		if (qf->isUniv()) {
			// This is a "cheap rule"
			if (_data->_rule_types.find(ApproximatingDefinition::RuleType::CHEAP)
					!= _data->_rule_types.end()) {
				add(_bottomuprules, map[qf], &Gen::exists(qf->quantVars(), *map[qf->subformula()]), _data);
			}
		} else {
			// This is a "cheap rule"
			if (_data->_rule_types.find(ApproximatingDefinition::RuleType::FORALL)
					!= _data->_rule_types.end()) {
				add(_bottomuprules, map[qf], &Gen::forall(qf->quantVars(), *map[qf->subformula()]), _data);
			}
		}
	}
public:
	template<class T>
	const std::vector<Rule*>& execute(T* f, ApproxDefGeneratorData* data) {
		_data = data;
		_bottomuprules.clear();
		f->accept(this);
		return _bottomuprules;
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
		traverse(bf);
		if(_data->_derivations->hasDerivation(
				ApproximatingDefinition::TruthPropagation::FALSE,
				ApproximatingDefinition::Direction::UP)) {
			generateBottomUpApproximation(bf, _data->_mappings->_formula2cf);

		}
		if(_data->_derivations->hasDerivation(
				ApproximatingDefinition::TruthPropagation::TRUE,
				ApproximatingDefinition::Direction::UP)) {
			generateBottomUpApproximation(bf, _data->_mappings->_formula2ct);
		}
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
		traverse(qf);
		if(_data->_derivations->hasDerivation(
				ApproximatingDefinition::TruthPropagation::FALSE,
				ApproximatingDefinition::Direction::UP)) {

			if (qf->isUniv()) {
				add(_bottomuprules, _data->_mappings->_formula2cf[qf], &Gen::exists(qf->quantVars(), *_data->_mappings->_formula2cf[qf->subformula()]), _data);
			} else {
				add(_bottomuprules, _data->_mappings->_formula2cf[qf], &Gen::forall(qf->quantVars(), *_data->_mappings->_formula2cf[qf->subformula()]), _data);
			}
		}
		if(_data->_derivations->hasDerivation(
				ApproximatingDefinition::TruthPropagation::TRUE,
				ApproximatingDefinition::Direction::UP)) {
		}
	}

	void visit(const PredForm* pf) {
		handlePredForm(pf, _data, &_bottomuprules);
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

ApproximatingDefinition* GenerateApproximatingDefinition::doGenerateApproximatingDefinition(
		const AbstractTheory* orig_theory,
		ApproximatingDefinition::DerivationTypes* derivations,
		std::set<ApproximatingDefinition::RuleType> rule_types,
		const Structure* structure,
		const set<PFSymbol*>& freesymbols) {
	if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
		clog << "Generating the approximating definition...\n";
	}

	if (not isa<Theory>(*orig_theory)) {
		throw notyetimplemented("Generate approximating definition for something other than a normal theory");
	}
	auto normal_orig_theory = dynamic_cast<const Theory*>(orig_theory);
	if(normal_orig_theory->sentences().empty()) {
		Warning::warning("Trying to generate an approximating definition of a theory with no sentences");
//		return new ApproximatingDefinition(derivations, normal_orig_theory);
	}
	if (getOption(IntType::VERBOSE_APPROXDEF) >= 2) {
		clog << "Transforming the following theory into a format suitable for approxdef generation:\n" <<
				toString(normal_orig_theory) << "\n";
	}


	const vector<Formula*>& transformedSentences = performTransformations(normal_orig_theory->sentences(),structure);
	auto transformed_theory = normal_orig_theory->clone();
	transformed_theory->sentences(transformedSentences);
	if (getOption(IntType::VERBOSE_APPROXDEF) >= 2) {
		clog << "Resulted in the following theory:\n" <<
				toString(transformed_theory) << "\n";
	}
	ApproximatingDefinition* ret = new ApproximatingDefinition(derivations, rule_types, transformed_theory);
	auto generator = new GenerateApproximatingDefinition(transformedSentences, freesymbols, derivations, rule_types);
	auto approx_def = generator->getDefinition();
	auto approx_voc = generator->constructVocabulary(normal_orig_theory->vocabulary(), approx_def);
	if (getOption(IntType::VERBOSE_APPROXDEF) >= 1) {
		clog << "Generated the following approximating definition:\n" << toString(approx_def) << "\n";
	}
	ret->setApproximatingDefinition(approx_def);
	ret->setVocabulary(approx_voc);
	ret->setMappings(generator->_approxdefgeneratordata->_mappings);
	return ret;
}

GenerateApproximatingDefinition::GenerateApproximatingDefinition(
		const std::vector<Formula*>& sentences,
		const std::set<PFSymbol*>& actions,
		ApproximatingDefinition::DerivationTypes* derivations,
		std::set<ApproximatingDefinition::RuleType> rule_types)
			: _approxdefgeneratordata(
					new ApproxDefGeneratorData(actions, derivations,rule_types)),
					_sentences(sentences) {
	for(auto sentence : sentences) {
		setFormula2PredFormMap(sentence);
	}
}

Definition* GenerateApproximatingDefinition::getDefinition() {
	auto d = getBasicDefinition();

	if (_approxdefgeneratordata->_derivations->hasDerivation(ApproximatingDefinition::Direction::DOWN)) {
		d->add(getallDownRules());
		_approxdefgeneratordata->_baseformulas_already_added=true;
	}
	if (_approxdefgeneratordata->_derivations->hasDerivation(ApproximatingDefinition::Direction::UP)) {
		d->add(getallUpRules());
		_approxdefgeneratordata->_baseformulas_already_added=true;
	}
	return d;
}

// Get the definition that is the basis for all approximating definitions.
// The definition for the top-level formulas saying they are true.
Definition* GenerateApproximatingDefinition::getBasicDefinition() {
	auto d = new Definition();


	for (auto i = _sentences.cbegin(); i < _sentences.cend(); ++i) {
		auto true_formula = new BoolForm(SIGN::POS, true, { }, FormulaParseInfo());
		auto sentence_ct = _approxdefgeneratordata->_mappings->_formula2ct[*i];
		if(not sentence_ct->symbol()->builtin()) {
			d->add(new Rule(sentence_ct->freeVars(), sentence_ct, true_formula, ParseInfo()));
		}
	}
	return d;
}

std::vector<Rule*> GenerateApproximatingDefinition::getallDownRules() {
	std::vector<Rule*> result;
	for (auto i = _sentences.cbegin(); i < _sentences.cend(); ++i) {
		auto rules = TopDownApproximatingDefinition().execute(*i, _approxdefgeneratordata);
		insertAtEnd(result, rules);
	}
	return result;
}

std::vector<Rule*> GenerateApproximatingDefinition::getallUpRules() {
	std::vector<Rule*> result;
	for (auto i = _sentences.cbegin(); i < _sentences.cend(); ++i) {
		auto rules = BottomUpApproximatingDefinition().execute(*i, _approxdefgeneratordata);
		insertAtEnd(result, rules);
	}
	return result;
}

void GenerateApproximatingDefinition::setFormula2PredFormMap(Formula* f) {
	auto ctcfpair = std::pair<PredForm*,PredForm*>();
	auto swapIfNegated = true;
	if(isa<PredForm>(*f)) {
		auto fPredForm = dynamic_cast<PredForm*>(f);
		if (_approxdefgeneratordata->_mappings->_pred2predCt.find(fPredForm->symbol()) == _approxdefgeneratordata->_mappings->_pred2predCt.cend()) {
			if (fPredForm->symbol()->builtin()) {
				ctcfpair.first = fPredForm;
				auto clone = fPredForm->clone();
				clone->negate();
				ctcfpair.second = clone;
				swapIfNegated = false;
			}
//			else if (s->inter(fPredForm->symbol())->approxTwoValued()) {
//				ctcfpair.first = fPredForm;
//				auto clone = fPredForm->clone();
//				clone->negate();
//				ctcfpair.second = clone;
//				swapIfNegated = false;
//				data->actions.insert(fPredForm->symbol());
//			}
			else {
				Predicate* ctpred = new Predicate((fPredForm->symbol()->nameNoArity() + "_ct"),fPredForm->symbol()->sorts());
				Predicate* cfpred = new Predicate((fPredForm->symbol()->nameNoArity() + "_cf"),fPredForm->symbol()->sorts());
				ctcfpair.first = new PredForm(SIGN::POS, ctpred, fPredForm->subterms(), FormulaParseInfo());
				ctcfpair.second = new PredForm(SIGN::POS, cfpred, fPredForm->subterms(), FormulaParseInfo());

				_approxdefgeneratordata->_mappings->_pred2predCt.insert( std::pair<PFSymbol*,PFSymbol*>(fPredForm->symbol(),ctcfpair.first->symbol()) );
				_approxdefgeneratordata->_mappings->_pred2predCf.insert( std::pair<PFSymbol*,PFSymbol*>(fPredForm->symbol(),ctcfpair.second->symbol()) );
			}
		} else {
			ctcfpair.first = new PredForm(SIGN::POS, _approxdefgeneratordata->_mappings->_pred2predCt[fPredForm->symbol()], fPredForm->subterms(), FormulaParseInfo());
			ctcfpair.second = new PredForm(SIGN::POS, _approxdefgeneratordata->_mappings->_pred2predCf[fPredForm->symbol()], fPredForm->subterms(), FormulaParseInfo());
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
	_approxdefgeneratordata->_mappings->_formula2ct.insert( std::pair<Formula*,PredForm*>(f,ctcfpair.first) );
	_approxdefgeneratordata->_mappings->_formula2cf.insert( std::pair<Formula*,PredForm*>(f,ctcfpair.second) );

	for (auto subf : f->subformulas()) {
		setFormula2PredFormMap(subf);
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

const std::vector<Formula*> GenerateApproximatingDefinition::performTransformations(
		const std::vector<Formula*>& sentences, const Structure* structure) {
	std::vector<Formula*> ret;
	for(auto sentence : sentences) {
		auto copyToWorkOn = sentence->clone();
		auto context = Context::POSITIVE;
		if (copyToWorkOn->sign() == SIGN::NEG) {
			context = Context::NEGATIVE;
		}
		const set<PFSymbol*>* emptySymbolsSet = new set<PFSymbol*>();
		auto sentence2 = FormulaUtils::unnestFuncsAndAggs(copyToWorkOn,structure);
		auto sentence3 = FormulaUtils::graphFuncsAndAggs(sentence2,structure,(*emptySymbolsSet),true,false,context);
		auto sentence4 = FormulaUtils::removeEquivalences(sentence3);
		auto sentence5 = FormulaUtils::pushNegations(sentence4);
		ret.push_back(sentence5);
	}
	return ret;
}

Vocabulary* GenerateApproximatingDefinition::constructVocabulary(Vocabulary* orig_voc, Definition* d) {
	auto ret = new Vocabulary(orig_voc->name());

	for(auto ctf : _approxdefgeneratordata->_mappings->_formula2ct) {
		ret->add(ctf.second->symbol());
	}
	for(auto ctf : _approxdefgeneratordata->_mappings->_predCt2InputPredCt) {
		ret->add(ctf.second);
	}

	for(auto cff : _approxdefgeneratordata->_mappings->_formula2cf) {
		ret->add(cff.second->symbol());
	}
	for(auto cff : _approxdefgeneratordata->_mappings->_predCf2InputPredCf) {
		ret->add(cff.second);
	}

	for (auto sort : orig_voc->getSorts()) {
		ret->add(sort.second);
	}
	for (auto opensymbol: DefinitionUtils::opens(d)) {
		ret->add(opensymbol);
	}

	return ret;
}
