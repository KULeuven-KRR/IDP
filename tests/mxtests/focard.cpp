#include "FileEnumerator.hpp"

using namespace std;

namespace Tests {
TEST_P(SimpleMXnbTest, Card2FO) {
	runTests("modelexpansion.idp", GetParam(), "mxCardFO()");
}
}
