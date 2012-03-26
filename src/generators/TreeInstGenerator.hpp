/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef TREEINSTGENERATOR_HPP_
#define TREEINSTGENERATOR_HPP_

#include "InstGenerator.hpp"
#include "GeneratorNodes.hpp"

class TreeInstGenerator: public InstGenerator {
private:
	GeneratorNode* _root;
	bool _reset;

public:
	TreeInstGenerator(GeneratorNode* r)
			: _root(r), _reset(true) {
		Assert(r!=NULL);
	}

	TreeInstGenerator* clone() const {
		auto t = new TreeInstGenerator(*this);
		t->_root = _root->clone();
		Assert(t->_reset==_reset);
		return t;
	}

	~TreeInstGenerator(){
		delete(_root);
	}

	void reset() {
		_reset = true;
	}

	void next() {
		if (_reset) {
			_reset = false;
			_root->begin();
		} else {
			_root->next();
		}
		if (_root->isAtEnd()) {
			notifyAtEnd();
		}
	}

	void setVarsAgain(){
		_root->setVarsAgain();
	}

	virtual void put(std::ostream& stream) {
		pushtab();
		stream << "TreeInstGenerator" <<nt() << toString(_root);
		poptab();
	}
};

#endif /* TREEINSTGENERATOR_HPP_ */
