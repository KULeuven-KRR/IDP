#ifndef BASICCHECKERS_HPP_
#define BASICCHECKERS_HPP_

#include "generators/InstGenerator.hpp"

class FalseInstChecker : public InstChecker {
public:
	bool check() { return false; }
	FalseInstChecker* clone() const{
		return new FalseInstChecker(*this);
	}
};

class TrueInstChecker : public InstChecker {
public:
	bool check() { return true; }
	TrueInstChecker* clone() const{
		return new TrueInstChecker(*this);
	}
};

#endif /* BASICCHECKERS_HPP_ */
