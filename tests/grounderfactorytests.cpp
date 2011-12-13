/*
 * Copyright 2007-2011 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat and Maarten Mariën, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <cmath>
#include <map>


#include "gtest/gtest.h"
#include "rungidl.hpp"
#include "structure.hpp"
#include <iostream>
#include "inferences/grounding/GrounderFactory.hpp"
#include "options.hpp"
#include "inferences/grounding/grounders/FormulaGrounders.hpp"
#include "vocabulary.hpp"
#include "theory.hpp"
#include "term.hpp"
#include "IdpException.hpp"
#include "testingtools.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"
#include "fobdds/FoBddManager.hpp"


using namespace std;

namespace Tests {
TEST(Grounderfactory, Context) {
	TestingSet1 ts = getTestingSet1();


	auto options = new Options("opt", ParseInfo());
	//TODO: move the following line to the constructor of GrounderFactory for in case no GenerateBDD... is given?
	auto gddatb = new GenerateBDDAccordingToBounds(new FOBDDManager(),*(new Bound()),  *(new Bound()),*(new std::map<PFSymbol*, std::vector<const FOBDDVariable*> >()));

	auto gf = new GrounderFactory(ts.s, gddatb);

	gf->InitContext();
	auto t = new Theory("T", ts.vocabulary, ParseInfo());
	t->add(ts.p0vq0);
	auto context = dynamic_cast<BoolGrounder*>((gf->create(t)))->getSubGrounders().at(0)->context();
	ASSERT_TRUE(CompContext::SENTENCE == context._component);
	//FIXME: ASSERTEQ didn't work since there is no tostring for enum classes
	ASSERT_FALSE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	delete t;

	gf->InitContext();
	t = new Theory("T", ts.vocabulary, ParseInfo());
	t->add(ts.np0iffq0);
	context = dynamic_cast<BoolGrounder*>((gf->create(t)))->getSubGrounders().at(0)->context();
	ASSERT_TRUE(CompContext::SENTENCE == context._component);
	ASSERT_FALSE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	delete t;

	gf->InitContext();
	t = new Theory("T", ts.vocabulary, ParseInfo());
	t->add(ts.Axpx);
	QuantGrounder* qg = dynamic_cast<QuantGrounder*>(dynamic_cast<BoolGrounder*>((gf->create(t)))->getSubGrounders().at(0));
	context = qg->context();
	ASSERT_TRUE(CompContext::SENTENCE==context._component);
	ASSERT_TRUE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	context = qg->getSubGrounder()->context();
	//ASSERT_TRUE(CompContext::FORMULA==context._component); removed this because of the conjpathfromroot simplifications TODO: rethink about this
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	ASSERT_TRUE(GenType::CANMAKEFALSE==context.gentype);
	delete t;

	gf->InitContext();
	t = new Theory("T", ts.vocabulary, ParseInfo());
	t->add(ts.nAxpx);
	qg = dynamic_cast<QuantGrounder*>(dynamic_cast<BoolGrounder*>((gf->create(t)))->getSubGrounders().at(0));
	context = qg->context();
	ASSERT_TRUE(CompContext::SENTENCE==context._component);
	ASSERT_FALSE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	context = qg->getSubGrounder()->context();
	//ASSERT_TRUE(CompContext::FORMULA==context._component); removed this because of the conjpathfromroot simplifications TODO: rethink about this
	ASSERT_FALSE(context._conjunctivePathFromRoot);
	ASSERT_TRUE(GenType::CANMAKETRUE==context.gentype);
	delete t;

	gf->InitContext();
	t = new Theory("T", ts.vocabulary, ParseInfo());
	t->add(ts.nExqx);
	qg = dynamic_cast<QuantGrounder*>(dynamic_cast<BoolGrounder*>((gf->create(t)))->getSubGrounders().at(0));
	context = qg->context();
	ASSERT_TRUE(CompContext::SENTENCE==context._component);
	ASSERT_TRUE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	context = qg->getSubGrounder()->context();
	//ASSERT_TRUE(CompContext::FORMULA==context._component); removed this because of the conjpathfromroot simplifications TODO: rethink about this
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	ASSERT_TRUE(GenType::CANMAKEFALSE==context.gentype);
	delete t;

	gf->InitContext();
	t = new Theory("T", ts.vocabulary, ParseInfo());
	t->add(ts.xF);
	context = dynamic_cast<BoolGrounder*>((gf->create(t)))->getSubGrounders().at(0)->context();
	ASSERT_TRUE(CompContext::SENTENCE==context._component);
	ASSERT_TRUE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	delete t;

	gf->InitContext();
	t = new Theory("T", ts.vocabulary, ParseInfo());
	t->add(ts.maxxpxgeq0);
	context = dynamic_cast<BoolGrounder*>((gf->create(t)))->getSubGrounders().at(0)->context();
	ASSERT_TRUE(CompContext::SENTENCE==context._component);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	delete t;

	//TODO: test definitions (and fixpdefinitions)


	delete options;
	delete gf;

	//clean(ts);

}

}
