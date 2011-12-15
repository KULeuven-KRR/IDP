/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef THEORYUTILS_HPP_
#define THEORYUTILS_HPP_

#include "common.hpp"
#include <iostream>
#include "GlobalData.hpp"
#include "options.hpp"
#include <typeinfo>

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

// FIXME printing
template<typename Transformer, typename ReturnType, typename Construct, typename ... Values>
ReturnType transform(Construct* object, Values ... parameters) {
	Transformer t;
	if(getOption(IntType::GROUNDVERBOSITY)>1){
		std::clog <<"Executing " <<typeid(Transformer).name() <<" on: " <<toString(object) <<"\nResulted in: ";
	}
	ReturnType result = t.execute(object, parameters...);
	if(getOption(IntType::GROUNDVERBOSITY)>1){
		std::clog <<toString(result) <<"\n";
		//FIXME make everything printable? std::clog <<toString(result) <<"\n";
		// => create a static checkable condition whether something is printable via put! If not, do <<
	}
	return result;
}

template<typename Transformer, typename Construct, typename ... Values>
void transform(Construct* object, Values ... parameters) {
	Transformer t;
	if(getOption(IntType::GROUNDVERBOSITY)>1){
		std::clog <<"Executing " <<typeid(Transformer).name() <<" on: " <<toString(object) <<"\nResulted in: ";
	}
	t.execute(object, parameters...);
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
Formula* unnestTerms(Formula*, Context context = Context::POSITIVE);

/** Rewrite all equivalences into implications **/
Formula* removeEquivalences(Formula*);

/** Flatten all nested formulas **/
Formula* flatten(Formula*);

void checkSorts(Vocabulary* v, Term* f);
void checkSorts(Vocabulary* v, Rule* f);
void checkSorts(Vocabulary* v, Formula* f);

void deriveSorts(Vocabulary* v, Term* f);
void deriveSorts(Vocabulary* v, Rule* f);
void deriveSorts(Vocabulary* v, Formula* f);

/** Recursively rewrite all function terms to their predicate form **/
Formula* graphFuncsAndAggs(Formula* f);

Formula* splitProducts(Formula* f);

Formula* splitIntoMonotoneAgg(Formula* f);

/** Recursively move all partial terms outside atoms **/
Formula* unnestPartialTerms(Formula* f, Context context, Vocabulary* voc = NULL);

/** Non-recursively move terms that are three-valued in a given structure outside of the given atom **/
Formula* unnestThreeValuedTerms(Formula*, AbstractStructure*, Context context, bool cpsupport = false, const std::set<const PFSymbol*> cpsymbols =
		std::set<const PFSymbol*>());

/** Non-recursively move terms that are three-valued in a given structure outside of the head of the rule **/
Rule* unnestThreeValuedTerms(Rule*, AbstractStructure*, Context context, bool cpsupport = false, const std::set<const PFSymbol*> cpsymbols =
		std::set<const PFSymbol*>());

/** Returns true iff at least one FuncTerm/AggTerm occurs in the given formula **/
bool containsFuncTerms(Formula* f);
bool containsAggTerms(Formula* f);
bool containsSymbol(const PFSymbol* s, const Formula* f);

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

/** Rewrite (F(x) = y) or (y = F(x)) to Graph_F(x,y) 
  * Rewrite (AggTerm op BoundTerm) to an aggregate formula (op = '=', '<', or '>') **/
AbstractTheory* graphFuncsAndAggs(AbstractTheory*);

AbstractTheory* splitProducts(AbstractTheory* f);

/** Replace all definitions in the theory by their completion **/
AbstractTheory* addCompletion(AbstractTheory*);

/** Count the number of subformulas in the theory **/
int nrSubformulas(AbstractTheory*);

/** Merge two theories **/
AbstractTheory* merge(AbstractTheory*, AbstractTheory*);
}

namespace TermUtils {
/** Rewrite set expressions by moving three-valued terms **/
SetExpr* moveThreeValuedTerms(SetExpr*, AbstractStructure*, Context context, bool cpsupport = false, const std::set<const PFSymbol*> cpsymbols = std::set<const PFSymbol*>());
}

namespace SetUtils {
/** Returns false if the set expression is not two-valued in the given structure. 
May return true if the set expression is two-valued in the structure. **/
bool approxTwoValued(SetExpr*,AbstractStructure*);	
}

namespace DefinitionUtils {
/** Compute the open symbols of a definition **/
std::set<PFSymbol*> opens(Definition*);
}

#endif /* THEORYUTILS_HPP_ */
