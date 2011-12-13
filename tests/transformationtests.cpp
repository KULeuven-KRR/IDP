#include "gtest/gtest.h"
//#include "testingtools.hpp"

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
	auto p = new Predicate("P",{});
	auto q = new Predicate("Q",{});
	auto r = new Predicate("R",{});

	auto pform = new PredForm(SIGN::POS,p,{},FormulaParseInfo());
	auto qform = new PredForm(SIGN::POS,q,{},FormulaParseInfo());
	auto rform = new PredForm(SIGN::POS,r,{},FormulaParseInfo());

	auto qvr = new BoolForm(SIGN::POS,false,qform,rform,FormulaParseInfo());
	auto pvqvr = new BoolForm(SIGN::POS,false,pform,qvr,FormulaParseInfo());

	// Flattening (P | (Q | R)) to (P | Q | R).
	auto result = FormulaUtils::flatten(pvqvr);

	EXPECT_TRUE(sametypeid<BoolForm>(*result));
	EXPECT_EQ(result->subformulas().size(),3);

	delete result; //TODO Does this delete all of the other elements as well?
}

TEST(FlattenTest,QuantForm) {
	auto sorttable = new SortTable(new IntRangeInternalSortTable(-2,2));
	auto sort = new Sort("sort",sorttable);

	auto x = new Variable(sort);
	auto y = new Variable(sort);
	auto xterm = new VarTerm(x,TermParseInfo());
	auto yterm = new VarTerm(y,TermParseInfo());

	auto p = new Predicate("P",{ sort,sort });
	auto pxy = new PredForm(SIGN::POS,p,{ xterm,yterm },FormulaParseInfo());

	auto aypxy = new QuantForm(SIGN::POS,QUANT::UNIV,{ y },pxy,FormulaParseInfo());
	auto axaypxy = new QuantForm(SIGN::POS,QUANT::UNIV,{ x },aypxy,FormulaParseInfo());

	// Flattening (! x : ! y : P(x,y)) to (! x y : P(x,y)).
	auto result = FormulaUtils::flatten(axaypxy);

	EXPECT_TRUE(sametypeid<QuantForm>(*result));
	EXPECT_EQ(result->quantVars().size(),2);

	delete result;
}

TEST(FlattenTest,Theory) {
	auto sorttable = new SortTable(new IntRangeInternalSortTable(-2,2));
	auto sort = new Sort("sort",sorttable);

	auto x = new Variable(sort);
	auto xterm = new VarTerm(x,TermParseInfo());
	auto y = new Variable(sort);
	auto yterm = new VarTerm(y,TermParseInfo());

	auto p = new Predicate("P",{ sort,sort });
	auto pxy = new PredForm(SIGN::POS,p,{ xterm,yterm },FormulaParseInfo());
	auto q = new Predicate("Q",{ sort,sort });
	auto qxy = new PredForm(SIGN::POS,q,{ xterm,yterm },FormulaParseInfo());
	auto r = new Predicate("R",{ sort,sort });
	auto rxy = new PredForm(SIGN::POS,r,{ xterm,yterm },FormulaParseInfo());

	auto qvr = new BoolForm(SIGN::POS,false,qxy,rxy,FormulaParseInfo());
	auto pvqvr = new BoolForm(SIGN::POS,false,pxy,qvr,FormulaParseInfo());
	auto aypvqvr = new QuantForm(SIGN::POS,QUANT::UNIV,{ y },pvqvr,FormulaParseInfo());
	auto axaypvqvr = new QuantForm(SIGN::POS,QUANT::UNIV,{ x },aypvqvr,FormulaParseInfo());

	auto voc = new Vocabulary("V");
	voc->add(sort);
	voc->add(p);
	voc->add(q);
	voc->add(r);

	auto theory = new Theory("T",voc,ParseInfo());
	theory->add(axaypvqvr);

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
	auto sorttable = new SortTable(new IntRangeInternalSortTable(-2,2));
	auto sort = new Sort("sort",sorttable);

	auto null = DomainElementFactory::createGlobal()->create(0);
	auto nullterm = new DomainTerm(sort,null,TermParseInfo());

	auto f = new Function("F",{ sort },sort,ParseInfo());
	auto fterm = new FuncTerm(f,{ nullterm },TermParseInfo());

	auto eq = VocabularyUtils::equal(sort);
	auto eqf00 = new PredForm(SIGN::POS,eq,{ fterm,nullterm },FormulaParseInfo());

	// Rewrite (F(0) = 0) to (F(0,0))
	auto result = FormulaUtils::graphFuncsAndAggs(eqf00);

	ASSERT_TRUE(sametypeid<PredForm>(*result));
	auto respredform = dynamic_cast<PredForm*>(result);
	EXPECT_TRUE(sametypeid<Function>(*(respredform->symbol())));
	EXPECT_EQ(respredform->symbol()->name(),f->name());

	delete result;
}

TEST(GraphFuncsAndAggsTest,OneAggTerm) {
	auto sorttable = new SortTable(new IntRangeInternalSortTable(-2,2));
	auto sort = new Sort("sort",sorttable);

	auto null = DomainElementFactory::createGlobal()->create(0);
	auto nullterm = new DomainTerm(sort,null,TermParseInfo());

	auto x = new Variable(sort);
	auto xterm = new VarTerm(x,TermParseInfo());

	auto p = new Predicate("P",{ sort });
	auto px = new PredForm(SIGN::POS,p,{ xterm },FormulaParseInfo());

	auto setpx = new QuantSetExpr({ x },px,xterm,SetParseInfo());
	auto sumterm = new AggTerm(setpx,AggFunction::SUM,TermParseInfo());

	auto eq = VocabularyUtils::equal(sort);
	auto eq0sumterm = new PredForm(SIGN::POS,eq,{ nullterm,sumterm },FormulaParseInfo());

	// Rewrite (0 = sum{ x : P(x) : x })
	auto result = FormulaUtils::graphFuncsAndAggs(eq0sumterm);

	ASSERT_TRUE(sametypeid<AggForm>(*result));
	auto resaggform = dynamic_cast<AggForm*>(result);
	EXPECT_EQ(resaggform->left(),nullterm);
	EXPECT_EQ(resaggform->comp(),CompType::EQ);
	EXPECT_EQ(resaggform->right(),sumterm);

	delete result;
}

TEST(GraphFuncsAndAggsTest,TwoFuncTerms) {
	auto sorttable = new SortTable(new IntRangeInternalSortTable(-2,2));
	auto sort = new Sort("sort",sorttable);

	auto null = DomainElementFactory::createGlobal()->create(0);
	auto nullterm = new DomainTerm(sort,null,TermParseInfo());

	auto x = new Variable(sort);
	auto xterm = new VarTerm(x,TermParseInfo());
	auto y = new Variable(sort);
	auto yterm = new VarTerm(y,TermParseInfo());

	auto f = new Function("F",{ sort },sort,ParseInfo());
	auto fterm = new FuncTerm(f,{ xterm },TermParseInfo());
	auto g = new Function("G",{ sort },sort,ParseInfo());
	auto gterm = new FuncTerm(g,{ yterm },TermParseInfo());

	auto eq = VocabularyUtils::equal(sort);
	auto eqf0g0 = new PredForm(SIGN::POS,eq,{ fterm,gterm },FormulaParseInfo());

	// Rewrite (F(0) = G(0)) to (! x : 
	//std::clog << "Transforming " << toString(eqf0g0) << "\n";
	auto result = FormulaUtils::graphFuncsAndAggs(eqf0g0);
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
