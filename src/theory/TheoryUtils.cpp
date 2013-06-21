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

#include "TheoryUtils.hpp"

#include "IncludeComponents.hpp"
#include "inferences/approximatingdefinition/GenerateApproximatingDefinition.hpp"
#include "information/ApproxCheckTwoValued.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/Estimations.hpp"
#include "information/CollectOpensOfDefinitions.hpp"
#include "information/CheckContainment.hpp"
#include "information/CheckContainsFuncTerms.hpp"
#include "information/CheckContainsDomainTerms.hpp"
#include "information/CheckContainsFuncTermsOutsideOfSets.hpp"
#include "information/CheckContainsAggTerms.hpp"
#include "information/CheckPartialTerm.hpp"
#include "information/CheckSorts.hpp"
#include "information/CollectOpensOfDefinitions.hpp"
#include "information/HasRecursionOverNegation.hpp"
#include "information/HasRecursiveAggregate.hpp"
#include "information/CountNbOfSubFormulas.hpp"
#include "information/DeriveTermBounds.hpp"
#include "transformations/PushNegations.hpp"
#include "transformations/Flatten.hpp"
#include "transformations/DeriveSorts.hpp"
#include "transformations/AddCompletion.hpp"
#include "transformations/AddIfCompletion.hpp"
#include "transformations/GraphFuncsAndAggs.hpp"
#include "transformations/RemoveEquivalences.hpp"
#include "transformations/PushQuantifications.hpp"
#include "transformations/EliminateUniversalQuantifications.hpp"
#include "transformations/SplitComparisonChains.hpp"
#include "transformations/SubstituteTerm.hpp"
#include "transformations/SplitDefinitions.hpp"
#include "transformations/SubstituteVarWithDom.hpp"
#include "transformations/UnnestFuncsAndAggs.hpp"
#include "transformations/UnnestFuncsAndAggsNonRecursive.hpp"
#include "transformations/UnnestPartialTerms.hpp"
#include "transformations/CombineAggregates.hpp"
#include "transformations/UnnestTerms.hpp"
#include "transformations/UnnestDomainTerms.hpp"
#include "transformations/UnnestThreeValuedTerms.hpp"
#include "transformations/UnnestVarContainingTerms.hpp"
#include "transformations/CalculateKnownArithmetic.hpp"
#include "transformations/IntroduceSharedTseitins.hpp"
#include "transformations/SplitIntoMonotoneAgg.hpp"
#include "information/CollectSymbols.hpp"
#include "transformations/ReplaceNestedWithTseitin.hpp"
#include "transformations/Skolemize.hpp"
#include "transformations/AddFuncConstraints.hpp"
#include "transformations/RemoveQuantificationsOverSort.hpp"
#include "information/FindDelayPredForms.hpp"
#include "information/ContainedVariables.hpp"

using namespace std;

/* TermUtils */
namespace TermUtils {

bool isAgg(Term* t) {
	return t->type() == TermType::AGG;
}
bool isFunc(Term* t) {
	return t->type() == TermType::FUNC;
}

bool isDom(Term* t) {
	return t->type() == TermType::DOM;
}

bool isVar(Term* t) {
	return t->type() == TermType::VAR;
}

bool isAggOrFunc(Term* t) {
	return isAgg(t) || isFunc(t);
}

bool isVarOrDom(Term* t) {
	return isVar(t) || isDom(t);
}

bool approxTwoValued(const Term* t, const Structure* str) {
	return transform<ApproxCheckTwoValued, bool>(t, str);
}

bool containsSymbol(const PFSymbol* s, const Term* f) {
	return transform<CheckContainment, bool>(s, f);
}

void checkSorts(Vocabulary* voc, Term* term) {
	transform<CheckSorts>(term, voc);
}

void deriveSorts(Vocabulary* voc, Term* term) {
	transform<DeriveSorts>(term, voc, false);
	transform<DeriveSorts>(term, voc, true);
}

ElementTuple deriveTermBounds(const Term* term, const Structure* str) {
	return transform<DeriveTermBounds, ElementTuple>(term, str);
}

bool isPartial(Term* term) {
	return transform<CheckPartialTerm, bool>(term);
}

}

/* SetUtils */
namespace SetUtils {
bool approxTwoValued(const SetExpr* exp, const Structure* str) {
	return transform<ApproxCheckTwoValued, bool>(exp, str);
}

SetExpr* unnestThreeValuedTerms(SetExpr* exp, Structure* structure, const std::set<PFSymbol*>& definedsymbols, bool cpsupport, TruthValue cpablerelation) {
	return transform<UnnestThreeValuedTerms, SetExpr*>(exp, structure, definedsymbols, cpsupport, cpablerelation);
}
}

/* DefinitionUtils */
namespace DefinitionUtils {
void checkSorts(Vocabulary* voc, Rule* rule) {
	transform<CheckSorts>(rule, voc);
}

void deriveSorts(Vocabulary* voc, Rule* rule) {
	transform<DeriveSorts>(rule, voc, false);
	transform<DeriveSorts>(rule, voc, true);
}

std::set<PFSymbol*> opens(Definition* d) {
	return transform<CollectOpensOfDefinitions, std::set<PFSymbol*>>(d);
}

bool hasRecursionOverNegation(Definition* d) {
	return transform<HasRecursionOverNegation, bool>(d);
}

void splitDefinitions(Theory* t) {
	transform<SplitDefinitions>(t);
}

bool hasRecursiveAggregate(Definition* d) {
	return transform<HasRecursiveAggregate, bool>(d);
}

Rule* unnestThreeValuedTerms(Rule* rule, const Structure* structure, const std::set<PFSymbol*>& definedsymbols, bool cpsupport) {
	return transform<UnnestThreeValuedTerms, Rule*>(rule, structure, definedsymbols, cpsupport);
}
Rule* unnestNonVarHeadTerms(Rule* rule, const Structure* structure){
	return transform<UnnestTerms, Rule*, Rule, const Structure*, Vocabulary*, bool>(rule, structure, NULL, true);
}
Rule* falseRule(PFSymbol* s) {
	auto terms = TermUtils::makeNewVarTerms(s->sorts());
	auto head = new PredForm(SIGN::POS, s, terms, FormulaParseInfo());
	auto rule = new Rule(head->freeVars(), head, FormulaUtils::falseFormula(), ParseInfo());
	return rule;
}

Rule* unnestHeadTermsNotVarsOrDomElems(Rule* rule, const Structure* structure) {
	return transform<UnnestHeadTermsNotVarsOrDomElems, Rule*>(rule, structure);
}
Rule* moveOnlyBodyQuantifiers(Rule* rule){
	ContainedVariables v;
	auto occursinhead = v.execute(rule->head());
	varset notinhead;
	for(auto var: rule->quantVars()){
		if(occursinhead.find(var)==occursinhead.cend()){
			notinhead.insert(var);
		}
	}
	if(notinhead.empty()){
		return rule;
	}
	return new Rule(occursinhead, rule->head(), new QuantForm(SIGN::POS, QUANT::EXIST, notinhead, rule->body(), rule->body()->pi()), rule->pi());
}

Definition* eliminateUniversalQuantifications(Definition* d) {
	ruleset new_rules = ruleset();
	for (auto rule : d->rules()) {
		rule->body(FormulaUtils::eliminateUniversalQuantifications(rule->body()));
	}
	return d;
}

}

/* FormulaUtils */
namespace FormulaUtils {

void addFuncConstraints(AbstractTheory* theory, Vocabulary* voc, std::map<Function*, Formula*>& funcconstraints, bool alsoCPableFunctions) {
	transform<AddFuncConstraints, AbstractTheory, Vocabulary*, std::map<Function*, Formula*>&, bool>(theory, voc, funcconstraints, alsoCPableFunctions);
}
void addFuncConstraints(TheoryComponent* tc, Vocabulary* voc, std::map<Function*, Formula*>& funcconstraints, bool alsoCPableFunctions) {
	transform<AddFuncConstraints, TheoryComponent, Vocabulary*, std::map<Function*, Formula*>&, bool>(tc, voc, funcconstraints, alsoCPableFunctions);
}
void addFuncConstraints(Term* term, Vocabulary* voc, std::map<Function*, Formula*>& funcconstraints, bool alsoCPableFunctions) {
	transform<AddFuncConstraints, Term, Vocabulary*, std::map<Function*, Formula*>&, bool>(term, voc, funcconstraints, alsoCPableFunctions);
}

bool approxTwoValued(const Formula* f, Structure* str) {
	return transform<ApproxCheckTwoValued, bool>(f, str);
}

void checkSorts(Vocabulary* v, Formula* f) {
	transform<CheckSorts>(f, v);
}

Formula* combineAggregates(Formula* f){
	return transform<CombineAggregates, Formula*>(f);
}
void combineAggregates(AbstractTheory* f){
	return transform<CombineAggregates>(f);
}

bool containsFuncTerms(Formula* f) {
	return transform<CheckContainsFuncTerms, bool>(f);
}

bool containsDomainTerms(Formula* f) {
	return transform<CheckContainsDomainTerms, bool>(f);
}

bool containsFuncTermsOutsideOfSets(Formula* f) {
	return transform<CheckContainsFuncTermsOutsideOfSets, bool>(f);
}

bool containsAggTerms(Formula* f) {
	return transform<CheckContainsAggTerms, bool>(f);
}

bool containsSymbol(const PFSymbol* s, const Formula* f) {
	return transform<CheckContainment, bool>(s, f);
}

std::shared_ptr<Delay> findDelay(const Formula* f, const var2dommap& varmap, const LazyGroundingManager* manager) {
	// NOTE: need complete specification to guarantee output parameter to be passed correctly!
	return transform<FindDelayPredForms, std::shared_ptr<Delay>, const Formula, const var2dommap&, const LazyGroundingManager*>(f, varmap, manager);
}

void deriveSorts(Vocabulary* v, Formula* f) {
	transform<DeriveSorts>(f, v, false);
	transform<DeriveSorts>(f, v, true);
}

Formula* flatten(Formula* f) {
	return transform<Flatten, Formula*>(f);
}

Formula* graphFuncsAndAggs(Formula* f, const Structure* str, const std::set<PFSymbol*>& definedsymbols, bool unnestall, bool cpsupport, Context con) {
	return transform<GraphFuncsAndAggs, Formula*>(f, str, definedsymbols, unnestall, cpsupport, con);
}

Formula* pushNegations(Formula* f) {
	return transform<PushNegations, Formula*>(f);
}

Formula* calculateArithmetic(Formula* f, const Structure* s) {
	return transform<CalculateKnownArithmetic, Formula*>(f,s);
}

Formula* removeEquivalences(Formula* f) {
	return transform<RemoveEquivalences, Formula*>(f);
}

Theory* replaceWithNestedTseitins(Theory* theory) {
	return transform<ReplaceNestedWithTseitinTerm, Theory*>(theory);
}

Formula* splitComparisonChains(Formula* f, Vocabulary* v) {
	return transform<SplitComparisonChains, Formula*>(f, v);
}

Formula* splitIntoMonotoneAgg(Formula* f) {
	return transform<SplitIntoMonotoneAgg, Formula*>(f);
}

Formula* skolemize(Formula* t, Vocabulary* v) {
	return transform<Skolemize, Formula*>(t, v);
}
AbstractTheory* skolemize(AbstractTheory* t) {
	return transform<Skolemize, AbstractTheory*>(t, t->vocabulary());
}
Theory* skolemize(Theory* t) {
	return transform<Skolemize, Theory*>(t, t->vocabulary());
}

Theory* sharedTseitinTransform(Theory* t, Structure* s) {
	return transform<IntroduceSharedTseitins, Theory*>(t, s);
}

Formula* substituteTerm(Formula* f, Term* t, Variable* v) {
	return transform<SubstituteTerm, Formula*>(f, t, v);
}

Formula* substituteVarWithDom(Formula* formula, const std::map<Variable*, const DomainElement*>& var2domelem){
	return transform<SubstituteVarWithDom, Formula*>(formula, var2domelem);
}

Formula* pushQuantifiers(Formula* t) {
	return transform<PushQuantifications, Formula*>(t);
}

Formula* unnestFuncsAndAggs(Formula* f, const Structure* str) {
	return transform<UnnestFuncsAndAggs, Formula*>(f, str);
}

Formula* unnestFuncsAndAggsNonRecursive(Formula* f, const Structure* str) {
	return transform<UnnestFuncsAndAggsNonRecursive, Formula*>(f, str);
}

Formula* unnestDomainTerms(Formula* f) {
	return transform<UnnestDomainTerms, Formula*>(f);
}

Formula* unnestDomainTermsFromNonBuiltins(Formula* f) {
	return transform<UnnestDomainTermsFromNonBuiltins, Formula*>(f);
}

Formula* unnestPartialTerms(Formula* f, const Structure* str, Vocabulary* voc) {
	return transform<UnnestPartialTerms, Formula*>(f, str, voc);
}

AbstractTheory* unnestPartialTerms(AbstractTheory* f, const Structure* str, Vocabulary* voc) {
	return transform<UnnestPartialTerms, AbstractTheory*>(f, str, voc);
}

Formula* unnestTerms(Formula* f, const Structure* str, Vocabulary* voc) {
	return transform<UnnestTerms, Formula*>(f, str, voc);
}

Formula* unnestThreeValuedTerms(Formula* f, const Structure* structure, const std::set<PFSymbol*>& definedsymbols, bool cpsupport) {
	return transform<UnnestThreeValuedTerms, Formula*>(f, structure, definedsymbols, cpsupport);
}

void addCompletion(AbstractTheory* t, const Structure* s) {
	auto newt = transform<AddCompletion, AbstractTheory*>(t, s);
	Assert(newt==t);
}

void addIfCompletion(AbstractTheory* t) {
	auto newt = transform<AddIfCompletion, AbstractTheory*>(t);
	Assert(newt==t);
}

void flatten(AbstractTheory* t) {
	auto newt = transform<Flatten, AbstractTheory*>(t);
	Assert(newt==t);
}

Theory* graphFuncsAndAggs(Theory* t, const Structure* str, const std::set<PFSymbol*>& definedsymbols, bool unnestall, bool cpsupport, Context con) {
	return transform<GraphFuncsAndAggs, Theory*>(t, str, definedsymbols, unnestall, cpsupport, con);
}
AbstractTheory* graphFuncsAndAggs(AbstractTheory* t, const Structure* str, const std::set<PFSymbol*>& definedsymbols, bool unnestall, bool cpsupport, Context con) {
	return transform<GraphFuncsAndAggs, AbstractTheory*>(t, str, definedsymbols, unnestall, cpsupport, con);
}

void pushNegations(AbstractTheory* t) {
	auto newt = transform<PushNegations, AbstractTheory*>(t);
	Assert(newt==t);
}

Theory* calculateArithmetic(Theory* t, const Structure* s) {
	return transform<CalculateKnownArithmetic, Theory*>(t,s);
}

Formula* pushQuantifiersAndNegations(Formula* t) {
	t = FormulaUtils::pushNegations(t);
	t = FormulaUtils::flatten(t);
	return transform<PushQuantifications, Formula*>(t);
}
AbstractTheory* pushQuantifiersAndNegations(AbstractTheory* t) {
	FormulaUtils::pushNegations(t);
	FormulaUtils::flatten(t);
	return transform<PushQuantifications, AbstractTheory*>(t);
}

Formula* eliminateUniversalQuantifications(Formula* f) {
	return transform<EliminateUniversalQuantifications, Formula*>(f);
}

Theory* eliminateUniversalQuantifications(Theory* t) {
	for(unsigned int it = 0; it < t->sentences().size(); it++) {
		t->sentence(it,transform<EliminateUniversalQuantifications, Formula*>(t->sentences().at(it)));
	}
	for(unsigned int it = 0; it < t->definitions().size(); it++) {
		t->definition(it,DefinitionUtils::eliminateUniversalQuantifications(t->definitions().at(it)));
	}
	return t;
}

AbstractTheory* removeEquivalences(AbstractTheory* t) {
	return transform<RemoveEquivalences, AbstractTheory*>(t);
}

AbstractTheory* splitComparisonChains(AbstractTheory* t, Vocabulary* voc) {
	return transform<SplitComparisonChains, AbstractTheory*>(t, voc);
}

AbstractTheory* unnestFuncsAndAggs(AbstractTheory* t, const Structure* str) {
	return transform<UnnestFuncsAndAggs, AbstractTheory*>(t, str);
}

AbstractTheory* unnestFuncsAndAggsNonRecursive(AbstractTheory* t, const Structure* str) {
	return transform<UnnestFuncsAndAggsNonRecursive, AbstractTheory*>(t, str);
}

AbstractTheory* unnestDomainTerms(AbstractTheory* t) {
	return transform<UnnestDomainTerms, AbstractTheory*>(t);
}

AbstractTheory* unnestDomainTermsFromNonBuiltins(AbstractTheory* t) {
	return transform<UnnestDomainTermsFromNonBuiltins, AbstractTheory*>(t);
}
/*AbstractTheory* mergeRulesOnSameSymbol(AbstractTheory* t) {
 return transform<MergeRulesOnSameSymbol, AbstractTheory*>(t);
 }*/

void unnestTerms(AbstractTheory* t, const Structure* str, Vocabulary* voc) {
	auto newt = transform<UnnestTerms, AbstractTheory*>(t, str, voc);
	Assert(newt==t);
}

int nrSubformulas(AbstractTheory* t) {
	return transform<CountNbOfSubFormulas, int>(t);
}

AbstractTheory* merge(AbstractTheory* at1, AbstractTheory* at2) {
	if (not isa<Theory>(*at1) or not isa<Theory>(*at2)) {
		throw notyetimplemented("Only merging of normal theories has been implemented...");
	}
	auto voc = at1->vocabulary();
	if (at1->vocabulary() != at2->vocabulary()) {
		if (VocabularyUtils::isSubVocabulary(at1->vocabulary(), at2->vocabulary())) {
			// We can safely add components from the first theory into the second theory.
			std::swap(at1, at2);
			voc = at1->vocabulary();
		} else if (VocabularyUtils::isSubVocabulary(at2->vocabulary(), at1->vocabulary())) {
			// We can safely add components from the second theory into the first theory.
			voc = at1->vocabulary();
		} else {
			// Merge the two vocabularies.
			voc = new Vocabulary(at1->vocabulary()->name() + at2->vocabulary()->name());
			voc->add(at1->vocabulary());
			voc->add(at2->vocabulary());
		}
	}
	auto at = at1->clone();
	at->name(at1->name() + at2->name());
	at->vocabulary(voc);
	auto t2 = static_cast<Theory*>(at2);
	for (auto sentence : t2->sentences()) {
		at->add(sentence->clone());
	}
	for (auto definition : t2->definitions()) {
		at->add(definition->clone());
	}
	for (auto fixpdef : t2->fixpdefs()) {
		at->add(fixpdef->clone());
	}
	return at;
}

double estimatedCostAll(Formula* query, const varset& freevars /*Shouldn't this be outvars?*/, bool inverse, const Structure* structure) {
	auto manager = FOBDDManager::createManager();
	FOBDDFactory factory(manager);
	auto bdd = factory.turnIntoBdd(query);
	if (inverse) {
		bdd = manager->negation(bdd);
	}
	return BddStatistics::estimateCostAll(bdd, manager->getVariables(freevars), { }, structure, manager);
}

BoolForm* trueFormula() {
	return new BoolForm(SIGN::POS, true, vector<Formula*>(0), FormulaParseInfo());
}

BoolForm* falseFormula() {
	return new BoolForm(SIGN::POS, false, vector<Formula*>(0), FormulaParseInfo());
}

bool isMonotone(const AggForm* af) {
	switch (af->comp()) {
	case CompType::EQ:
	case CompType::NEQ:
		return false;
	case CompType::LT:
	case CompType::LEQ: {
		switch (af->getAggTerm()->function()) {
		case AggFunction::CARD:
		case AggFunction::MAX:
			return isPos(af->sign());
		case AggFunction::MIN:
			return isNeg(af->sign());
		case AggFunction::SUM:
			return isPos(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
		case AggFunction::PROD:
			return isPos(af->sign()); //FIXME: Asserts that weights are larger than one! Not correct otherwise.
		}
		break;
	}
	case CompType::GT:
	case CompType::GEQ: {
		switch (af->getAggTerm()->function()) {
		case AggFunction::CARD:
		case AggFunction::MAX:
			return isNeg(af->sign());
		case AggFunction::MIN:
			return isPos(af->sign());
		case AggFunction::SUM:
			return isNeg(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
		case AggFunction::PROD:
			return isNeg(af->sign()); //FIXME: Asserts that weights are larger than one! Not correct otherwise.
		}
		break;
	}
	}
	return false;
}

bool isAntimonotone(const AggForm* af) {
	switch (af->comp()) {
	case CompType::EQ:
	case CompType::NEQ:
		return false;
	case CompType::LT:
	case CompType::LEQ: {
		switch (af->getAggTerm()->function()) {
		case AggFunction::CARD:
		case AggFunction::MAX:
			return isNeg(af->sign());
		case AggFunction::MIN:
			return isPos(af->sign());
		case AggFunction::SUM:
			return isNeg(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
		case AggFunction::PROD:
			return isNeg(af->sign()); //FIXME: Asserts that weights are larger than one! Not correct otherwise.
		}
		break;
	}
	case CompType::GT:
	case CompType::GEQ: {
		switch (af->getAggTerm()->function()) {
		case AggFunction::CARD:
		case AggFunction::MAX:
			return isPos(af->sign());
		case AggFunction::MIN:
			return isNeg(af->sign());
		case AggFunction::SUM:
			return isPos(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
		case AggFunction::PROD:
			return isPos(af->sign()); //FIXME: Asserts that weights are larger than one! Not correct otherwise.
		}
		break;
	}
	}
	return false;
}

/*
 * Note: following three methods are notcollectQuantifiedVariables implemented using the transformer because
 * QuantType cannot be printed.
 */
std::map<Variable*, QuantType> collectQuantifiedVariables(Formula* f, bool recursive) {
	CollectQuantifiedVariables t;
	return t.execute(f, recursive);
}
std::map<Variable*, QuantType> collectQuantifiedVariables(Rule* f, bool recursive) {
	CollectQuantifiedVariables t;
	return t.execute(f, recursive);
}
std::map<Variable*, QuantType> collectQuantifiedVariables(AbstractTheory* f, bool recursive) {
	CollectQuantifiedVariables t;
	return t.execute(f, recursive);
}

std::set<PFSymbol*> collectSymbols(Formula* f) {
	return transform<CollectSymbols, std::set<PFSymbol*> >(f);
}
std::set<PFSymbol*> collectSymbols(Rule* f) {
	return transform<CollectSymbols, std::set<PFSymbol*> >(f);

}
std::set<PFSymbol*> collectSymbols(AbstractTheory* f) {
	return transform<CollectSymbols, std::set<PFSymbol*> >(f);

}

Formula* removeQuantificationsOverSort(Formula* f, const Sort* s) {
	return transform<RemoveQuantificationsOverSort, Formula*>(f, s);
}
Rule* removeQuantificationsOverSort(Rule* f, const Sort* s) {
	return transform<RemoveQuantificationsOverSort, Rule*>(f, s);
}
AbstractTheory* removeQuantificationsOverSort(AbstractTheory* f, const Sort* s) {
	return transform<RemoveQuantificationsOverSort, AbstractTheory*>(f, s);
}

}
