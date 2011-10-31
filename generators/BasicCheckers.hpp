#ifndef BASICCHECKERS_HPP_
#define BASICCHECKERS_HPP_

#include "generators/InstGenerator.hpp"

class FalseInstChecker : public InstChecker {
public:
	bool check() { return false; }
};

class TrueInstChecker : public InstChecker {
public:
	bool check() { return true; }
};

#endif /* BASICCHECKERS_HPP_ */
