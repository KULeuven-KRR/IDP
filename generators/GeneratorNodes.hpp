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
	GeneratorNode()
		: atEnd(false) {
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
	}
};

class LeafGeneratorNode: public GeneratorNode {
private:
	InstGenerator* _generator;
public:
	LeafGeneratorNode(InstGenerator* gt)
			: GeneratorNode(), _generator(gt) {
	}

	virtual void next() {
		_generator->operator ++();
		if (_generator->isAtEnd()) {
			notifyAtEnd();
		}
	}
	virtual void reset() {
		_generator->begin();
		if (_generator->isAtEnd()) {
			notifyAtEnd();
		}
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

	virtual void next() {
		_child->next();
		if (_child->isAtEnd()) {
			_generator->operator ++();
			if (_generator->isAtEnd()) {
				notifyAtEnd();
			}else{
				next();
			}
		}
	}
	virtual void reset() {
		for (_generator->begin(); not _generator->isAtEnd(); _generator->operator ++()) {
			_child->begin();
			if (not _child->isAtEnd()) {
				break;
			}
		}
		if (_generator->isAtEnd()) {
			notifyAtEnd();
		}
	}
};

class TwoChildGeneratorNode: public GeneratorNode {
private:
	InstChecker* _checker;
	InstGenerator* _generator;
	GeneratorNode* _truecheckbranch, *_falsecheckbranch;

public:
	TwoChildGeneratorNode(InstChecker* c, InstGenerator* g, GeneratorNode* falsecheckbranch,
			GeneratorNode* truecheckbranch)
			: _checker(c), _generator(g), _falsecheckbranch(falsecheckbranch), _truecheckbranch(truecheckbranch) {
	}

	virtual void next() {
		bool ended = false;
		if (_checker->check()) {
			_truecheckbranch->next();
			if (_truecheckbranch->isAtEnd()) {
				ended = true;
			}
		} else {
			_falsecheckbranch->next();
			if (_falsecheckbranch->isAtEnd()) {
				ended = true;
			}
		}
		if(ended){
			_generator->operator ++();
			if (_generator->isAtEnd()) {
				notifyAtEnd();
			}else{
				next();
			}
		}
	}
	virtual void reset() {
		for (_generator->begin(); not _generator->isAtEnd(); _generator->operator ++()) {
			if (_checker->check()) {
				_truecheckbranch->begin();
				if (not _truecheckbranch->isAtEnd()) {
					break;
				}
			} else {
				_falsecheckbranch->begin();
				if (not _falsecheckbranch->isAtEnd()) {
					break;
				}
			}
		}
		if (_generator->isAtEnd()) {
			notifyAtEnd();
		}
	}
};

#endif /* GENERATORNODES_HPP_ */
