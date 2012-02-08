/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef IDP_THEORYUTILS_HPP_
#define IDP_THEORYUTILS_HPP_

#include "common.hpp"
#include "GlobalData.hpp"
#include "options.hpp"
#include <typeinfo>
#include <iostream>

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
class GroundTranslator;

// TODO what does it mean to pass NULL as vocabulary?

template<typename Transformer, typename ReturnType, typename Construct, typename ... Values>
ReturnType transform(Construct* object, Values ... parameters) {
	Transformer t;
	if (getOption(IntType::GROUNDVERBOSITY) > 1) {
		std::clog << "" <<nt() << "Executing " << typeid(Transformer).name() << " on: " << toString(object);
		pushtab();
	}
	ReturnType result = t.execute(object, parameters...);
	if (getOption(IntType::GROUNDVERBOSITY) > 1) {
		poptab();
		std::clog << "" <<nt() << "Resulted in: " << toString(result) <<nt();
	}
	return result;
}

template<typename Transformer, typename Construct, typename ... Values>
void transform(Construct* object, Values ... parameters) {
	Transformer t;
	if (getOption(IntType::GROUNDVERBOSITY) > 1) {
		std::clog <<nt() << "Executing " << typeid(Transformer).name() << " on: " << toString(object);
		pushtab();
	}
	t.execute(object, parameters...);
	if (getOption(IntType::GROUNDVERBOSITY) > 1) {
		poptab();
		std::clog <<nt() << "Resulted in: " << toString(object) <<nt();
	}
}

namespace FormulaUtils {
/** Returns true iff the aggregate formula is anti-monotone */
bool isAntimonotone(const AggForm* af);

/** Returns true iff the aggregate formula is monotone */
bool isMonotone(const AggForm* af);

/** Create the formula 'true' */
BoolForm* trueFormula();

/** Create the formula 'false' */
BoolForm* falseFormula();

/** Check sorts in the given formula */
void checkSorts(Vocabulary* v, Formula* f);

/** Returns true iff at least one FuncTerm/AggTerm occurs in the given formula */
bool containsFuncTerms(Formula* f);
bool containsAggTerms(Formula* f);
bool containsSymbol(const PFSymbol* s, const Formula* f);

/** If some predform can be found which can make the formula true by itself, one such symbol is returned, otherwise NULL **/
const PredForm* findUnknownBoundLiteral(const Formula* f, const AbstractStructure* structure, const GroundTranslator* translator);

/** Derive sorts in the given formula **/
void deriveSorts(Vocabulary* v, Formula* f);

/** Estimate the cost of the given query
 *		Precondition:
 *			- query does not contain any FuncTerm or AggTerm subterms
 *			- the query has a twovalue result in the given structure
 */
double estimatedCostAll(PredForm* query, const std::set<Variable*> freevars, bool inverse,const  AbstractStructure* structure);

/** Flatten all nested formulas */
Formula* flatten(Formula*);

/** Recursively rewrite all function terms to their predicate form, and aggregate terms to aggregate formulas */
Formula* graphFuncsAndAggs(Formula* f, AbstractStructure* str = NULL, Context con = Context::POSITIVE);

/** Push negations inside */
Formula* pushNegations(Formula* f);

/** Rewrite all equivalences into implications */
Formula* removeEquivalences(Formula*);

/** Recursively rewrite all EqChainForms in the given formula to BoolForms */
Formula* splitComparisonChains(Formula*, Vocabulary* voc = NULL);

Formula* splitIntoMonotoneAgg(Formula* f);

/** Replace the given term by the given variable in the given formula */
Formula* substituteTerm(Formula*, Term*, Variable*);

/** Recursively move all function and aggregate terms */
Formula* unnestFuncsAndAggs(Formula*, AbstractStructure* str = NULL, Context con = Context::POSITIVE);

/** Recursively move all partial terms outside atoms */
Formula* unnestPartialTerms(Formula*, Context con = Context::POSITIVE, AbstractStructure* str = NULL, Vocabulary* voc = NULL);

/** Recursively remove all nested terms */
Formula* unnestTerms(Formula*, Context con = Context::POSITIVE, AbstractStructure* str = NULL, Vocabulary* voc = NULL);

/** Non-recursively move terms that are three-valued in a given structure outside of the given atom */
Formula* unnestThreeValuedTerms(Formula*, AbstractStructure*, Context context, bool cpsupport = false,
		const std::set<const PFSymbol*> cpsymbols = std::set<const PFSymbol*>());

/** Replace all definitions in the theory by their completion */
void addCompletion(AbstractTheory*);

/** Rewrite (! x : ! y : phi) to (! x y : phi), rewrite ((A & B) & C) to (A & B & C), etc. */
void flatten(AbstractTheory*);

/** Rewrite (F(x) = y) or (y = F(x)) to Graph_F(x,y) 
 * Rewrite (AggTerm op BoundTerm) to an aggregate formula (op = '=', '<', or '>') */
AbstractTheory* graphFuncsAndAggs(AbstractTheory*, AbstractStructure* str = NULL, Context con = Context::POSITIVE);

/** Merge two theories */
AbstractTheory* merge(AbstractTheory*, AbstractTheory*);

/** Count the number of subformulas in the theory */
int nrSubformulas(AbstractTheory*);

/** Push negations inside */
void pushNegations(AbstractTheory*);

/** Rewrite (! x : phi & chi) to ((! x : phi) & (!x : chi)), and similarly for ?. */
AbstractTheory* pushQuantifiers(AbstractTheory*);

/** Rewrite A <=> B to (A => B) & (B => A) */
AbstractTheory* removeEquivalences(AbstractTheory*);

/** Rewrite chains of equalities to a conjunction or disjunction of atoms. */
AbstractTheory* splitComparisonChains(AbstractTheory*, Vocabulary* voc = NULL);

/** Recursively move all function and aggregate terms */
AbstractTheory* unnestFuncsAndAggs(AbstractTheory*, AbstractStructure* str = NULL, Context con = Context::POSITIVE);

/** Rewrite the theory so that there are no nested terms */
void unnestTerms(AbstractTheory*, Context con = Context::POSITIVE, AbstractStructure* str = NULL, Vocabulary* voc = NULL);
void unnestThreeValuedTerms(AbstractTheory*, Context con = Context::POSITIVE, AbstractStructure* str = NULL, bool cpsupport = false,
		const std::set<const PFSymbol*> cpsymbols = std::set<const PFSymbol*>());
}

namespace TermUtils {
/** Check sorts in the given term */
void checkSorts(Vocabulary*, Term*);

/** Derive sorts in the given term */
void deriveSorts(Vocabulary*, Term*);

/** Derive bounds of the given term in the given structure */
std::vector<const DomainElement*> deriveTermBounds(Term*, const AbstractStructure*);

/** Check whether the term is partial (contains partial functions) */
bool isPartial(Term*);
}

namespace SetUtils {
/** Returns false if the set expression is not two-valued in the given structure. 
 May return true if the set expression is two-valued in the structure. */
bool approxTwoValued(SetExpr*, AbstractStructure*);

/** Rewrite set expressions by moving three-valued terms */
SetExpr* unnestThreeValuedTerms(SetExpr*, AbstractStructure*, Context context, bool cpsupport = false,
		const std::set<const PFSymbol*> cpsymbols = std::set<const PFSymbol*>());
}

namespace DefinitionUtils {
/** Check sorts in the given rule */
void checkSorts(Vocabulary* v, Rule* f);

/** Derive sorts in the given rule */
void deriveSorts(Vocabulary* v, Rule* f);

/** Compute the open symbols of a definition */
std::set<PFSymbol*> opens(Definition*);

/** Non-recursively move terms that are three-valued in a given structure outside of the head of the rule */
Rule* unnestThreeValuedTerms(Rule*, AbstractStructure*, Context context, bool cpsupport = false,
		const std::set<const PFSymbol*> cpsymbols = std::set<const PFSymbol*>());

Rule* unnestHeadTermsContainingVars(Rule* rule, AbstractStructure* structure, Context context);
}

#endif /* IDP_THEORYUTILS_HPP_ */
