#include "generator.hpp"

/** First instance **/

bool TableInstGenerator::first() const {
	if(_table->empty()) return false;
	else {
		_currpos = 0;
		for(unsigned int n = 0; n < _outvars.size(); ++n) {
			*(_outvars[n]) = _table->delement(_currpos,n);
		}
		return true;	
	}
}

bool SortInstGenerator::first() const {
	if(_table->empty()) return false;
	else {
		_currpos = 0;
		*_outvar = _table->delement(_currpos);
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
	for(unsigned int n = 0; n < _outvars.size(); ++n) {
		_currargs[n] = _tables[n]->delement(0);
		*(_outvars[n]) = _currargs[n];
		_currpositions[n] = 0;
	}
	if(_checker->run(_currargs)) {
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
	if(!_root) return true; //FIXME probably not correct in every situation
	_curr = _root->first(); 
	return (_curr ? true : false);
}


/** Next instance **/

bool TableInstGenerator::next() const {
	++_currpos;
	if(_currpos < _table->size()) {
		for(unsigned int n = 0; n < _outvars.size(); ++n) {
			*(_outvars[n]) = _table->delement(_currpos,n);
		}
		return true;
	}
	return false;
}

bool SortInstGenerator::next() const {
	++_currpos;
	if(_currpos < _table->size()) {
		*_outvar = _table->delement(_currpos);
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
		unsigned int n = 0;
		for( ; n < _outvars.size(); ++n) {
			if(_currpositions[n] < _tables[n]->size()) {
				_currpositions[n]++;
				_currargs[n] = _tables[n]->delement(_currpositions[n]);
				*(_outvars[n]) = _currargs[n];
				break;
			}
			else {
				_currpositions[n] = 0;
				_currargs[n] = _tables[n]->delement(0);
				*(_outvars[n]) = _currargs[n];
			}
		}
		if(n == _outvars.size()) return 0;
		else {
			if(_checker->run(_currargs)) {
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
	if(!_root) return false; //FIXME probably not correct in every situation...
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

/**************
	Factory
**************/

InstGenerator* GeneratorFactory::create(const vector<domelement*>& vars, const vector<SortTable*>& tabs) {
	InstGenerator* gen = 0;
	GeneratorNode* node = 0;
	for(unsigned int n = 0; n < vars.size(); ++n) {
		SortInstGenerator* tig = new SortInstGenerator(tabs[n],vars[n]);
		if(vars.size() == 1) {
			gen = tig;
			break;
		}
		else if(n == 0) node = new LeafGeneratorNode(tig);
		else node = new OneChildGeneratorNode(tig,node);
	}
	if(!gen) gen = new TreeInstGenerator(node);
	return gen;
}
