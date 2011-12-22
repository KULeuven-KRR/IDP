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

	// FIXME reimplement (deep clone)
	TreeInstGenerator* clone() const{
		return new TreeInstGenerator(*this);
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

	virtual void put(std::ostream& stream){
		stream <<"TreeInstGenerator\n";
		pushtab();
		stream <<tabs() <<toString(_root);
		poptab();
	}
};

#endif /* TREEINSTGENERATOR_HPP_ */
