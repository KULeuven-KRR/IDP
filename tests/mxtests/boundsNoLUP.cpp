#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXnbTest, DoesMXwithoutLUP) {
	runTests("modelexpansion.idp", GetParam(), "mxwithboundswithoutLUP()");
}

}
