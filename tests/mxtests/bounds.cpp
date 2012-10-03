#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXnbTest, DoesMX) {
	runTests("modelexpansion.idp", GetParam(), "mxwithbounds()");
}

TEST_P(MXnbTest, DoesMXNonReduced) {
	runTests("modelexpansion.idp", GetParam(), "mxnonreduced()");
}

}
