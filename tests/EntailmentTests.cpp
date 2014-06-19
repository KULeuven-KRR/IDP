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
#include <cstdio>

#include "gtest/gtest.h"
#include "external/runidp.hpp"
#include "creation/cppinterface.hpp"
#include "options.hpp"
#include "GlobalData.hpp"
#include "utils/FileManagement.hpp"
#include "TestUtils.hpp"
#include "theory/TheoryUtils.hpp"
#include "inferences/functiondetection/FunctionDetection.hpp"

#include <exception>

using namespace std;
using namespace Gen;

namespace Tests {

class EntailmentTests: public ::testing::TestWithParam<std::string> {
};

vector<string> generateListOfEntailmentFiles() {
	vector<string> testdirs { "entails/" };
	return getAllFilesInDirs(getTestDirectory(), testdirs);
}

TEST_P(EntailmentTests, Entails) {
	stringstream ss;
	ss <<getInstallDirectoryPath() <<"/bin/SPASS";
	setOption(PROVERCOMMAND, ss.str());
	setOption(PROVER_SUPPORTS_TFA, false);
	runTests("entailment.idp", GetParam(), "checkEntails()");
}
INSTANTIATE_TEST_CASE_P(Entailment, EntailmentTests, ::testing::ValuesIn(generateListOfEntailmentFiles()));

TEST(FunctionDetection, Skolemize){
	Vocabulary V("FuncDetect");
	auto s = new Sort("X");
	V.add(s);
	auto p = Gen::pred("P", {s,s});
	V.add(p.p());
	Theory T("T", &V,{});
	auto var = new Variable(s);
	auto var2 = new Variable(s);
	T.add(&Gen::forall(var,Gen::exists(var2, p({var,var2}))));
	auto origfuncs = V.getFuncs().size();
	FormulaUtils::skolemize(&T);
	ASSERT_EQ(origfuncs+1, T.vocabulary()->getFuncs().size());  // TODO and should be partial! TODO for easier checking, return detected function dependencies?
}

TEST(FunctionDetection, Pred2PartialFunc){
	Vocabulary V("FuncDetect");
	auto s = new Sort("X");
	V.add(s);
	auto p = Gen::pred("P", {s,s});
	V.add(p.p());
	Theory T("T", &V,{});
	auto var = new Variable(s);
	auto var2 = new Variable(s);
	auto var3 = new Variable(s);
	auto& pred1 = p({var,var2});
	auto& pred2 = p({var,var3});
	pred1.negate();
	pred2.negate();
	T.add(&Gen::forall({var,var2,var3},Gen::disj({&pred1,&pred2,&PredWrapper(get(STDPRED::EQ, s))({var2,var3})})));
	FunctionDetection::doDetectAndRewriteIntoFunctions(&T);
	ASSERT_EQ(V.getFuncs().size()+1, T.vocabulary()->getFuncs().size());  // TODO and should be partial! TODO for easier checking, return detected function dependencies?
}

TEST(FunctionDetection, Pred2TotalFunc){
	Vocabulary V("FuncDetect");
	auto s = new Sort("X");
	V.add(s);
	auto p = Gen::pred("P", {s,s});
	V.add(p.p());
	Theory T("T", &V,{});
	auto var = new Variable(s);
	auto var2 = new Variable(s);
	auto var3 = new Variable(s);
	T.add(&Gen::forall(var,Gen::exists(var2, p({var, var2}))));
	auto& pred1 = p({var,var2});
	auto& pred2 = p({var,var3});
	pred1.negate();
	pred2.negate();
	T.add(&Gen::forall({var,var2,var3},Gen::disj({&pred1,&pred2,&PredWrapper(get(STDPRED::EQ, s))({var2,var3})})));
	FunctionDetection::doDetectAndRewriteIntoFunctions(&T);
	ASSERT_EQ(V.getFuncs().size()+1, T.vocabulary()->getFuncs().size());
}

TEST(FunctionDetection, Pred2TotalDefFunc){
	Vocabulary V("FuncDetect");
	auto s = new Sort("X");
	V.add(s);
	auto p = Gen::pred("P", {s,s});
	auto q = Gen::pred("Q", {s,s});
	V.add(p.p());
	V.add(q.p());
	Theory T("T", &V,{});
	auto var = new Variable(s);
	auto var2 = new Variable(s);
	auto var3 = new Variable(s);
	T.add(&Gen::forall(var,Gen::exists(var2, p({var, var2}))));
	auto& pred1 = p({var,var2});
	auto& pred2 = p({var,var3});
	pred1.negate();
	pred2.negate();
	T.add(&Gen::forall({var,var2,var3},Gen::disj({&pred1,&pred2,&PredWrapper(get(STDPRED::EQ, s))({var2,var3})})));
	auto def = new Definition();
	def->add(new Rule(getVarSet(std::vector<Variable*>{var,var2}),&q({var,var2}), &p({var,var2}),{}));
	T.add(def);
	FunctionDetection::doDetectAndRewriteIntoFunctions(&T);
	ASSERT_EQ(V.getFuncs().size()+2, T.vocabulary()->getFuncs().size());
}

}
