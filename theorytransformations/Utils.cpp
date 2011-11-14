#include "theorytransformations/Utils.hpp"

#include "theory.hpp"
#include "vocabulary.hpp"
#include "term.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "theoryinformation/CollectOpensOfDefinitions.hpp"
#include "theoryinformation/CheckContainment.hpp"
#include "theoryinformation/CheckSorts.hpp"
#include "theoryinformation/CountNbOfSubFormulas.hpp"
#include "theorytransformations/PushNegations.hpp"
#include "theorytransformations/Flatten.hpp"
#include "theorytransformations/CheckFuncTerms.hpp"
#include "theorytransformations/DeriveSorts.hpp"
#include "theorytransformations/AddCompletion.hpp"
#include "theorytransformations/GraphFunctions.hpp"
#include "theorytransformations/GraphAggregates.hpp"
#include "theorytransformations/RemoveEquivalences.hpp"
#include "theorytransformations/PushQuantifications.hpp"
#include "theorytransformations/SplitComparisonChains.hpp"
#include "theorytransformations/SubstituteTerm.hpp"
#include "theorytransformations/UnnestPartialTerms.hpp"
#include "theorytransformations/UnnestTerms.hpp"
#include "theorytransformations/UnnestThreeValuedTerms.hpp"
#include "theoryinformation/CheckContainsFuncTerms.hpp"

using namespace std;

namespace TermUtils {
SetExpr* moveThreeValuedTerms(SetExpr* f, AbstractStructure* structure, Context context, bool cpsupport, const std::set<const PFSymbol*> cpsymbols) {
	return transform<UnnestThreeValuedTerms>(f, structure, context, cpsupport, cpsymbols);
}
}

namespace DefinitionUtils {
std::set<PFSymbol*> opens(Definition* d) {
	CollectOpensOfDefinitions collector;
	return collector.run(d);
}
}

namespace FormulaUtils {
Formula* splitComparisonChains(Formula* f, Vocabulary* v) {
	return transform<SplitComparisonChains>(f, v);
}

Formula* unnestTerms(Formula* f, Context poscontext) {
	return transform<UnnestTerms>(f, poscontext);
}

Formula* removeEquivalences(Formula* f) {
	return transform<RemoveEquivalences>(f);
}

Formula* flatten(Formula* f) {
	return transform<RemoveEquivalences>(f);
}

Formula* graphFunctions(Formula* f) {
	return transform<GraphFunctions>(f);
}

Formula* graphAggregates(Formula* f) {
	return transform<GraphAggregates>(f);
}

Formula* unnestPartialTerms(Formula* f, Context context, Vocabulary* voc) {
	return transform<UnnestPartialTerms>(f, context, voc);
}

Formula* unnestThreeValuedTerms(Formula* f, AbstractStructure* structure, Context context, bool cpsupport, const std::set<const PFSymbol*> cpsymbols) {
	return transform<UnnestThreeValuedTerms>(f, structure, context, cpsupport, cpsymbols);
}

bool containsFuncTerms(Formula* f) {
	CheckContainsFuncTerms checker;
	return checker.containsFuncTerms(f);
}

AbstractTheory* pushNegations(AbstractTheory* f) {
	return transform<PushNegations>(f);
}

AbstractTheory* graphFunctions(AbstractTheory* f) {
	return transform<GraphFunctions>(f);
}

AbstractTheory* unnestTerms(AbstractTheory* f) {
	return transform<UnnestTerms>(f);
}

Formula* substituteTerm(Formula* f, Term* t, Variable* v) {
	return transform<SubstituteTerm>(f, t, v);
}

AbstractTheory* removeEquivalences(AbstractTheory* f){
	return transform<RemoveEquivalences>(f);
}

AbstractTheory* flatten(AbstractTheory* f){
	return transform<Flatten>(f);
}

AbstractTheory* splitComparisonChains(AbstractTheory* f){
	return transform<SplitComparisonChains>(f);
}

AbstractTheory* pushQuantifiers(AbstractTheory* f){
	return transform<PushQuantifications>(f);
}

AbstractTheory* graphAggregates(AbstractTheory* f){
	return transform<GraphAggregates>(f);
}

AbstractTheory* addCompletion(AbstractTheory* f){
	return transform<AddCompletion>(f);
}

int nrSubformulas(AbstractTheory* f){
	CountNbOfSubFormulas checker;
	f->accept(&checker);
	return checker.result();
}

AbstractTheory* merge(AbstractTheory* at1, AbstractTheory* at2) {
	if (typeid(*at1) != typeid(Theory) || typeid(*at2) != typeid(Theory)) {
		notyetimplemented("Only merging of normal theories has been implemented...");
	}
	//TODO merge vocabularies?
	if (at1->vocabulary() == at2->vocabulary()) {
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
	} else {
		return NULL;
	}
}

double estimatedCostAll(PredForm* query, const std::set<Variable*> freevars, bool inverse, AbstractStructure* structure) {
	FOBDDManager manager;
	FOBDDFactory factory(&manager);
	const FOBDD* bdd = factory.run(query);
	if (inverse) {
		bdd = manager.negation(bdd);
	}
	set<const FOBDDDeBruijnIndex*> indices;
//cerr << "Estimating the cost of bdd\n";
//manager.put(cerr,bdd);
//cerr << "With variables ";
//for(auto it = freevars.cbegin(); it != freevars.cend(); ++it) cerr << *(*it) << ' ';
//cerr << endl;
	double res = manager.estimatedCostAll(bdd, manager.getVariables(freevars), indices, structure);
//cerr << "Estimated " << res << endl;
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
