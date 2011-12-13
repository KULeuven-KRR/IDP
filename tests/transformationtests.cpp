#include "gtest/gtest.h"
#include "cppinterface.hpp"

#include "common.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "utils/TheoryUtils.hpp"

//TODO Remove following includes
//#include <iostream>

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

	// Rewrite (F(0) = 0) to (F(0,0))
	auto result = FormulaUtils::graphFuncsAndAggs(&eqf00);

	ASSERT_TRUE(sametypeid<PredForm>(*result));
	auto respredform = dynamic_cast<PredForm*>(result);
	EXPECT_TRUE(sametypeid<Function>(*(respredform->symbol())));
	EXPECT_EQ(respredform->symbol()->name(),f.f()->name());

	delete result;
}

TEST(GraphFuncsAndAggsTest,OneAggTerm) {
	auto s = sort("X",-2,2);
	auto one = domainterm(s,1);
	auto xt = varterm(s);
	auto x = xt->var();
	auto p = pred("P",{s});

	Formula& px = p({x});

	auto setpx = new QuantSetExpr({x},&px,xt,SetParseInfo());
	auto sumterm = new AggTerm(setpx,AggFunction::SUM,TermParseInfo());

	Formula& eq0sumterm = (*one == *sumterm);

	// Rewrite (0 = sum{ x : P(x) : x })
	auto result = FormulaUtils::graphFuncsAndAggs(&eq0sumterm);

	ASSERT_TRUE(sametypeid<AggForm>(*result));
	auto resaggform = dynamic_cast<AggForm*>(result);
	EXPECT_EQ(resaggform->left(),one);
	EXPECT_EQ(resaggform->comp(),CompType::EQ);
	EXPECT_EQ(resaggform->right(),sumterm);

	delete result;
}

TEST(GraphFuncsAndAggsTest,TwoFuncTerms) {
	auto s = sort("X",-2,2); //TODO isa int
	auto x = var(s);
	auto y = var(s);
	auto f = func("F",{s},s);
	auto g = func("G",{s},s);

	Formula& eqf0g0 = (f({x}) == g({y}));

	// Rewrite (F(x) = G(y)) to (! z : G(y) = z => F(x) = z) 
	//std::clog << "Transforming " << toString(&eqf0g0) << "\n";
	auto result = FormulaUtils::graphFuncsAndAggs(&eqf0g0);
	//std::clog << "Resulted in " << toString(result) << "\n";

	EXPECT_TRUE(sametypeid<QuantForm>(*result));
	EXPECT_EQ(result->subformulas().size(),1);
	auto ressubformula = result->subformulas()[0];
	EXPECT_TRUE(sametypeid<BoolForm>(*ressubformula));
	EXPECT_EQ(ressubformula->subformulas().size(),2);
	//for(size_t n = 0; n < ressubformula->subformulas().size(); ++n) {
	//	ASSERT_TRUE(sametypeid<PredForm>(*(ressubformula->subformulas()[n])));
	//	auto respredform = dynamic_cast<PredForm*>(result);
	//	EXPECT_TRUE(sametypeid<Function>(*(respredform->symbol())));
	//}

	delete result;
}

//TEST(GraphFuncsAndAggsTest,TwoAggTerm) {
//TODO
//}

//TEST(GraphFuncsAndAggsTest,FuncTermAndAggTerm) {
//TODO
//}

// PushNegations - theory
//TODO

// PushQuantifications - theory
//TODO

// RemoveEquivalences - formula,theory
//TODO

// SplitComparisonChains - formula,theory
//TODO

// SplitIntoMonotoneAgg
//TODO

// SplitProducts - formula,theory
//TODO

// SubstituteTerm - formula
//TODO

// UnnestPartialTerms
//TODO

// UnnestTerms - formula,theory
//TODO

// UnnestThreeValuedTerms - formula,rule
//TODO


}
