/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include <cmath>
#include <map>

#include "gtest/gtest.h"
#include "external/runidp.hpp"
#include "IncludeComponents.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "inferences/grounding/LazyGroundingManager.hpp"
#include "options.hpp"
#include "inferences/grounding/grounders/FormulaGrounders.hpp"
#include "testingtools.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"
#include "fobdds/FoBddManager.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"

using namespace std;

namespace Tests {

Grounder* getGrounder(Theory& t, Structure* s) {
	auto d=DataManager();
	bool LUP = getOption(BoolType::LIFTEDUNITPROPAGATION);
	bool propagate = LUP || getOption(BoolType::GROUNDWITHBOUNDS); //OK TO GET OPTION HERE?
	auto gddatb = generateBounds(&t, s, propagate, LUP);
	auto topgrounder = dynamic_cast<LazyGroundingManager*>((GrounderFactory::create({&t, {s, gddatb}, NULL, true})));
	if(topgrounder==NULL){
		cerr <<"NULL topgrounder\n";
		throw IdpException("Error");
	}
	auto grounder = topgrounder->getFirstSubGrounder();
	if(grounder==NULL){
		cerr <<"Empty topgrounder\n";
		throw IdpException("Error");
	}
	return grounder;
}

TEST(Grounderfactory, DisjContext) {
	auto ts = getTestingSet1();

	auto t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.p0vq0);
	auto context = getGrounder(t, ts.structure)->getContext();
	ASSERT_FALSE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
}
TEST(Grounderfactory, EquivContext) {
	auto ts = getTestingSet1();
	auto t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.np0iffq0);
	auto context = getGrounder(t, ts.structure)->getContext();
	ASSERT_FALSE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
}
TEST(Grounderfactory, ForallContext) {
	auto ts = getTestingSet1();
	auto t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.Axpx);
	auto qg = dynamic_cast<QuantGrounder*>(getGrounder(t, ts.structure));
	auto context = qg->getContext();
	ASSERT_TRUE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	context = qg->getSubGrounder()->getContext();
	//ASSERT_TRUE(CompContext::FORMULA==context._component); removed this because of the conjpathfromroot simplifications TODO: rethink about this
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	ASSERT_TRUE(GenType::CANMAKEFALSE==context.gentype);
}
TEST(Grounderfactory, NegForallContext) {
	auto ts = getTestingSet1();
	auto t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.nAxpx);
	auto qg = dynamic_cast<QuantGrounder*>(getGrounder(t, ts.structure));
	auto context = qg->getContext();
	ASSERT_FALSE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	context = qg->getSubGrounder()->getContext();
	//ASSERT_TRUE(CompContext::FORMULA==context._component); removed this because of the conjpathfromroot simplifications TODO: rethink about this
	ASSERT_FALSE(context._conjunctivePathFromRoot);
	ASSERT_TRUE(GenType::CANMAKETRUE==context.gentype);
}
TEST(Grounderfactory, NegExistsContext) {
	auto ts = getTestingSet1();
	auto t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.nExqx);
	auto qg = dynamic_cast<QuantGrounder*>(getGrounder(t, ts.structure));
	auto context = qg->getContext();
	ASSERT_TRUE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	context = qg->getSubGrounder()->getContext();
	//ASSERT_TRUE(CompContext::FORMULA==context._component); removed this because of the conjpathfromroot simplifications TODO: rethink about this
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	ASSERT_TRUE(GenType::CANMAKEFALSE==context.gentype);
}
TEST(Grounderfactory, EqChainContext) {
	auto ts = getTestingSet1();
	auto t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.xF);
	auto context = getGrounder(t, ts.structure)->getContext();
	ASSERT_TRUE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
}
TEST(Grounderfactory, AggContext) {
	auto ts = getTestingSet1();
	auto t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.maxxpxgeq0);
	auto context = getGrounder(t, ts.structure)->getContext();
	ASSERT_TRUE(context._conjunctivePathFromRoot);
}

//TODO: test definitions (and fixpdefinitions)

TEST(Grounderfactory, BoolFormContext) {
	auto ts = getTestingSet1();

	auto theory = Theory("T", ts.vocabulary, ParseInfo());
	theory.add(new BoolForm(SIGN::POS, false, { ts.Axpx }, FormulaParseInfo()));
	auto grounder = getGrounder(theory, ts.structure);
	auto context = grounder->getContext();
	ASSERT_TRUE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	ASSERT_TRUE(dynamic_cast<QuantGrounder*>(grounder)!=NULL);
}

}
