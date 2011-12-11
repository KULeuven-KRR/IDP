#include "common.hpp"
#include "InstGenerator.hpp"
#include "GlobalData.hpp"

void InstChecker::put(std::ostream& stream) {
	stream << "generate: " << typeid(*this).name();
}

// Can also be used for resets
// SETS the instance to the FIRST value if it exists
void InstGenerator::begin() {
	end = false;
	reset();
	if (not isAtEnd()) {
		next();
	}
}

void InstGenerator::operator++() {
	Assert(not isAtEnd());
	if (getGlobal()->terminateRequested()) {
		throw IdpException("Terminate requested");
	}
	next();
}
