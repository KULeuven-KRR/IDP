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

class FullGenerator : public InstGenerator {
private:
	bool first;
public:
	FullGenerator():first(true){}
	virtual void next() {
		if(first){
			first = false;
		}else{
			notifyAtEnd();
		}
	}
	virtual void reset() { first = true;}
};

#endif /* EMPTYGENERATOR_HPP_ */
