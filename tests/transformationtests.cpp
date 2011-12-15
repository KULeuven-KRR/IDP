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
	EXPECT_EQ(result->subformulas().size(),3);

	delete result; //TODO Does this delete all of the other elements as well?
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
	EXPECT_EQ(result->quantVars().size(),2);

	delete result;
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
	auto result = FormulaUtils::flatten(theory);

	ASSERT_TRUE(sametypeid<Theory>(*result));
	auto restheory = dynamic_cast<Theory*>(result);
	ASSERT_EQ(restheory->sentences().size(),1);
	auto resformula = restheory->sentences()[0];
	EXPECT_TRUE(sametypeid<QuantForm>(*resformula));
	EXPECT_EQ(resformula->quantVars().size(),2);
	ASSERT_EQ(resformula->subformulas().size(),1);
	auto ressubformula = resformula->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	EXPECT_EQ(ressubformula->subformulas().size(),3);

	delete result;
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
	EXPECT_EQ(respredform->symbol()->name(),f.f()->name());

	delete result;
}

TEST(GraphFuncsAndAggsTest,TwoFuncTerms) {
	auto s = sort("X",-2,2);
	auto f = func("F",{s},s);
	auto g = func("G",{s},s);
	auto x = var(s);
	auto y = var(s);

	Formula& eqf0g0 = (f({x}) == g({y}));

	// Rewriting (F(x) = G(y)) to (! z : G(y) = z => F(x) = z) 
	//std::clog << "Transforming " << toString(&eqf0g0) << "\n";
	auto result = FormulaUtils::graphFuncsAndAggs(&eqf0g0);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(sametypeid<QuantForm>(*result));
	ASSERT_EQ(result->subformulas().size(),1);
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	ASSERT_EQ(ressubformula->subformulas().size(),2);
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[0]));
	auto subf1 = dynamic_cast<PredForm*>(ressubformula->subformulas()[0]);
	ASSERT_TRUE(sametypeid<Function>(*subf1->symbol()));
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[1]));
	auto subf2 = dynamic_cast<PredForm*>(ressubformula->subformulas()[1]);
	ASSERT_TRUE(sametypeid<Function>(*subf2->symbol()));

	delete result;
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
	EXPECT_EQ(resaggform->left(),one);
	EXPECT_EQ(resaggform->comp(),CompType::EQ);
	EXPECT_EQ(resaggform->right(),sumterm);

	delete result;
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
	ASSERT_EQ(result->subformulas().size(),1);
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	ASSERT_EQ(ressubformula->subformulas().size(),2);
	ASSERT_TRUE(sametypeid<AggForm>(*ressubformula->subformulas()[0]));
	ASSERT_TRUE(sametypeid<AggForm>(*ressubformula->subformulas()[1]));

	delete result;
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
	ASSERT_EQ(result->subformulas().size(),1);
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	ASSERT_EQ(ressubformula->subformulas().size(),2);
	ASSERT_TRUE(sametypeid<PredForm>(*ressubformula->subformulas()[0]));
	auto subf = dynamic_cast<PredForm*>(ressubformula->subformulas()[0]);
	ASSERT_TRUE(sametypeid<Function>(*subf->symbol()));
	ASSERT_TRUE(sametypeid<AggForm>(*ressubformula->subformulas()[1]));

	delete result;
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
	ASSERT_EQ(result->subformulas().size(),2);
	EXPECT_TRUE(isNeg(result->subformulas()[0]->sign()));
	EXPECT_TRUE(isNeg(result->subformulas()[1]->sign()));
	
	delete result;
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
	ASSERT_EQ(result->subformulas().size(),2);
	EXPECT_TRUE(isNeg(result->subformulas()[0]->sign()));
	EXPECT_TRUE(isPos(result->subformulas()[1]->sign()));

	delete result;
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
	ASSERT_EQ(result->subformulas().size(),1);
	EXPECT_TRUE(isNeg(result->subformulas()[0]->sign()));

	delete result;
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

	auto result = FormulaUtils::pushNegations(theory);

	ASSERT_TRUE(sametypeid<Theory>(*result));
	auto restheory = dynamic_cast<Theory*>(result);
	ASSERT_EQ(restheory->sentences().size(),1);
	auto resformula = restheory->sentences()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*resformula));
	ASSERT_EQ(resformula->subformulas().size(),2);
	EXPECT_TRUE(isNeg(resformula->subformulas()[0]->sign()));
	EXPECT_TRUE(isNeg(resformula->subformulas()[1]->sign()));

	delete result;
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
	ASSERT_EQ(restheory->sentences().size(),1);
	auto resformula = restheory->sentences()[0];
	ASSERT_TRUE(sametypeid<BoolForm>(*resformula));
	ASSERT_EQ(resformula->subformulas().size(),2);
	ASSERT_TRUE(sametypeid<QuantForm>(*resformula->subformulas()[0]));
	ASSERT_TRUE(sametypeid<QuantForm>(*resformula->subformulas()[1]));

	delete result;
}

// RemoveEquivalences - formula,theory
TEST(RemoveEquivalencesTest,EquivForm) {
	auto p = pred("P",{});
	auto q = pred("Q",{});

	auto piffq = new EquivForm(SIGN::POS,&p({}),&q({}),FormulaParseInfo());

	// Rewriting (P <=> Q) to ((P => Q) & (Q => P)).
	auto result = FormulaUtils::removeEquivalences(piffq);

	ASSERT_TRUE(sametypeid<BoolForm>(*result));
	ASSERT_EQ(result->subformulas().size(),2);

	delete result;
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
	ASSERT_EQ(result->subformulas().size(),2);

	delete result;
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
	ASSERT_EQ(result->subformulas().size(),2);

	delete result;
}

// SplitIntoMonotoneAgg
//TODO

// SplitProducts - formula,theory
//TODO

// SubstituteTerm - formula
TEST(SubstituteTermTest,Formula) {
	auto s = sort("X",-2,2);
	auto p = pred("P",{s});
	auto x = var(s);
	auto y = var(s);

	Formula& px = p({x});

	ASSERT_EQ(px.subterms().size(),1);
	auto xt = px.subterms()[0];

	auto result = FormulaUtils::substituteTerm(&px,xt,y);

	ASSERT_EQ(result->subterms().size(),1);
	auto subterm = result->subterms()[0];
	ASSERT_TRUE(sametypeid<VarTerm>(*subterm));
	ASSERT_EQ(dynamic_cast<VarTerm*>(subterm)->var(),y);

	delete result;
}

// UnnestPartialTerms
//TODO

// UnnestTerms - formula,theory
//TODO

// UnnestThreeValuedTerms - formula,rule
//TODO


}
