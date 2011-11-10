#ifndef THEORYUTILS_HPP_
#define THEORYUTILS_HPP_

#include <set>
#include "commontypes.hpp"

class Definition;
class SetExpr;
class PFSymbol;
class PredForm;
class BoolForm;
class Variable;
class AbstractStructure;
class AbstractTheory;
class Formula;
class Vocabulary;
class Term;
class AggForm;

// TODO what does it mean to pass NULL as vocabulary?

template<typename Transformer, typename Construct>
Construct* transform(Construct* object) {
	Transformer t;
	return object->accept(&t);
}

template<typename Transformer, typename Construct, typename ... Values>
Construct* transform(Construct* object, Values ... parameters) {
	Transformer t(parameters...);
	return object->accept(&t);
}

namespace FormulaUtils {
/** Recursively rewrite all EqChainForms in the given formula to BoolForms **/
Formula* splitComparisonChains(Formula*, Vocabulary* v = NULL);

/** Estimate the cost of the given query
 *		Precondition:
 *			- query does not contain any FuncTerm or AggTerm subterms
 *			- the query has a twovalue result in the given structure
 */
double estimatedCostAll(PredForm* query, const std::set<Variable*> freevars, bool inverse, AbstractStructure* structure);

/** Recursively remove all nested terms **/
Formula* unnestTerms(Formula*, Context poscontext);

/** Rewrite all equivalences into impiclations **/
Formula* removeEquivalences(Formula*);

/** Move all nested terms out of all formulas **/
Formula* flatten(Formula*);

/** Recursively rewrite all function terms to their predicate form **/
Formula* graphFunctions(Formula* f);
Formula* graphAggregates(Formula* f);

/** Recursively move all partial terms outside atoms **/
Formula* unnestPartialTerms(Formula* f, Context context, Vocabulary* voc = NULL);

/** Non-recursively move terms that are three-valued in a given structure outside of the given atom **/
Formula* unnestThreeValuedTerms(Formula*, AbstractStructure*, Context context, bool cpsupport = false, const std::set<const PFSymbol*> cpsymbols =
		std::set<const PFSymbol*>());

/** Returns true iff at least one FuncTerm occurs in the given formula **/
bool containsFuncTerms(Formula* f);

/** Replace the given term by the given variable in the given formula **/
Formula* substituteTerm(Formula*, Term*, Variable*);

/** Returns true iff the aggregate formula is monotone **/
bool isMonotone(const AggForm* af);

/** Returns true iff the aggregate formula is anti-monotone **/
bool isAntimonotone(const AggForm* af);

/** Create the formula 'true' **/
BoolForm* trueFormula();

/** Create the formula 'false' **/
BoolForm* falseFormula();

/** Push negations inside **/
AbstractTheory* pushNegations(AbstractTheory*);

/** Rewrite A <=> B to (A => B) & (B => A) **/
AbstractTheory* removeEquivalences(AbstractTheory*);

/** Rewrite (! x : ! y : phi) to (! x y : phi), rewrite ((A & B) & C) to (A & B & C), etc. **/
AbstractTheory* flatten(AbstractTheory*);

/** Rewrite chains of equalities to a conjunction or disjunction of atoms. **/
AbstractTheory* splitComparisonChains(AbstractTheory*);

/** Rewrite (! x : phi & chi) to ((! x : phi) & (!x : chi)), and similarly for ?. **/
AbstractTheory* pushQuantifiers(AbstractTheory*);

/** Rewrite the theory so that there are no nested terms **/
AbstractTheory* unnestTerms(AbstractTheory*);

/** Rewrite (F(x) = y) or (y = F(x)) to Graph_F(x,y) **/
AbstractTheory* graphFunctions(AbstractTheory*);

/** Rewrite (AggTerm op BoundTerm) to an aggregate formula (op = '=', '<', or '>') **/
AbstractTheory* graphAggregates(AbstractTheory* t);

/** Replace all definitions in the theory by their completion **/
AbstractTheory* addCompletion(AbstractTheory*);

/** Count the number of subformulas in the theory **/
int nrSubformulas(AbstractTheory*);

/** Merge two theories **/
AbstractTheory* merge(AbstractTheory*, AbstractTheory*);
}

namespace TermUtils {
/** Rewrite set expressions by moving three-valued terms **/
SetExpr* moveThreeValuedTerms(SetExpr*, AbstractStructure*, Context context, bool cpsupport = false, const std::set<const PFSymbol*> cpsymbols =
		std::set<const PFSymbol*>());
}

namespace DefinitionUtils {
/** Compute the open symbols of a definition **/
std::set<PFSymbol*> opens(Definition*);
}

#endif /* THEORYUTILS_HPP_ */
