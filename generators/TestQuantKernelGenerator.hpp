/************************************
	TestQuantKernelGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef TESTQUANTKERNELGENERATOR_HPP_
#define TESTQUANTKERNELGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

using namespace std;

class TestQuantKernelGenerator : public InstGenerator {
	private:
		InstGenerator*	_quantgenerator;
	public:
		TestQuantKernelGenerator(InstGenerator* gen) : _quantgenerator(gen) { }
		bool first()	const { return _quantgenerator->first();	}
		bool next()		const { return false;						}
};

#endif /* TESTQUANTKERNELGENERATOR_HPP_ */
