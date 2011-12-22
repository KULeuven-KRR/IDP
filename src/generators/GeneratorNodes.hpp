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

	virtual void reset()= 0;

public:
	GeneratorNode() :
			atEnd(false) {
	}
	virtual ~GeneratorNode() {
	}

	bool isAtEnd() const {
		return atEnd;
	}
	virtual void next() = 0;
	void begin() {
		atEnd = false;
		reset();
		if(not isAtEnd()){
			next();
		}
	}

	virtual void put(std::ostream& stream) = 0;
};

class LeafGeneratorNode: public GeneratorNode {
private:
	InstGenerator* _generator;
	bool _reset;
public:
	LeafGeneratorNode(InstGenerator* gt) :
			GeneratorNode(), _generator(gt), _reset(true) {
	}

	virtual void next() {
		if(_reset){
			_reset = false;
			_generator->begin();
		}else{
			_generator->operator ++();
		}
		if (_generator->isAtEnd()) {
			notifyAtEnd();
		}
	}
	virtual void reset() {
		_reset = true;
	}

	virtual void put(std::ostream& stream){
		stream <<"generate " <<toString(_generator);
	}
};

class OneChildGeneratorNode: public GeneratorNode {
private:
	InstGenerator* _generator;
	GeneratorNode* _child;

	bool _reset;

public:
	OneChildGeneratorNode(InstGenerator* gt, GeneratorNode* c) :
			_generator(gt), _child(c), _reset(true) {
	}

	virtual void next() {
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
		if (_generator->isAtEnd()) {
			notifyAtEnd();
		}
	}

	virtual void reset() {
		_reset = true;
	}

	virtual void put(std::ostream& stream){
		stream <<"generate: " <<toString(_generator) <<"\n";
		stream <<tabs() <<"then ";
		pushtab();
		stream <<toString(_child);
		poptab();
	}
};

class TwoChildGeneratorNode: public GeneratorNode {
private:
	InstChecker* _checker;
	InstGenerator* _generator;
	GeneratorNode *_falsecheckbranch, *_truecheckbranch;
	bool _reset;

public:
	TwoChildGeneratorNode(InstChecker* c, InstGenerator* g, GeneratorNode* falsecheckbranch, GeneratorNode* truecheckbranch) :
			_checker(c), _generator(g), _falsecheckbranch(falsecheckbranch), _truecheckbranch(truecheckbranch), _reset(true) {
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
					}
				}
			} else {
				if (_falsecheckbranch->isAtEnd()) {
					_generator->operator ++();
				} else {
					_falsecheckbranch->next();
					if (not _falsecheckbranch->isAtEnd()) {
						return;
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

	virtual void reset() {
		_reset = true;
	}

	virtual void put(std::ostream& stream){
		stream <<"generate: " <<toString(_generator) <<"\n";
		stream <<tabs() <<"then\n";
		pushtab();
		stream <<tabs() <<"truebranch "<<toString(_truecheckbranch) <<"\n";
		stream <<tabs() <<"falsebranch "<<toString(_falsecheckbranch);
		poptab();
	}
};

#endif /* GENERATORNODES_HPP_ */
