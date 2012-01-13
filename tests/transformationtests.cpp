/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "gtest/gtest.h"
#include "cppinterface.hpp"

#include "common.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "utils/TheoryUtils.hpp"

namespace Tests {

// AddCompletion - theory
//TODO

// DeriveSorts - term,rule,formula
//TODO

// Flatten - formula,theory - boolform,quantform
TEST(FlattenTest,BoolForm) {
	auto p = pred("P",{});
	auto q = pred("Q",{});
	auto r = pred("R",{});

	Formula& pvqvr = p({}) | (q({}) | r({}));

	// Flattening (P | (Q | R)) to (P | Q | R).
	auto result = FormulaUtils::flatten(&pvqvr);

	EXPECT_TRUE(sametypeid<BoolForm>(*result));
	EXPECT_EQ(3,result->subformulas().size());

	result->recursiveDelete();
}

TEST(FlattenTest,QuantForm) {
	auto s = sort("S",-2,2);
	auto x = var(s);
	auto y = var(s);
	auto p = pred("P",{s,s});

	Formula& axaypxy = all(x, all(y, p({x,y})));

	// Flattening (! x : ! y : P(x,y)) to (! x y : P(x,y)).
	auto result = FormulaUtils::flatten(&axaypxy);

	EXPECT_TRUE(sametypeid<QuantForm>(*result));
	EXPECT_EQ(2,result->quantVars().size());

	result->recursiveDelete();
}

TEST(FlattenTest,Theory) {
	auto s = sort("S",-2,2);
	auto x = var(s);
	auto y = var(s);
	auto p = pred("P",{s,s});
	auto q = pred("Q",{s,s});
	auto r = pred("R",{s,s});

	auto voc = new Vocabulary("V");
	voc->add(s);
	voc->add(p.p());
	voc->add(q.p());
	voc->add(r.p());

	Formula& axaypvqvr = all(x, all(y, p({x,y}) | (q({x,y}) | r({x,y})) ));

	auto theory = new Theory("T",voc,ParseInfo());
	theory->add(&axaypvqvr);

	// Flattening (! x : ! y : P(x,y) | (Q(x,y) | R(x,y))) to (! x y : P(x,y) | Q(x,y) | R(x,y)).
	FormulaUtils::flatten(theory);

	ASSERT_EQ(1,theory->sentences().size());
	auto resformula = theory->sentences()[0];
	EXPECT_TRUE(sametypeid<QuantForm>(*resformula));
	EXPECT_EQ(2,resformula->quantVars().size());
	ASSERT_EQ(1,resformula->subformulas().size());
	auto ressubformula = resformula->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	EXPECT_EQ(3,ressubformula->subformulas().size());

	theory->recursiveDelete();
}

// GraphFuncsAndAggs - formula,theory
TEST(GraphFuncsAndAggsTest,OneFuncTerm) {
	auto s = sort("X",-2,2);
	auto one = domainterm(s,1);
	auto f = func("F",{s},s);

	Formula& eqf00 = (f({one}) == *one);

	// Rewriting (F(0) = 0) to (F(0,0))
	auto result = FormulaUtils::graphFuncsAndAggs(&eqf00);

	ASSERT_TRUE(sametypeid<PredForm>(*result));
	auto respredform = dynamic_cast<PredForm*>(result);
	EXPECT_TRUE(sametypeid<Function>(*(respredform->symbol())));
	EXPECT_EQ(f.f()->name(),respredform->symbol()->name());

	result->recursiveDelete();
}

TEST(GraphFuncsAndAggsTest,TwoFuncTerms) {
	auto s = sort("X",-2,2);
	auto f = func("F",{s},s);
	auto g = func("G",{s},s);
	auto x = var(s);
	auto y = var(s);

	Formula& eqf0g0 = (f({x}) == g({y}));

	// Rewriting (F(x) = G(y)) to (! z : ~G(y,z) | F(x,z)) 
	//std::clog << "Transforming " << toString(&eqf0g0) << "\n";
	auto result = FormulaUtils::graphFuncsAndAggs(&eqf0g0);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(sametypeid<QuantForm>(*result));
	ASSERT_EQ(1,result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	ASSERT_EQ(2,ressubformula->subformulas().size());
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[0]));
	auto subf1 = dynamic_cast<PredForm*>(ressubformula->subformulas()[0]);
	ASSERT_TRUE(sametypeid<Function>(*subf1->symbol()));
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[1]));
	auto subf2 = dynamic_cast<PredForm*>(ressubformula->subformulas()[1]);
	ASSERT_TRUE(sametypeid<Function>(*subf2->symbol()));

	result->recursiveDelete();
}

TEST(GraphFuncsAndAggsTest,OneAggTerm) {
	auto s = sort("X",-2,2);
	auto one = domainterm(s,1);
	auto p = pred("P",{s});
	auto xt = varterm(s);
	auto x = xt->var();

	auto sumterm = sum(qset({x},p({x}),xt));

	Formula& eq0sumterm = (*one == *sumterm);

	// Rewriting (0 = sum{ x : P(x) : x })
	auto result = FormulaUtils::graphFuncsAndAggs(&eq0sumterm);

	ASSERT_TRUE(sametypeid<AggForm>(*result));
	auto resaggform = dynamic_cast<AggForm*>(result);
	EXPECT_EQ(one,resaggform->left());
	EXPECT_EQ(CompType::EQ,resaggform->comp());
	EXPECT_EQ(sumterm,resaggform->right());

	result->recursiveDelete();
}

TEST(GraphFuncsAndAggsTest,TwoAggTerm) {
	auto s = sort("X",-2,2);
	auto p = pred("P",{s});
	auto q = pred("Q",{s});
	auto xt = varterm(s);
	auto x = xt->var();
	auto yt = varterm(s);
	auto y = yt->var();

	auto sumpx = sum(qset({x},p({x}),xt));
	auto sumqy = sum(qset({y},q({y}),yt));

	Formula& eqsumpxsumqy = (*sumpx == *sumqy);

	// Rewrite (sum{ x : P(x) : x } = sum{ y : Q(y) : y })
	//std::clog << "Transforming " << toString(&eqsumpxsumqy) << "\n";
	auto result = FormulaUtils::graphFuncsAndAggs(&eqsumpxsumqy);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(sametypeid<QuantForm>(*result));
	ASSERT_EQ(1,result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	ASSERT_EQ(2,ressubformula->subformulas().size());
	ASSERT_TRUE(sametypeid<AggForm>(*ressubformula->subformulas()[0]));
	ASSERT_TRUE(sametypeid<AggForm>(*ressubformula->subformulas()[1]));

	result->recursiveDelete();
}

TEST(GraphFuncsAndAggsTest,FuncTermAndAggTerm) {
	auto s = sort("X",-2,2);
	auto p = pred("P",{s});
	auto f = func("F",{s},s);
	auto xt = varterm(s);
	auto x = xt->var();
	auto y = var(s);

	auto sumpx = sum(qset({x},p({x}),xt));

	Formula& eqsumpxfy = (*sumpx == f({y}));

	// Rewrite (sum{ x : P(x) : x } = f(y))
	//std::clog << "Transforming " << toString(&eqsumpxfy) << "\n";
	auto result = FormulaUtils::graphFuncsAndAggs(&eqsumpxfy);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(sametypeid<QuantForm>(*result));
	ASSERT_EQ(1,result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	ASSERT_EQ(2,ressubformula->subformulas().size());
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[0]));
	auto subf = dynamic_cast<PredForm*>(ressubformula->subformulas()[0]);
	ASSERT_TRUE(sametypeid<Function>(*subf->symbol()));
	ASSERT_TRUE(sametypeid<AggForm>(*ressubformula->subformulas()[1]));

	result->recursiveDelete();
}

// PushNegations - theory
TEST(PushNegationsTest,BoolForm) {
	auto p = pred("P",{});
	auto q = pred("Q",{});

	Formula& bf = not (p({}) | q({}));

	// Rewriting ~(P | Q) to (~P & ~Q).
	//std::clog << "Transforming " << toString(&bf) << "\n";
	auto result = FormulaUtils::pushNegations(&bf);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(sametypeid<BoolForm>(*result));
	EXPECT_TRUE(isPos(result->sign()));
	ASSERT_EQ(2,result->subformulas().size());
	EXPECT_TRUE(isNeg(result->subformulas()[0]->sign()));
	EXPECT_TRUE(isNeg(result->subformulas()[1]->sign()));
	
	result->recursiveDelete();
}

TEST(PushNegationsTest,NestedBoolForm) {
	auto p = pred("P",{});
	auto q = pred("Q",{});
	auto r = pred("R",{});

	Formula& bf = not (p({}) | (q({}) & r({})));

	// Rewriting ~(P | (Q & R)) to (~P & (~Q | ~R))
	//std::clog << "Transforming " << toString(&bf) << "\n";
	auto result = FormulaUtils::pushNegations(&bf);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(sametypeid<BoolForm>(*result));
	EXPECT_TRUE(isPos(result->sign()));
	ASSERT_EQ(2,result->subformulas().size());
	EXPECT_TRUE(isNeg(result->subformulas()[0]->sign()));
	EXPECT_TRUE(isPos(result->subformulas()[1]->sign()));

	result->recursiveDelete();
}

TEST(PushNegationsTest,QuantForm) {
	auto s = sort("X",-2,2);
	auto x = var(s);
	auto p = pred("P",{s});

	Formula& qf = not all(x, p({x}));

	// Rewriting ~(! x : P(x)) to (? x : ~P(x))
	auto result = FormulaUtils::pushNegations(&qf);

	ASSERT_TRUE(sametypeid<QuantForm>(*result));
	EXPECT_TRUE(isPos(result->sign()));
	EXPECT_FALSE(dynamic_cast<QuantForm*>(result)->isUniv());
	ASSERT_EQ(1,result->subformulas().size());
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
	auto p = pred("P",{});
	auto q = pred("Q",{});
	auto r = pred("R",{});

	auto voc = new Vocabulary("V");
	add(voc,{p.p(),q.p(),r.p()});

	Formula& bf = not (p({}) | q({}));

	auto theory = new Theory("T",voc,ParseInfo());
	theory->add(&bf);

	FormulaUtils::pushNegations(theory);
	auto restheory = dynamic_cast<Theory*>(theory);
	ASSERT_EQ(1,restheory->sentences().size());
	auto resformula = restheory->sentences()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*resformula));
	ASSERT_EQ(2,resformula->subformulas().size());
	EXPECT_TRUE(isNeg(resformula->subformulas()[0]->sign()));
	EXPECT_TRUE(isNeg(resformula->subformulas()[1]->sign()));

	theory->recursiveDelete();
}

// PushQuantifiers - theory
TEST(PushQuantifiersTest,Theory) {
	auto s = sort("X",-2,2);
	auto p = pred("P",{s});
	auto q = pred("Q",{s});
	auto x = var(s);

	auto voc = new Vocabulary("V");
	voc->add(s);
	voc->add(p.p());
	voc->add(q.p());

	Formula& axpxqx = all(x,(p({x}) & q({x})));

	auto theory = new Theory("T",voc,ParseInfo());
	theory->add(&axpxqx);

	// Rewriting (! x : P(x) & Q(x)) to ((! x : P(x)) & (! x : Q(x))).
	auto result = FormulaUtils::pushQuantifiers(theory);

	ASSERT_TRUE(sametypeid<Theory>(*result));
	auto restheory = dynamic_cast<Theory*>(result);
	ASSERT_EQ(1,restheory->sentences().size());
	auto resformula = restheory->sentences()[0];
	ASSERT_TRUE(sametypeid<BoolForm>(*resformula));
	ASSERT_EQ(2,resformula->subformulas().size());
	ASSERT_TRUE(sametypeid<QuantForm>(*resformula->subformulas()[0]));
	ASSERT_TRUE(sametypeid<QuantForm>(*resformula->subformulas()[1]));

	result->recursiveDelete();
}

// RemoveEquivalences - formula,theory
TEST(RemoveEquivalencesTest,EquivForm) {
	auto p = pred("P",{});
	auto q = pred("Q",{});

	auto piffq = new EquivForm(SIGN::POS,&p({}),&q({}),FormulaParseInfo());

	// Rewriting (P <=> Q) to ((P => Q) & (Q => P)).
	auto result = FormulaUtils::removeEquivalences(piffq);

	ASSERT_TRUE(sametypeid<BoolForm>(*result));
	ASSERT_EQ(2,result->subformulas().size());

	result->recursiveDelete();
}

// SplitComparisonChains - formula,theory
TEST(SplitComparisonChainsTest,NormalEqChainForm) {
	auto s = sort("X",-2,2);
	auto xt = varterm(s);
	auto yt = varterm(s);
	auto zt = varterm(s);

	auto aisbisc = new EqChainForm(SIGN::POS,true,{xt,yt,zt},{CompType::EQ,CompType::EQ},FormulaParseInfo());

	// Rewriting (x = y = z) to ((x = y) & (y = z)).
	auto result = FormulaUtils::splitComparisonChains(aisbisc);

	ASSERT_TRUE(sametypeid<BoolForm>(*result));
	ASSERT_EQ(2,result->subformulas().size());

	result->recursiveDelete();
}

TEST(SplitComparisonChainsTest,WeirdEqChainForm) {
	auto s = sort("X",-2,2);
	auto xt = varterm(s);
	auto yt = varterm(s);
	auto zt = varterm(s);

	auto weird = new EqChainForm(SIGN::NEG,false,{xt,yt,zt},{CompType::LEQ,CompType::GT},FormulaParseInfo());

	// Rewriting ~((x =< y) | (y > z)).
	auto result = FormulaUtils::splitComparisonChains(weird);

	ASSERT_TRUE(sametypeid<BoolForm>(*result));
	ASSERT_EQ(2,result->subformulas().size());

	result->recursiveDelete();
}

// SplitIntoMonotoneAgg
//TODO

// SubstituteTerm - formula
TEST(SubstituteTermTest,Formula) {
	auto s = sort("X",-2,2);
	auto p = pred("P",{s});
	auto x = var(s);
	auto y = var(s);

	Formula& px = p({x});

	ASSERT_EQ(1,px.subterms().size());
	auto xt = px.subterms()[0];

	auto result = FormulaUtils::substituteTerm(&px,xt,y);

	ASSERT_EQ(1,result->subterms().size());
	auto subterm = result->subterms()[0];
	ASSERT_TRUE(sametypeid<VarTerm>(*subterm));
	ASSERT_EQ(y,dynamic_cast<VarTerm*>(subterm)->var());

	result->recursiveDelete();
}

// UnnestPartialTerms
//TODO

// UnnestTerms - formula,theory
TEST(UnnestTermsTest,TwoFuncTermsEQ) {
	auto s = sort("X",-2,2);
	auto f = func("F",{s},s);
	auto g = func("G",{s},s);
	auto x = var(s);

	Formula& eqfxgx = (f({x}) == g({x}));

	// Rewriting (F(x) = G(x)) to (! y : ~=(y,G(x)) | =(F(x),y)).
	//std::clog << "Transforming " << toString(&eqfxgx) << "\n";
	auto result = FormulaUtils::unnestTerms(&eqfxgx);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(sametypeid<QuantForm>(*result));
	ASSERT_EQ(1,result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	ASSERT_EQ(2,ressubformula->subformulas().size());
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[0]));
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[1]));

	result->recursiveDelete();
}

TEST(UnnestTermsTest,TwoFuncTermsLT) {
	auto s = sort("X",-2,2);
	auto f = func("F",{s},s);
	auto g = func("G",{s},s);
	auto x = var(s);

	Formula& ltfxgx = (f({x}) < g({x}));

	// Rewriting (F(x) < G(x)) to (! y z : ~=(y,F(x)) | ~=(z,G(x)) | <(y,z)).
	auto result = FormulaUtils::unnestTerms(&ltfxgx);

	EXPECT_TRUE(sametypeid<QuantForm>(*result));
	ASSERT_EQ(1,result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	ASSERT_EQ(3,ressubformula->subformulas().size());
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[0]));
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[1]));
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[2]));

	result->recursiveDelete();
}

TEST(UnnestTermsTest,TwoVarTermsEQ) {
	auto s = sort("X",-2,2);
	auto xt = varterm(s);
	auto yt = varterm(s);

	Formula& eqxy = (*xt == *yt);

	// Rewriting (x = y) to (x = y).
	auto result = FormulaUtils::unnestTerms(&eqxy);

	ASSERT_EQ(&eqxy,result);

	result->recursiveDelete();
}

TEST(UnnestTermsTest,NestedFuncTerms) {
	auto s = sort("X",-2,2);
	auto f = func("F",{s},s);
	auto g = func("G",{s},s);
	auto h = func("H",{s},s);
	auto x = var(s);

	Formula& eqfgxhx = (f({&g({x})}) == h({x}));

	//std::clog << "Transforming " << toString(&eqfgxhx) << "\n";
	auto result = FormulaUtils::unnestTerms(&eqfgxhx);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(sametypeid<QuantForm>(*result));
	ASSERT_EQ(1,result->subformulas().size());
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	ASSERT_EQ(3,ressubformula->subformulas().size());
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[0]));
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[1]));
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[2]));

	result->recursiveDelete();
}

//TEST(UnnestTermsTest,NestedAggTerms) {
//	//TODO
//}

// UnnestThreeValuedTerms - formula,rule
//TODO


} /* namespace Tests */
