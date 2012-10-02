#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXnbTest, DISABLED_DoesMXWithLazyTseitinDelaying) {
	runTests("modelexpansion.idp", GetParam(), "mxlazy()");
}

}
