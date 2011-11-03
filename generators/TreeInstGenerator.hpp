#ifndef TREEINSTGENERATOR_HPP_
#define TREEINSTGENERATOR_HPP_

#include "generators/InstGenerator.hpp"
#include "generators/GeneratorNodes.hpp"

class TreeInstGenerator: public InstGenerator {
private:
	GeneratorNode* const _root;

public:
	TreeInstGenerator(GeneratorNode* r)
			: _root(r) {
	}

	void reset() {
		_root->begin();
		if (_root->isAtEnd()) {
			notifyAtEnd();
		}
	}

	void next() {
		_root->next();
		if (_root->isAtEnd()) {
			notifyAtEnd();
		}
	}
};

#endif /* TREEINSTGENERATOR_HPP_ */
