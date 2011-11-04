#ifndef BASICGENERATOR_HPP_
#define BASICGENERATOR_HPP_

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

#endif /* BASICGENERATOR_HPP_ */
