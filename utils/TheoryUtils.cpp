#include "utils/TheoryUtils.hpp"

#include "theory.hpp"
#include "vocabulary.hpp"
#include "term.hpp"
#include "theoryinformation/ApproxCheckTwoValued.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "theoryinformation/CollectOpensOfDefinitions.hpp"
#include "theoryinformation/CheckContainment.hpp"
#include "theoryinformation/CheckContainsFuncTerms.hpp"
#include "theoryinformation/CheckContainsAggTerms.hpp"
#include "theoryinformation/CheckFuncTerms.hpp"
#include "theoryinformation/CheckPartialTerm.hpp"
#include "theoryinformation/CheckSorts.hpp"
#include "theoryinformation/CollectOpensOfDefinitions.hpp"
#include "theoryinformation/CountNbOfSubFormulas.hpp"
#include "theorytransformations/PushNegations.hpp"
#include "theorytransformations/Flatten.hpp"
#include "theorytransformations/DeriveSorts.hpp"
#include "theorytransformations/AddCompletion.hpp"
#include "theorytransformations/GraphFunctions.hpp"
#include "theorytransformations/GraphAggregates.hpp"
#include "theorytransformations/GraphFuncsAndAggs.hpp"
#include "theorytransformations/RemoveEquivalences.hpp"
#include "theorytransformations/PushQuantifications.hpp"
#include "theorytransformations/SplitComparisonChains.hpp"
#include "theorytransformations/SubstituteTerm.hpp"
#include "theorytransformations/UnnestPartialTerms.hpp"
#include "theorytransformations/UnnestTerms.hpp"
#include "theorytransformations/UnnestThreeValuedTerms.hpp"
#include "theorytransformations/SplitProducts.hpp"

using namespace std;

namespace TermUtils {
SetExpr* moveThreeValuedTerms(SetExpr* f, AbstractStructure* structure, Context context, bool cpsupport, const std::set<const PFSymbol*> cpsymbols) {
	transform<UnnestThreeValuedTerms>(f, structure, context, cpsupport, cpsymbols);
	return f;
}

bool isPartial(Term* term) {
	return transform<CheckPartialTerm, bool>(term);
}
}

namespace SetUtils {
bool approxTwoValued(SetExpr* exp, AbstractStructure* str) {
	return transform<ApproxCheckTwoValued, bool>(str, exp);
}
}

namespace DefinitionUtils {
std::set<PFSymbol*> opens(Definition* d) {
	return transform<CollectOpensOfDefinitions, std::set<PFSymbol*>>(d);
}
}

namespace FormulaUtils {
Formula* splitComparisonChains(Formula* f, Vocabulary* v) {
	return transform<SplitComparisonChains, Formula*>(f, v);
}

Formula* unnestTerms(Formula* f, Context poscontext) {
	return transform<UnnestTerms, Formula*>(f, poscontext);
}

Formula* removeEquivalences(Formula* f) {
	return transform<RemoveEquivalences, Formula*>(f);
}

Formula* flatten(Formula* f) {
	return transform<RemoveEquivalences, Formula*>(f);
}

void checkSorts(Vocabulary* v, Rule* f){
	transform<CheckSorts>(f, v);
}
void checkSorts(Vocabulary* v, Formula* f){
	transform<CheckSorts>(f, v);
}
void checkSorts(Vocabulary* v, Term* f){
	transform<CheckSorts>(f, v);
}

void deriveSorts(Vocabulary* v, Rule* f){
	transform<DeriveSorts>(f, v);
}
void deriveSorts(Vocabulary* v, Formula* f){
	transform<DeriveSorts>(f, v);
}
void deriveSorts(Vocabulary* v, Term* f){
	transform<DeriveSorts>(f, v);
}

Formula* graphFunctions(Formula* f) {
	return transform<GraphFunctions, Formula*>(f);
}

Formula* graphAggregates(Formula* f) {
	return transform<GraphAggregates, Formula*>(f);
}

Formula* graphFuncsAndAggs(Formula* f) {
	return transform<GraphFuncsAndAggs, Formula*>(f);
}

Formula* splitProducts(Formula* f){
	return transform<SplitProducts, Formula*>(f);
}


Formula* unnestPartialTerms(Formula* f, Context context, Vocabulary* voc) {
	return transform<UnnestPartialTerms, Formula*>(f, context, voc);
}

Formula* unnestThreeValuedTerms(Formula* f, AbstractStructure* structure, Context context, bool cpsupport,
		const std::set<const PFSymbol*> cpsymbols) {
	return transform<UnnestThreeValuedTerms, Formula*>(f, structure, context, cpsupport, cpsymbols);
}

Rule* unnestThreeValuedTerms(Rule* r, AbstractStructure* structure, Context context, bool cpsupport,
		const std::set<const PFSymbol*> cpsymbols) {
	return transform<UnnestThreeValuedTerms, Rule*>(r, structure, context, cpsupport, cpsymbols);
}

bool containsFuncTerms(Formula* f) {
	return transform<CheckContainsFuncTerms, bool>(f);
}

bool containsAggTerms(Formula* f) {
	return transform<CheckContainsFuncTerms, bool>(f);
}

bool containsSymbol(const PFSymbol* s, const Formula* f) {
	return transform<CheckContainment, bool>(s, f);
}

AbstractTheory* pushNegations(AbstractTheory* f) {
	return transform<PushNegations, AbstractTheory*>(f);
}

AbstractTheory* graphFunctions(AbstractTheory* f) {
	return transform<GraphFunctions, AbstractTheory*>(f);
}

AbstractTheory* unnestTerms(AbstractTheory* f) {
	return transform<UnnestTerms, AbstractTheory*>(f);
}

Formula* substituteTerm(Formula* f, Term* t, Variable* v) {
	return transform<SubstituteTerm, Formula*>(f, t, v);
}

AbstractTheory* removeEquivalences(AbstractTheory* f) {
	return transform<RemoveEquivalences, AbstractTheory*>(f);
}

AbstractTheory* flatten(AbstractTheory* f) {
	return transform<Flatten, AbstractTheory*>(f);
}

AbstractTheory* splitComparisonChains(AbstractTheory* f) {
	return transform<SplitComparisonChains, AbstractTheory*>(f);
}

AbstractTheory* pushQuantifiers(AbstractTheory* f) {
	return transform<PushQuantifications, AbstractTheory*>(f);
}

AbstractTheory* graphAggregates(AbstractTheory* f) {
	return transform<GraphAggregates, AbstractTheory*>(f);
}

AbstractTheory* addCompletion(AbstractTheory* f) {
	return transform<AddCompletion, AbstractTheory*>(f);
}

int nrSubformulas(AbstractTheory* f) {
	return transform<CountNbOfSubFormulas, int>(f);
}

AbstractTheory* merge(AbstractTheory* at1, AbstractTheory* at2) {
	if (typeid(*at1) != typeid(Theory) || typeid(*at2) != typeid(Theory)) {
		thrownotyetimplemented("Only merging of normal theories has been implemented...");
	}
	if (at1->vocabulary() != at2->vocabulary()) {
		thrownotyetimplemented("Only merging of theories over the same vocabularies has been implemented...");
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

double estimatedCostAll(PredForm* query, const std::set<Variable*> freevars, bool inverse, AbstractStructure* structure) {
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
