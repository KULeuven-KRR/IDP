/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "TheoryUtils.hpp"

#include "IncludeComponents.hpp"
#include "information/ApproxCheckTwoValued.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
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
#include "transformations/SplitIntoMonotoneAgg.hpp"
#include "information/FindUnknBoundLiteral.hpp"
#include "information/FindDoubleDelayLiteral.hpp"

using namespace std;

/* TermUtils */
namespace TermUtils {
void checkSorts(Vocabulary* voc, Term* term) {
	transform<CheckSorts>(term, voc);
}

void deriveSorts(Vocabulary* voc, Term* term) {
	transform<DeriveSorts>(term, voc, false);
	transform<DeriveSorts>(term, voc, true);
}

ElementTuple deriveTermBounds(Term* term, const AbstractStructure* str) {
	return transform<DeriveTermBounds, ElementTuple>(term, str);
}

bool isPartial(Term* term) {
	return transform<CheckPartialTerm, bool>(term);
}
}

/* SetUtils */
namespace SetUtils {
bool approxTwoValued(const SetExpr* exp, AbstractStructure* str) {
	return transform<ApproxCheckTwoValued, bool>(exp, str);
}

SetExpr* unnestThreeValuedTerms(SetExpr* exp, AbstractStructure* structure, Context context) {
	return transform<UnnestThreeValuedTerms, SetExpr*>(exp, structure, context);
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

Rule* unnestThreeValuedTerms(Rule* rule, AbstractStructure* structure, Context context) {
	return transform<UnnestThreeValuedTerms, Rule*>(rule, structure, context);
}
Rule* unnestHeadTermsContainingVars(Rule* rule, AbstractStructure* structure, Context context) {
	return transform<UnnestHeadTermsContainingVars, Rule*>(rule, structure, context);
}
}

/* FormulaUtils */
namespace FormulaUtils {
bool approxTwoValued(const Formula* f, AbstractStructure* str) {
	return transform<ApproxCheckTwoValued,bool>(f,str);
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

const PredForm* findUnknownBoundLiteral(const Formula* f, const AbstractStructure* structure, const GroundTranslator* translator, Context& context){
	// NOTE: need complete specification to guarantee output parameter to be passed correctly!
	return transform<FindUnknownBoundLiteral, const PredForm*, const Formula, const AbstractStructure*, const GroundTranslator*, Context&>(f, structure, translator, context);
}
std::vector<const PredForm*> findDoubleDelayLiteral(const Formula* f, const AbstractStructure* structure, const GroundTranslator* translator, Context& context){
	// NOTE: need complete specification to guarantee output parameter to be passed correctly!
	return transform<FindDoubleDelayLiteral, std::vector<const PredForm*>, const Formula, const AbstractStructure*, const GroundTranslator*, Context&>(f, structure, translator, context);
}

void deriveSorts(Vocabulary* v, Formula* f) {
	transform<DeriveSorts>(f, v, false);
	transform<DeriveSorts>(f, v, true);
}

Formula* flatten(Formula* f) {
	return transform<Flatten, Formula*>(f);
}

Formula* graphFuncsAndAggs(Formula* f, AbstractStructure* str, Context con) {
	return transform<GraphFuncsAndAggs, Formula*>(f, str, con);
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

Formula* splitComparisonChains(Formula* f, Vocabulary* v) {
	return transform<SplitComparisonChains, Formula*>(f, v);
}

Formula* splitIntoMonotoneAgg(Formula* f) {
	return transform<SplitIntoMonotoneAgg, Formula*>(f);
}

Formula* substituteTerm(Formula* f, Term* t, Variable* v) {
	return transform<SubstituteTerm, Formula*>(f, t, v);
}

Formula* unnestFuncsAndAggs(Formula* f, AbstractStructure* str, Context con) {
	return transform<UnnestFuncsAndAggs, Formula*>(f, str, con);
}

Formula* unnestFuncsAndAggsNonRecursive(Formula* f, AbstractStructure* str, Context con) {
	return transform<UnnestFuncsAndAggsNonRecursive, Formula*>(f, str, con);
}

Formula* unnestDomainTerms(Formula* f, AbstractStructure* str,  Context con ) {
	return transform<UnnestDomainTerms, Formula*>(f, str, con);
}


Formula* unnestPartialTerms(Formula* f, Context con, AbstractStructure* str, Vocabulary* voc) {
	return transform<UnnestPartialTerms, Formula*>(f, con, str, voc);
}

Formula* unnestTerms(Formula* f, Context con, AbstractStructure* str, Vocabulary* voc) {
	return transform<UnnestTerms, Formula*>(f, con, str, voc);
}

Formula* unnestThreeValuedTerms(Formula* f, AbstractStructure* structure, Context context) {
	return transform<UnnestThreeValuedTerms, Formula*>(f, structure, context);
}

void addCompletion(AbstractTheory* t) {
	auto newt = transform<AddCompletion, AbstractTheory*>(t);
	Assert(newt==t);
}

void flatten(AbstractTheory* t) {
	auto newt = transform<Flatten, AbstractTheory*>(t);
	Assert(newt==t);
}

AbstractTheory* graphFuncsAndAggs(AbstractTheory* t, AbstractStructure* str, Context con) {
	return transform<GraphFuncsAndAggs, AbstractTheory*>(t, str, con);
}

void pushNegations(AbstractTheory* t) {
	auto newt = transform<PushNegations, AbstractTheory*>(t);
	Assert(newt==t);
}

AbstractTheory* calculateArithmetic(AbstractTheory* t) {
	return transform<CalculateKnownArithmetic, AbstractTheory*>(t);
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

AbstractTheory* unnestFuncsAndAggs(AbstractTheory* t, AbstractStructure* str, Context con) {
	return transform<UnnestFuncsAndAggs, AbstractTheory*>(t, str, con);
}

AbstractTheory* unnestFuncsAndAggsNonRecursive(AbstractTheory* t, AbstractStructure* str, Context con) {
	return transform<UnnestFuncsAndAggsNonRecursive, AbstractTheory*>(t, str, con);
}

AbstractTheory* unnestDomainTerms(AbstractTheory* t, AbstractStructure* str, Context con) {
	return transform<UnnestDomainTerms, AbstractTheory*>(t, str, con);
}

/*AbstractTheory* mergeRulesOnSameSymbol(AbstractTheory* t) {
	return transform<MergeRulesOnSameSymbol, AbstractTheory*>(t);
}*/

void unnestTerms(AbstractTheory* t, Context con, AbstractStructure* str, Vocabulary* voc) {
	auto newt = transform<UnnestTerms, AbstractTheory*>(t, con, str, voc);
	Assert(newt==t);
}

void unnestThreeValuedTerms(AbstractTheory* t, Context con, AbstractStructure* str) {
	auto newt = transform<UnnestThreeValuedTerms, AbstractTheory*>(t, str, con);
	Assert(newt==t);
}

int nrSubformulas(AbstractTheory* t) {
	return transform<CountNbOfSubFormulas, int>(t);
}

AbstractTheory* merge(AbstractTheory* at1, AbstractTheory* at2) {
	if (not sametypeid<Theory>(*at1) || not sametypeid<Theory>(*at2)) {
		throw notyetimplemented("Only merging of normal theories has been implemented...");
	}
	if (at1->vocabulary() != at2->vocabulary()) {
		throw notyetimplemented("Only merging of theories over the same vocabularies has been implemented...");
	}
	AbstractTheory* at = at1->clone();
	Theory* t2 = static_cast<Theory*>(at2);
	for (auto it = t2->sentences().cbegin(); it != t2->sentences().cend(); ++it) {
		at->add((*it)->clone());
	}
	for (auto it = t2->definitions().cbegin(); it != t2->definitions().cend(); ++it) {
		at->add((*it)->clone());
	}
	for (auto it = t2->fixpdefs().cbegin(); it != t2->fixpdefs().cend(); ++it) {
		at->add((*it)->clone());
	}
	return at;
}

double estimatedCostAll(Formula* query, const std::set<Variable*> freevars, bool inverse,const  AbstractStructure* structure) {
	FOBDDManager manager;
	FOBDDFactory factory(&manager);
	auto bdd = factory.turnIntoBdd(query);
	if (inverse) {
		bdd = manager.negation(bdd);
	}
	double res = manager.estimatedCostAll(bdd, manager.getVariables(freevars), { }, structure);
	return res;
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
		switch (af->right()->function()) {
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
		switch (af->right()->function()) {
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
		switch (af->right()->function()) {
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
		switch (af->right()->function()) {
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
