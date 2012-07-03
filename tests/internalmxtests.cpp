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
#include "TestUtils.hpp"
#include "IncludeComponents.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "groundtheories/GroundPolicy.hpp"
#include "groundtheories/GroundTheory.hpp"

#include <exception>

using namespace std;

namespace Tests {

struct MxTesting {
	Vocabulary v, v2;
	Theory t;
	Structure s;
	Sort sort;
	Variable var;
	VarTerm o;

	MxTesting(bool differentvoc)
			: 	v(Vocabulary("one")),
				v2(Vocabulary("two")),
				t(Theory("t", &v, ParseInfo())),
				s(Structure("s", differentvoc ? &v2 : &v, ParseInfo())),
				sort(Sort("s")),
				var(Variable(&sort)),
				o(VarTerm(&var, TermParseInfo())) {
	}
};

TEST(MXTest, NullTheory) {
	auto mx = MxTesting(false);
	ASSERT_THROW(ModelExpansion::doModelExpansion(NULL, &mx.s), IdpException);
}

TEST(MXTest, NullStructure) {
	auto mx = MxTesting(false);
	ASSERT_THROW(ModelExpansion::doModelExpansion(&mx.t, NULL), IdpException);
}

TEST(MinimTest, NullTheory) {
	auto mx = MxTesting(false);
	ASSERT_THROW(ModelExpansion::doMinimization(NULL, &mx.s, &mx.o), IdpException);
}

TEST(MinimTest, NullStructure) {
	auto mx = MxTesting(false);
	ASSERT_THROW(ModelExpansion::doMinimization(&mx.t, NULL, &mx.o), IdpException);
}

TEST(MXTest, NullOptim) {
	auto mx = MxTesting(false);
	ASSERT_THROW(ModelExpansion::doMinimization(&mx.t, &mx.s, NULL), IdpException);
}

TEST(MXTest, DifferentVocabulary) {
	auto mx = MxTesting(true);
	ASSERT_THROW(ModelExpansion::doModelExpansion(&mx.t, &mx.s), IdpException);
}

TEST(OptimTest, DifferentVocabulary) {
	auto mx = MxTesting(true);
	ASSERT_THROW(ModelExpansion::doMinimization(&mx.t, &mx.s, &mx.o), IdpException);
}

TEST(MXTest, GroundTheory) {
	auto v = Vocabulary("one");
	auto s = Structure("s", &v, ParseInfo());
	auto t = GroundTheory<GroundPolicy>(&v, NULL);
	ASSERT_THROW(ModelExpansion::doModelExpansion(&t, &s), IdpException);
}

TEST(OptimTest, GroundTheory) {
	auto v = Vocabulary("one");
	auto s = Structure("s", &v, ParseInfo());
	auto t = GroundTheory<GroundPolicy>(&v, NULL);
	auto sort = Sort("s");
	auto var = Variable(&sort);
	auto o = VarTerm(&var, TermParseInfo());
	ASSERT_THROW(ModelExpansion::doMinimization(&t, &s, &o), IdpException);
}

TEST(OptimTest, NegProdOptimTheory) {
	auto v = Vocabulary("one");
	auto s = Structure("s", &v, ParseInfo());
	auto t = Theory("t", &v, ParseInfo());
	auto sort = new Sort("s"); // NOTE: sort ownership is passed to the voc, so has to be a real pointer!
	v.add(sort);
	auto var = Variable(sort);
	auto varterm = VarTerm(&var, TermParseInfo());
	auto term = FuncTerm(get(STDFUNC::UNARYMINUS), {&varterm}, TermParseInfo());
	auto trueform = BoolForm(SIGN::POS, true, { }, FormulaParseInfo());
	auto qset = QuantSetExpr({&var}, &trueform, &term, SetParseInfo());
	auto set = EnumSetExpr({&qset}, SetParseInfo());
	auto o = AggTerm(&set, AggFunction::PROD, TermParseInfo());
	s.inter(sort)->add(-10,10);
	ASSERT_THROW(ModelExpansion::doMinimization(&t, &s, &o), IdpException);
}

}
