#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXnbTest, DoesMX) {
	runTests("modelexpansion.idp", GetParam(), "mxwithboundswithoutLUP()");
}

}
