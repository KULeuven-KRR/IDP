#include "generator.hpp"

/** First instance **/

bool TableInstGenerator::first() const {
	if(_table->empty()) return false;
	else {
		_currpos = 0;
		for(unsigned int n = 0; n < _outvars.size(); ++n) {
			_outvars[n]->_element = _table->element(_currpos,n);
			_outvars[n]->_type = _table->type(n);
		}
		return true;	
	}
}

GeneratorNode* LeafGeneratorNode::first() const {
	if(_generator->first()) return _this;
	return 0;
}

GeneratorNode* OneChildGeneratorNode::first() const {
	if(_generator->first()) {
		GeneratorNode* r = _child->first();
		if(r) return r;
		else return next();
	}
	return 0;
}

GeneratorNode* TwoChildGeneratorNode::first() const {
	for(int n = 0; n < _outvars.size(); ++n) {
		_outvars[n]->_element = _tables[n]->element(0);
		_outvars[n]->_type = _tables[n]->type();
		_currpositions[n] = 0;
	}
	if(_checker->run()) {
		GeneratorNode* r = _right->first();
		if(r) return r;
	}
	else {
		GeneratorNode* r = _left->first();
		if(r) return r;
	}
	return next();
}

bool TreeInstGenerator::first() const {
	_curr = _root->first(); 
	return (_curr ? true : false);
}


/** Next instance **/

bool TableInstGenerator::next() const {
	++_currpos;
	if(_currpos < _table->size()) {
		for(unsigned int n = 0; n < _outvars.size(); ++n) {
			_outvars[n]->_element = _table->element(_currpos,n);
			_outvars[n]->_type = _table->type(n);
		}
		return true;
	}
	return false;
}

GeneratorNode* LeafGeneratorNode::next() const {
	if(_generator->next()) return _this;
	return 0;
}

GeneratorNode* OneChildGeneratorNode::next() const {
	while(_generator->next()) {
		GeneratorNode* r = _child->first();
		if(r) return r;
	}
	return 0;
}

GeneratorNode*	TwoChildGeneratorNode::next() const {
	while(true) {
		int n = 0;
		for( ; n < _outvars.size(); ++n) {
			if(_currpositions[n] < _tables[n]->size()) {
				_currpositions[n]++;
				_outvars[n]->_element = _tables[n]->element(_currpositions[n]);
				_outvars[n]->_type = _tables[n]->type();
				break;
			}
			else {
				_currpositions[n] = 0;
				_outvars[n]->_element = _tables[n]->element(0);
				_outvars[n]->_type = _tables[n]->type();
			}
		}
		if(n == _outvars.size()) return 0;
		else {
			if(_checker->run()) {
				GeneratorNode* r = _right->first();
				if(r) return r;
			}
			else {
				GeneratorNode* r = _left->first();
				if(r) return r;
			}
		}
	}
}

bool TreeInstGenerator::next() const {
	assert(_curr);
	GeneratorNode* temp = 0;
	while(!temp) {
		temp = _curr->next();
		if(!temp) _curr = _curr->parent();
		if(!_curr) return false;
	}
	_curr = temp;
	return true;
}


