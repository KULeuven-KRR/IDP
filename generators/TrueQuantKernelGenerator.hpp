/************************************
 TrueQuantKernelGenerator.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef TRUEQUANTKERNELGENERATOR_HPP_
#define TRUEQUANTKERNELGENERATOR_HPP_

#include <set>
#include "generators/InstGenerator.hpp"

using namespace std;

/**
 * Generate all x such that ?x phi(x) is true.
 * Given is a generator which returns tuples for which phi(x) is false.
 */
class TrueQuantKernelGenerator: public InstGenerator {
private:
	InstGenerator* _quantgenerator;
public:

	TrueQuantKernelGenerator(InstGenerator* gen) :
			_quantgenerator(gen) {
	}

	bool check() const{
		return not _quantgenerator->check();
	}

	void reset(){
		_quantgenerator->begin();
		if(_quantgenerator->isAtEnd()){
			notifyAtEnd();
		}
	}

	void next(){
		while(not _quantgenerator->isAtEnd()){
			_quantgenerator->operator ++();
		}
		if(_quantgenerator->isAtEnd()){
			notifyAtEnd();
		}
	}
};

#endif /* TRUEQUANTKERNELGENERATOR_HPP_ */
