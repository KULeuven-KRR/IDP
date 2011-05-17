/************************************
	generator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "parseinfo.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "checker.hpp"
#include "generator.hpp"

using namespace std;

TwoChildGeneratorNode::TwoChildGeneratorNode(InstanceChecker* t, const vector<const DomainElement**>& ov, const vector<SortTable*>& dom, GeneratorNode* l, GeneratorNode* r) :
	_checker(t), _outvars(ov), _currargs(ov.size()), _left(l), _right(r) {
	PredTable* domtab = new PredTable(new FullInternalPredTable(),Universe(dom));
	_currposition = new TableInstGenerator(domtab,ov);
}

/** First instance **/

bool TableInstGenerator::first() const {
	if(_table->approxempty()) return false;
	else {
		_currpos = _table->begin();
		if(_currpos.hasNext()) {
			const ElementTuple& tuple = *_currpos;
			for(unsigned int n = 0; n < _outvars.size(); ++n) *(_outvars[n]) = tuple[n];
			return true;	
		} 
		else return false;
	}
}

bool SortInstGenerator::first() const {
	if(_table->approxempty()) return false;
	else {
		_currpos = _table->sortbegin();
		if(_currpos.hasNext()) { *_outvar = *_currpos; return true;	}
		else return false;
	}
}

bool SimpleLookupGenerator::first() const {
	for(unsigned int n = 0; n < _invars.size(); ++n) {
		_currargs[n] = *(_invars[n]);
	}
	return _table->contains(_currargs);
}

bool EnumLookupGenerator::first() const {
	for(unsigned int n = 0; n < _invars.size(); ++n) {
		_currargs[n] = *(_invars[n]);
	}
	_currpos = _table.find(_currargs);
	if(_currpos == _table.end()) return false;
	else {
		_iter = _currpos->second.begin();
		for(unsigned int n = 0; n < _outvars.size(); ++n) {
			*(_outvars[n]) = (*_iter)[n];
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
#ifdef NDEBUG
	_currposition->first();
#else
	bool b = _currposition->first();
	assert(b);
#endif
	for(unsigned int n = 0; n < _outvars.size(); ++n) {
		_currargs[n] = *(_outvars[n]);
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
	if(!_root) return true; 
	_curr = _root->first(); 
	return (_curr ? true : false);
}


/** Next instance **/

bool TableInstGenerator::next() const {
	++_currpos;
	if(_currpos.hasNext()) {
		const ElementTuple& tuple = *_currpos;
		for(unsigned int n = 0; n < _outvars.size(); ++n) *(_outvars[n]) = tuple[n];
		return true;	
	}
	return false;
}

bool SortInstGenerator::next() const {
	++_currpos;
	if(_currpos.hasNext()) {
		*_outvar = *_currpos;
		return true;
	}
	return false;
}

bool EnumLookupGenerator::next() const {
	++_iter;
	if(_iter != _currpos->second.end()) {
		for(unsigned int n = 0; n < _outvars.size(); ++n) {
			*(_outvars[n]) = (*_iter)[n];
		}
		return true;
	}
	else return false;
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
		if(!_currposition->next()) return 0;
		else {
			for(unsigned int n = 0; n < _outvars.size(); ++n) {
				_currargs[n] = *(_outvars[n]);
			}
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
	if(!_root) return false; 
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

InstGenerator* GeneratorFactory::create(const vector<const DomainElement**>& vars, const vector<SortTable*>& tabs) {
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

InstGenerator*	GeneratorFactory::create(PredTable* pt, std::vector<bool> pattern, const std::vector<const DomainElement**>& vars) {
	unsigned int firstout = 0;
	for( ; firstout < pattern.size(); ++firstout) {
		if(!pattern[firstout]) break;
	}
	if(firstout == pattern.size()) {	// no output variables
		return new SimpleLookupGenerator(pt,vars);
	}
	else {	// there are output variables
		// TODO TODO TODO
	}
}

void GeneratorFactory::visit(EnumeratedInternalPredTable* table) {
	// TODO const map<ElementTuple,vector<ElementTuple> >& lookuptab = EnumFactory::instance()->get(table,pattern);
	// return new EnumLookupGenerator(lookuptab,invars,outvars)
}
