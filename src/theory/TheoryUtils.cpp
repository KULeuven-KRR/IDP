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
#include "creation/cppinterface.hpp"
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
#include "information/CountNbOfSubFormulas.hpp"
#include "information/DeriveTermBounds.hpp"
#include "transformations/CardConstrToFO.hpp"
#include "transformations/PushNegations.hpp"
#include "transformations/Flatten.hpp"
#include "transformations/DeriveSorts.hpp"
#include "transformations/ReplaceDefinitionsWithCompletion.hpp"
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
#include "transformations/ReplacePredByPred.hpp"
#include "transformations/ReplaceNestedWithTseitin.hpp"
#include "transformations/Skolemize.hpp"
#include "transformations/AddFuncConstraints.hpp"
#include "transformations/RemoveQuantificationsOverSort.hpp"
#include "information/FindDelayPredForms.hpp"
#include "information/ContainedVariables.hpp"
#include "information/CheckContainsRecursivelyDefinedAggTerms.hpp"
#include "transformations/SubstituteVarWithVar.hpp"
#ifdef WITHXSB
#include "transformations/JoinDefinitionsForXSB.hpp"
#endif

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

bool contains(const Variable* v, const Term* f){
	ContainedVariables cv;
	auto vs = cv.execute(f); // TODO only in TERM positions!!!
	return ::contains(vs,const_cast<Variable*>(v));
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

bool isFactor(const Term* term, const Structure* structure) {
	return approxTwoValued(term, structure);
}

bool isNumber(const Term* term) {
	return isIntNumber(term) || isFloatNumber(term);
}

bool isIntNumber(const Term* term) {
	return (SortUtils::isSubsort(term->sort(),get(STDSORT::INTSORT))||
			SortUtils::isSubsort(term->sort(),get(STDSORT::NATSORT)));
}

bool isFloatNumber(const Term* term) {
	return SortUtils::isSubsort(term->sort(),get(STDSORT::FLOATSORT));
}

int getInt(const Term* term) {
	if (isa<DomainTerm>(*term)) {
		auto domelem = dynamic_cast<const DomainTerm*>(term)->value();
		return domelem->value()._int;
	} else if (isa<FuncTerm>(*term)) {
		throw notyetimplemented("Retrieving integers from functerms.");
	} else {
		throw IdpException("Retrieving integers from terms that are"
				" not domain terms or interpreted function terms");
	}
}

bool isTermWithIntFactor(const FuncTerm* term, const Structure* structure) {
	if (term->subterms().size() == 2 and FuncUtils::isIntProduct(term->function(), structure->vocabulary())) {
		for (auto it = term->subterms().cbegin(); it != term->subterms().cend(); ++it) {
			if (isFactor(*it, structure)) {
				return true;
			}
		}
	}
	return false;
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
std::set<PFSymbol*> approxTwoValuedOpens(Definition* d, Structure* s) {
	std::set<PFSymbol*> ret;
	for (auto symbol : DefinitionUtils::opens(d)) {
		if (s->inter(symbol)->approxTwoValued()) {
			ret.insert(symbol);
		}
	}
	return ret;
}
std::map<Definition*, std::set<PFSymbol*> > opens(std::vector<Definition*> defs) {
	std::map<Definition*, std::set<PFSymbol*> > opens;
	for (auto def : defs) {
		opens[def] = DefinitionUtils::opens(def);
	}
	return opens;
}
std::set<PFSymbol*> defined(Definition* d) {
	return d->defsymbols();
}

bool approxTotal(Definition* def) {
	auto total = true;
	total &= not DefinitionUtils::approxHasRecursionOverNegation(def);
	total &= not DefinitionUtils::approxContainsRecDefAggTerms(def);
	for (auto ds : DefinitionUtils::defined(def)) {
		total &= ds->isPredicate(); // TODO currently no way of knowing whether a function definition will be total!!! (e.g. f(x)=y <- true)
	}
	return total;
}

bool approxHasRecursionOverNegation(const Definition* d) {
	return transform<ApproxHasRecursionOverNegation, bool>(d);
}

std::set<PFSymbol*> approxRecurionsOverNegationSymbols(const Definition* d) {
	return transform<ApproxRecursionOverNegationSymbols, std::set<PFSymbol*> >(d);
}

bool hasRecursionOverNegation(const Definition* d) {
	return transform<HasRecursionOverNegation, bool>(d);
}

std::set<PFSymbol*> recurionsOverNegationSymbols(const Definition* d) {
	return transform<RecursionOverNegationSymbols, std::set<PFSymbol*> >(d);
}


void splitDefinitions(Theory* t) {
	transform<SplitDefinitions>(t);
}

#ifdef WITHXSB
/** Group definitions for minimum overhead with XSB */
void joinDefinitionsForXSB(Theory* t) {
	transform<JoinDefinitionsForXSB>(t);
}
#endif

bool approxContainsRecDefAggTerms(const Definition* def) {
	return transform<CheckApproxContainsRecDefAggTerms, bool>(def);
}

/** Add a "symbol <- false" body to open symbols with a 3-valued interpretation */
Definition* makeDefinitionCalculable(Definition* d, Structure* s) {
	auto ret = new Definition();
	for (auto rule : d->rules()) {
		ret->add(rule);
	}
	for(auto open : DefinitionUtils::opens(d)) {
		if (not s->inter(open)->approxTwoValued()) {
			std::vector<Term*> subterms = {};
			for (auto sort : open->sorts()) {
				auto new_var_term = new VarTerm(new Variable(sort),TermParseInfo());
				subterms.push_back(new_var_term);
			}
			auto rule_head = new PredForm(SIGN::POS, open, subterms, FormulaParseInfo());
			auto rule_body = FormulaUtils::falseFormula();
			ret->add(new Rule({},rule_head,rule_body, ParseInfo()));
		}
	}
	return ret;
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

    
 CompType getComparison(const PredForm* pf) {
	auto sign = pf->sign();
	auto symbol = pf->symbol();
	Assert(VocabularyUtils::isComparisonPredicate(symbol));
	if (is(symbol, STDPRED::EQ)) {
		return isPos(sign) ? CompType::EQ : CompType::NEQ;
	} else if (is(symbol, STDPRED::LT)) {
		return isPos(sign) ? CompType::LT : CompType::GEQ;
	} else {
		Assert(is(symbol, STDPRED::GT));
		return isPos(sign) ? CompType::GT : CompType::LEQ;
	}
}

 AbstractTheory* replaceCardinalitiesWithFOFormulas(AbstractTheory* t, int maxbound) {
	return transform<CardConstrToFO, AbstractTheory*>(t, maxbound);
}

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

// Change the interpretation of defined symbols such that all elements are unknown
void removeInterpretationOfDefinedSymbols(const Theory* t, Structure* s) {
	for (auto def : t->definitions()) {
		removeInterpretationOfDefinedSymbols(def, s);
	}
}

// Change the interpretation of defined symbols such that all elements are unknown
void removeInterpretationOfDefinedSymbols(const Definition* d, Structure* s) {
	for (auto defsymbol : d->defsymbols()) {
		auto emptytable1 = Gen::predtable(SortedElementTable(),s->universe(defsymbol));
		s->inter(defsymbol)->ct(emptytable1);
		auto emptytable2 = Gen::predtable(SortedElementTable(),s->universe(defsymbol));
		s->inter(defsymbol)->cf(emptytable2);
	}
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

Theory* replacePredByPred(Predicate* origPred, Predicate* newPred, Theory* theory){
	return transform<ReplacePredByPred, Theory*>(theory, origPred, newPred);
}

Formula* substituteVarWithVar(Formula* formula, const std::map<Variable*, Variable*>& var2var){
	return transform<SubstituteVarWithVar, Formula*>(formula, var2var);
}

Formula* pushQuantifiers(Formula* t) {
	return transform<PushQuantifications, Formula*>(t);
}

Formula* replacePredByPred(Predicate* origPred, Predicate* newPred, Formula* theory){
	return transform<ReplacePredByPred, Formula*>(theory, origPred, newPred);
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

void replaceDefinitionsWithCompletion(AbstractTheory* t, const Structure* s) {
	auto newt = transform<ReplaceDefinitionsWithCompletion, AbstractTheory*>(t, s);
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

template<class T>
T calculateArithmetic(T t, const Structure* s) {
	return transform<CalculateKnownArithmetic, T>(t,s);
}

template<class T>
T improveTheoryForInference(T theory, Structure* structure, bool skolemize, bool nbmodelsequivalent) {
	FormulaUtils::combineAggregates(theory);
	theory = FormulaUtils::calculateArithmetic(theory, structure);

	if (skolemize && nbmodelsequivalent) {
		// TODO: skolemization is not nb-model-equivalent out of the box (might help this in future by changing solver)
		Warning::warning("Skolemization does not preserve the number of models, so will not be applied as model-equivalence was also requested.");
	}
	if (skolemize && not nbmodelsequivalent) {
		theory = FormulaUtils::skolemize(theory);
	}
	if (getOption(BoolType::SPLIT_DEFS)) {
		if (isa<Theory>(*theory)) {
			DefinitionUtils::splitDefinitions(dynamic_cast<Theory*>(theory));
		}
	}
	return theory;
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

/**
 * Detect whether the EqChainForm is of the form:
 * VALUE (=)< VAR (=)< VALUE or VALUE >(=) VAR >(=) VALUE
 * using only types that are subtypes of INT (or NAT) */
bool isRange(const EqChainForm* ecf) {
	bool ret = false;
	if(ecf->subterms().size() == 3) {
		if (TermUtils::isIntNumber(ecf->subterms()[0]) &&
			TermUtils::isIntNumber(ecf->subterms()[2])) {
			if (ecf->subterms()[0]->freeVars().empty() &&
					ecf->subterms()[1]->freeVars().size() == 1 &&
					ecf->subterms()[2]->freeVars().empty()) {
				if ((ecf->comps()[0] == CompType::LEQ || ecf->comps()[0] == CompType::LT)
					&& (ecf->comps()[1] == CompType::LEQ || ecf->comps()[1] == CompType::LT)) {
					ret = true;
				}
				if ((ecf->comps()[0] == CompType::GEQ || ecf->comps()[0] == CompType::GT)
					&& (ecf->comps()[1] == CompType::GEQ || ecf->comps()[1] == CompType::GT)) {
					ret = true;
				}
			}
		}
	} else if (ecf->subterms().size() == 2 && ecf->comps()[0] == CompType::EQ) {
	} else if (ecf->subterms().size() == 2 && ecf->comps()[0] != CompType::NEQ) {
	}
	return ret;
}

/**
 * Retrieve the lower bound for EqChainForms that are ranges
 * It is very important to note that the implementation is very relient on
 * what isRange(EqChainForm*) detects to be ranges.
 * Every type of detected range has to be supported. */
int getLowerBound(const EqChainForm* ecf) {
	if (not isRange(ecf)) {
		stringstream ss;
		ss << "EqChainForm " << toString(ecf) << " not detected to be a range, but a lower bound was asked.\n";
		throw IdpException(ss.str());
	}
	// Sort of the variable is assumed to be int or nat (domelem of last value has a value of _int)
	if (ecf->subterms().size() == 3) {
		if (ecf->comps()[0] == CompType::LEQ ) {
			return TermUtils::getInt(ecf->subterms()[0]);
		} else if (ecf->comps()[0] == CompType::LT) {
			return TermUtils::getInt(ecf->subterms()[0])+1;
		}
		if (ecf->comps()[1] == CompType::GEQ) {
			return TermUtils::getInt(ecf->subterms()[2]);
		} else if (ecf->comps()[1] == CompType::GT) {
			return TermUtils::getInt(ecf->subterms()[2])+1;
		}
	}
	stringstream ss;
	ss << "Unsupported retrieval of lower bound for range EqChainForm " << toString(ecf) << "\n";
	throw IdpException(ss.str());
	return 0;
}

/**
 * Retrieve the upper bound for EqChainForms that are ranges
 * It is very important to note that the implementation is very relient on
 * what isRange(EqChainForm*) detects to be ranges.
 * Every type of detected range has to be supported. */
int getUpperBound(const EqChainForm* ecf) {
	if (not isRange(ecf)) {
		stringstream ss;
		ss << "EqChainForm " << toString(ecf) << " not detected to be a range, but an upper bound was asked.\n";
		throw IdpException(ss.str());
	}
	// Sort of the variable is assumed to be int or nat (domelem of last value has a value of _int)
	if (ecf->subterms().size() == 3) {
		if (ecf->comps()[0] == CompType::GEQ ) {
			return TermUtils::getInt(ecf->subterms()[0]);
		} else if (ecf->comps()[0] == CompType::GT) {
			return TermUtils::getInt(ecf->subterms()[0])-1;
		}
		if (ecf->comps()[1] == CompType::LEQ) {
			return TermUtils::getInt(ecf->subterms()[2]);
		} else if (ecf->comps()[1] == CompType::LT) {
			return TermUtils::getInt(ecf->subterms()[2])-1;
		}
	}
	stringstream ss;
	ss << "Unsupported retrieval of lower bound for range EqChainForm " << toString(ecf) << "\n";
	throw IdpException(ss.str());
	return 0;
}

/**
 * Retrieve the variable from EqChainForms that are ranges
 * It is very important to note that the implementation is very relient on
 * what isRange(EqChainForm*) detects to be ranges.
 * Every type of detected range has to be supported. */
Variable* getVariable(const EqChainForm* ecf) {
	if (not isRange(ecf)) {
		stringstream ss;
		ss << "EqChainForm " << toString(ecf) << " not detected to be a range, but the concerning variable was asked.\n";
		throw IdpException(ss.str());
	}
	// Only one type of range, one where the variable is the second subterm.
	if (not isa<VarTerm>(*ecf->subterms()[1])) {
		stringstream ss;
		ss << "Term " << toString(ecf->subterms()[1]) << " is not a VarTerm, but was expected.\n";
		throw IdpException(ss.str());
	}
	return dynamic_cast<const VarTerm*>(ecf->subterms()[1])->var();
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

template Theory* FormulaUtils::calculateArithmetic(Theory* t, const Structure* s);
template AbstractTheory* FormulaUtils::calculateArithmetic(AbstractTheory* t, const Structure* s);
template Theory* FormulaUtils::improveTheoryForInference(Theory* theory, Structure* structure, bool skolemize, bool nbmodelsequivalent);
template AbstractTheory* FormulaUtils::improveTheoryForInference(AbstractTheory* theory, Structure* structure, bool skolemize, bool nbmodelsequivalent);
