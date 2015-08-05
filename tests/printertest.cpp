/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "mxtests/FileEnumerator.hpp"
//#include "TestUtils.hpp"
#include "gtest/gtest.h"
#include "external/runidp.hpp"
#include "utils/FileManagement.hpp"

using namespace std;

//std::string getTestDirectory() {
//	return string(TESTDIR) + '/';
//}

namespace Tests {
void runTests2(const string& instancefile, const std::string& command="") {
	string testfile(getTestDirectory() + "printtest.idp");
	string testfile2(getTestDirectory() + "printtest2.idp");
	cerr << "Testing " << instancefile << "\n";
	Status result = Status::FAIL;

	ASSERT_NO_THROW(result=test( {instancefile, testfile}, "shouldPrint()"););
	if(result == Status::FAIL){
		cerr << "Ignoring this file\n";
		return;
	}

	ASSERT_NO_THROW(result=test( {instancefile, testfile}, command););
	ASSERT_EQ(Status::SUCCESS, result);
	ASSERT_NO_THROW( result = test( {getTestDirectory() +"modelexpansion.idp", testfile2, "tmp.idp"}, "mxwithoutcp()"););
	ASSERT_EQ(Status::SUCCESS, result);

}


//void throwexc() {
//	throw exception();
//}

TEST_P(SimpleMXnbTest, PrintEqualTest){
	runTests2(GetParam());
}


}
