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

using namespace std;

namespace Tests{
	class MXTest : public ::testing::TestWithParam<string> {

	};

	class LazyMXTest : public ::testing::TestWithParam<string> {

	};

	TEST_P(MXTest, DoesMX){
		string testfile(string(TESTDIR)+"mxnbofmodelstest.idp");
		auto result = run({string(TESTDIR)+GetParam(), testfile});
		if(result==Status::FAIL){
			cerr <<"Tested file " <<string(TESTDIR)+GetParam() <<"\n";
		}
		EXPECT_EQ(result, Status::SUCCESS);
	}

	TEST_P(LazyMXTest, DoesMX){
		string testfile(string(TESTDIR)+"mxlazynbofmodelstest.idp");
		auto result = run({string(TESTDIR)+GetParam(), testfile});
		if(result==Status::FAIL){
			cerr <<"Tested file " <<string(TESTDIR)+GetParam() <<"\n";
		}
		EXPECT_EQ(result, Status::SUCCESS);
	}

	vector<string> testlist{
		"equiv.idp", "forall.idp", "exists.idp", "impl.idp", "revimpl.idp",
		"conj.idp", "disj.idp",
		"negequiv.idp", "negforall.idp", "negexists.idp", "negimpl.idp",
		"negrevimpl.idp", "negconj.idp", "negdisj.idp",
		"atom.idp", "doubleneg.idp", "negatom.idp", "arbitrary.idp",
		"func.idp",
		"defonerule.idp", "defmultihead.idp", "defunwellf.idp", "defunfset.idp", "multidef.idp",
		"eq.idp", //"neq.idp", "leq.idp", "geq.idp", "lower.idp", "greater.idp",
		//"card.idp", "sum.idp", "min.idp", "max.idp", "prod.idp",
		// FIXME those tests go in infinite loop, quite irritating :)
		// FIXME on parsing error of one of the files, a lot of later ones will also fail!
	};

	INSTANTIATE_TEST_CASE_P(ModelExpansion,
					  MXTest,
					  ::testing::ValuesIn(testlist));

	INSTANTIATE_TEST_CASE_P(LazyModelExpansion,
					  LazyMXTest,
					  ::testing::ValuesIn(testlist));
}
