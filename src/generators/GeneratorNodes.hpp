/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef GENERATORNODES_HPP_
#define GENERATORNODES_HPP_

class GeneratorNode {
private:
	bool atEnd;
protected:
	void notifyAtEnd() {
		atEnd = true;
	}

	bool _reset;
	inline void reset() {
		_reset = true;
	}

public:
	GeneratorNode()
			: atEnd(false), _reset(true) {
	}
	virtual ~GeneratorNode() {
	}

	inline bool isAtEnd() const {
		return atEnd;
	}
	virtual void next() = 0;
	inline void begin() {
		atEnd = false;
		reset();
		if (not isAtEnd()) {
			next();
		}
	}

	virtual void setVarsAgain() = 0;

	virtual GeneratorNode* clone() const = 0;

	virtual void put(std::ostream& stream) = 0;
};

class LeafGeneratorNode: public GeneratorNode {
private:
	InstGenerator* _generator;
public:
	LeafGeneratorNode(InstGenerator* gt)
			: GeneratorNode(), _generator(gt) {
	}

	~LeafGeneratorNode() {
		delete (_generator);
	}

	virtual void next() {
		if (_reset) {
			_reset = false;
			_generator->begin();
		} else {
			_generator->operator ++();
		}
		if (_generator->isAtEnd()) {
			notifyAtEnd();
		}
	}

	void setVarsAgain() {
		_generator->setVarsAgain();
	}

	virtual LeafGeneratorNode* clone() const {
		auto t = new LeafGeneratorNode(*this);
		t->_generator = _generator->clone();
		return t;
	}

	virtual void put(std::ostream& stream) {
		stream << toString(_generator);
	}
};

class OneChildGeneratorNode: public GeneratorNode {
private:
	InstGenerator* _generator;
	GeneratorNode* _child;

public:
	OneChildGeneratorNode(InstGenerator* gt, GeneratorNode* c)
			: _generator(gt), _child(c) {
	}

	~OneChildGeneratorNode() {
		delete (_generator);
		delete (_child);
	}

	virtual OneChildGeneratorNode* clone() const {
		auto t = new OneChildGeneratorNode(*this);
		t->_generator = _generator->clone();
		t->_child = _child->clone();
		return t;
	}

	void setVarsAgain() {
		_generator->setVarsAgain();
		_child->setVarsAgain();
	}

	virtual void next() {
		//std::cerr << "CALLING NEXT ON GENERATORNODE WITH " << nt() << "generator: " << toString(_generator) << " of type " << typeid(*_generator).name()
		//<< " and child " << toString(_child) << "of type" << typeid(*_child).name()<<nt();
		if (_reset) {
			_reset = false;
			_generator->begin();
		} else {
			if (not _child->isAtEnd()) {
				_child->next();
			}
			if (not _child->isAtEnd()) {
				return;
			}
			_generator->operator ++();
		}
		// Here, the generator is at a new value
		for (; not _generator->isAtEnd(); _generator->operator ++()) {
			_child->begin();
			if (not _child->isAtEnd()) {
				return;
			}
		}
		Assert(_generator->isAtEnd());
		notifyAtEnd();

	}

	virtual void put(std::ostream& stream) {
		stream << "generate: " << toString(_generator) << nt();
		stream << "then ";
		pushtab();
		stream << toString(_child);
		poptab();
	}
};

class TwoChildGeneratorNode: public GeneratorNode {
private:
	InstChecker* _checker;
	InstGenerator* _generator;
	GeneratorNode *_falsecheckbranch, *_truecheckbranch;

public:
	TwoChildGeneratorNode(InstChecker* c, InstGenerator* g, GeneratorNode* falsecheckbranch, GeneratorNode* truecheckbranch)
			: _checker(c), _generator(g), _falsecheckbranch(falsecheckbranch), _truecheckbranch(truecheckbranch) {
	}

	~TwoChildGeneratorNode() {
		delete (_checker);
		delete (_generator);
		delete (_falsecheckbranch);
		delete (_truecheckbranch);
	}

	virtual TwoChildGeneratorNode* clone() const {
		auto t = new TwoChildGeneratorNode(*this);
		t->_checker = _checker->clone();
		t->_generator = _generator->clone();
		t->_falsecheckbranch = _falsecheckbranch->clone();
		t->_truecheckbranch = _truecheckbranch->clone();
		return t;
	}

	void setVarsAgain() {
		_generator->setVarsAgain();
		_truecheckbranch->setVarsAgain();
		_falsecheckbranch->setVarsAgain();
	}

	virtual void next() {
		if (_reset) {
			_reset = false;
			_generator->begin();
		} else {
			if (_checker->check()) {
				if (_truecheckbranch->isAtEnd()) {
					_generator->operator ++();
				} else {
					_truecheckbranch->next();
					if (not _truecheckbranch->isAtEnd()) {
						return;
					} else {
						_generator->operator ++();
					}
				}
			} else {
				if (_falsecheckbranch->isAtEnd()) {
					_generator->operator ++();
				} else {
					_falsecheckbranch->next();
					if (not _falsecheckbranch->isAtEnd()) {
						return;
					}else {
						_generator->operator ++();
					}
				}
			}

		}
		// Here, the generator is at a new value
		for (; not _generator->isAtEnd(); _generator->operator ++()) {
			if (_checker->check()) {
				_truecheckbranch->begin();
				if (not _truecheckbranch->isAtEnd()) {
					return;
				}
			} else {
				_falsecheckbranch->begin();
				if (not _falsecheckbranch->isAtEnd()) {
					return;
				}
			}

		}
		if (_generator->isAtEnd()) {
			notifyAtEnd();
		}
	}

	virtual void put(std::ostream& stream) {
		stream << "generate: " << toString(_generator) << nt();
		stream << "if result is in " << toString(_checker) << nt();
		pushtab();
		stream << "THEN" << nt();
		poptab();
		stream << toString(_truecheckbranch) << nt();
		stream << "ELSE" << nt();
		pushtab();
		stream << toString(_falsecheckbranch);
		poptab();
	}
};

#endif /* GENERATORNODES_HPP_ */
