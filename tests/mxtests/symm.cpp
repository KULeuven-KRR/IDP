#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXnbTest, DoesMXWithSymmetryBreaking) {
	runTests("modelexpansion.idp", GetParam(), "mxwithsymm()");
}

}
