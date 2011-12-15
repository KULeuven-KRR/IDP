/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include <vector>
#include <cassert>
#include <iostream> //TODO remove (debug only)
#include "theorytransformations/SplitProducts.hpp"
#include "theorytransformations/SplitComparisonChains.hpp"
#include "theorytransformations/DeriveSorts.hpp"

#include "utils/TheoryUtils.hpp"

#include "structure.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"

using namespace std;

Formula* SplitProducts::execute(Formula* af){
	return af->accept(this);
}

/*Here, we transform something of the form PROD{x:p(x):f(x)} = c to
 *
 * (c = 0 <=> #{x: p(x) & f(x) = 0:f(x)}~= 0) &
 * (c ~= 0 <=>
 * 		(#{x: p(x) & f(x) = 0:f(x)} = 0))
 * 		&
 * 				((prod{x : p(x) & x>0 : x} * prod{x : p(x) & x<0 : -x} = c)
 *  			& card{x : p(x) & x<0}%2=0)
 *  		|
 *				(prod{x : p(x) & x>0 : x} * prod{x : p(x) & x<0 : -x} * -1 = c)
 *  			& card{x : p(x) & x<0}%2~=0))
 *  */
Formula* SplitProducts::visit(AggForm* af) {
	auto aggterm = af->right();
	auto otherterm = af->left();
	if (aggterm->function() != AggFunction::PROD) {
		return af;
	}

	//TODO: opitmize: first check: are there possible zero/negative values? Only than start splitting

	auto set = aggterm->set();
	auto posset = set->positiveSubset();
	auto nulset = set->zeroSubset();
	auto negset = set->negativeSubset();

	auto sort = aggterm->sort();
	auto voc = *(sort->firstVocabulary());
	auto equalitySymbol = voc->pred("=/2");

	auto nul = DomainElementFactory::createGlobal()->create(0);
	auto nulterm = new DomainTerm(sort, nul, aggterm->pi());
	auto otherTermNul = new PredForm(SIGN::POS, equalitySymbol, { otherterm, nulterm }, FormulaParseInfo()); //c = 0
	auto otherTermNotNul = new PredForm(SIGN::NEG, equalitySymbol, { otherterm, nulterm }, FormulaParseInfo()); //c ~= 0
	auto nulOccurences = new AggForm(SIGN::POS, nulterm, CompType::NEQ, new AggTerm(nulset, AggFunction::CARD, aggterm->pi()), af->pi()); //#{x: p(x) & f(x) = 0:f(x)}~= 0
	auto noNulOccurences = new AggForm(SIGN::POS, nulterm, CompType::EQ, new AggTerm(nulset, AggFunction::CARD, aggterm->pi()), af->pi()); //#{x: p(x) & f(x) = 0:f(x)}= 0

	auto intsort = VocabularyUtils::intsort();
	auto minusone = DomainElementFactory::createGlobal()->create(-1);
	auto minusoneterm = new DomainTerm(intsort, minusone, aggterm->pi());

	auto prodPos = new AggTerm(posset, AggFunction::PROD, aggterm->pi());

	auto prodNeg = new AggTerm(negset, AggFunction::PROD, aggterm->pi());
	auto trueform = new BoolForm(SIGN::POS, true, { }, af->pi());
	auto prodsetone = new EnumSetExpr( { trueform, trueform }, { prodPos, prodNeg }, set->pi());
	auto prodsetminusone = new EnumSetExpr( { trueform, trueform, trueform }, { prodPos, prodNeg, minusoneterm }, set->pi());

	auto prodone = new AggTerm(prodsetone, AggFunction::PROD, aggterm->pi());
	auto prodminusone = new AggTerm(prodsetminusone, AggFunction::PROD, aggterm->pi());

	auto prodWithOne = new AggForm(SIGN::POS, otherterm, af->comp(), prodone, af->pi()); //prod{x : p(x) & x>0 : x} * prod{x : p(x) & x<0 : -x} = c)
	auto prodWithMinusOne = new AggForm(SIGN::POS, otherterm, af->comp(), prodminusone, af->pi()); //prod{x : p(x) & x>0 : x} * prod{x : p(x) & x<0 : -x} * -1= c)

	auto two = DomainElementFactory::createGlobal()->create(2);
	auto twoterm = new DomainTerm(intsort, two, aggterm->pi());

	auto cardNeg = new AggTerm(negset, AggFunction::CARD, aggterm->pi());
	auto modSymbol = voc->func("%/2");
	auto cardNegMod2 = new FuncTerm(modSymbol, { cardNeg, twoterm }, aggterm->pi());

	auto cardNegMod2Is0 = new EqChainForm(SIGN::POS, true, { cardNegMod2, nulterm }, { CompType::EQ }, af->pi());
	auto cardNegMod2IsNot0 = new EqChainForm(SIGN::NEG, true, { cardNegMod2, nulterm }, { CompType::EQ }, af->pi());
	auto prodWithOneAndCardOK = new BoolForm(SIGN::POS, true, prodWithOne, cardNegMod2Is0, af->pi());
	/*	prod{x : p(x) & x>0 : x} * prod{x : p(x) & x<0 : -x} = c)
	 *  & card{x : p(x) & x<0}%2=0)
	 */

	auto prodWithMinusOneAndCardOK = new BoolForm(SIGN::POS, true, prodWithMinusOne, cardNegMod2IsNot0, af->pi());
	/*	prod{x : p(x) & x>0 : x} * prod{x : p(x) & x<0 : -x} * -1 = c)
	 *  & card{x : p(x) & x<0}%2~=0)
	 */

	auto prodAndSign = new BoolForm(SIGN::POS, false, prodWithOneAndCardOK, prodWithMinusOneAndCardOK, af->pi());
	/*	(prod{x : p(x) & x>0 : x} * prod{x : p(x) & x<0 : -x} = c)
	 *  & card{x : p(x) & x<0}%2=0))
	 *  |
	 *	(prod{x : p(x) & x>0 : x} * prod{x : p(x) & x<0 : -x} * -1 = c)
	 *  & card{x : p(x) & x<0}%2~=0))
	 */

	auto otherTermNotNulConstraint = new BoolForm(SIGN::POS, true, noNulOccurences, prodAndSign, af->pi());
	auto firstLine = new EquivForm(SIGN::POS, otherTermNul, nulOccurences, af->pi());
	auto secondLine = new EquivForm(SIGN::POS, otherTermNotNul, otherTermNotNulConstraint, af->pi());
	auto newformula = new BoolForm(SIGN::POS, true, firstLine, secondLine, af->pi());
	FormulaUtils::deriveSorts(NULL, newformula);
	return newformula;
}
