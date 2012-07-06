#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXnbTest, DoesMXWithCP) {
	runTests("modelexpansion.idp", GetParam(), "mxwithcp()");
}

}
