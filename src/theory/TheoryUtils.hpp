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

#pragma once

#include "common.hpp"
#include "GlobalData.hpp"
#include "options.hpp"
#include <typeinfo>
#include <iostream>
#include "vocabulary/VarCompare.hpp"
#include "information/GetQuantifiedVariables.hpp"
#include "information/CollectSymbols.hpp"
#include "information/ContainedVariables.hpp"
#include "information/CollectSymbolOccurences.hpp"

class Definition;
class SetExpr;
class PFSymbol;
class PredForm;
class BoolForm;
class FuncTerm;
class Variable;
class Structure;
class AbstractTheory;
class Formula;
class Vocabulary;
class Term;
class AggForm;
class GroundTranslator;
class Rule;
class Function;
class Theory;
class TheoryComponent;
class Sort;
class EqChainForm;
class LazyGroundingManager;
struct Delay;
class Term;

class DomElemContainer;
typedef std::map<Variable*, const DomElemContainer*> var2dommap;

class Predicate;

// TODO what does it mean to pass NULL as vocabulary?

template<typename Transformer, typename ReturnType, typename Construct, typename ... Values>
ReturnType transform(Construct* object, Values ... parameters) {
	Transformer t;
	if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 1) {
		std::clog << tabs() << "Executing " << typeid(Transformer).name() << " on: " << nt() << print(object) << "\n";
		pushtab();
	}
	auto result = t.execute(object, parameters...);
	if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 1) {
		poptab();
		std::clog << tabs() << "Resulted in: " << nt() << print(result) << "\n\n";
	}
	return result;
}

template<typename Transformer, typename Construct, typename ... Values>
void transform(Construct* object, Values ... parameters) {
	Transformer t;
	if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 1) {
		std::clog << tabs() << "Executing " << typeid(Transformer).name() << " on: " << nt() << print(object) << "\n";
		pushtab();
	}
	t.execute(object, parameters...);
	if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 1) {
		poptab();
		std::clog << tabs() << "Resulted in: " << nt() << print(object) << "\n\n";
	}
}

namespace FormulaUtils {
    
CompType getComparison(const PredForm* pf);

    
/** Returns true iff the aggregate formula is anti-monotone */
bool isAntimonotone(const AggForm* af);

/** Returns true iff the aggregate formula is monotone */
bool isMonotone(const AggForm* af);

/** Create the formula 'true' */
BoolForm* trueFormula();

/** Create the formula 'false' */
BoolForm* falseFormula();

/** Returns false if the formula is not two-valued in the given structure.
 May return true if the formula is two-valued in the structure. */
bool approxTwoValued(const Formula*, Structure*);

/** Check sorts in the given formula */
void checkSorts(Vocabulary* v, Formula* f);

Formula* combineAggregates(Formula* f);
void combineAggregates(AbstractTheory* t);

/** Returns true iff at least one FuncTerm/AggTerm occurs in the given formula */
bool containsFuncTerms(Formula* f);
bool containsDomainTerms(Formula* f);
bool containsFuncTermsOutsideOfSets(Formula* f);
bool containsAggTerms(Formula* f);
bool containsSymbol(const PFSymbol* s, const Formula* f);

int countQuantVars(const Theory* t);
int countQuantVars(const Rule* t);
int countQuantVars(const Formula* t);

/** Modifies the theory by transforming cardinality constraints bounded by a value smaller than maxbound to its FO-counterpart. **/
AbstractTheory* replaceCardinalitiesWithFOFormulas(AbstractTheory* t, int maxVarsToIntroduce);

/** If some predform can be found which can make the formula true by itself, one such symbol is returned, otherwise NULL **/
std::shared_ptr<Delay> findDelay(const Formula* f, const var2dommap& varmap, const LazyGroundingManager* manager);

/** Derive sorts in the given formula **/
void deriveSorts(Vocabulary* v, Formula* f);

/** Estimate the cost of the given query
 *		Precondition:
 *			- query does not contain any FuncTerm or AggTerm subterms
 *			- the query has a twovalue result in the given structure
 */
double estimatedCostAll(Formula* query, const varset& freevars, bool inverse,const  Structure* structure);

/** Recursively rewrite all function terms to their predicate form, and aggregate terms to aggregate formulas
 *  definedsymbols parameter:
 *    One cannot always replace terms and atoms of recursively defined symbols with their value if they are 2-valued in the structure
 *    It is possible that this replacemant leads to a different result of the well-foundedness check */
Formula* graphFuncsAndAggs(Formula* f, const Structure* str, const std::set<PFSymbol*>& definedsymbols, bool unnestall, bool cpsupport, Context con = Context::POSITIVE);

/** Rewrite all equivalences into implications */
Formula* removeEquivalences(Formula*);

/** Remove the interpretation of all defined symbols in the theory (make them completely unknown) */
void removeInterpretationOfDefinedSymbols(const Theory*, Structure*);
/** Remove the interpretation of all defined symbols for the definition (make them completely unknown) */
void removeInterpretationOfDefinedSymbols(const Definition*, Structure*);

/** Replace atoms in which functions occur nested with new atoms without those arguments and add the correct equivalences.*/
Theory* replaceWithNestedTseitins(Theory* theory);

Theory* replacePredByPred(Predicate* origPred, Predicate* newPred, Theory* theory);
Formula* replacePredByPred(Predicate* origPred, Predicate* newPred, Formula* theory);

template<class T>
T replaceVariablesUsingEqualities(T t);

Theory* replacePredByFunctions(Theory* newTheory, Predicate* pred, bool addinoutputdef, const std::set<int>& domainindices, const std::set<int>& codomainsindices, bool partialfunctions);

Formula* splitIntoMonotoneAgg(Formula* f);

Formula* skolemize(Formula* t, Vocabulary* v);
Theory* skolemize(Theory* t);
AbstractTheory* skolemize(AbstractTheory* t);

Theory* sharedTseitinTransform(Theory* t, Structure* s = NULL);

/** Replace the given term by the given variable in the given formula */
Formula* substituteTerm(Formula*, Term*, Variable*);
Formula* substituteVarWithDom(Formula* formula, const std::map<Variable*, const DomainElement*>& var2domelem);
/** Replace the variables according to the given map */
Formula* substituteVarWithVar(Formula* formula, const std::map<Variable*, Variable*>& var2var);

/** Recursively move all function and aggregate terms */
Formula* unnestFuncsAndAggs(Formula*, const Structure* str = NULL);

/** Non-recursively move all function and aggregate terms */
Formula* unnestFuncsAndAggsNonRecursive(Formula*, const Structure* str = NULL);

/** Recursively move all domain terms */
Formula* unnestDomainTerms(Formula*);
Formula* unnestDomainTermsFromNonBuiltins(Formula*);


/** Recursively move all partial terms outside atoms */
Formula* unnestPartialTerms(Formula*, const Structure* str = NULL, Vocabulary* voc = NULL);
AbstractTheory* unnestPartialTerms(AbstractTheory*, const Structure* str = NULL, Vocabulary* voc = NULL);

/** Recursively remove all nested terms */
Formula* unnestTerms(Formula*, const Structure* str = NULL, Vocabulary* voc = NULL);

/** NON-RECURSIVELY move terms that are three-valued in a given structure outside of the given atom, EXCEPT for atoms over equality
 *  definedsymbols parameter:
 *    One cannot always replace terms and atoms of recursively defined symbols with their value if they are 2-valued in the structure
 *    It is possible that this replacemant leads to a different result of the well-foundedness check */
Formula* unnestThreeValuedTerms(Formula*, const Structure*, const std::set<PFSymbol*>& definedsymbols, bool cpsupport);

/** Replace all definitions in the theory by their completion */
void replaceDefinitionsWithCompletion(AbstractTheory*, const Structure* s);

/**
 * Add the If direction of the definition semantics. The old definition is not removed, but can be regarded as the remaining only if and ufs constraint.
 */
void addIfCompletion(AbstractTheory*);

/**
 * Given a vocabulary and a map of function symbols to their function constraint, add
 * a new function constraints for all functions not already occurring in the list.
 */
void addFuncConstraints(AbstractTheory*, Vocabulary*, std::map<Function*, Formula*>& funcconstraints, bool alsoCPableFunctions = true);
void addFuncConstraints(TheoryComponent*, Vocabulary*, std::map<Function*, Formula*>& funcconstraints, bool alsoCPableFunctions = true);
void addFuncConstraints(Term*, Vocabulary*, std::map<Function*, Formula*>& funcconstraints, bool alsoCPableFunctions = true);

/** Rewrite (! x : ! y : phi) to (! x y : phi), rewrite ((A & B) & C) to (A & B & C), etc. */
template<class T>
T flatten(T);
/** Push negations inside */
template<class T>
T pushNegations(T);
/** Pull quantifications outside */
template<class T>
T pullQuantifiers(T);
/** Non-recursively push quantifiers down as far as possible */
template<class T>
T pushQuantifiers(T t);
/** Rewrite chains of equalities to a conjunction or disjunction of atoms. */
template<class T>
T splitComparisonChains(T, Vocabulary* voc = NULL);

/** Rewrite (F(x) = y) or (y = F(x)) to Graph_F(x,y) 
 * Rewrite (AggTerm op BoundTerm) to an aggregate formula (op = '=', '<', or '>')
 *  definedsymbols parameter:
 *    One cannot always replace terms and atoms of recursively defined symbols with their value if they are 2-valued in the structure
 *    It is possible that this replacemant leads to a different result of the well-foundedness check */
Theory* graphFuncsAndAggs(Theory*, const Structure* str, const std::set<PFSymbol*>& definedsymbols, bool unnestall, bool cpsupport, Context con = Context::POSITIVE);
Theory* graphFuncsAndAggsForXSB(Theory*, const Structure* str, const std::set<PFSymbol*>& definedsymbols, bool unnestall, bool cpsupport, Context con = Context::POSITIVE);
AbstractTheory* graphFuncsAndAggs(AbstractTheory*, const Structure* str, const std::set<PFSymbol*>& definedsymbols, bool unnestall, bool cpsupport, Context con = Context::POSITIVE);

/** Merge two theories, returns a full clone */
AbstractTheory* merge(AbstractTheory*, AbstractTheory*);

/** Count the number of subformulas in the theory */
int nrSubformulas(AbstractTheory*);

template<class T>
T simplify(T theory, const Structure* structure);

/** Applies some useful transformations to reduce the grounding size of the theory **/
template<class T>
T improveTheoryForInference(T theory, Structure* structure, bool skolemize, bool nbmodelsequivalent);

/** Rewrite (!x y: phi(x) | psi(y)) to (!x phi(x)) | (!y psi(y)) in the case phi does not contain y and psi not x. */
Formula* pushQuantifiersAndNegations(Formula*);
AbstractTheory* pushQuantifiersAndNegations(AbstractTheory*);
/** Does what the above does, with the extra rewrite of (! x : phi & chi) to ((! x : phi) & (!x : chi)), and similarly for ?. */
AbstractTheory* pushQuantifiersCompletely(AbstractTheory*);

/** Rewrite (! x : phi(x)) to (~ ? x : ~ phi(x)) with the negation in front of phi pushed down. */
Formula* eliminateUniversalQuantifications(Formula*);
Theory* eliminateUniversalQuantifications(Theory*);

/** Rewrite A <=> B to (A => B) & (B => A) */
AbstractTheory* removeEquivalences(AbstractTheory*);

/** Recursively move all function and aggregate terms, except for constructor functions */
AbstractTheory* unnestForXSB(AbstractTheory*, const Structure* str = NULL);

/** Reduce definitions to a "normal form", where tseitinisation is used to transform definitions into definitions
that only have rules of one of the following 4 forms:
 predform(..) <- conjunction of predforms
 predform(..) <- disjunction of predforms
 predform(..) <- forall with nested predform
 predform(..) <- exists with nested predform // TODO: allow a conjunction here instead of a nested predform (for XSB this leads to fewer Tseitins)
 */
Theory* definitionsToNormalForm(Theory*);

/** Recursively move all function and aggregate terms */
AbstractTheory* unnestFuncsAndAggs(AbstractTheory*, const Structure* str = NULL);

/** Non-recursively move all function and aggregate terms */
AbstractTheory* unnestFuncsAndAggsNonRecursive(AbstractTheory*,const Structure* str = NULL);

/** Recursively move all domain terms */
AbstractTheory* unnestDomainTerms(AbstractTheory*);
AbstractTheory* unnestDomainTermsFromNonBuiltins(AbstractTheory*);


/** Rewrite the theory so that there are no nested terms */
void unnestTerms(AbstractTheory*, const Structure* str = NULL, Vocabulary* voc = NULL);

std::map<Variable*, QuantType> collectQuantifiedVariables(Formula* f, bool recursive);
std::map<Variable*, QuantType> collectQuantifiedVariables(Rule* f, bool recursive);
std::map<Variable*, QuantType> collectQuantifiedVariables(AbstractTheory* f, bool recursive);

/** Returns all variables contained in t */
template<class T>
varset collectVariables(T* t){
	ContainedVariables c;
	return c.execute(t);
}


template<class T>
std::set<PFSymbol* > collectSymbols(const T* f){
	return transform<CollectSymbols, std::set<PFSymbol*> >(f);
}

template<class T>
std::set<std::pair<PFSymbol*, Context> > collectSymbolOccurences(const T* f){
	CollectSymbolOccurences t;
	return t.execute(f);
}

Formula* removeQuantificationsOverSort(Formula* f, const Sort* s);
Rule* removeQuantificationsOverSort(Rule* f, const Sort* s);
AbstractTheory* removeQuantificationsOverSort(AbstractTheory* f, const Sort* s);

/** Detect whether the EqChainForm is a range */
bool isRange(const EqChainForm*);

/** Retrieve the lower bound for EqChainForms that are ranges */
int getLowerBound(const EqChainForm*);

/** Retrieve the upper bound for EqChainForms that are ranges */
int getUpperBound(const EqChainForm*);

/** Retrieve the variable from EqChainForms that are ranges */
Variable* getVariable(const EqChainForm*);
} /* namespace FormulaUtils */


namespace TermUtils {

bool isAgg(Term* t);
bool isFunc(Term* t);
bool isDom(Term* t);
bool isVar(Term* t);
bool isAggOrFunc(Term* t);
bool isAggOrNonConstructorFunction(Term* t);
bool isVarOrDom(Term* t);

bool isCard(Term* term);

/** Returns false if the term is not two-valued in the given structure.
 *  May return true if the term is two-valued in the structure. */
bool approxTwoValued(const Term*, const Structure*);

/** Check sorts in the given term */
void checkSorts(Vocabulary*, Term*);

bool contains(const Variable* v, const Term* f);
bool containsSymbol(const PFSymbol* s, const Term* f);

/** Derive sorts in the given term */
void deriveSorts(Vocabulary*, Term*);

template<class T>
T simplify(T theory, const Structure* structure);

/** Derive bounds of the given term in the given structure, returned as a tuple <minbound, maxbound>
 * 	If no bounds can be derived, NULL is returned for both bounds
 */
std::vector<const DomainElement*> deriveTermBounds(const Term*, const Structure*);

/** Returns false if the value of the term is defined for all possible instantiations of its free variables */
bool isPartial(Term*);

/** Check whether a function term is a term multiplied by a factor */
bool isTermWithIntFactor(const FuncTerm* term, const Structure* structure);
bool isFactor(const Term* term, const Structure* structure);
bool isNumber(const Term* term);
bool isIntNumber(const Term* term);
bool isFloatNumber(const Term* term);
int getInt(const Term* term);
} /* namespace TermUtils */


namespace SetUtils {
/** Returns false if the set expression is not two-valued in the given structure. 
 May return true if the set expression is two-valued in the structure. */
bool approxTwoValued(const SetExpr*, const Structure*);

/** Rewrite set expressions by moving three-valued terms
 *  definedsymbols parameter:
 *    One cannot always replace terms and atoms of recursively defined symbols with their value if they are 2-valued in the structure
 *    It is possible that this replacemant leads to a different result of the well-foundedness check */
SetExpr* unnestThreeValuedTerms(SetExpr* exp, Structure* structure, const std::set<PFSymbol*>& definedsymbols, bool cpsupport, TruthValue cpablerelation);
} /* namespace SetUtils */


namespace DefinitionUtils {
/** Check sorts in the given rule */
void checkSorts(Vocabulary* v, Rule* f);

/** Derive sorts in the given rule */
void deriveSorts(Vocabulary* v, Rule* f);

/** Compute the open symbols of a definition */
std::set<PFSymbol*> opens(const Definition*);
std::set<PFSymbol*> approxTwoValuedOpens(const Definition* d, Structure* s);

/** Collect the open symbols of all definitions, put them in a map */
std::map<Definition*, std::set<PFSymbol*> > opens(std::vector<Definition*>);

/**
 * Given a definition and a symbol, returns the direct dependencies of the symbol in the definition.
 * If the given symbol is not defined in the definition, this returns an empty set.
 * 
 * This method does not return INDIRECT dependencies of the symbol.
 */
std::set<PFSymbol*> getDirectDependencies(const Definition*, PFSymbol*);

/** Get the defined symbols of the given definition */
std::set<PFSymbol*> defined(const Definition*);

/** Inspect whether the given rule defines the given symbol */
bool definesSymbol(const Rule*, PFSymbol*);

/** Approximate check whether the given definition is total */
bool approxTotal(Definition*);

/** Approximate Check whether the definition has recursion over negation
 * Guaranteed to be complete in case definitions are split.*/
bool approxHasRecursionOverNegation(const Definition*);
std::set<PFSymbol*> approxRecurionsOverNegationSymbols(const Definition*);

/** Check whether the definition has recursion over negation
 * WARNING: expensive! If you are certain that definitions are split, better use approx method!*/
bool hasRecursionOverNegation(const Definition*);
std::set<PFSymbol*> recurionsOverNegationSymbols(const Definition*);

/** Stratify all definitions in a theory */
void splitDefinitions(Theory* t);

#ifdef WITHXSB
/** Group definitions for minimum overhead with XSB */
void joinDefinitionsForXSB(Theory* t, const Structure* s);
#endif

/** Check whether the definition has a recursive aggregate */
bool approxContainsRecDefAggTerms(const Definition*);

/** Add a "symbol <- false" body to open symbols with a 3-valued interpretation */
Definition* makeDefinitionCalculable(Definition*, Structure*);

/** Rewrite (! x : phi(x)) to (~ ? x : ~ phi(x)) with the negation in front of phi pushed down. */
Definition* eliminateUniversalQuantifications(Definition*);

/** Non-recursively move terms that are three-valued in a given structure outside of the head of the rule
 *  definedsymbols parameter:
 *    One cannot always replace terms and atoms of recursively defined symbols with their value if they are 2-valued in the structure
 *    It is possible that this replacemant leads to a different result of the well-foundedness check */
Rule* unnestThreeValuedTerms(Rule*, const Structure*, const std::set<PFSymbol*>& definedsymbols, bool cpsupport);

Rule* unnestNonVarHeadTerms(Rule* rule, const Structure* structure);

/** Create the rule P(\bar x) \lrule false*/
Rule* falseRule(PFSymbol*);

Rule* unnestHeadTermsNotVarsOrDomElems(Rule* rule, const Structure* structure);

// Move head quantifiers of variables only occurring in the body to the body.
Rule* moveOnlyBodyQuantifiers(Rule* rule);

} /* namespace DefinitionUtils */

