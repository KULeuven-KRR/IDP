#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXsatTest, DoesMXWithBounds) {
	runTests("satisfiability.idp", GetParam(), "satwithbounds()");
}

}
