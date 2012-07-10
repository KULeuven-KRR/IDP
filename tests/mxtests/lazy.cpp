#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXnbTest, DoesMXWithLazyTseitinDelaying) {
	runTests("modelexpansion.idp", GetParam(), "mxlazy()");
}

}
