#include "FileEnumerator.hpp"

using namespace std;

namespace Tests {
TEST_P(SimpleMXnbTest, MetaRep) {
	runTests("modelexpansion.idp", GetParam(), "mxwithmeta()");
}
}
