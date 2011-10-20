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

class TrueQuantKernelGenerator : public InstGenerator {
	private:
		InstGenerator*					_quantgenerator;
		std::vector<const DomElemContainer*>	_projectvars;
		mutable set<ElementTuple>		_memory;
	public:

		TrueQuantKernelGenerator(InstGenerator* gen, const std::vector<const DomElemContainer*> projv) :
			_quantgenerator(gen), _projectvars(projv) { }

		bool storetuple() const {
			ElementTuple tuple;
			for(auto it = _projectvars.cbegin(); it != _projectvars.cend(); ++it){
				tuple.push_back((*it)->get());
			}
			auto p = _memory.insert(tuple);
			return p.second;
		}

		bool first() const {
			if(_quantgenerator->first()) {
				if(storetuple()) return true;
				else return next();
			}
			else return false;
		}

		bool next() const {
			while(_quantgenerator->next()) {
				if(storetuple()) return true;
			}
			return false;
		}
};

#endif /* TRUEQUANTKERNELGENERATOR_HPP_ */
