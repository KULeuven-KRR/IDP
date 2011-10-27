
#include <cassert>
#include "generators/GeneratorFactory.hpp"
#include "generators/SimpleFuncGenerator.hpp"
#include "generators/TreeInstGenerator.hpp"
#include "generators/GenerateAndTestGenerator.hpp"
#include "generators/InverseInstGenerator.hpp"
#include "generators/SortInstGenerator.hpp"
#include "generators/SimpleLookupGenerator.hpp"
#include "generators/EnumLookupGenerator.hpp"

#include "theory.hpp"
#include "structure.hpp"

using namespace std;

TwoChildGeneratorNode::TwoChildGeneratorNode(InstGenerator* c, InstGenerator* g, GeneratorNode* l, GeneratorNode* r) :
	_checker(c), _generator(g), _left(l), _right(r) {
	_left->parent(this);
	_right->parent(this);
}

/*******************
	Constructors
*******************/

GenerateAndTestGenerator::GenerateAndTestGenerator(const PredTable* t, const vector<bool>& pattern, const vector<const DomElemContainer*>& vars, const vector<unsigned int>& firstocc, const Universe& univ) : _table(t), _firstocc(firstocc), _currtuple(pattern.size()) {
	vector<SortTable*> outuniv;
	for(unsigned int n = 0; n < pattern.size(); ++n) {
		if(pattern[n]) {
			_invars.push_back(vars[n]);
			_inposs.push_back(n);
		}
		else {
			_outposs.push_back(n);
			if(firstocc[n] == n) {
				_outvars.push_back(vars[n]);
				outuniv.push_back(univ.tables()[n]);
			}
		}
	}
	_full = new PredTable(new FullInternalPredTable(),Universe(outuniv));
}

InverseInstGenerator::InverseInstGenerator(PredTable* t, const vector<bool>& pattern, const vector<const DomElemContainer*>& vars) {
	// TODO Code below is correct, but can be optimized in case there are output variables that occur more than once in 'vars'
	vector<const DomElemContainer*> tabvars;
	for(unsigned int n = 0; n < pattern.size(); ++n) {
		if(pattern[n]) {
			tabvars.push_back(vars[n]);
		}
		else {
			_univvars.push_back(vars[n]);
			const DomElemContainer* d = new const DomElemContainer();
			_outtabvars.push_back(d);
			tabvars.push_back(d);
		}
	}
	_outtablegen = GeneratorFactory::create(t,pattern,tabvars,t->universe());
	PredTable temp(new FullInternalPredTable(),t->universe());
	_univgen = GeneratorFactory::create(&temp,pattern,vars,t->universe());
}

bool InverseInstGenerator::outIsSmaller() const {
	for(unsigned int n = 0; n < _outtabvars.size(); ++n) {
		if(*_outtabvars[n] < *_univvars[n]) return true;
		else if (*_outtabvars[n] > *_univvars[n]) return false;
	}
	return false;
}

bool InverseInstGenerator::univIsSmaller() const {
	for(unsigned int n = 0; n < _outtabvars.size(); ++n) {
		if(*_outtabvars[n] > *_univvars[n]) return true;
		else if (*_outtabvars[n] < *_univvars[n]) return false;
	}
	return false;
}

/*********************
	First instance
*********************/

bool InverseInstGenerator::first() const {
	_outend = false;
	if(_univgen->first()) {
		if(_outtablegen->first()) {
			while(outIsSmaller() && !_outend)  _outend = !_outtablegen->next();
			if(_outend || univIsSmaller()) return true;
			else return next();
		}
		else {
			_outend = true;
			return true;
		}
	}
	else return false;
}

bool GenerateAndTestGenerator::first() const {
	_currpos = _full->begin();
	if(_currpos.hasNext()) {
		const ElementTuple& tuple = *_currpos;
		for(unsigned int n = 0; n < _inposs.size(); ++n) {
			_currtuple[_inposs[n]] = _invars[n]->get();
		}
		for(unsigned int n = 0; n < _outposs.size(); ++n) {
			_currtuple[_outposs[n]] = tuple[_firstocc[_outposs[n]]];
		}
		if(_table->contains(_currtuple)) {
			for(unsigned int n = 0; n < tuple.size(); ++n) {
				*(_outvars[n]) = tuple[n];
			}
			return true;
		}
		else return next();
	}
	else return false;
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
	if(_generator->first()) {
		if(_checker->first()) {
			GeneratorNode* r = _right->first();
			if(r) return r;
		}
		else {
			GeneratorNode* r = _left->first();
			if(r) return r;
		}
		GeneratorNode* r = TwoChildGeneratorNode::next();
		return r;
	}
	else return 0;
}

bool TreeInstGenerator::first() const {
	if(!_root) return true;
	_curr = _root->first();
	return (_curr ? true : false);
}


/********************
	Next instance
********************/

bool InverseInstGenerator::next() const {
	if(_univgen->next()) {
		if(_outend || univIsSmaller()) return true;
		else {
			do {
				while(outIsSmaller() && !_outend) _outend = !_outtablegen->next();
				if(_outend || univIsSmaller()) return true;
			} while (_univgen->next());
		}
		return false;
	}
	else return false;
}

bool GenerateAndTestGenerator::next() const {
	++_currpos;
	while(_currpos.hasNext()) {
		const ElementTuple& tuple = *_currpos;
		for(unsigned int n = 0; n < _outposs.size(); ++n) {
			_currtuple[_outposs[n]] = tuple[_firstocc[_outposs[n]]];
		}
		if(_table->contains(_currtuple)) {
			for(unsigned int n = 0; n < tuple.size(); ++n) {
				*(_outvars[n]) = tuple[n];
			}
			return true;
		}
		++_currpos;
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

GeneratorNode* TwoChildGeneratorNode::next() const {
	while(true) {
		if(not _generator->next()){
			return NULL;
		} else {
			GeneratorNode* r;
			if(_checker->first()) {
				r = _right->first();
			} else {
				r = _left->first();
			}
			if(r != NULL){
				return r;
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
