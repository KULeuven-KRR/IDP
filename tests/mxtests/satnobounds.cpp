#include "FileEnumerator.hpp"

namespace Tests {

TEST_P(MXsatTest, DoesMX) {
	runTests("satisfiability.idp", GetParam(), "satnobounds()");
}

}
