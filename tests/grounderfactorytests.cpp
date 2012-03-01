/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include <cmath>
#include <map>

#include "gtest/gtest.h"
#include "external/rungidl.hpp"
#include "IncludeComponents.hpp"
#include "inferences/grounding/GrounderFactory.hpp"
#include "options.hpp"
#include "inferences/grounding/grounders/FormulaGrounders.hpp"
#include "testingtools.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"
#include "fobdds/FoBddManager.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"

using namespace std;

namespace Tests {

Grounder* getBoolGrounder(Theory& t, AbstractStructure* s){
	auto gddatb = generateNaiveApproxBounds(&t, s);
	return dynamic_cast<BoolGrounder*>((GrounderFactory::create({&t, s, gddatb})))->getSubGrounders().at(0);
}

TEST(Grounderfactory, Context) {
	TestingSet1 ts = getTestingSet1();

	auto t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.p0vq0);
	auto context = getBoolGrounder(t, ts.s)->context();
	ASSERT_TRUE(CompContext::SENTENCE == context._component);
	//FIXME: ASSERTEQ didn't work since there is no tostring for enum classes
	ASSERT_FALSE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);

	t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.np0iffq0);
	context = getBoolGrounder(t, ts.s)->context();
	ASSERT_TRUE(CompContext::SENTENCE == context._component);
	ASSERT_FALSE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);

	t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.Axpx);
	QuantGrounder* qg = dynamic_cast<QuantGrounder*>(getBoolGrounder(t, ts.s));
	context = qg->context();
	ASSERT_TRUE(CompContext::SENTENCE==context._component);
	ASSERT_TRUE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	context = qg->getSubGrounder()->context();
	//ASSERT_TRUE(CompContext::FORMULA==context._component); removed this because of the conjpathfromroot simplifications TODO: rethink about this
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	ASSERT_TRUE(GenType::CANMAKEFALSE==context.gentype);

	t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.nAxpx);
	qg = dynamic_cast<QuantGrounder*>(getBoolGrounder(t, ts.s));
	context = qg->context();
	ASSERT_TRUE(CompContext::SENTENCE==context._component);
	ASSERT_FALSE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	context = qg->getSubGrounder()->context();
	//ASSERT_TRUE(CompContext::FORMULA==context._component); removed this because of the conjpathfromroot simplifications TODO: rethink about this
	ASSERT_FALSE(context._conjunctivePathFromRoot);
	ASSERT_TRUE(GenType::CANMAKETRUE==context.gentype);

	t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.nExqx);
	qg = dynamic_cast<QuantGrounder*>(getBoolGrounder(t, ts.s));
	context = qg->context();
	ASSERT_TRUE(CompContext::SENTENCE==context._component);
	ASSERT_TRUE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	context = qg->getSubGrounder()->context();
	//ASSERT_TRUE(CompContext::FORMULA==context._component); removed this because of the conjpathfromroot simplifications TODO: rethink about this
	ASSERT_TRUE(context._conjunctivePathFromRoot);
	ASSERT_TRUE(GenType::CANMAKEFALSE==context.gentype);

	t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.xF);
	context = getBoolGrounder(t, ts.s)->context();
	ASSERT_TRUE(CompContext::SENTENCE==context._component);
	ASSERT_TRUE(context._conjPathUntilNode);
	ASSERT_TRUE(context._conjunctivePathFromRoot);

	t = Theory("T", ts.vocabulary, ParseInfo());
	t.add(ts.maxxpxgeq0);
	context = getBoolGrounder(t, ts.s)->context();
	ASSERT_TRUE(CompContext::SENTENCE==context._component);
	ASSERT_TRUE(context._conjunctivePathFromRoot);

	//TODO: test definitions (and fixpdefinitions)
	//clean(ts);
}

}
