#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXsatTest, DoesMXWithBounds) {
	runTests("satisfiability.idp", GetParam(), "satwithbounds()");
}

TEST_P(MXsatTest, DoesSatMXNonReduced) {
	runTests("satisfiability.idp", GetParam(), "satnonreduced()");
}

TEST_P(MXsatStableTest, DoesSatMXStable) {
	runTests("satisfiability.idp", GetParam(), "satstable()");
}

}
