/************************************
	EmptyGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EMPTYGENERATOR_HPP_
#define EMPTYGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

/**
 * Generates an empty set of instances
 */
class EmptyGenerator : public InstGenerator {
public:
	virtual void next() {}
	virtual void reset() { notifyAtEnd(); }
};

#endif /* EMPTYGENERATOR_HPP_ */
