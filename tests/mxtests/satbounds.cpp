#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXsatTest, DoesMXWithBounds) {
	runTests("satisfiability.idp", GetParam(), "satwithbounds()");
}

TEST_P(MXsatTest, DoesSatMXNonReduced) {
	runTests("satisfiability.idp", GetParam(), "satnonreduced()");
}

}
