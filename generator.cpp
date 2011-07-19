/************************************
	generator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "common.hpp"
#include "parseinfo.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "checker.hpp"
#include "fobdd.hpp"
#include "generator.hpp"
#include <cstdlib>
#include <typeinfo>
using namespace std;

/**************
	Classes
**************/

class EmptyGenerator : public InstGenerator {
	public:
		bool first()	const { return false;	}
		bool next()		const { return false;	}
};

class StrLessGenerator : public InstGenerator {
	private:
		SortTable*				_table;
		const DomainElement**	_leftvar;
		const DomainElement**	_rightvar;
		bool					_leftisinput;
		mutable SortIterator	_left;
		mutable SortIterator	_right;
	public:
		StrLessGenerator(SortTable* st, const DomainElement** lv, const DomainElement** rv, bool inp) : 
			_table(st), _leftvar(lv), _rightvar(rv), _leftisinput(inp), _left(_table->sortbegin()), _right(_table->sortbegin()) { }
		bool first() const {
			if(_leftisinput) _left = _table->sortiterator(*_leftvar);
			else _left = _table->sortbegin();
			_right = _left;
			if(_right.hasNext()) {
				++_right;
				if(_right.hasNext()) {
					*_rightvar = *_right;
					if(!_leftisinput) *_leftvar = *_left;
					return true;
				}
				else return false;
			}
			else return false;
		}

		bool next() const {
			++_right;
			if(_right.hasNext()) {
				*_rightvar = *_right;
				if(!_leftisinput) *_leftvar = *_left;
				return true;
			}
			else if(!_leftisinput) {
				++_left;
				if(_left.hasNext()) {
					_right = _left;
					++_right;
					if(_right.hasNext()) {
						*_rightvar = *_right;
						*_leftvar = *_left;
						return true;
					}
					else return false;
				}
				else return false;
			}
			else return false;
		}
};

class StrGreaterGenerator : public InstGenerator {
	private:
		SortTable*				_table;
		const DomainElement**	_leftvar;
		const DomainElement**	_rightvar;
		bool					_leftisinput;
		mutable SortIterator	_left;
		mutable SortIterator	_right;
	public:
		StrGreaterGenerator(SortTable* st, const DomainElement** lv, const DomainElement** rv, bool inp) : 
			_table(st), _leftvar(lv), _rightvar(rv), _leftisinput(inp), _left(_table->sortbegin()), _right(_table->sortbegin()) { }
		bool first() const {
			if(_leftisinput) _left = _table->sortiterator(*_leftvar);
			else _left = _table->sortbegin();
			_right = _table->sortbegin();
			if(_left.hasNext() && _right.hasNext()) {
				if(*(*_left) > *(*_right)) {
					*_rightvar = *_right;
					if(!_leftisinput) *_leftvar = *_left;
					return true;
				}
				else return false;
			}
			else return false;
		}

		bool next() const {
			++_right;
			if(_left.hasNext() && _right.hasNext()) {
				if(*(*_left) > *(*_right)) {
					*_rightvar = *_right;
					if(!_leftisinput) *_leftvar = *_left;
					return true;
				}
			}
			if(!_leftisinput) {
				++_left;
				_right = _table->sortbegin();
				if(_left.hasNext() && _right.hasNext()) {
					if(*(*_left) > *(*_right)) {
						*_rightvar = *_right;
						*_leftvar = *_left;
						return true;
					}
					else return false;
				}
				else return false;
			}
			else return false;
		}
};

class InvUNAGenerator : public InstGenerator {
	private:
		vector<const DomainElement**>	_outvars;
		vector<unsigned int>			_outpos;
		const DomainElement**			_resvar;
	public:
		InvUNAGenerator(const vector<bool>& pattern, const vector<const DomainElement**>& vars) {
			for(unsigned int n = 0; n < pattern.size(); ++n) {
				if(!pattern[n]) {
					_outvars.push_back(vars[n]);
					_outpos.push_back(n);
				}
			}
			_resvar = vars.back();
		}
		bool first() const {
			assert((*_resvar)->type() == DET_COMPOUND);
			const Compound* c = (*_resvar)->value()._compound;
			for(unsigned int n = 0; n < _outpos.size(); ++n) {
				*(_outvars[n]) = c->arg(_outpos[n]);
			}
			return true;
		}
		bool next() const { 
			return false;
		}
};

class PlusGenerator : public InstGenerator {
	private:
		const DomainElement**	_in1;
		const DomainElement**	_in2;
		const DomainElement**	_out;
		bool					_int;
	public:
		PlusGenerator(const DomainElement** in1, const DomainElement** in2, const DomainElement** out, bool i) :
			_in1(in1), _in2(in2), _out(out), _int(i) { }
		bool first() const { 
			if(_int) *_out = DomainElementFactory::instance()->create((*_in1)->value()._int + (*_in2)->value()._int);	
			else {
				double d1 = (*_in1)->type() == DET_DOUBLE ? (*_in1)->value()._double : double((*_in1)->value()._int);
				double d2 = (*_in2)->type() == DET_DOUBLE ? (*_in2)->value()._double : double((*_in2)->value()._int);
				*_out = DomainElementFactory::instance()->create(d1 + d2);
			}
			return true;
		}
		bool next()	const	{ return false;	}
};

class MinusGenerator : public InstGenerator {
	private:
		const DomainElement**	_in1;
		const DomainElement**	_in2;
		const DomainElement**	_out;
		bool					_int;
	public:
		MinusGenerator(const DomainElement** in1, const DomainElement** in2, const DomainElement** out, bool i) :
			_in1(in1), _in2(in2), _out(out), _int(i) { }
		bool first() const { 
			if(_int) *_out = DomainElementFactory::instance()->create((*_in1)->value()._int - (*_in2)->value()._int);	
			else {
				double d1 = (*_in1)->type() == DET_DOUBLE ? (*_in1)->value()._double : double((*_in1)->value()._int);
				double d2 = (*_in2)->type() == DET_DOUBLE ? (*_in2)->value()._double : double((*_in2)->value()._int);
				*_out = DomainElementFactory::instance()->create(d1 - d2);
			}
			return true;
		}
		bool next()	const	{ return false;	}
};

class TimesGenerator : public InstGenerator {
	private:
		const DomainElement**	_in1;
		const DomainElement**	_in2;
		const DomainElement**	_out;
		bool					_int;
	public:
		TimesGenerator(const DomainElement** in1, const DomainElement** in2, const DomainElement** out, bool i) :
			_in1(in1), _in2(in2), _out(out), _int(i) { }
		bool first() const { 
			if(_int) *_out = DomainElementFactory::instance()->create((*_in1)->value()._int * (*_in2)->value()._int);	
			else {
				double d1 = (*_in1)->type() == DET_DOUBLE ? (*_in1)->value()._double : double((*_in1)->value()._int);
				double d2 = (*_in2)->type() == DET_DOUBLE ? (*_in2)->value()._double : double((*_in2)->value()._int);
				*_out = DomainElementFactory::instance()->create(d1 * d2);
			}
			return true;
		}
		bool next()	const	{ return false;	}
};

class DivGenerator : public InstGenerator {
	private:
		const DomainElement**	_in1;
		const DomainElement**	_in2;
		const DomainElement**	_out;
		bool					_int;
	public:
		DivGenerator(const DomainElement** in1, const DomainElement** in2, const DomainElement** out, bool i) :
			_in1(in1), _in2(in2), _out(out), _int(i) { }
		bool first() const { 
			if(_int) {
				if((*_in2)->value()._int == 0) return false;
				*_out = DomainElementFactory::instance()->create((*_in1)->value()._int * (*_in2)->value()._int);	
			}
			else {
				double d1 = (*_in1)->type() == DET_DOUBLE ? (*_in1)->value()._double : double((*_in1)->value()._int);
				double d2 = (*_in2)->type() == DET_DOUBLE ? (*_in2)->value()._double : double((*_in2)->value()._int);
				if(d2 == 0) return false;
				*_out = DomainElementFactory::instance()->create(d1 / d2);
			}
			return true;
		}
		bool next() const	{ return false;	}
};

class InvAbsGenerator : public InstGenerator {
	private:
		const DomainElement**	_in;
		const DomainElement**	_out;
		bool					_int;
	public:
		InvAbsGenerator(const DomainElement** in, const DomainElement** out, bool i) :
			_in(in), _out(out), _int(i) { }
		bool first() const { 
			if(_int) {
				*_out =  DomainElementFactory::instance()->create(-((*_in)->value()._int));
			}
			else {
				double d = (*_in)->type() == DET_DOUBLE ? (*_in)->value()._double : double((*_in)->value()._int);
				*_out = DomainElementFactory::instance()->create(-d);
			}
			return true;
		}
		bool next()	const { 
			if(*_out == *_in) return false;
			else { *_out = *_in; return true;	}
		}
};

class UminGenerator : public InstGenerator {
	private:
		const DomainElement**	_in;
		const DomainElement**	_out;
		bool					_int;
	public:
		UminGenerator(const DomainElement** in, const DomainElement** out, bool i) :
			_in(in), _out(out), _int(i) { }
		bool first() const { 
			if(_int) {
				*_out =  DomainElementFactory::instance()->create(-((*_in)->value()._int));
			}
			else {
				double d = (*_in)->type() == DET_DOUBLE ? (*_in)->value()._double : double((*_in)->value()._int);
				*_out = DomainElementFactory::instance()->create(-d);
			}
			return true;
		}
		bool next()	const { return false;	}
};

class TableInstGenerator : public InstGenerator { 
	private:
		const PredTable*					_table;
		vector<const DomainElement**>		_outvars;
		mutable TableIterator				_currpos;
	public:
		TableInstGenerator(const PredTable* t, const vector<const DomainElement**>& out) : 
			_table(t), _outvars(out), _currpos(t->begin()) { }
		bool first() const;
		bool next() const;
};

class SimpleFuncGenerator : public InstGenerator {
	private:
		const FuncTable*						_function;
		InstGenerator*							_univgen;
		vector<unsigned int>					_inposs;
		vector<unsigned int>					_outposs;
		vector<const DomainElement**>			_invars;
		const DomainElement**					_outvar;
		mutable vector<const DomainElement*>	_currinput;
	public:
		SimpleFuncGenerator(const FuncTable*, const vector<bool>&, const vector<const DomainElement**>&, const Universe&);
		bool first()	const;
		bool next()		const;
};

SimpleFuncGenerator::SimpleFuncGenerator(const FuncTable* ft, const vector<bool>& pattern, const vector<const DomainElement**>& vars, const Universe& univ) : _function(ft), _currinput(pattern.size()-1) {
	_invars = vars; _invars.pop_back();
	_outvar = vars.back();
	vector<const DomainElement**> univvars;
	vector<SortTable*> univtabs;
	for(unsigned int n = 0; n < pattern.size() - 1; ++n) {
		if(pattern[n]) {
			_inposs.push_back(n);
		}
		else {
			_outposs.push_back(n);
			univvars.push_back(vars[n]);
			univtabs.push_back(univ.tables()[n]);
		}
	}
	GeneratorFactory gf;
	_univgen = gf.create(univvars,univtabs);
}

bool SimpleFuncGenerator::first() const {
	if(_univgen->first()) {
		for(unsigned int n = 0; n < _inposs.size(); ++n) {
			_currinput[_inposs[n]] = *_invars[n];
		}
		for(unsigned int n = 0; n < _outposs.size(); ++n) {
			_currinput[_outposs[n]] = *_invars[n];
		}
		const DomainElement* d = _function->operator[](_currinput);
		if(d) {
			*_outvar = d;
			return true;
		}
		else return next();
	}
	else return false;
}

bool SimpleFuncGenerator::next() const {
	while(_univgen->next()) {
		for(unsigned int n = 0; n < _outposs.size(); ++n) {
			_currinput[_outposs[n]] = *_invars[n];
		}
		const DomainElement* d = _function->operator[](_currinput);
		if(d) {
			*_outvar = d;
			return true;
		}
	}
	return false;
}

class EqualGenerator : public InstGenerator {
	private:
		const DomainElement**	_in;
		const DomainElement**	_out;
	public:
		EqualGenerator(const DomainElement** in, const DomainElement** out) : _in(in), _out(out) { }
		bool first() const { *_out = *_in;	return true;	}
		bool next()	const { return false;	}
};

class GenerateAndTestGenerator : public InstGenerator {
	private:	
		const PredTable*							_table;		
		PredTable*									_full;
		vector<const DomainElement**>				_invars;
		vector<const DomainElement**>				_outvars;
		vector<unsigned int>						_inposs;
		vector<unsigned int>						_outposs;
		vector<unsigned int>						_firstocc;
		mutable TableIterator						_currpos;
		mutable ElementTuple						_currtuple;
	public:
		GenerateAndTestGenerator(const PredTable*,const vector<bool>&, const vector<const DomainElement**>&, const vector<unsigned int>&);
		bool first()	const;
		bool next()		const;
};

class SimpleLookupGenerator : public InstGenerator {
	private:
		const PredTable*						_table;
		vector<const DomainElement**>			_invars;
		mutable vector<const DomainElement*>	_currargs;
	public:
		SimpleLookupGenerator(const PredTable* t, const vector<const DomainElement**> in) :
			_table(t), _invars(in), _currargs(in.size()) { }
		bool first()	const;
		bool next()		const { return false;	}
};

typedef map<ElementTuple,vector<ElementTuple>,StrictWeakTupleOrdering> LookupTable;

class EnumLookupGenerator : public InstGenerator {
	private:
		const LookupTable&												_table;
		vector<const DomainElement**>									_invars;
		vector<const DomainElement**>									_outvars;
		mutable vector<const DomainElement*>							_currargs;
		mutable LookupTable::const_iterator								_currpos;
		mutable vector<vector<const DomainElement*> >::const_iterator	_iter;
	public:
		EnumLookupGenerator(const LookupTable& t, const vector<const DomainElement**>& in, const vector<const DomainElement**>& out) : _table(t), _invars(in), _outvars(out), _currargs(in.size()) { }
		bool first()	const;
		bool next()		const;
};

class SortInstGenerator : public InstGenerator { 
	private:
		const InternalSortTable*	_table;
		const DomainElement**		_outvar;
		mutable SortIterator		_currpos;
	public:
		SortInstGenerator(const InternalSortTable* t, const DomainElement** out) : 
			_table(t), _outvar(out), _currpos(t->sortbegin()) { }
		bool first() const;
		bool next() const;
};

class SortLookUpGenerator : public InstGenerator {
	private:
		const InternalSortTable*	_table;
		const DomainElement**		_invar;
	public:
		SortLookUpGenerator(const InternalSortTable* t, const DomainElement** in) : _table(t), _invar(in) { }
		bool first()	const { return _table->contains(*_invar);	}
		bool next()		const { return false;						}
};

class InverseInstGenerator : public InstGenerator {
	private:
		vector<const DomainElement**>	_univvars;
		vector<const DomainElement**>	_outtabvars;
		InstGenerator*					_univgen;
		InstGenerator*					_outtablegen;
		mutable bool					_outend;
		bool outIsSmaller() const;
		bool univIsSmaller() const;
	public:
		InverseInstGenerator(PredTable* t, const vector<bool>&, const vector<const DomainElement**>&);
		bool first() const;
		bool next() const;
};


/********************************************
	Tree structure for generating instances
********************************************/

class GeneratorNode {
	protected:
		GeneratorNode*	_parent;

	public:
		// Constructors
		GeneratorNode() : _parent(0) { }

		// Mutators
		void	parent(GeneratorNode* n) { _parent = n;	}

		// Inspectors
		GeneratorNode*	parent()	const { return _parent;	}

		// Generate instances
		virtual	GeneratorNode*	first()	const = 0;
		virtual	GeneratorNode*	next()	const = 0;
};

class LeafGeneratorNode : public GeneratorNode {
	private:
		InstGenerator*	_generator;
		GeneratorNode*	_this;		//	equal to 'this'
	
	public:
		// Constructor
		LeafGeneratorNode(InstGenerator* gt) : GeneratorNode(), _generator(gt) { _this = this;	}

		// Generate instances
		GeneratorNode*	first()	const;
		GeneratorNode*	next()	const;
};

class OneChildGeneratorNode : public GeneratorNode {
	private:
		InstGenerator*	_generator;
		GeneratorNode*	_child;
	
	public:
		// Constructor
		OneChildGeneratorNode(InstGenerator* gt, GeneratorNode* c) : GeneratorNode(), _generator(gt), _child(c) { _child->parent(this);	}

		// Generate instances
		GeneratorNode*	first()	const;
		GeneratorNode*	next()	const;
};

class TwoChildGeneratorNode : public GeneratorNode {
	private:
		InstanceChecker*						_checker;
		vector<const DomainElement**>			_outvars;
		InstGenerator*							_currposition;
		mutable vector<const DomainElement*>	_currargs;
		GeneratorNode*							_left;
		GeneratorNode*							_right;
	
	public:
		// Constructor
		TwoChildGeneratorNode(InstanceChecker* t, const vector<const DomainElement**>& ov, const vector<SortTable*>& tbs, GeneratorNode* l, GeneratorNode* r);

		// Generate instances
		GeneratorNode*	first()	const;
		GeneratorNode*	next()	const;
};


/***************************************
	Root node for generating instances
***************************************/

class TreeInstGenerator : public InstGenerator {
	private:
				GeneratorNode*	_root;
		mutable	GeneratorNode*	_curr;	// Remember the last position of a match

	public:
		// Constructor
		TreeInstGenerator(GeneratorNode* r) : _root(r), _curr(0) { }

		// Generate instances
		bool	first()	const;
		bool	next()	const;
};

using namespace std;

TwoChildGeneratorNode::TwoChildGeneratorNode(InstanceChecker* t, const vector<const DomainElement**>& ov, const vector<SortTable*>& dom, GeneratorNode* l, GeneratorNode* r) :
	_checker(t), _outvars(ov), _currargs(ov.size()), _left(l), _right(r) {
	PredTable* domtab = new PredTable(new FullInternalPredTable(),Universe(dom));
	_currposition = new TableInstGenerator(domtab,ov);
}

/*******************
	Constructors
*******************/

GenerateAndTestGenerator::GenerateAndTestGenerator(const PredTable* t, const vector<bool>& pattern, const vector<const DomainElement**>& vars, const vector<unsigned int>& firstocc) : _table(t), _firstocc(firstocc), _currtuple(pattern.size()) {
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
				outuniv.push_back(t->universe().tables()[n]);
			}
		}
	}
	_full = new PredTable(new FullInternalPredTable(),Universe(outuniv));
}

InverseInstGenerator::InverseInstGenerator(PredTable* t, const vector<bool>& pattern, const vector<const DomainElement**>& vars) {
	vector<const DomainElement**> tabvars;
	for(unsigned int n = 0; n < pattern.size(); ++n) {
		if(pattern[n]) {
			tabvars.push_back(vars[n]);
		}
		else {
			_univvars.push_back(vars[n]);
			const DomainElement** d = new const DomainElement*();
			_outtabvars.push_back(d);
			tabvars.push_back(d);
		}
	}
	GeneratorFactory gf;
	_outtablegen = gf.create(t,pattern,tabvars);
	PredTable temp(new FullInternalPredTable(),t->universe());
	_univgen = gf.create(&temp,pattern,vars);
}

bool InverseInstGenerator::outIsSmaller() const {
	for(unsigned int n = 0; n < _outtabvars.size(); ++n) {
		if(*(*_outtabvars[n]) < *(*_univvars[n])) return true;
		else if (*(*_outtabvars[n]) > *(*_univvars[n])) return false;
	}
	return false;
}

bool InverseInstGenerator::univIsSmaller() const {
	for(unsigned int n = 0; n < _outtabvars.size(); ++n) {
		if(*(*_outtabvars[n]) > *(*_univvars[n])) return true;
		else if (*(*_outtabvars[n]) < *(*_univvars[n])) return false;
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
			_currtuple[_inposs[n]] = *(_invars[n]);
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
		SortInstGenerator* tig = new SortInstGenerator(tabs[n]->interntable(),vars[n]);
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

InstGenerator*	GeneratorFactory::create(const PredTable* pt, vector<bool> pattern, const vector<const DomainElement**>& vars) {
	_table = pt;
	_pattern = pattern;
	_vars = vars;
	for(unsigned int n = 0; n < _vars.size(); ++n) {
		_firstocc.push_back(n);
		for(unsigned int m = 0; m < n; ++m) {
			if(_vars[n] == _vars[m]) {
				_firstocc[n] = m;
				break;
			}
		}
	}

	unsigned int firstout = 0;
	for( ; firstout < pattern.size(); ++firstout) {
		if(!pattern[firstout]) break;
	}
	if(firstout == pattern.size()) {	// no output variables
		return new SimpleLookupGenerator(pt,vars);
	}
	else {
		StructureVisitor::visit(pt);
		return _generator;
	}
}

void GeneratorFactory::visit(const ProcInternalPredTable* ) {
	_generator = new GenerateAndTestGenerator(_table,_pattern,_vars,_firstocc);
}

/******************************
	From BDDs to generators
******************************/

BDDToGenerator::BDDToGenerator(FOBDDManager* manager) : _manager(manager) { }

InstGenerator* BDDToGenerator::create(const FOBDD* bdd, const vector<bool>& pattern, const vector<const DomainElement**>& vars, const vector<const FOBDDVariable*>& bddvars, AbstractStructure* structure) {
	if(bdd == _manager->falsebdd()) {
		return new EmptyGenerator();
	}
	else if(bdd == _manager->truebdd()) {
		vector<const DomainElement**> outvars;
		vector<SortTable*> tables;
		for(unsigned int n = 0; n < pattern.size(); ++n) {
			if(!pattern[n]) {
				outvars.push_back(vars[n]);
				tables.push_back(structure->inter(bddvars[n]->sort()));
			}
		}
		GeneratorFactory gf;
		InstGenerator* result = gf.create(outvars,tables);
		return result;
	}
	else {
		GeneratorNode* gn = createnode(bdd,pattern,vars,bddvars,structure);
		return new TreeInstGenerator(gn);
	}
}

GeneratorNode* BDDToGenerator::createnode(const FOBDD* bdd, const vector<bool>& pattern, const vector<const DomainElement**>& vars, const vector<const FOBDDVariable*>& bddvars, AbstractStructure* structure) {
	if(bdd == _manager->falsebdd()) {
		EmptyGenerator* eg = new EmptyGenerator();
		return new LeafGeneratorNode(eg);
	}
	else if(bdd == _manager->truebdd()) {
		vector<const DomainElement**> outvars;
		vector<SortTable*> tables;
		for(unsigned int n = 0; n < pattern.size(); ++n) {
			if(!pattern[n]) {
				outvars.push_back(vars[n]);
				tables.push_back(structure->inter(bddvars[n]->sort()));
			}
		}
		GeneratorFactory gf;
		InstGenerator* ig = gf.create(outvars,tables);
		return new LeafGeneratorNode(ig);
	}
	else {
		// split variables
		vector<bool> kernpattern;
		vector<const DomainElement**> kerngenvars;
		vector<const FOBDDVariable*> kernvars;
		vector<bool> branchpattern;
		for(unsigned int n = 0; n < pattern.size(); ++n) {
			if(_manager->contains(bdd->kernel(),bddvars[n])) {
				kernpattern.push_back(pattern[n]);
				kerngenvars.push_back(vars[n]);
				kernvars.push_back(bddvars[n]);
				branchpattern.push_back(true);
			}
			else {
				branchpattern.push_back(pattern[n]);
			}
		}
		
		
		// recursive case
		if(bdd->falsebranch() == _manager->falsebdd()) {
			InstGenerator* kernelgenerator = create(bdd->kernel(),kernpattern,kerngenvars,kernvars,structure);
			GeneratorNode* truegenerator = createnode(bdd->truebranch(),branchpattern,vars,bddvars,structure);
			return new OneChildGeneratorNode(kernelgenerator,truegenerator);
		}
		/*
		else if(bdd->truebranch() == _manager->falsebdd()) {
			InstGenerator* kernelgenerator = // TODO
			GeneratorNode* falsegenerator = // TODO
			OneChildGeneratorNode* ocgn = new OneChildGeneratorNode(kernelgenerator,falsegenerator);
			return new TreeInstGenerator(ocgn);
		}
		else {
			InstanceChecker* kernelchecker = // TODO
			GeneratorNode* truegenerator = // TODO
			GeneratorNode* falsegenerator = // TODO	*/
			//TwoChildGeneratorNode tcgn = new TwoChildGeneratorNode(kernelchecker,/*TODO*/,/*TODO*/,falsegenerator,truegenerator);
			//return new TreeInstGenerator(tcgn);
		//}
		return 0;
	}
}

InstGenerator* BDDToGenerator::create(const FOBDDKernel* kernel, const vector<bool>& pattern, const vector<const DomainElement**>& vars, const vector<const FOBDDVariable*>& kernelvars, AbstractStructure* structure) {
	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atom = dynamic_cast<const FOBDDAtomKernel*>(kernel);

		// Create the pattern for the atom
		vector<bool> atompattern;
		vector<const DomainElement**> atomvars;
		for(vector<const FOBDDArgument*>::const_iterator it = atom->args().begin(); it != atom->args().end(); ++it) {
			if(typeid(*(*it)) == typeid(FOBDDVariable)) {
				const FOBDDVariable* var = dynamic_cast<const FOBDDVariable*>(*it);
				unsigned int pos = 0;
				for(; pos < pattern.size(); ++pos) {
					if(kernelvars[pos] == var) break;
				}
				assert(pos < pattern.size());
				atompattern.push_back(pattern[pos]);
				atomvars.push_back(vars[pos]);
			}
			else {
				// TODO
				return 0;
			}
		}

		// Construct the generator
		PFSymbol* symbol = atom->symbol();
		const PredTable* table = 0;
		if(typeid(*symbol) == typeid(Predicate)) {
			table = structure->inter(dynamic_cast<Predicate*>(symbol))->ct();
		}
		else {
			assert(typeid(*symbol) == typeid(Function));
			table = structure->inter(dynamic_cast<Function*>(symbol))->graphinter()->ct();
		}
		GeneratorFactory gf;
		return gf.create(table,atompattern,atomvars);
	}
	else {
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		// TODO
		return 0;
	}
}

void GeneratorFactory::visit(const BDDInternalPredTable* table) {

	// Add necessary types to the bdd to ensure, if possible, finite querying
	FOBDDManager optimizemanager;
	const FOBDD* copybdd = optimizemanager.getBDD(table->bdd(),table->manager());
	// TODO

	// Optimize the bdd for querying
	set<const FOBDDVariable*> outvars;
	vector<const FOBDDVariable*> allvars;
	for(unsigned int n = 0; n < _pattern.size(); ++n) {
		const FOBDDVariable* var = optimizemanager.getVariable(table->vars()[n]);
		allvars.push_back(var);
		if(!_pattern[n]) outvars.insert(var);
	}
	set<const FOBDDDeBruijnIndex*> indices;
	optimizemanager.optimizequery(copybdd,outvars,indices,table->structure());

	// Generate a generator for the optimized bdd
	BDDToGenerator btg(&optimizemanager);
	_generator = btg.create(copybdd,_pattern,_vars,allvars,table->structure());
}

void GeneratorFactory::visit(const FullInternalPredTable* ) {
	vector<const DomainElement**> outvars;
	vector<SortTable*> outtables;
	for(unsigned int n = 0; n < _pattern.size(); ++n) {
		if((!_pattern[n]) && _firstocc[n] == n) {
			outvars.push_back(_vars[n]);
			outtables.push_back(_table->universe().tables()[n]);
		}
	}
	_generator = create(outvars,outtables);
}

void GeneratorFactory::visit(const FuncInternalPredTable* fipt) {
	visit(fipt->table());
}

void GeneratorFactory::visit(const UnionInternalPredTable* ) {
	// TODO
}

void GeneratorFactory::visit(const UnionInternalSortTable*) {
	// TODO
}

void GeneratorFactory::visit(const AllNaturalNumbers* t) {
	if(_pattern[0]) _generator = new SortLookUpGenerator(t,_vars[0]);
	else _generator = new SortInstGenerator(t,_vars[0]);
}
void GeneratorFactory::visit(const AllIntegers* t) {						
	if(_pattern[0]) _generator = new SortLookUpGenerator(t,_vars[0]);
	else _generator = new SortInstGenerator(t,_vars[0]);
}
void GeneratorFactory::visit(const AllFloats* t) {					
	if(_pattern[0]) _generator = new SortLookUpGenerator(t,_vars[0]);
	else _generator = new SortInstGenerator(t,_vars[0]);
}
void GeneratorFactory::visit(const AllChars* t) {				
	if(_pattern[0]) _generator = new SortLookUpGenerator(t,_vars[0]);
	else _generator = new SortInstGenerator(t,_vars[0]);
}
void GeneratorFactory::visit(const AllStrings* t)	{				
	if(_pattern[0]) _generator = new SortLookUpGenerator(t,_vars[0]);
	else _generator = new SortInstGenerator(t,_vars[0]);
}
void GeneratorFactory::visit(const EnumeratedInternalSortTable* t) {
	if(_pattern[0]) _generator = new SortLookUpGenerator(t,_vars[0]);
	else _generator = new SortInstGenerator(t,_vars[0]);
}
void GeneratorFactory::visit(const IntRangeInternalSortTable* t) {
	if(_pattern[0]) _generator = new SortLookUpGenerator(t,_vars[0]);
	else _generator = new SortInstGenerator(t,_vars[0]);
}

void GeneratorFactory::visit(const EnumeratedInternalPredTable* ) {
	// TODO: Use dynamic programming to improve this
	LookupTable* lpt = new LookupTable();
	LookupTable& lookuptab = *lpt;
	vector<const DomainElement**> invars;
	vector<const DomainElement**> outvars;
	for(unsigned int n = 0; n < _pattern.size(); ++n) {
		if(_firstocc[n] == n) {
			if(_pattern[n]) invars.push_back(_vars[n]);
			else outvars.push_back(_vars[n]);
		}
	}
	for(TableIterator it = _table->begin(); it.hasNext(); ++it) {
		const ElementTuple& tuple = *it;
		bool ok = true;
		for(unsigned int n = 0; n < _pattern.size(); ++n) {
			if(_firstocc[n] != n && tuple[n] != tuple[_firstocc[n]]) {
				ok = false;
				break;
			}
		}
		if(ok) {
			ElementTuple intuple;
			ElementTuple outtuple;
			for(unsigned int n = 0; n < _pattern.size(); ++n) {
				if(_firstocc[n] == n) {
					if(_pattern[n]) {
						intuple.push_back(tuple[n]);
					}
					else if(_firstocc[n] == n) {
						outtuple.push_back(tuple[n]);
					}
				}
			}
			lookuptab[intuple].push_back(outtuple);
		}
	}
	_generator = new EnumLookupGenerator(lookuptab,invars,outvars);
}

void GeneratorFactory::visit(const EqualInternalPredTable*) {
	if(_pattern[0]) { assert(!_pattern[1]); _generator = new EqualGenerator(_vars[0],_vars[1]); }
	else if(_pattern[1]) _generator = new EqualGenerator(_vars[1],_vars[0]);
	else if(_firstocc[1] == 0) {
		create(vector<const DomainElement**>(1,_vars[0]), vector<SortTable*>(1,_table->universe().tables()[0]));
	}
	else _generator = new TableInstGenerator(_table,_vars); 
}

void GeneratorFactory::visit(const StrLessInternalPredTable* ) {
	if(_pattern[0]) {
		_generator = new StrLessGenerator(_table->universe().tables()[0],_vars[0],_vars[1],true);
	}
	else if(_pattern[1]) {
		_generator = new StrGreaterGenerator(_table->universe().tables()[0],_vars[1],_vars[0],true);
	}
	else {
		_generator = new StrLessGenerator(_table->universe().tables()[0],_vars[0],_vars[1],false);
	}
}

void GeneratorFactory::visit(const StrGreaterInternalPredTable* ) {
	if(_pattern[0]) {
		_generator = new StrGreaterGenerator(_table->universe().tables()[0],_vars[0],_vars[1],true);
	}
	else if(_pattern[1]) {
		_generator = new StrLessGenerator(_table->universe().tables()[0],_vars[1],_vars[0],true);
	}
	else {
		_generator = new StrGreaterGenerator(_table->universe().tables()[0],_vars[0],_vars[1],false);
	}
}

void GeneratorFactory::visit(const InverseInternalPredTable* iip) {
	// TODO: optimize by checking the type of the internal table!!!
	PredTable temp(iip->table(),_table->universe());
	_generator = new InverseInstGenerator(&temp,_pattern,_vars);
}

void GeneratorFactory::visit(const FuncTable* ft) {
	if(!_pattern.back()) {
		_generator = new SimpleFuncGenerator(ft,_pattern,_vars,_table->universe());
	}
	else ft->interntable()->accept(this);
}

void GeneratorFactory::visit(const ProcInternalFuncTable* ) {
	_generator = new GenerateAndTestGenerator(_table,_pattern,_vars,_firstocc);
}

void GeneratorFactory::visit(const UNAInternalFuncTable* ) {
	_generator = new InvUNAGenerator(_pattern,_vars);
}

void GeneratorFactory::visit(const EnumeratedInternalFuncTable*) {
	// TODO: Use dynamic programming to improve this
	LookupTable lookuptab;
	vector<const DomainElement**> invars;
	vector<const DomainElement**> outvars;
	for(unsigned int n = 0; n < _pattern.size(); ++n) {
		if(_pattern[n]) invars.push_back(_vars[n]);
		else outvars.push_back(_vars[n]);
	}
	for(TableIterator it = _table->begin(); it.hasNext(); ++it) {
		const ElementTuple& tuple = *it;
		ElementTuple intuple;
		ElementTuple outtuple;
		for(unsigned int n = 0; n < _pattern.size(); ++n) {
			if(_pattern[n]) intuple.push_back(tuple[n]);
			else outtuple.push_back(tuple[n]);
		}
		lookuptab[intuple].push_back(outtuple);
	}
	_generator = new EnumLookupGenerator(lookuptab,invars,outvars);
}

void GeneratorFactory::visit(const PlusInternalFuncTable* pift) {
	if(_pattern[0]) {
		_generator = new MinusGenerator(_vars[2],_vars[0],_vars[1],pift->isInt());
	}
	else if(_pattern[1]) {
		_generator = new MinusGenerator(_vars[2],_vars[1],_vars[0],pift->isInt());
	}
	else {
		notyetimplemented("Infinite generator for addition pattern (out,out,in)");
		exit(1);
	}
}

void GeneratorFactory::visit(const MinusInternalFuncTable* pift) {
	if(_pattern[0]) {
		_generator = new MinusGenerator(_vars[0],_vars[2],_vars[1],pift->isInt());
	}
	else if(_pattern[1]) {
		_generator = new PlusGenerator(_vars[1],_vars[2],_vars[0],pift->isInt());
	}
	else {
		notyetimplemented("Infinite generator for subtraction pattern (out,out,in)");
		exit(1);
	}
}

void GeneratorFactory::visit(const TimesInternalFuncTable* pift) {
	if(_pattern[0]) {
		_generator = new DivGenerator(_vars[2],_vars[0],_vars[1],pift->isInt());
	}
	else if(_pattern[1]) {
		_generator = new DivGenerator(_vars[2],_vars[1],_vars[0],pift->isInt());
	}
	else {
		notyetimplemented("Infinite generator for multiplication pattern (out,out,in)");
		exit(1);
	}
}

void GeneratorFactory::visit(const DivInternalFuncTable* pift) {
	if(_pattern[0]) {
		_generator = new DivGenerator(_vars[0],_vars[2],_vars[1],pift->isInt());
	}
	else if(_pattern[1]) {
		// FIXME: wrong in case of integers. E.g., a / 2 = 1 should result in a \in { 2,3 } instead of a \in { 2 }
		_generator = new TimesGenerator(_vars[1],_vars[2],_vars[0],pift->isInt());
	}
	else {
		notyetimplemented("Infinite generator for division pattern (out,out,in)");
		exit(1);
	}
}

void GeneratorFactory::visit(const ExpInternalFuncTable* ) {
	notyetimplemented("Infinite generator for exponentiation pattern (?,?,in)");
	exit(1);
}

void GeneratorFactory::visit(const ModInternalFuncTable* ) {
	notyetimplemented("Infinite generator for remainder pattern (?,?,in)");
	exit(1);
}

void GeneratorFactory::visit(const AbsInternalFuncTable* aift) {
	assert(!_pattern[0]);
	_generator = new InvAbsGenerator(_vars[1],_vars[0],aift->isInt());
}

void GeneratorFactory::visit(const UminInternalFuncTable* uift) {
	assert(!_pattern[0]);
	_generator = new UminGenerator(_vars[1],_vars[0],uift->isInt());
}


