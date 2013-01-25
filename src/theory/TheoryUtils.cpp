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
#include "transformations/SplitComparisonChains.hpp"
#include "transformations/SubstituteTerm.hpp"
#include "transformations/UnnestFuncsAndAggs.hpp"
#include "transformations/UnnestFuncsAndAggsNonRecursive.hpp"
#include "transformations/UnnestPartialTerms.hpp"
#include "transformations/UnnestTerms.hpp"
#include "transformations/UnnestDomainTerms.hpp"
#include "transformations/UnnestThreeValuedTerms.hpp"
#include "transformations/UnnestVarContainingTerms.hpp"
#include "transformations/CalculateKnownArithmetic.hpp"
#include "transformations/IntroduceSharedTseitins.hpp"
#include "transformations/SplitIntoMonotoneAgg.hpp"
#include "information/FindUnknBoundLiteral.hpp"
#include "information/FindDoubleDelayLiteral.hpp"

#include "transformations/ReplaceNestedWithTseitin.hpp"
#include "transformations/Skolemize.hpp"
#include "transformations/AddFuncConstraints.hpp"

using namespace std;

/* TermUtils */
namespace TermUtils {
bool approxTwoValued(const Term* t, const AbstractStructure* str) {
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

ElementTuple deriveTermBounds(const Term* term, const AbstractStructure* str) {
	return transform<DeriveTermBounds, ElementTuple>(term, str);
}

bool isPartial(Term* term) {
	return transform<CheckPartialTerm, bool>(term);
}

bool isFactor(const Term* term, const AbstractStructure* structure) {
	return approxTwoValued(term, structure);
}

bool isTermWithIntFactor(const FuncTerm* term, const AbstractStructure* structure) {
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
bool approxTwoValued(const SetExpr* exp, const AbstractStructure* str) {
	return transform<ApproxCheckTwoValued, bool>(exp, str);
}

SetExpr* unnestThreeValuedTerms(SetExpr* exp, AbstractStructure* structure, Context context, bool cpsupport, TruthValue cpablerelation) {
	return transform<UnnestThreeValuedTerms, SetExpr*>(exp, structure, context, cpsupport, cpablerelation);
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

Rule* unnestThreeValuedTerms(Rule* rule, const AbstractStructure* structure, Context context, bool cpsupport) {
	return transform<UnnestThreeValuedTerms, Rule*>(rule, structure, context, cpsupport);
}
Rule* unnestHeadTermsContainingVars(Rule* rule, const AbstractStructure* structure, Context context) {
	return transform<UnnestHeadTermsContainingVars, Rule*>(rule, structure, context);
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

bool approxTwoValued(const Formula* f, AbstractStructure* str) {
	return transform<ApproxCheckTwoValued, bool>(f, str);
}

void checkSorts(Vocabulary* v, Formula* f) {
	transform<CheckSorts>(f, v);
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

const PredForm* findUnknownBoundLiteral(const Formula* f, const AbstractStructure* structure, const GroundTranslator* translator, Context& context) {
	// NOTE: need complete specification to guarantee output parameter to be passed correctly!
	return transform<FindUnknownBoundLiteral, const PredForm*, const Formula, const AbstractStructure*, const GroundTranslator*, Context&>(f, structure,
			translator, context);
}
std::vector<const PredForm*> findDoubleDelayLiteral(const Formula* f, const AbstractStructure* structure, const GroundTranslator* translator,
		Context& context) {
	// NOTE: need complete specification to guarantee output parameter to be passed correctly!
	return transform<FindDoubleDelayLiteral, std::vector<const PredForm*>, const Formula, const AbstractStructure*, const GroundTranslator*, Context&>(f,
			structure, translator, context);
}

void deriveSorts(Vocabulary* v, Formula* f) {
	transform<DeriveSorts>(f, v, false);
	transform<DeriveSorts>(f, v, true);
}

Formula* flatten(Formula* f) {
	return transform<Flatten, Formula*>(f);
}

Formula* graphFuncsAndAggs(Formula* f, const AbstractStructure* str, bool unnestall, bool cpsupport, Context con) {
	return transform<GraphFuncsAndAggs, Formula*>(f, str, unnestall, cpsupport, con);
}

Formula* pushNegations(Formula* f) {
	return transform<PushNegations, Formula*>(f);
}

Formula* calculateArithmetic(Formula* f) {
	return transform<CalculateKnownArithmetic, Formula*>(f);
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

Theory* sharedTseitinTransform(Theory* t, AbstractStructure* s) {
	return transform<IntroduceSharedTseitins, Theory*>(t, s);
}

Formula* substituteTerm(Formula* f, Term* t, Variable* v) {
	return transform<SubstituteTerm, Formula*>(f, t, v);
}

Formula* unnestFuncsAndAggs(Formula* f, const AbstractStructure* str, Context con) {
	return transform<UnnestFuncsAndAggs, Formula*>(f, str, con);
}

Formula* unnestFuncsAndAggsNonRecursive(Formula* f, const AbstractStructure* str, Context con) {
	return transform<UnnestFuncsAndAggsNonRecursive, Formula*>(f, str, con);
}

Formula* unnestDomainTerms(Formula* f) {
	return transform<UnnestDomainTerms, Formula*>(f);
}

Formula* unnestDomainTermsFromNonBuiltins(Formula* f) {
	return transform<UnnestDomainTermsFromNonBuiltins, Formula*>(f);
}


Formula* unnestPartialTerms(Formula* f, Context con, const AbstractStructure* str, Vocabulary* voc) {
	return transform<UnnestPartialTerms, Formula*>(f, con, str, voc);
}

AbstractTheory* unnestPartialTerms(AbstractTheory* f, Context con, const AbstractStructure* str, Vocabulary* voc) {
	return transform<UnnestPartialTerms, AbstractTheory*>(f, con, str, voc);
}

Formula* unnestTerms(Formula* f, Context con, const AbstractStructure* str, Vocabulary* voc) {
	return transform<UnnestTerms, Formula*>(f, con, str, voc);
}

Formula* unnestThreeValuedTerms(Formula* f, const AbstractStructure* structure, Context context, bool cpsupport) {
	return transform<UnnestThreeValuedTerms, Formula*>(f, structure, context, cpsupport);
}

void addCompletion(AbstractTheory* t) {
	auto newt = transform<AddCompletion, AbstractTheory*>(t);
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

Theory* graphFuncsAndAggs(Theory* t, const AbstractStructure* str, bool unnestall, bool cpsupport, Context con) {
	return transform<GraphFuncsAndAggs, Theory*>(t, str, unnestall, cpsupport, con);
}
AbstractTheory* graphFuncsAndAggs(AbstractTheory* t, const AbstractStructure* str, bool unnestall, bool cpsupport, Context con) {
	return transform<GraphFuncsAndAggs, AbstractTheory*>(t, str, unnestall, cpsupport, con);
}

void pushNegations(AbstractTheory* t) {
	auto newt = transform<PushNegations, AbstractTheory*>(t);
	Assert(newt==t);
}

AbstractTheory* calculateArithmetic(AbstractTheory* t) {
	return transform<CalculateKnownArithmetic, AbstractTheory*>(t);
}

Formula* pushQuantifiers(Formula* t) {
	return transform<PushQuantifications, Formula*>(t);
}
AbstractTheory* pushQuantifiers(AbstractTheory* t) {
	return transform<PushQuantifications, AbstractTheory*>(t);
}

AbstractTheory* removeEquivalences(AbstractTheory* t) {
	return transform<RemoveEquivalences, AbstractTheory*>(t);
}

AbstractTheory* splitComparisonChains(AbstractTheory* t, Vocabulary* voc) {
	return transform<SplitComparisonChains, AbstractTheory*>(t, voc);
}

AbstractTheory* unnestFuncsAndAggs(AbstractTheory* t, const AbstractStructure* str, Context con) {
	return transform<UnnestFuncsAndAggs, AbstractTheory*>(t, str, con);
}

AbstractTheory* unnestFuncsAndAggsNonRecursive(AbstractTheory* t, const AbstractStructure* str, Context con) {
	return transform<UnnestFuncsAndAggsNonRecursive, AbstractTheory*>(t, str, con);
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

void unnestTerms(AbstractTheory* t, Context con, const AbstractStructure* str, Vocabulary* voc) {
	auto newt = transform<UnnestTerms, AbstractTheory*>(t, con, str, voc);
	Assert(newt==t);
}

void unnestThreeValuedTerms(AbstractTheory* t, bool cpsupport, const AbstractStructure* str, Context con) {
	auto newt = transform<UnnestThreeValuedTerms, AbstractTheory*>(t, str, con, cpsupport);
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
		if (VocabularyUtils::isSubVocabulary(at1->vocabulary(),at2->vocabulary())) {
			// We can safely add components from the first theory into the second theory.
			std::swap(at1,at2);
			voc = at1->vocabulary();
		} else if (VocabularyUtils::isSubVocabulary(at2->vocabulary(),at1->vocabulary())) {
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

double estimatedCostAll(Formula* query, const varset& freevars /*Shouldn't this be outvars?*/, bool inverse, const AbstractStructure* structure) {
	FOBDDManager manager;
	FOBDDFactory factory(&manager);
	auto bdd = factory.turnIntoBdd(query);
	if (inverse) {
		bdd = manager.negation(bdd);
	}
	return BddStatistics::estimateCostAll(bdd, manager.getVariables(freevars), { }, structure, &manager);
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
}
