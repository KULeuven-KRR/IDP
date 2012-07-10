#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXsatTest, DoesMXWithCP) {
	runTests("satisfiability.idp", GetParam(), "satwithcp()");
}

}
