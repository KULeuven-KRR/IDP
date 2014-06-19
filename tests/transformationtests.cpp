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

#include "gtest/gtest.h"
#include "creation/cppinterface.hpp"

#include <iostream>

#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"

#include "inferences/grounding/LazyGroundingManager.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/GroundPolicy.hpp"

using namespace Gen;

// AddCompletion - theory
//TODO

// DeriveSorts - term,rule,formula
//TODO

// Flatten - formula,theory - boolform,quantform
TEST(FlattenTest,BoolForm) {
	auto p = pred("P", { });
	auto q = pred("Q", { });
	auto r = pred("R", { });

	Formula& pvqvr = p( { }) | (q( { }) | r( { }));

	// Flattening (P | (Q | R)) to (P | Q | R).
	auto result = FormulaUtils::flatten(&pvqvr);

	EXPECT_TRUE(isa<BoolForm>(*result));
	EXPECT_EQ((uint)3, result->subformulas().size());

	result->recursiveDelete();
}

TEST(FlattenTest,QuantForm) {
	auto s = sort("S", -2, 2);
	auto x = var(s);
	auto y = var(s);
	auto p = pred("P", { s, s });

	Formula& axaypxy = forall(x, forall(y, p( { x, y })));

	// Flattening (! x : ! y : P(x,y)) to (! x y : P(x,y)).
	auto result = FormulaUtils::flatten(&axaypxy);

	EXPECT_TRUE(isa<QuantForm>(*result));
	EXPECT_EQ((uint)2, result->quantVars().size());

	result->recursiveDelete();
}

TEST(FlattenTest,Theory) {
	auto s = sort("S", -2, 2);
	auto x = var(s);
	auto y = var(s);
	auto p = pred("P", { s, s });
	auto q = pred("Q", { s, s });
	auto r = pred("R", { s, s });

	auto voc = new Vocabulary("V");
	voc->add(s);
	voc->add(p.p());
	voc->add(q.p());
	voc->add(r.p());

	Formula& axaypvqvr = forall(x, forall(y, p( { x, y }) | (q( { x, y }) | r( { x, y }))));

	auto theory = new Theory("T", voc, ParseInfo());
	theory->add(&axaypvqvr);

	// Flattening (! x : ! y : P(x,y) | (Q(x,y) | R(x,y))) to (! x y : P(x,y) | Q(x,y) | R(x,y)).
	FormulaUtils::flatten(theory);

	ASSERT_EQ((uint)1, theory->sentences().size());
	auto resformula = theory->sentences()[0];
	EXPECT_TRUE(isa<QuantForm>(*resformula));
	EXPECT_EQ((uint)2, resformula->quantVars().size());
	ASSERT_EQ((uint)1, resformula->subformulas().size());
	auto ressubformula = resformula->subformulas()[0];
	EXPECT_TRUE(isa<BoolForm>(*ressubformula));
	EXPECT_EQ((uint)3, ressubformula->subformulas().size());

	theory->recursiveDelete();
}

// GraphFuncsAndAggs - formula,theory
TEST(GraphFuncsAndAggsTest,OneFuncTerm) {
	auto s = sort("X", -2, 2);
	auto one = domainterm(s, 1);
	auto two = domainterm(s, 2);
	auto f = func("F", { s }, s);

	Formula& eqf00 = (f( { one }) == *two);

	// Rewriting (F(1) = 2) to (F(1,2))
	auto result = FormulaUtils::graphFuncsAndAggs(&eqf00, NULL, {}, true, false);

	ASSERT_TRUE(isa<PredForm>(*result));
	auto respredform = dynamic_cast<PredForm*>(result);
	EXPECT_TRUE(isa<Function>(*(respredform->symbol())));
	EXPECT_EQ(f.f()->name(), respredform->symbol()->name());

	result->recursiveDelete();
}

TEST(GraphFuncsAndAggsTest,TwoFuncTerms) {
	auto s = sort("X", -2, 2);
	auto f = func("F", { s }, s);
	auto g = func("G", { s }, s);
	auto x = var(s);
	auto y = var(s);

	Formula& eqf0g0 = (f( { x }) == g( { y }));

	// Rewriting (F(x) = G(y)) to (! z : ~G(y,z) | F(x,z)) 
	//std::clog << "Transforming " << toString(&eqf0g0) << "\n";
	auto result = FormulaUtils::graphFuncsAndAggs(&eqf0g0, NULL, {}, true, false);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(isa<QuantForm>(*result));
	ASSERT_EQ((uint)1, result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(isa<BoolForm>(*ressubformula));
	ASSERT_EQ((uint)2, ressubformula->subformulas().size());
	ASSERT_TRUE(isa<PredForm>(*ressubformula->subformulas()[0]));
	auto subf1 = dynamic_cast<PredForm*>(ressubformula->subformulas()[0]);
	ASSERT_TRUE(isa<Function>(*subf1->symbol()));
	ASSERT_TRUE(isa<PredForm>(*ressubformula->subformulas()[1]));
	auto subf2 = dynamic_cast<PredForm*>(ressubformula->subformulas()[1]);
	ASSERT_TRUE(isa<Function>(*subf2->symbol()));

	result->recursiveDelete();
}

TEST(GraphFuncsAndAggsTest,OneAggTerm) {
	auto s = sort("X", -2, 2);
	auto one = domainterm(s, 1);
	auto p = pred("P", { s });
	auto xt = varterm(s);
	auto x = xt->var();

	auto sumterm = sum(qset( { x }, p( { x }), xt));

	Formula& eq0sumterm = (*one == *sumterm);

	// Rewriting (0 = sum{ x : P(x) : x })
	auto result = FormulaUtils::graphFuncsAndAggs(&eq0sumterm, NULL, {}, true, false);

	ASSERT_TRUE(isa<AggForm>(*result));
	auto resaggform = dynamic_cast<AggForm*>(result);
	EXPECT_EQ(one, resaggform->getBound());
	EXPECT_EQ(CompType::EQ, resaggform->comp());
	EXPECT_EQ(sumterm, resaggform->getAggTerm());

	result->recursiveDelete();
}

TEST(GraphFuncsAndAggsTest,TwoAggTerm) {
	auto s = sort("X", -2, 2);
	auto p = pred("P", { s });
	auto q = pred("Q", { s });
	auto xt = varterm(s);
	auto x = xt->var();
	auto yt = varterm(s);
	auto y = yt->var();

	auto sumpx = sum(qset( { x }, p( { x }), xt));
	auto sumqy = sum(qset( { y }, q( { y }), yt));

	Formula& eqsumpxsumqy = (*sumpx == *sumqy);

	// Rewrite (sum{ x : P(x) : x } = sum{ y : Q(y) : y })
	//std::clog << "Transforming " << toString(&eqsumpxsumqy) << "\n";
	auto result = FormulaUtils::graphFuncsAndAggs(&eqsumpxsumqy, NULL, {}, true, false);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(isa<QuantForm>(*result));
	ASSERT_EQ((uint)1, result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(isa<BoolForm>(*ressubformula));
	ASSERT_EQ((uint)2, ressubformula->subformulas().size());
	ASSERT_TRUE(isa<AggForm>(*ressubformula->subformulas()[0]));
	ASSERT_TRUE(isa<AggForm>(*ressubformula->subformulas()[1]));

	result->recursiveDelete();
}

TEST(GraphFuncsAndAggsTest,FuncTermAndAggTerm) {
	auto s = sort("X", -2, 2);
	auto p = pred("P", { s });
	auto f = func("F", { s }, s);
	auto xt = varterm(s);
	auto x = xt->var();
	auto y = var(s);

	auto sumpx = sum(qset( { x }, p( { x }), xt));

	Formula& eqsumpxfy = (*sumpx == f( { y }));

	// Rewrite (sum{ x : P(x) : x } = f(y))
	//std::clog << "Transforming " << toString(&eqsumpxfy) << "\n";
	auto result = FormulaUtils::graphFuncsAndAggs(&eqsumpxfy, NULL, {}, true, false);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(isa<QuantForm>(*result));
	ASSERT_EQ((uint)1, result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(isa<BoolForm>(*ressubformula));
	ASSERT_EQ((uint)2, ressubformula->subformulas().size());
	ASSERT_TRUE(isa<PredForm>(*ressubformula->subformulas()[0]));
	auto subf = dynamic_cast<PredForm*>(ressubformula->subformulas()[0]);
	ASSERT_TRUE(isa<Function>(*subf->symbol()));
	ASSERT_TRUE(isa<AggForm>(*ressubformula->subformulas()[1]));

	result->recursiveDelete();
}

// PushNegations - theory
TEST(PushNegationsTest,BoolForm) {
	auto p = pred("P", { });
	auto q = pred("Q", { });

	Formula& bf = not (p( { }) | q( { }));

	// Rewriting ~(P | Q) to (~P & ~Q).
	//std::clog << "Transforming " << toString(&bf) << "\n";
	auto result = FormulaUtils::pushNegations(&bf);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(isa<BoolForm>(*result));
	EXPECT_TRUE(isPos(result->sign()));
	ASSERT_EQ((uint)2, result->subformulas().size());
	EXPECT_TRUE(isNeg(result->subformulas()[0]->sign()));
	EXPECT_TRUE(isNeg(result->subformulas()[1]->sign()));

	result->recursiveDelete();
}

TEST(PushNegationsTest,NestedBoolForm) {
	auto p = pred("P", { });
	auto q = pred("Q", { });
	auto r = pred("R", { });

	Formula& bf = not (p( { }) | (q( { }) & r( { })));

	// Rewriting ~(P | (Q & R)) to (~P & (~Q | ~R))
	//std::clog << "Transforming " << toString(&bf) << "\n";
	auto result = FormulaUtils::pushNegations(&bf);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(isa<BoolForm>(*result));
	EXPECT_TRUE(isPos(result->sign()));
	ASSERT_EQ((uint)2, result->subformulas().size());
	EXPECT_TRUE(isNeg(result->subformulas()[0]->sign()));
	EXPECT_TRUE(isPos(result->subformulas()[1]->sign()));

	result->recursiveDelete();
}

TEST(PushNegationsTest,QuantForm) {
	auto s = sort("X", -2, 2);
	auto x = var(s);
	auto p = pred("P", { s });

	Formula& qf = not forall(x, p( { x }));

	// Rewriting ~(! x : P(x)) to (? x : ~P(x))
	auto result = FormulaUtils::pushNegations(&qf);

	ASSERT_TRUE(isa<QuantForm>(*result));
	EXPECT_TRUE(isPos(result->sign()));
	EXPECT_FALSE(dynamic_cast<QuantForm*>(result)->isUniv());
	ASSERT_EQ((uint)1, result->subformulas().size());
	EXPECT_TRUE(isNeg(result->subformulas()[0]->sign()));

	result->recursiveDelete();
}

//TEST(PushNegationsTest,EqChainForm) {
// 	//TODO
//}

//TEST(PushNegationsTest,EquivForm) {
//	//TODO
//}

TEST(PushNegationsTest,Theory) {
	auto p = pred("P", { });
	auto q = pred("Q", { });
	auto r = pred("R", { });

	auto voc = new Vocabulary("V");
	add(voc, { p.p(), q.p(), r.p() });

	Formula& bf = not (p( { }) | q( { }));

	auto theory = new Theory("T", voc, ParseInfo());
	theory->add(&bf);

	FormulaUtils::pushNegations(theory);
	auto restheory = dynamic_cast<Theory*>(theory);
	ASSERT_EQ((uint)1, restheory->sentences().size());
	auto resformula = restheory->sentences()[0];
	EXPECT_TRUE(isa<BoolForm>(*resformula));
	ASSERT_EQ((uint)2, resformula->subformulas().size());
	EXPECT_TRUE(isNeg(resformula->subformulas()[0]->sign()));
	EXPECT_TRUE(isNeg(resformula->subformulas()[1]->sign()));

	theory->recursiveDelete();
}

// RemoveEquivalences - formula,theory
TEST(RemoveEquivalencesTest,EquivForm) {
	auto p = pred("P", { });
	auto q = pred("Q", { });

	auto piffq = new EquivForm(SIGN::POS, &p( { }), &q( { }), FormulaParseInfo());

	// Rewriting (P <=> Q) to ((P => Q) & (Q => P)).
	auto result = FormulaUtils::removeEquivalences(piffq);

	ASSERT_TRUE(isa<BoolForm>(*result));
	ASSERT_EQ((uint)2, result->subformulas().size());

	result->recursiveDelete();
}

// SplitComparisonChains - formula,theory
TEST(SplitComparisonChainsTest,NormalEqChainForm) {
	auto s = sort("X", -2, 2);
	auto xt = varterm(s);
	auto yt = varterm(s);
	auto zt = varterm(s);

	auto aisbisc = new EqChainForm(SIGN::POS, true, { xt, yt, zt }, { CompType::EQ, CompType::EQ }, FormulaParseInfo());

	// Rewriting (x = y = z) to ((x = y) & (y = z)).
	auto result = FormulaUtils::splitComparisonChains((Formula*)aisbisc);

	ASSERT_TRUE(isa<BoolForm>(*result));
	ASSERT_EQ((uint)2, result->subformulas().size());

	result->recursiveDelete();
}

TEST(SplitComparisonChainsTest,WeirdEqChainForm) {
	auto s = sort("X", -2, 2);
	auto xt = varterm(s);
	auto yt = varterm(s);
	auto zt = varterm(s);

	auto weird = new EqChainForm(SIGN::NEG, false, { xt, yt, zt }, { CompType::LEQ, CompType::GT }, FormulaParseInfo());

	// Rewriting ~((x =< y) | (y > z)).
	auto result = FormulaUtils::splitComparisonChains((Formula*)weird);

	ASSERT_TRUE(isa<BoolForm>(*result));
	ASSERT_EQ((uint)2, result->subformulas().size());

	result->recursiveDelete();
}

// SplitIntoMonotoneAgg
//TODO

// SubstituteTerm - formula
TEST(SubstituteTermTest,Formula) {
	auto s = sort("X", -2, 2);
	auto p = pred("P", { s });
	auto x = var(s);
	auto y = var(s);

	Formula& px = p( { x });

	ASSERT_EQ((uint)1, px.subterms().size());
	auto xt = px.subterms()[0];

	auto result = FormulaUtils::substituteTerm(&px, xt, y);

	ASSERT_EQ((uint)1, result->subterms().size());
	auto subterm = result->subterms()[0];
	ASSERT_TRUE(isa<VarTerm>(*subterm));
	ASSERT_EQ(y, dynamic_cast<VarTerm*>(subterm)->var());

	result->recursiveDelete();
}

// UnnestPartialTerms
//TODO

// UnnestTerms - formula,theory
TEST(UnnestTermsTest,TwoFuncTermsEQ) {
	auto s = sort("X", -2, 2);
	auto f = func("F", { s }, s);
	auto g = func("G", { s }, s);
	auto x = var(s);

	Formula& eqfxgx = (f( { x }) == g( { x }));

	// Rewriting (F(x) = G(x)) to (! y : ~=(y,G(x)) | =(F(x),y)).
	//std::clog << "Transforming " << toString(&eqfxgx) << "\n";
	auto result = FormulaUtils::unnestTerms(&eqfxgx);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(isa<QuantForm>(*result));
	ASSERT_EQ((uint)1, result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(isa<BoolForm>(*ressubformula));
	ASSERT_EQ((uint)2, ressubformula->subformulas().size());
	ASSERT_TRUE(isa<PredForm>(*ressubformula->subformulas()[0]));
	ASSERT_TRUE(isa<PredForm>(*ressubformula->subformulas()[1]));

	result->recursiveDelete();
}

TEST(UnnestTermsTest,TwoFuncTermsLT) {
	auto s = sort("X", -2, 2);
	auto f = func("F", { s }, s);
	auto g = func("G", { s }, s);
	auto x = var(s);

	Formula& ltfxgx = (f( { x }) < g( { x }));

	// Rewriting (F(x) < G(x)) to (! y z : ~=(y,F(x)) | ~=(z,G(x)) | <(y,z)).
	auto result = FormulaUtils::unnestTerms(&ltfxgx);

	EXPECT_TRUE(isa<QuantForm>(*result));
	ASSERT_EQ((uint)1, result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(isa<BoolForm>(*ressubformula));
	ASSERT_EQ((uint)3, ressubformula->subformulas().size());
	ASSERT_TRUE(isa<PredForm>(*ressubformula->subformulas()[0]));
	ASSERT_TRUE(isa<PredForm>(*ressubformula->subformulas()[1]));
	ASSERT_TRUE(isa<PredForm>(*ressubformula->subformulas()[2]));

	result->recursiveDelete();
}

TEST(UnnestTermsTest,TwoVarTermsEQ) {
	auto s = sort("X", -2, 2);
	auto xt = varterm(s);
	auto yt = varterm(s);

	Formula& eqxy = (*xt == *yt);

	// Rewriting (x = y) to (x = y).
	auto result = FormulaUtils::unnestTerms(&eqxy);

	ASSERT_EQ(&eqxy, result);

	result->recursiveDelete();
}

TEST(UnnestTermsTest,NestedFuncTerms) {
	auto s = sort("X", -2, 2);
	auto f = func("F", { s }, s);
	auto g = func("G", { s }, s);
	auto h = func("H", { s }, s);
	auto x = var(s);

	Formula& eqfgxhx = (f( { &g( { x }) }) == h( { x }));

	//std::clog << "Transforming " << toString(&eqfgxhx) << "\n";
	auto result = FormulaUtils::unnestTerms(&eqfgxhx);
	//std::clog << "Resulted in " << toString(result) << "\n";
std::cerr << toString(result);
	EXPECT_TRUE(isa<QuantForm>(*result));
	ASSERT_EQ((uint)1, result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(isa<BoolForm>(*ressubformula));
	ASSERT_EQ((uint)3, ressubformula->subformulas().size());
	ASSERT_TRUE(isa<PredForm>(*ressubformula->subformulas()[0]));
	ASSERT_TRUE(isa<PredForm>(*ressubformula->subformulas()[1]));
	ASSERT_TRUE(isa<PredForm>(*ressubformula->subformulas()[2]));

	result->recursiveDelete();
}
TEST(UnnestThreeValuedTermsTest,NestedFuncThreeValuedInTwoValued) {
	auto s = sort("X", -2, 2);
	auto f = func("F", { s }, s);
	std::vector<Sort*> sorts = { s, s, s };
	auto plus = get(STDFUNC::ADDITION)->disambiguate(sorts, NULL);
	auto x = var(s);

	auto voc = new Vocabulary("voc");
	voc->add(s);
	voc->add(f.f());
	auto struc = new Structure("struc", voc, ParseInfo());
	auto zero = new DomainTerm(get(STDSORT::INTSORT), domainelement(0), TermParseInfo());
	Term& fx = f( { x });
	auto sum = new FuncTerm(plus, { &fx, zero->clone() }, TermParseInfo());
	auto lt = get(STDPRED::LT)->disambiguate( { s, s });
	std::vector<Term*> terms = { sum, zero };
	auto form = new PredForm(SIGN::POS, lt, terms, FormulaParseInfo());
	auto result = FormulaUtils::unnestThreeValuedTerms(form, struc, {}, false);
	EXPECT_TRUE(isa<QuantForm>(*result));

	result->recursiveDelete();
	delete struc;
	delete voc;

}

//TEST(UnnestTermsTest,NestedAggTerms) {
//	//TODO
//}

// UnnestThreeValuedTerms - formula,rule
//TODO

TEST(DeriveTermBoundsTest,Product) {
	auto s1 = sort("S", -1, 1);
	auto s2 = sort("S2", -1, 2);
	auto x = var(s1);
	auto y = var(s2);
	auto xt = new VarTerm(x, TermParseInfo());
	auto yt = new VarTerm(y, TermParseInfo());

	auto voc = new Vocabulary("voc");
	voc->add(s1);
	voc->add(s2);
	auto func = get(STDFUNC::PRODUCT)->disambiguate( { s1, s2, get(STDSORT::INTSORT) }, voc);
	auto xy = new FuncTerm(func, { xt, yt }, TermParseInfo());
	auto struc = new Structure("struc", voc, ParseInfo());
	auto bounds = TermUtils::deriveTermBounds(xy, struc);
	ASSERT_EQ(DomainElementType::DET_INT, bounds[0]->type());
	ASSERT_EQ(DomainElementType::DET_INT, bounds[1]->type());
	ASSERT_EQ(-2, bounds[0]->value()._int);
	ASSERT_EQ(2, bounds[1]->value()._int);
}

/*class TestGrounder: public DelayGrounder {
public:
	TestGrounder(PredForm& pred, Context context)
			: DelayGrounder(pred.symbol(), pred.args(), context, -1, new GroundTheory<GroundPolicy>(NULL)) {

	}
	void doGround(const Lit&, const ElementTuple&) {

	}
};

TEST(FindUnknTest,QuantFormulaFirstWatched) {
	auto s = sort("S", -2, 2);
	auto x = var(s);
	auto y = var(s);
	auto p = pred("P", { s, s });
	auto q = pred("Q", { s, s });
	auto r = pred("R", { s, s });

	auto voc = new Vocabulary("V");
	add(voc, { s });
	add(voc, { p.p(), r.p(), q.p() });

	auto& pf_p = p( { x, y });
	auto& pf_q = not q( { x, y });
	auto& formula = forall( { x, y }, pf_p | pf_q | r( { x, y }));

	GroundTranslator translator(NULL);
	TestGrounder grounder(pf_p, Context::BOTH);
	translator.notifyDelay(p.p(), &grounder);
	ASSERT_TRUE(not translator.canBeDelayedOn(p.p(), Context::BOTH, -1));

	Context context = Context::BOTH;
	auto predform = FormulaUtils::findUnknownBoundLiteral(&formula, NULL, &translator, context);
	ASSERT_EQ(predform, &pf_q);
}

TEST(FindUnknTest,QuantFormulaPred) {
	auto s = sort("S", -2, 2);
	auto x = var(s);
	auto y = var(s);
	auto p = pred("P", { s, s });

	auto voc = new Vocabulary("V");
	add(voc, { s });
	add(voc, { p.p() });

	GroundTranslator translator(NULL);

	auto& pf_p = p( { x, y });
	auto& formula = forall( { x, y }, pf_p);

	Context context = Context::BOTH;
	auto predform = FormulaUtils::findUnknownBoundLiteral(&formula, NULL, &translator, context);
	ASSERT_EQ(predform, &pf_p);
}

TEST(FindUnknTest,WithMultipleMono) {
	auto s = sort("S", -2, 2);
	auto p = pred("P", { s });

	auto voc = new Vocabulary("V");
	add(voc, { s });
	add(voc, { p.p() });

	GroundTranslator translator(NULL);

	auto x1 = var(s);
	auto x2 = var(s);
	auto& p1 = p( { x1 });
	auto& p2 = p( { x2 });
	auto& formula1 = forall( { x1 }, p1);
	auto& formula2 = exists( { x2 }, p2);

	Context context;
	auto predform = FormulaUtils::findUnknownBoundLiteral(&formula1, NULL, &translator, context);
	ASSERT_EQ(predform, &p1);
	ASSERT_EQ(context, Context::POSITIVE);

	TestGrounder grounder(p1, context);
	translator.notifyDelay(p.p(), &grounder);
	ASSERT_TRUE(translator.canBeDelayedOn(p.p(), Context::POSITIVE, -1));
	ASSERT_FALSE(translator.canBeDelayedOn(p.p(), Context::NEGATIVE, -1));
	ASSERT_FALSE(translator.canBeDelayedOn(p.p(), Context::BOTH, -1));

	auto predform2 = FormulaUtils::findUnknownBoundLiteral(&formula2, NULL, &translator, context);
	ASSERT_EQ(context, Context::POSITIVE);
	ASSERT_EQ(predform2, &p2);

	TestGrounder grounder2(p2, context);
	translator.notifyDelay(p.p(), &grounder2);

	auto& formula3 = exists( { x2 }, not p2);
	auto predform3 = FormulaUtils::findUnknownBoundLiteral(&formula3, NULL, &translator, context);
	ASSERT_TRUE(predform3==NULL);
}*/
