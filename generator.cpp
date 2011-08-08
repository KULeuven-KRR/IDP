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
#include "term.hpp"
#include "theory.hpp"
#include <cstdlib>
#include <typeinfo>
#include <iostream>
using namespace std;

/**************
	Classes
**************/

class FalseQuantKernelGenerator : public InstGenerator {
	private:
		InstGenerator*	_quantgenerator;
		InstGenerator*	_univgenerator;
	public:
		FalseQuantKernelGenerator(InstGenerator* q, InstGenerator* u) : _quantgenerator(q), _univgenerator(u) { }
		bool first() const {
			if(_univgenerator->first()) {
				if(_quantgenerator->first()) return next();
				else return true;
			}
			else return false;
		}
		bool next() const {
			while(_univgenerator->next()) {
				if(!_quantgenerator->first()) return true;
			}
			return false;
		}
};

class TestQuantKernelGenerator : public InstGenerator {
	private:
		InstGenerator*	_quantgenerator;
	public:
		TestQuantKernelGenerator(InstGenerator* gen) : _quantgenerator(gen) { }
		bool first()	const { return _quantgenerator->first();	}
		bool next()		const { return false;						}
};

class TrueQuantKernelGenerator : public InstGenerator {
	private:
		InstGenerator*					_quantgenerator;
		vector<const DomainElement**>	_projectvars;
		mutable set<ElementTuple>		_memory;	
	public:

		TrueQuantKernelGenerator(InstGenerator* gen, const vector<const DomainElement**> projv) :
			_quantgenerator(gen), _projectvars(projv) { }

		bool storetuple() const {
			ElementTuple tuple;
			for(vector<const DomainElement**>::const_iterator it = _projectvars.begin(); it != _projectvars.end(); ++it)
				tuple.push_back(*(*it));
			pair<set<ElementTuple>::iterator, bool> p = _memory.insert(tuple);
			return p.second;
		}

		bool first() const {
			if(_quantgenerator->first()) {
				if(storetuple()) return true;
				else return next();
			}
			else return false;
		}

		bool next() const {
			while(_quantgenerator->next()) {
				if(storetuple()) return true;
			}
			return false;
		}
};

class EmptyGenerator : public InstGenerator {
	public:
		bool first()	const { return false;	}
		bool next()		const { return false;	}
};

class StrLessGenerator : public InstGenerator {
	// FIXME: allow for different domains for left and right variable
	private:
		SortTable*				_table;
		const DomainElement**	_leftvar;
		const DomainElement**	_rightvar;
		bool					_leftisinput;
		mutable SortIterator	_left;
		mutable SortIterator	_right;
	public:
		StrLessGenerator(SortTable* st, SortTable* fixme, const DomainElement** lv, const DomainElement** rv, bool inp) : 
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
	// FIXME: allow for different domains for left and right variable
	private:
		SortTable*				_table;
		const DomainElement**	_leftvar;
		const DomainElement**	_rightvar;
		bool					_leftisinput;
		mutable SortIterator	_left;
		mutable SortIterator	_right;
	public:
		StrGreaterGenerator(SortTable* st, SortTable* fixme, const DomainElement** lv, const DomainElement** rv, bool inp) : 
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
				else return next();
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
			if(!_leftisinput && _left.hasNext()) {
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
		Universe						_universe;
		const DomainElement**			_resvar;
	public:
		InvUNAGenerator(const vector<bool>& pattern, const vector<const DomainElement**>& vars, const Universe& univ) {
			_universe = univ;
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
			unsigned int n = 0;
			for(; n < _outpos.size(); ++n) {
				if(_universe.tables()[_outpos[n]]->contains(c->arg(_outpos[n]))) {
					*(_outvars[n]) = c->arg(_outpos[n]);
				}
				else break;
			}
			return (n == _outpos.size());
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
		SortTable*				_outdom;
	public:
		PlusGenerator(const DomainElement** in1, const DomainElement** in2, const DomainElement** out, bool i, SortTable* dom) :
			_in1(in1), _in2(in2), _out(out), _int(i), _outdom(dom) { }
		bool first() const { 
			if(_int) *_out = DomainElementFactory::instance()->create((*_in1)->value()._int + (*_in2)->value()._int);	
			else {
				double d1 = (*_in1)->type() == DET_DOUBLE ? (*_in1)->value()._double : double((*_in1)->value()._int);
				double d2 = (*_in2)->type() == DET_DOUBLE ? (*_in2)->value()._double : double((*_in2)->value()._int);
				*_out = DomainElementFactory::instance()->create(d1 + d2);
			}
			return _outdom->contains(*_out);
		}
		bool next()	const	{ return false;	}
};

class MinusGenerator : public InstGenerator {
	private:
		const DomainElement**	_in1;
		const DomainElement**	_in2;
		const DomainElement**	_out;
		bool					_int;
		SortTable*				_outdom;
	public:
		MinusGenerator(const DomainElement** in1, const DomainElement** in2, const DomainElement** out, bool i, SortTable* dom) :
			_in1(in1), _in2(in2), _out(out), _int(i), _outdom(dom) { }
		bool first() const { 
			if(_int) *_out = DomainElementFactory::instance()->create((*_in1)->value()._int - (*_in2)->value()._int);	
			else {
				double d1 = (*_in1)->type() == DET_DOUBLE ? (*_in1)->value()._double : double((*_in1)->value()._int);
				double d2 = (*_in2)->type() == DET_DOUBLE ? (*_in2)->value()._double : double((*_in2)->value()._int);
				*_out = DomainElementFactory::instance()->create(d1 - d2);
			}
			return _outdom->contains(*_out);
		}
		bool next()	const	{ return false;	}
};

class TimesGenerator : public InstGenerator {
	private:
		const DomainElement**	_in1;
		const DomainElement**	_in2;
		const DomainElement**	_out;
		bool					_int;
		SortTable*				_outdom;
	public:
		TimesGenerator(const DomainElement** in1, const DomainElement** in2, const DomainElement** out, bool i, SortTable* dom) :
			_in1(in1), _in2(in2), _out(out), _int(i), _outdom(dom) { }
		bool first() const { 
			if(_int) *_out = DomainElementFactory::instance()->create((*_in1)->value()._int * (*_in2)->value()._int);	
			else {
				double d1 = (*_in1)->type() == DET_DOUBLE ? (*_in1)->value()._double : double((*_in1)->value()._int);
				double d2 = (*_in2)->type() == DET_DOUBLE ? (*_in2)->value()._double : double((*_in2)->value()._int);
				*_out = DomainElementFactory::instance()->create(d1 * d2);
			}
			return _outdom->contains(*_out);
		}
		bool next()	const	{ return false;	}
};

class DivGenerator : public InstGenerator {
	private:
		const DomainElement**	_in1;
		const DomainElement**	_in2;
		const DomainElement**	_out;
		bool					_int;
		SortTable*				_outdom;
	public:
		DivGenerator(const DomainElement** in1, const DomainElement** in2, const DomainElement** out, bool i, SortTable* dom) :
			_in1(in1), _in2(in2), _out(out), _int(i), _outdom(dom) { }
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
			return _outdom->contains(*_out);
		}
		bool next() const	{ return false;	}
};

class InvAbsGenerator : public InstGenerator {
	private:
		const DomainElement**	_in;
		const DomainElement**	_out;
		bool					_int;
		SortTable*				_outdom;
	public:
		InvAbsGenerator(const DomainElement** in, const DomainElement** out, bool i, SortTable* dom) :
			_in(in), _out(out), _int(i), _outdom(dom) { }
		bool first() const { 
			if(_int) {
				*_out =  DomainElementFactory::instance()->create(-((*_in)->value()._int));
			}
			else {
				double d = (*_in)->type() == DET_DOUBLE ? (*_in)->value()._double : double((*_in)->value()._int);
				*_out = DomainElementFactory::instance()->create(-d);
			}
			if(_outdom->contains(*_out)) return true;
			else return next();
		}
		bool next()	const { 
			if(*_out == *_in) return false;
			else { *_out = *_in; return _outdom->contains(*_out);	}
		}
};

class UminGenerator : public InstGenerator {
	private:
		const DomainElement**	_in;
		const DomainElement**	_out;
		bool					_int;
		SortTable*				_outdom;
	public:
		UminGenerator(const DomainElement** in, const DomainElement** out, bool i, SortTable* dom) :
			_in(in), _out(out), _int(i), _outdom(dom) { }
		bool first() const { 
			if(_int) {
				*_out =  DomainElementFactory::instance()->create(-((*_in)->value()._int));
			}
			else {
				double d = (*_in)->type() == DET_DOUBLE ? (*_in)->value()._double : double((*_in)->value()._int);
				*_out = DomainElementFactory::instance()->create(-d);
			}
			return _outdom->contains(*_out);
		}
		bool next()	const { return false;	}
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
		unsigned int							_check;
	public:
		SimpleFuncGenerator(const FuncTable*, const vector<bool>&, const vector<const DomainElement**>&, const Universe&, const vector<unsigned int>&);
		bool first()	const;
		bool next()		const;
};

SimpleFuncGenerator::SimpleFuncGenerator(const FuncTable* ft, const vector<bool>& pattern, const vector<const DomainElement**>& vars, const Universe& univ, const vector<unsigned int>& firstocc) : _function(ft), _currinput(pattern.size()-1) {
	_invars = vars; _invars.pop_back();
	_outvar = vars.back();
	_check = firstocc[pattern.size() -1];
	vector<const DomainElement**> univvars;
	vector<SortTable*> univtabs;
	for(unsigned int n = 0; n < pattern.size() - 1; ++n) {
		if(pattern[n]) {
			_inposs.push_back(n);
		}
		else {
			_outposs.push_back(n);
			if(firstocc[n] == n) {
				univvars.push_back(vars[n]);
				univtabs.push_back(univ.tables()[n]);
			}
		}
	}
	GeneratorFactory gf;
	_univgen = gf.create(univvars,univtabs);
}

bool SimpleFuncGenerator::first() const {
	if(_univgen->first()) {
		for(unsigned int n = 0; n < _inposs.size(); ++n) {
			_currinput[_inposs[n]] = *_invars[_inposs[n]];
		}
		for(unsigned int n = 0; n < _outposs.size(); ++n) {
			_currinput[_outposs[n]] = *_invars[_outposs[n]];
		}
		const DomainElement* d = _function->operator[](_currinput);
		if(d) {
			if(_check == _invars.size()) {
				*_outvar = d;
				return true;
			}
			else if(*_invars[_check] == d) {
				return true;
			}
			else return next();
		}
		else return next();
	}
	else return false;
}

bool SimpleFuncGenerator::next() const {
	while(_univgen->next()) {
		for(unsigned int n = 0; n < _outposs.size(); ++n) {
			_currinput[_outposs[n]] = *_invars[_outposs[n]];
		}
		const DomainElement* d = _function->operator[](_currinput);
		if(d) {
			if(_check == _invars.size()) {
				*_outvar = d;
				return true;
			}
			else if(*_invars[_check] == d) {
				return true;
			}
		}
	}
	return false;
}

class EqualGenerator : public InstGenerator {
	private:
		const DomainElement**	_in;
		const DomainElement**	_out;
		SortTable*				_indom;
		SortTable*				_outdom;
		mutable SortIterator	_curr;
	public:
		EqualGenerator(const DomainElement** in, const DomainElement** out, SortTable* outdom) : 
			_in(in), _out(out), _indom(0), _outdom(outdom), _curr(_outdom->sortbegin()) { }
		EqualGenerator(const DomainElement** in, const DomainElement** out, SortTable* indom, SortTable* outdom) : 
			_in(in), _out(out), _indom(indom), _outdom(outdom), _curr(_indom->sortbegin()) { }
		bool first() const { 
			if(_indom) {
				_curr = _indom->sortbegin();
				if(!_curr.hasNext()) return false;
				*_in = *_curr;
			}
			*_out = *_in;	
			if(_outdom->contains(*_out)) return true;
			else return next();
		}
		bool next()	const { 
			if(_indom) {
				++_curr;
				while(_curr.hasNext()) {
					*_in = *_curr;
					*_out = *_in;
					if(_outdom->contains(*_out)) return true;
					++_curr;
				}
			}
			return false;	
		}
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
		GenerateAndTestGenerator(const PredTable*,const vector<bool>&, const vector<const DomainElement**>&, const vector<unsigned int>&, const Universe& univ);
		bool first()	const;
		bool next()		const;
};

class SimpleLookupGenerator : public InstGenerator {
	private:
		const PredTable*						_table;
		vector<const DomainElement**>			_invars;
		Universe								_universe;
		mutable vector<const DomainElement*>	_currargs;
	public:
		SimpleLookupGenerator(const PredTable* t, const vector<const DomainElement**> in, const Universe& univ) :
			_table(t), _invars(in), _universe(univ), _currargs(in.size()) { }
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
		InstGenerator*		_checker;
		InstGenerator*		_generator;
		GeneratorNode*		_left;
		GeneratorNode*		_right;
	
	public:
		// Constructor
		TwoChildGeneratorNode(InstGenerator* c, InstGenerator* g, GeneratorNode* l, GeneratorNode* r);

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

TwoChildGeneratorNode::TwoChildGeneratorNode(InstGenerator* c, InstGenerator* g, GeneratorNode* l, GeneratorNode* r) :
	_checker(c), _generator(g), _left(l), _right(r) { 
	_left->parent(this);
	_right->parent(this);
}

/*******************
	Constructors
*******************/

GenerateAndTestGenerator::GenerateAndTestGenerator(const PredTable* t, const vector<bool>& pattern, const vector<const DomainElement**>& vars, const vector<unsigned int>& firstocc, const Universe& univ) : _table(t), _firstocc(firstocc), _currtuple(pattern.size()) {
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

InverseInstGenerator::InverseInstGenerator(PredTable* t, const vector<bool>& pattern, const vector<const DomainElement**>& vars) {
	// TODO Code below is correct, but can be optimized in case there are output variables that occur more than once in 'vars'
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
	_outtablegen = gf.create(t,pattern,tabvars,t->universe());
	PredTable temp(new FullInternalPredTable(),t->universe());
	_univgen = gf.create(&temp,pattern,vars,t->universe());	
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

bool SortInstGenerator::first() const {
	if(_table->approxempty()) return false;
	else {
		_currpos = _table->sortbegin();
		if(_currpos.hasNext()) { 
			*_outvar = *_currpos; 
			return true;	
		}
		else return false;
	}
}

bool SimpleLookupGenerator::first() const {
	for(unsigned int n = 0; n < _invars.size(); ++n) {
		_currargs[n] = *(_invars[n]);
	}
	return (_table->contains(_currargs) && _universe.contains(_currargs));
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

GeneratorNode* TwoChildGeneratorNode::next() const {
	while(true) {
		if(!_generator->next()) return 0;
		else {
			if(_checker->first()) {
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
	vector<SortTable*>::const_reverse_iterator jt = tabs.rbegin();
	for(vector<const DomainElement**>::const_reverse_iterator it = vars.rbegin(); it != vars.rend(); ++it, ++jt) {
		SortInstGenerator* tig = new SortInstGenerator((*jt)->interntable(),*it);
		if(vars.size() == 1) {
			gen = tig;
			break;
		}
		else if(it == vars.rbegin()) node = new LeafGeneratorNode(tig);
		else node = new OneChildGeneratorNode(tig,node);
	}
	if(!gen) gen = new TreeInstGenerator(node);
	return gen;
}

InstGenerator*	GeneratorFactory::create(const PredTable* pt, vector<bool> pattern, const vector<const DomainElement**>& vars, const Universe& universe) {
//cerr << "Create on table " << pt << endl;
//cerr << "Pattern = "; for(unsigned int n = 0; n < pattern.size(); ++n) cerr << (pattern[n] ? "true " : "false "); cerr << endl;
//cerr << "Vars = "; for(unsigned int n = 0; n < vars.size(); ++n) cerr << "  " << vars[n]; cerr << endl;
	_table = pt;
	_pattern = pattern;
	_vars = vars;
	_universe = universe;
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
		if(typeid(*(pt->interntable())) != typeid(BDDInternalPredTable)) 
			return new SimpleLookupGenerator(pt,vars,_universe);
		else { 
			StructureVisitor::visit(pt);
			return _generator;
		}
	}
	else {
		StructureVisitor::visit(pt);
		return _generator;
	}
}

void GeneratorFactory::visit(const ProcInternalPredTable* ) {
	_generator = new GenerateAndTestGenerator(_table,_pattern,_vars,_firstocc,_universe);
}

/******************************
	From BDDs to generators
******************************/

BDDToGenerator::BDDToGenerator(FOBDDManager* manager) : _manager(manager) { }

InstGenerator* BDDToGenerator::create(const FOBDD* bdd, const vector<bool>& pattern, const vector<const DomainElement**>& vars, const vector<const FOBDDVariable*>& bddvars, AbstractStructure* structure, const Universe& universe) {

//cerr << "Create on bdd\n";
//_manager->put(cerr,bdd);
//cerr << "Pattern = "; for(unsigned int n = 0; n < pattern.size(); ++n) cerr << (pattern[n] ? "true " : "false "); cerr << endl;
//cerr << "bddvars = "; for(unsigned int n = 0; n < bddvars.size(); ++n) cerr << "  " << *(bddvars[n]->variable()); cerr << endl;

	// Detect double occurrences
	vector<unsigned int> firstocc;
	for(unsigned int n = 0; n < vars.size(); ++n) {
		firstocc.push_back(n);
		for(unsigned int m = 0; m < n; ++m) {
			if(vars[n] == vars[m]) {
				firstocc[n] = m;
				break;
			}
		}
	}

	if(bdd == _manager->falsebdd()) {
		return new EmptyGenerator();
	}
	else if(bdd == _manager->truebdd()) {
		vector<const DomainElement**> outvars;
		vector<SortTable*> tables;
		for(unsigned int n = 0; n < pattern.size(); ++n) {
			if(!pattern[n]) {
				if(firstocc[n] == n) {
					outvars.push_back(vars[n]);
					tables.push_back(universe.tables()[n]);
				}
			}
		}
		GeneratorFactory gf;
		InstGenerator* result = gf.create(outvars,tables);
		return result;
	}
	else {
		GeneratorNode* gn = createnode(bdd,pattern,vars,bddvars,structure,universe);
		return new TreeInstGenerator(gn);
	}
}

GeneratorNode* BDDToGenerator::createnode(const FOBDD* bdd, const vector<bool>& pattern, const vector<const DomainElement**>& vars, const vector<const FOBDDVariable*>& bddvars, AbstractStructure* structure, const Universe& universe) {

	// Detect double occurrences
	vector<unsigned int> firstocc;
	for(unsigned int n = 0; n < vars.size(); ++n) {
		firstocc.push_back(n);
		for(unsigned int m = 0; m < n; ++m) {
			if(vars[n] == vars[m]) {
				firstocc[n] = m;
				break;
			}
		}
	}

	if(bdd == _manager->falsebdd()) {
		EmptyGenerator* eg = new EmptyGenerator();
		return new LeafGeneratorNode(eg);
	}
	else if(bdd == _manager->truebdd()) {
		vector<const DomainElement**> outvars;
		vector<SortTable*> tables;
		for(unsigned int n = 0; n < pattern.size(); ++n) {
			if(!pattern[n]) {
				if(firstocc[n] == n) {
					outvars.push_back(vars[n]);
					tables.push_back(universe.tables()[n]);
				}
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
		vector<SortTable*> kerntables;
		vector<bool> branchpattern;
		for(unsigned int n = 0; n < pattern.size(); ++n) {
			if(_manager->contains(bdd->kernel(),bddvars[n])) {
				kernpattern.push_back(pattern[n]);
				kerngenvars.push_back(vars[n]);
				kernvars.push_back(bddvars[n]);
				kerntables.push_back(universe.tables()[n]);
				branchpattern.push_back(true);
			}
			else {
				branchpattern.push_back(pattern[n]);
			}
		}
		
		
		// recursive case
		if(bdd->falsebranch() == _manager->falsebdd()) {
			InstGenerator* kernelgenerator = create(bdd->kernel(),kernpattern,kerngenvars,kernvars,structure,false,Universe(kerntables));
			GeneratorNode* truegenerator = createnode(bdd->truebranch(),branchpattern,vars,bddvars,structure,universe);
			return new OneChildGeneratorNode(kernelgenerator,truegenerator);
		}
		
		else if(bdd->truebranch() == _manager->falsebdd()) {
			InstGenerator* kernelgenerator = create(bdd->kernel(),kernpattern,kerngenvars,kernvars,structure,true,Universe(kerntables));
			GeneratorNode* falsegenerator = createnode(bdd->falsebranch(),branchpattern,vars,bddvars,structure,universe);
			return new OneChildGeneratorNode(kernelgenerator,falsegenerator);
		}
		else {
			vector<bool> checkpattern(kernpattern.size(),true);
			InstGenerator* kernelchecker = create(bdd->kernel(),checkpattern,kerngenvars,kernvars,structure,false,Universe(kerntables));
			vector<const DomainElement**> kgvars(0);
			vector<SortTable*> kguniv;
			for(unsigned int n = 0; n < kerngenvars.size(); ++n) {
				if(!kernpattern[n]) {
					unsigned int m = 0;
					for( ; m < kgvars.size(); ++m) {
						if(kgvars[m] == kerngenvars[n]) break;
					}
					if(m == kgvars.size()) {
						kgvars.push_back(kerngenvars[n]);
						kguniv.push_back(kerntables[n]);
					}
				}
			}
			GeneratorFactory gf;
			InstGenerator* kernelgenerator = gf.create(kgvars,kguniv);
			GeneratorNode* truegenerator = createnode(bdd->truebranch(),branchpattern,vars,bddvars,structure,universe);
			GeneratorNode* falsegenerator = createnode(bdd->falsebranch(),branchpattern,vars,bddvars,structure,universe);
			return new TwoChildGeneratorNode(kernelchecker,kernelgenerator,falsegenerator,truegenerator);
		}
		return 0;
	}
}

InstGenerator* BDDToGenerator::create(PredForm* atom, const vector<bool>& pattern, const vector<const DomainElement**>& vars, const vector<Variable*>& atomvars, AbstractStructure* structure, bool inverse, const Universe& universe) {

//cerr << "Create on atom " << *atom << endl;
//cerr << "Pattern = "; for(unsigned int n = 0; n < pattern.size(); ++n) cerr << (pattern[n] ? "true " : "false "); cerr << endl;
//cerr << "Atomvars = "; for(unsigned int n = 0; n < atomvars.size(); ++n) cerr << "  " << *(atomvars[n]) << ' ' << atomvars[n]; cerr << endl;
//cerr << "Inverse = " << (inverse ? "true" : "false") << endl;

	// The atom is of one of the following forms: 
	//	(A)		P(t1,...,tn), 
	//	(B)		F(t1,...,tn) = t, 
	//  (C)		t = F(t1,...,tn),
	//  (D)		(t_1 * x_11 * ... * x_1n_1) + ... + (t_m * x_m1 * ... * x_mn_m) = 0,
	//  (E)		0 = (t_1 * x_11 * ... * x_1n_1) + ... + (t_m * x_m1 * ... * x_mn_m).

	// Convert all cases to case (A)
	if(atom->symbol()->name() == "=/2") {	// cases (B), (C), (D), and (E)
		if(typeid(*(atom->subterms()[0])) == typeid(DomainTerm)) {	// Case (C) or (E)
			assert(typeid(*(atom->subterms()[1])) == typeid(FuncTerm));
			FuncTerm* ft = dynamic_cast<FuncTerm*>(atom->subterms()[1]);
			if(SortUtils::resolve(ft->sort(),VocabularyUtils::floatsort()) && (ft->function()->name() == "*/2" || ft->function()->name() == "+/2")) {	// Case (E)
				assert(false); // TODO solve towards a variable...
			}
			else {	// Case (C)
				vector<Term*> vt = ft->subterms(); vt.push_back(atom->subterms()[0]);
				PredForm* newatom = new PredForm(atom->sign(),ft->function(),vt,atom->pi().clone());
				delete(atom); delete(ft);
				atom = newatom;
			}
		}
		else if(typeid(*(atom->subterms()[1])) == typeid(DomainTerm)) {	// Case (B) or (D)
			assert(typeid(*(atom->subterms()[0])) == typeid(FuncTerm));
			FuncTerm* ft = dynamic_cast<FuncTerm*>(atom->subterms()[0]);
			if(SortUtils::resolve(ft->sort(),VocabularyUtils::floatsort()) && (ft->function()->name() == "*/2" || ft->function()->name() == "+/2")) {	// Case (D)
				assert(false); // TODO solve towards a variable...
			}
			else {	// Case (B)
				vector<Term*> vt = ft->subterms(); vt.push_back(atom->subterms()[1]);
				PredForm* newatom = new PredForm(atom->sign(),ft->function(),vt,atom->pi().clone());
				delete(atom); delete(ft);
				atom = newatom;
			}
		}
		else if(typeid(*(atom->subterms()[0])) == typeid(FuncTerm)) {	// Case (B)
			FuncTerm* ft = dynamic_cast<FuncTerm*>(atom->subterms()[0]);
			vector<Term*> vt = ft->subterms(); vt.push_back(atom->subterms()[1]);
			PredForm* newatom = new PredForm(atom->sign(),ft->function(),vt,atom->pi().clone());
			delete(atom); delete(ft);
			atom = newatom;
		}
		else {	// Case (C)
			FuncTerm* ft = dynamic_cast<FuncTerm*>(atom->subterms()[1]);
			vector<Term*> vt = ft->subterms(); vt.push_back(atom->subterms()[0]);
			PredForm* newatom = new PredForm(atom->sign(),ft->function(),vt,atom->pi().clone());
			delete(atom); delete(ft);
			atom = newatom;
		}
	}

	if(FormulaUtils::containsFuncTerms(atom)) {
		Formula* newform = FormulaUtils::remove_nesting(atom,PC_NEGATIVE);
		newform = FormulaUtils::remove_eqchains(newform);
		newform = FormulaUtils::graph_functions(newform);
		assert(typeid(*newform) == typeid(QuantForm));
		QuantForm* quantform = dynamic_cast<QuantForm*>(newform);
		assert(typeid(*(quantform->subf())) == typeid(BoolForm));
		BoolForm* boolform = dynamic_cast<BoolForm*>(quantform->subf());
		vector<PredForm*> conjunction;
		for(auto it = boolform->subformulas().begin(); it != boolform->subformulas().end(); ++it) {
			assert(typeid(*(*it)) == typeid(PredForm));
			conjunction.push_back(dynamic_cast<PredForm*>(*it));
		}
		PredForm* origatom = conjunction.back();
		set<Variable*> still_free;
		for(unsigned int n = 0; n < pattern.size(); ++n) {
			if(!pattern[n]) still_free.insert(atomvars[n]);
		}
		for(auto it = quantform->quantvars().begin(); it != quantform->quantvars().end(); ++it) {
			still_free.insert(*it);
		}
		set<PredForm*> atoms_to_order(conjunction.begin(),conjunction.end());
		vector<PredForm*> orderedconjunction;
		while(!atoms_to_order.empty()) {
			PredForm* bestatom = 0;
			double bestcost = numeric_limits<double>::max();
			for(auto it = atoms_to_order.begin(); it != atoms_to_order.end(); ++it) {
				bool currinverse = false;
				if(*it == origatom) currinverse = inverse;
				double currcost = FormulaUtils::estimatedCostAll(*it,still_free,currinverse,structure);
				if(currcost < bestcost) {
					bestcost = currcost;
					bestatom = *it;
				}
			}
			if(!bestatom) bestatom = *(atoms_to_order.begin());
			orderedconjunction.push_back(bestatom);
			atoms_to_order.erase(bestatom);
			for(auto it = bestatom->freevars().begin(); it != bestatom->freevars().end(); ++it) {
				still_free.erase(*it);
			}
		}

		vector<InstGenerator*> generators;
		vector<bool> branchpattern = pattern;
		vector<const DomainElement**> branchvars = vars;
		vector<Variable*> branchfovars = atomvars;
		vector<SortTable*> branchuniverse = universe.tables();
		for(auto it = quantform->quantvars().begin(); it != quantform->quantvars().end(); ++it) {
			branchpattern.push_back(false);
			branchvars.push_back(new const DomainElement*());
			branchfovars.push_back(*it);
			branchuniverse.push_back(structure->inter((*it)->sort()));
		}
		for(auto it = orderedconjunction.begin(); it != orderedconjunction.end(); ++it) {
			vector<bool> kernpattern;
			vector<const DomainElement**> kernvars;
			vector<Variable*> kernfovars;
			vector<SortTable*> kerntables;
			vector<bool> newbranchpattern;
			for(unsigned int n = 0; n < branchpattern.size(); ++n) {
				if((*it)->freevars().find(branchfovars[n]) == (*it)->freevars().end()) {
					newbranchpattern.push_back(branchpattern[n]);
				}
				else {
					kernpattern.push_back(branchpattern[n]);
					kernvars.push_back(branchvars[n]);
					kernfovars.push_back(branchfovars[n]);
					kerntables.push_back(branchuniverse[n]);
					newbranchpattern.push_back(true);
				}
			}
			branchpattern = newbranchpattern;
			if(*it == origatom) 
				generators.push_back(create(*it,kernpattern,kernvars,kernfovars,structure,inverse,Universe(kerntables)));
			else 
				generators.push_back(create(*it,kernpattern,kernvars,kernfovars,structure,true,Universe(kerntables)));
		}

		if(generators.size() == 1) return generators[0];
		else {
			GeneratorNode* node = 0;
			for(auto it = generators.rbegin(); it != generators.rend(); ++it) {
				if(node) node = new OneChildGeneratorNode(*it,node);
				else node = new LeafGeneratorNode(*it);
			}
			return new TreeInstGenerator(node);
		}
	}
	else {
		// Create the pattern for the atom
		vector<bool> atompattern;
		vector<const DomainElement**> datomvars;
		vector<SortTable*> atomtables;
		for(vector<Term*>::const_iterator it = atom->subterms().begin(); it != atom->subterms().end(); ++it) {
			if(typeid(*(*it)) == typeid(VarTerm)) {
				Variable* var = (dynamic_cast<VarTerm*>(*it))->var();
				unsigned int pos = 0;
				for(; pos < pattern.size(); ++pos) {
					if(atomvars[pos] == var) break;
				}
				assert(pos < pattern.size());
				atompattern.push_back(pattern[pos]);
				datomvars.push_back(vars[pos]);
				atomtables.push_back(universe.tables()[pos]);
			}
			else if(typeid(*(*it)) == typeid(DomainTerm)) {
				DomainTerm* domterm = dynamic_cast<DomainTerm*>(*it);
				const DomainElement** domelement = new const DomainElement*();
				*domelement = domterm->value(); 

				Variable* var = new Variable(domterm->sort());
				PredForm* newatom = dynamic_cast<PredForm*>(FormulaUtils::substitute(atom,domterm,var));

				vector<bool> termpattern(pattern); termpattern.push_back(true);
				vector<const DomainElement**> termvars(vars); termvars.push_back(domelement);
				vector<Variable*> fotermvars(atomvars); fotermvars.push_back(var);
				vector<SortTable*> termuniv(universe.tables()); termuniv.push_back(structure->inter(domterm->sort()));
				
				return create(newatom,termpattern,termvars,fotermvars,structure,inverse,Universe(termuniv));
			}
			else assert(false);
		}

		// Construct the generator
		PFSymbol* symbol = atom->symbol();
		const PredInter* inter = 0;
		if(typeid(*symbol) == typeid(Predicate)) inter = structure->inter(dynamic_cast<Predicate*>(symbol));
		else {
			assert(typeid(*symbol) == typeid(Function));
			inter = structure->inter(dynamic_cast<Function*>(symbol))->graphinter();
		}
		const PredTable* table = 0;
		if(typeid(*(atom->symbol())) == typeid(Predicate)) {
			Predicate* predicate = dynamic_cast<Predicate*>(atom->symbol());
			switch(predicate->type()) {
				case ST_NONE:
					table = inverse ? inter->cf() : inter->ct();
					break;
				case ST_CT:
					table = inverse ? inter->pf() : inter->ct();
					break;
				case ST_CF:
					table = inverse ? inter->pt() : inter->cf();
					break;
				case ST_PT:
					table = inverse ? inter->cf() : inter->pt();
					break;
				case ST_PF:
					table = inverse ? inter->ct() : inter->pf();
					break;
				default:
					assert(false);
			}
		}
		else table = inverse ? inter->cf() : inter->ct();
		GeneratorFactory gf;
		return gf.create(table,atompattern,datomvars,Universe(atomtables));
	}
}

InstGenerator* BDDToGenerator::create(const FOBDDKernel* kernel, const vector<bool>& pattern, const vector<const DomainElement**>& vars, const vector<const FOBDDVariable*>& kernelvars, AbstractStructure* structure, bool inverse, const Universe& universe) {

//cerr << "Create on kernel\n";
//_manager->put(cerr,kernel);
//cerr << "Pattern = "; for(unsigned int n = 0; n < pattern.size(); ++n) cerr << (pattern[n] ? "true " : "false "); cerr << endl;
//cerr << "kernelvars = "; for(unsigned int n = 0; n < kernelvars.size(); ++n) cerr << "  " << *(kernelvars[n]->variable()); cerr << endl;
//cerr << "inverse = " << (inverse ? "true" : "false") << endl;

	if(typeid(*kernel) == typeid(FOBDDAtomKernel)) {
		const FOBDDAtomKernel* atom = dynamic_cast<const FOBDDAtomKernel*>(kernel);

		if(_manager->containsFuncTerms(atom)) {
			Formula* atomform = _manager->toFormula(atom);
			assert(typeid(*atomform) == typeid(PredForm));
			PredForm* pf = dynamic_cast<PredForm*>(atomform);
			vector<Variable*> atomvars;
			for(auto it = kernelvars.begin(); it != kernelvars.end(); ++it) atomvars.push_back((*it)->variable());
			return create(pf,pattern,vars,atomvars,structure,inverse,universe);
		}

		// Create the pattern for the atom
		vector<bool> atompattern;
		vector<const DomainElement**> atomvars;
		vector<SortTable*> atomtables;
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
				atomtables.push_back(universe.tables()[pos]);
			}
			else if(typeid(*(*it)) == typeid(FOBDDDomainTerm)) {
				const FOBDDDomainTerm* domterm = dynamic_cast<const FOBDDDomainTerm*>(*it);
				const DomainElement** domelement = new const DomainElement*();
				*domelement = domterm->value(); 

				Variable* termvar = new Variable(domterm->sort());
				const FOBDDVariable* bddtermvar = _manager->getVariable(termvar);
				const FOBDDKernel* termkernel = _manager->substitute(kernel,domterm,bddtermvar);

				vector<bool> termpattern(pattern); termpattern.push_back(true);
				vector<const DomainElement**> termvars(vars); termvars.push_back(domelement);
				vector<const FOBDDVariable*> termkernelvars(kernelvars); termkernelvars.push_back(bddtermvar);
				vector<SortTable*> termuniv(universe.tables()); termuniv.push_back(structure->inter(domterm->sort()));
				
				return create(termkernel,termpattern,termvars,termkernelvars,structure,inverse,Universe(termuniv));
			}
			else assert(false);
		}

		// Construct the generator
		PFSymbol* symbol = atom->symbol();
		const PredInter* inter = 0;
		if(typeid(*symbol) == typeid(Predicate)) inter = structure->inter(dynamic_cast<Predicate*>(symbol));
		else {
			assert(typeid(*symbol) == typeid(Function));
			inter = structure->inter(dynamic_cast<Function*>(symbol))->graphinter();
		}
		const PredTable* table = 0;
		switch(atom->type()) {
			case AKT_TWOVAL:
				table = inverse ? inter->cf() : inter->ct();
				break;
			case AKT_CF:
				table = inverse ? inter->pt() : inter->cf();
				break;
			case AKT_CT:
				table = inverse ? inter->pf() : inter->ct();
				break;
			default:
				assert(false);
		}
		GeneratorFactory gf;
		return gf.create(table,atompattern,atomvars,Universe(atomtables));
	}
	else {	// Quantification kernel
		assert(typeid(*kernel) == typeid(FOBDDQuantKernel));
		const FOBDDQuantKernel* quantkernel = dynamic_cast<const FOBDDQuantKernel*>(kernel);

		// Create a new variable
		Variable* quantvar = new Variable(quantkernel->sort());
		const FOBDDVariable* bddquantvar = _manager->getVariable(quantvar);
		const FOBDDDeBruijnIndex* quantindex = _manager->getDeBruijnIndex(quantkernel->sort(),0);

		// Substitute the variable for the De Bruyn index
		const FOBDD* quantbdd = _manager->substitute(quantkernel->bdd(),quantindex,bddquantvar);

		// Create a generator for then quantified formula
		vector<bool> quantpattern; 
		if(inverse) quantpattern = vector<bool>(pattern.size(),true);
		else quantpattern = pattern;
		quantpattern.push_back(false);
		vector<const DomainElement**> quantvars(vars); quantvars.push_back(new const DomainElement*());
		vector<const FOBDDVariable*> bddquantvars(kernelvars); bddquantvars.push_back(bddquantvar);
		vector<SortTable*> quantuniv = universe.tables(); quantuniv.push_back(structure->inter(quantkernel->sort()));
		BDDToGenerator btg(_manager);
		InstGenerator* quantgenerator = btg.create(quantbdd,quantpattern,quantvars,bddquantvars,structure,Universe(quantuniv));

		// Create a generator for the kernel
		InstGenerator* result = 0;
		if(inverse) {
			GeneratorFactory gf;
			vector<const DomainElement**> univgenvars;
			vector<SortTable*> univgentables;
			for(unsigned int n = 0; n < pattern.size(); ++n) {
				if(!pattern[n]) {
					univgenvars.push_back(vars[n]);
					univgentables.push_back(universe.tables()[n]);
				}
			}
			InstGenerator* univgenerator = gf.create(univgenvars,univgentables);
			result = new FalseQuantKernelGenerator(quantgenerator,univgenerator);
		}
		else {
			unsigned int firstout = 0;
			for(; firstout < pattern.size(); ++firstout) {
				if(!pattern[firstout]) break;
			}
			if(firstout == pattern.size()) {
				result = new TestQuantKernelGenerator(quantgenerator);
			}
			else {
				result = new TrueQuantKernelGenerator(quantgenerator,vars);
			}
		}

		return result;
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
	_generator = btg.create(copybdd,_pattern,_vars,allvars,table->structure(),_universe);
}

void GeneratorFactory::visit(const FullInternalPredTable* ) {
	vector<const DomainElement**> outvars;
	vector<SortTable*> outtables;
	for(unsigned int n = 0; n < _pattern.size(); ++n) {
		if((!_pattern[n]) && _firstocc[n] == n) {
			outvars.push_back(_vars[n]);
			outtables.push_back(_universe.tables()[n]);
		}
	}
	_generator = create(outvars,outtables);
}

void GeneratorFactory::visit(const FuncInternalPredTable* fipt) {
	visit(fipt->table());
}

void GeneratorFactory::visit(const UnionInternalPredTable* ) {
	assert(false); // TODO
}

void GeneratorFactory::visit(const UnionInternalSortTable*) {
	assert(false); // TODO
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
	if(_pattern[0]) { assert(!_pattern[1]); _generator = new EqualGenerator(_vars[0],_vars[1],_universe.tables()[1]); }
	else if(_pattern[1]) _generator = new EqualGenerator(_vars[1],_vars[0],_universe.tables()[0]);
	else if(_firstocc[1] == 0) {
		_generator = create(vector<const DomainElement**>(1,_vars[0]), vector<SortTable*>(1,_universe.tables()[0]));
	}
	else _generator = new EqualGenerator(_vars[0],_vars[1],_universe.tables()[0],_universe.tables()[1]); 
}

void GeneratorFactory::visit(const StrLessInternalPredTable* ) {
	if(_pattern[0]) {
		_generator = new StrLessGenerator(_universe.tables()[0],_universe.tables()[1],_vars[0],_vars[1],true);
	}
	else if(_pattern[1]) {
		_generator = new StrGreaterGenerator(_universe.tables()[0],_universe.tables()[1],_vars[1],_vars[0],true);
	}
	else if(_firstocc[1] == 0) {
		_generator = new EmptyGenerator();
	}
	else {
		_generator = new StrLessGenerator(_universe.tables()[0],_universe.tables()[1],_vars[0],_vars[1],false);
	}
}

void GeneratorFactory::visit(const StrGreaterInternalPredTable* ) {
	if(_pattern[0]) {
		_generator = new StrGreaterGenerator(_universe.tables()[0],_universe.tables()[1],_vars[0],_vars[1],true);
	}
	else if(_pattern[1]) {
		_generator = new StrLessGenerator(_universe.tables()[0],_universe.tables()[1],_vars[1],_vars[0],true);
	}
	else if(_firstocc[1] == 0) {
		_generator = new EmptyGenerator();
	}
	else {
		_generator = new StrGreaterGenerator(_universe.tables()[0],_universe.tables()[1],_vars[0],_vars[1],false);
	}
}

void GeneratorFactory::visit(const InverseInternalPredTable* iip) {
	// TODO: optimize by checking the type of the internal table!!!
	PredTable* temp = new PredTable(iip->table(),_universe);
	_generator = new InverseInstGenerator(temp,_pattern,_vars);
}

void GeneratorFactory::visit(const FuncTable* ft) {
	if(!_pattern.back()) {
		// TODO: for the input positions, change universe to the universe of ft if this is smaller
		_generator = new SimpleFuncGenerator(ft,_pattern,_vars,_universe,_firstocc);
	}
	else ft->interntable()->accept(this);
}

void GeneratorFactory::visit(const ProcInternalFuncTable* ) {
	_generator = new GenerateAndTestGenerator(_table,_pattern,_vars,_firstocc,_universe);
}

void GeneratorFactory::visit(const UNAInternalFuncTable* ) {
	_generator = new InvUNAGenerator(_pattern,_vars,_universe);
}

void GeneratorFactory::visit(const EnumeratedInternalFuncTable*) {
	// TODO: Use dynamic programming to improve this
	LookupTable lookuptab;
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
					if(_pattern[n]) intuple.push_back(tuple[n]);
					else outtuple.push_back(tuple[n]);
				}
			}
			lookuptab[intuple].push_back(outtuple);
		}
	}
	_generator = new EnumLookupGenerator(lookuptab,invars,outvars);
}

void GeneratorFactory::visit(const PlusInternalFuncTable* pift) {
	if(_pattern[0]) {
		_generator = new MinusGenerator(_vars[2],_vars[0],_vars[1],pift->isInt(),_universe.tables()[1]);
	}
	else if(_pattern[1]) {
		_generator = new MinusGenerator(_vars[2],_vars[1],_vars[0],pift->isInt(),_universe.tables()[0]);
	}
	else if(_firstocc[1] == 0) {
		if(pift->isInt()) {
			assert(false); // TODO
		}
		else {
			const DomainElement* two = DomainElementFactory::instance()->create(2);
			const DomainElement** twopointer = new const DomainElement*();
			twopointer = &two;
			_generator = new DivGenerator(_vars[2],twopointer,_vars[0],false,_universe.tables()[0]);
		}
	}
	else {
		notyetimplemented("Infinite generator for addition pattern (out,out,in)");
		exit(1);
	}
}

void GeneratorFactory::visit(const MinusInternalFuncTable* pift) {
	if(_pattern[0]) {
		_generator = new MinusGenerator(_vars[0],_vars[2],_vars[1],pift->isInt(),_universe.tables()[1]);
	}
	else if(_pattern[1]) {
		_generator = new PlusGenerator(_vars[1],_vars[2],_vars[0],pift->isInt(),_universe.tables()[0]);
	}
	else if(_firstocc[1] == 0) {
		assert(false); // TODO
	}
	else {
		notyetimplemented("Infinite generator for subtraction pattern (out,out,in)");
		exit(1);
	}
}

void GeneratorFactory::visit(const TimesInternalFuncTable* pift) {
	if(_pattern[0]) {
		_generator = new DivGenerator(_vars[2],_vars[0],_vars[1],pift->isInt(),_universe.tables()[1]);
	}
	else if(_pattern[1]) {
		_generator = new DivGenerator(_vars[2],_vars[1],_vars[0],pift->isInt(),_universe.tables()[0]);
	}
	else if(_firstocc[1] == 0) {
		assert(false); // TODO
	}
	else {
		notyetimplemented("Infinite generator for multiplication pattern (out,out,in)");
		exit(1);
	}
}

void GeneratorFactory::visit(const DivInternalFuncTable* pift) {
	if(_pattern[0]) {
		_generator = new DivGenerator(_vars[0],_vars[2],_vars[1],pift->isInt(),_universe.tables()[1]);
	}
	else if(_pattern[1]) {
		// FIXME: wrong in case of integers. E.g., a / 2 = 1 should result in a \in { 2,3 } instead of a \in { 2 }
		_generator = new TimesGenerator(_vars[1],_vars[2],_vars[0],pift->isInt(),_universe.tables()[0]);
	}
	else if(_firstocc[1] == 0) {
		assert(false); // TODO
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
	_generator = new InvAbsGenerator(_vars[1],_vars[0],aift->isInt(),_universe.tables()[0]);
}

void GeneratorFactory::visit(const UminInternalFuncTable* uift) {
	assert(!_pattern[0]);
	_generator = new UminGenerator(_vars[1],_vars[0],uift->isInt(),_universe.tables()[0]);
}


