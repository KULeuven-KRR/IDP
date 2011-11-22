/*
 * Copyright 2007-2011 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat and Maarten Mariën, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <cmath>

#include "gtest/gtest.h"
#include "rungidl.hpp"
#include "structure.hpp"
#include <iostream>
#include "ground.hpp"
#include "options.hpp"
#include "grounders/FormulaGrounders.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "IdpException.hpp"

using namespace std;


namespace Tests{
	TEST(Grounderfactory, Context){
		//CREATE SOME TESTING TOOLS
		;
		auto sorttable = new SortTable(new IntRangeInternalSortTable(-2, 2)); //[-2,2]
		auto sort = new Sort("sort", sorttable); // sort  [-2,2]
		auto x = new Variable(sort); // variable of [-2,2]
		auto sortterm = new VarTerm(x, TermParseInfo()); //varterm of [-2,2]
		auto nul = DomainElementFactory::createGlobal()->create(0);
		auto nulterm = new DomainTerm(sort, nul,TermParseInfo());
		auto p = new Predicate("P", {sort}, false);  //predicate P of [-2,2]
		auto q = new Predicate("Q", {sort}, false);  //predicate Q of [-2,2]
		auto vocabulary = new Vocabulary("V");
		vocabulary->add(sort);
		vocabulary->add(p);
		vocabulary->add(q);
		auto s = new Structure("S", ParseInfo());
		s->vocabulary(vocabulary);


		auto px = new PredForm(SIGN::POS, p, {sortterm}, FormulaParseInfo()); //P(x)
		auto qx = new PredForm(SIGN::POS, q, {sortterm}, FormulaParseInfo()); //Q(x)
		auto p0 = new PredForm(SIGN::POS, p, {nulterm}, FormulaParseInfo()); //P(0)
		auto q0 = new PredForm(SIGN::POS, q, {nulterm}, FormulaParseInfo()); //Q(0)
		auto xpx = new QuantSetExpr({x},px,sortterm,SetParseInfo()); //{x|p(x)}
		auto maxxpx = new AggTerm(xpx ,AggFunction::MAX,TermParseInfo()); //MAX{x|p(x)}

		auto np0iffq0 = new EquivForm(SIGN::NEG,p0,q0,FormulaParseInfo()); // ~(P(0) <=> Q(0))
		auto p0vq0 = new BoolForm(SIGN::POS,false,p0,q0,FormulaParseInfo()); //P(0) | Q(0)
		auto Axpx = new QuantForm(SIGN::POS,QUANT::UNIV,{x},px,FormulaParseInfo()); // !x: P(x)
		auto nAxpx = new QuantForm(SIGN::NEG,QUANT::UNIV,{x},px,FormulaParseInfo()); // ~!x: P(x)
		auto nExqx = new QuantForm(SIGN::POS,QUANT::EXIST,{x},qx,FormulaParseInfo()); // ?x: Q(x)
		auto xF = new EqChainForm(SIGN::POS,true,sortterm,FormulaParseInfo()); // x This is false (empty conjuction)
		auto maxxpxgeq0 = new AggForm(SIGN::POS,nulterm, CompType::LEQ,maxxpx,FormulaParseInfo());// MAX{x|P(x)} >= 0

		//TODO: definitions (and fixpdefinitions)

//
		//TEST
		auto options = new Options("opt", ParseInfo());
		auto gf = new GrounderFactory(s,options);



//		gf->InitContext();
//		auto t = new Theory("T",vocabulary,ParseInfo());
//		t->add(p0vq0);
//		auto context = dynamic_cast<BoolGrounder*>((gf->create(t)))->getSubGrounders().at(0)->context();
////		auto context = gf->getContext();
//		ASSERT_TRUE(CompContext::SENTENCE == context._component);//FIXME: ASSERTEQ didn't work since there is no tostring for enum classes
//		ASSERT_FALSE(context._conjPathUntilNode);
//		ASSERT_TRUE(context._conjunctivePathFromRoot);
//		delete t;

//		gf->InitContext();
//		t = new Theory("T",vocabulary,ParseInfo());
//		t->add(np0iffq0);
//		context = dynamic_cast<BoolGrounder*>((gf->create(t)))->getSubGrounders().at(0)->context();
//		ASSERT_TRUE(CompContext::SENTENCE == context._component);
//		ASSERT_FALSE(context._conjPathUntilNode);
//		ASSERT_TRUE(context._conjunctivePathFromRoot);
//		delete t;
////
//
//		gf->InitContext();
//		gf->visit(Axpx);
//		context = gf->getContext();
//		ASSERT_TRUE(CompContext::SENTENCE==context._component);
//		ASSERT_TRUE(context._conjPathUntilNode);
//		ASSERT_TRUE(context._conjunctivePathFromRoot);
//		context = dynamic_cast<QuantGrounder*>(gf->getFormGrounder())->getSubGrounder()->context();
//		ASSERT_TRUE(CompContext::FORMULA==context._component);
//		ASSERT_TRUE(context._conjunctivePathFromRoot);
//		ASSERT_TRUE(GenType::CANMAKEFALSE==context.gentype);
//
//		gf->InitContext();
//		gf->visit(nAxpx);
//		context = gf->getContext();
//		ASSERT_TRUE(CompContext::SENTENCE==context._component);
//		ASSERT_FALSE(context._conjPathUntilNode);
//		ASSERT_TRUE(context._conjunctivePathFromRoot);
//		context = dynamic_cast<QuantGrounder*>(gf->getFormGrounder())->getSubGrounder()->context();
//		ASSERT_TRUE(CompContext::FORMULA==context._component);
//		ASSERT_FALSE(context._conjunctivePathFromRoot);
//		ASSERT_TRUE(GenType::CANMAKETRUE==context.gentype);
//
//
//		gf->InitContext();
//		gf->visit(nExqx);
//		context = gf->getContext();
//		ASSERT_TRUE(CompContext::SENTENCE==context._component);
//		ASSERT_TRUE(context._conjPathUntilNode);
//		ASSERT_TRUE(context._conjunctivePathFromRoot);
//		context = dynamic_cast<QuantGrounder*>(gf->getFormGrounder())->getSubGrounder()->context();
//		ASSERT_TRUE(CompContext::FORMULA==context._component);
//		ASSERT_TRUE(context._conjunctivePathFromRoot);
//		ASSERT_TRUE(GenType::CANMAKEFALSE==context.gentype);
//
//
//		gf->InitContext();
//		gf->visit(xF);
//		context = gf->getContext();
//		ASSERT_TRUE(CompContext::SENTENCE==context._component);
//		ASSERT_TRUE(context._conjPathUntilNode);
//		ASSERT_TRUE(context._conjunctivePathFromRoot);
//
//		gf->InitContext();
//		gf->visit(maxxpxgeq0);
//		context = gf->getContext();
//		ASSERT_TRUE(CompContext::SENTENCE==context._component);
//		ASSERT_TRUE(context._conjunctivePathFromRoot);

//		//CLEAN UP
//		delete sorttable;//new SortTable(new IntRangeInternalSortTable(-2, 2)); //[-2,2]
//		//delete sort; Should not be done, will be deleted when it has no more vocabularies
//		delete x;//new Variable(sort); // variable of [-2,2]
//		delete sortterm;//new VarTerm(x, TermParseInfo()); //varterm of [-2,2]
//		delete nul;//new DomainElement(0);
//		delete nulterm;//new DomainTerm(sort, nul,TermParseInfo());
//		delete p;//new Predicate("P", {sort}, false);  //predicate P of [-2,2]
//		delete q;//new Predicate("Q", {sort}, false);  //predicate Q of [-2,2]
//		delete vocabulary;//new Vocabulary("V");
//
//		delete s;//new Structure("S", ParseInfo());
//		delete options;//new Options("opt", ParseInfo());
//		delete gf;//new GrounderFactory(s,options);
//
//		delete px;//new PredForm(SIGN::POS, p, {sortterm}, FormulaParseInfo()); //P(x)
//		delete qx;//new PredForm(SIGN::POS, q, {sortterm}, FormulaParseInfo()); //Q(x)
//		delete p0;//new PredForm(SIGN::POS, p, {nulterm}, FormulaParseInfo()); //P(0)
//		delete q0;//new PredForm(SIGN::POS, q, {nulterm}, FormulaParseInfo()); //Q(0)
//		delete xpx;//new QuantSetExpr({x},px,sortterm,SetParseInfo()); //{x|p(x)}
//		delete maxxpx;//new AggTerm(xpx ,AggFunction::MAX,TermParseInfo()); //MAX{x|p(x)}
//
//		delete np0iffq0;//new EquivForm(SIGN::NEG,p0,q0,FormulaParseInfo()); // ~(P(0) <=> Q(0))
//		delete p0vq0;//new BoolForm(SIGN::POS,true,p0,q0,FormulaParseInfo()); //P(0) | Q(0)
//		delete Axpx;//new QuantForm(SIGN::POS,QUANT::UNIV,{x},px,FormulaParseInfo()); // !x: P(x)
//		delete nAxpx;//new QuantForm(SIGN::NEG,QUANT::UNIV,{x},px,FormulaParseInfo()); // ~!x: P(x)
//		delete nExqx;//new QuantForm(SIGN::POS,QUANT::EXIST,{x},qx,FormulaParseInfo()); // ?x: Q(x)
//		delete xF;//new EqChainForm(SIGN::POS,true,sortterm,FormulaParseInfo()); // x This is false (empty conjuction)
//		delete maxxpxgeq0;//new AggForm(SIGN::POS,nulterm, CompType::LEQ,maxxpx,FormulaParseInfo());// MAX{x|P(x)} >= 0


	}

}
