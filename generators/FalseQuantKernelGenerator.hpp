/************************************
	FalseQuantKernelGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef FALSEQUANTKERNELGENERATOR_HPP_
#define FALSEQUANTKERNELGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

class FalseQuantKernelGenerator : public InstGenerator {
	private:
		InstGenerator*	_quantgenerator;
		InstGenerator*	_univgenerator;
	public:
		FalseQuantKernelGenerator(InstGenerator* q, InstGenerator* u) : _quantgenerator(q), _univgenerator(u) { }
		bool first() const {
			if(_univgenerator->first()) {
				if(_quantgenerator->first()) return next();
				else return true;
			}
			else return false;
		}
		bool next() const {
			while(_univgenerator->next()) {
				if(!_quantgenerator->first()) return true;
			}
			return false;
		}
};

#endif /* FALSEQUANTKERNELGENERATOR_HPP_ */
