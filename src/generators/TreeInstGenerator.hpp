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

class TreeInstGenerator: public InstGenerator {
private:
protected:
	bool _reset;

public:
	TreeInstGenerator()
			: _reset(true) {
	}

	~TreeInstGenerator() {
	}

	void reset() {
		_reset = true;
	}

};

/**OneChildGenerators are in fact generators for the logical operation "and" */
class OneChildGenerator: public TreeInstGenerator {
private:
	InstGenerator* _generator;
	InstGenerator* _child;

public:
	OneChildGenerator(InstGenerator* generator, InstGenerator* child)
			: _generator(generator), _child(child) {
	}

	~OneChildGenerator() {
		delete (_generator);
		delete (_child);
	}

	virtual OneChildGenerator* clone() const;

	void setVarsAgain() {
		_generator->setVarsAgain();
		_child->setVarsAgain();
	}

	virtual void next();

	virtual void put(std::ostream& stream);
};

/**
 * Generator for a general bdd
 * _generator generates the universe
 * _checker checks whether or not a kernel is satisfied.
 * Depending on this answer the generator for the false or the true branch is called
 */
class TwoChildGenerator: public TreeInstGenerator {
private:
	InstChecker* _checker;
	InstGenerator* _generator;
	InstGenerator *_falsecheckbranch, *_truecheckbranch;

public:
	TwoChildGenerator(InstChecker* c, InstGenerator* g, InstGenerator* falsecheckbranch, InstGenerator* truecheckbranch)
			: _checker(c), _generator(g), _falsecheckbranch(falsecheckbranch), _truecheckbranch(truecheckbranch) {
	}

	~TwoChildGenerator() {
		delete (_checker);
		delete (_generator);
		delete (_falsecheckbranch);
		delete (_truecheckbranch);
	}

	virtual TwoChildGenerator* clone() const;

	void setVarsAgain() {
		_generator->setVarsAgain();
		_truecheckbranch->setVarsAgain();
		_falsecheckbranch->setVarsAgain();
	}

	virtual void next();

	virtual void put(std::ostream& stream);

};

#endif /* TREEINSTGENERATOR_HPP_ */
