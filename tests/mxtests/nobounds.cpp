#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXnbTest, DoesMXNoBounds) {
	runTests("modelexpansion.idp", GetParam(), "mxnobounds()");
}

}
