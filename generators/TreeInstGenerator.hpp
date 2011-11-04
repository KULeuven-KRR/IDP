#ifndef TREEINSTGENERATOR_HPP_
#define TREEINSTGENERATOR_HPP_

#include "generators/InstGenerator.hpp"
#include "generators/GeneratorNodes.hpp"

class TreeInstGenerator: public InstGenerator {
private:
	GeneratorNode* const _root;
	bool _reset;

public:
	TreeInstGenerator(GeneratorNode* r)
			: _root(r), _reset(true) {
	}

	void reset() {
		_reset = true;
	}

	void next() {
		if(_reset){
			_reset = false;
			_root->begin();
		}else{
			_root->next();
		}
		if (_root->isAtEnd()) {
			notifyAtEnd();
		}
	}
};

#endif /* TREEINSTGENERATOR_HPP_ */
