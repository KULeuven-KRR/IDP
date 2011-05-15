/************************************
	ground.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "ground.hpp"
#include "common.hpp"

#include <typeinfo>
#include <iostream>
#include <sstream>
#include <limits>
#include <cmath>
#include <utility> // for relational operators (namespace rel_ops)

#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "ecnf.hpp"
#include "options.hpp"
#include "generator.hpp"
#include "checker.hpp"
#include "common.hpp"

using namespace std;
using namespace rel_ops;

/** The two built-in literals 'true' and 'false' **/
int _true = numeric_limits<int>::max();
int _false = 0;

/****************************************
	Comparison operators for TsBodies
****************************************/

bool operator==(const TsBody& a, const TsBody& b) {
	if(typeid(a) != typeid(b))
		return false;
	else
		return a.equal(b);
}

bool operator<(const TsBody& a, const TsBody& b) {
	// Order of TsBodies of different types is defined by lexical order of there typeids.
	if(typeid(a) != typeid(b))
		return (typeid(a).name() < typeid(b).name());
	else
		return a.compare(b);
}	

bool TsBody::equal(const TsBody& other) const {
	return _type == other.type();
}

bool TsBody::compare(const TsBody& other) const {
	return _type < other.type();
}

bool CPTsBody::equal(const TsBody& other) const {
	if(not TsBody::equal(other))
		return false;
	const CPTsBody& othercpt = static_cast<const CPTsBody&>(other);
	return (*_left == *othercpt.left()) && (_comp == othercpt.comp()) && (_right == othercpt.right());
}

bool CPTsBody::compare(const TsBody& other) const {
	if(TsBody::compare(other))
		return true;
	else if(TsBody::equal(other)) {
		const CPTsBody& othercpt = static_cast<const CPTsBody&>(other);
		if(*_left < *othercpt.left())
			return true;
		else if(*_left == *othercpt.left()) {
			if(_comp < othercpt.comp())
				return true;
			else if(_comp == othercpt.comp()) {
				if(_right < othercpt.right()) 
					return true;
			}
		}
	}
	return false;
}

/** Comparing CP terms and bounds **/

bool operator==(const CPTerm& a, const CPTerm& b) {
	if(typeid(a) != typeid(b))
		return false;
	else
		return a.equal(b);
}

bool operator<(const CPTerm& a, const CPTerm& b) {
	if(typeid(a) != typeid(b))
		return (typeid(a).name() < typeid(b).name());
	else
		return a.compare(b);
}

bool CPVarTerm::equal(const CPTerm& other) const {
	const CPVarTerm& othercpt = static_cast<const CPVarTerm&>(other);
	return _varid == othercpt._varid;
}

bool CPVarTerm::compare(const CPTerm& other) const {
	const CPVarTerm& othercpt = static_cast<const CPVarTerm&>(other);
	return _varid < othercpt._varid;
}

bool CPSumTerm::equal(const CPTerm& other) const {
	const CPSumTerm& othercpt = static_cast<const CPSumTerm&>(other);
	return _varids == othercpt._varids;
}

bool CPSumTerm::compare(const CPTerm& other) const {
	const CPSumTerm& othercpt = static_cast<const CPSumTerm&>(other);
	return _varids < othercpt._varids;
}

bool CPWSumTerm::equal(const CPTerm& other) const {
	const CPWSumTerm& othercpt = static_cast<const CPWSumTerm&>(other);
	return (_varids == othercpt._varids) && (_weights == othercpt._weights);
}

bool CPWSumTerm::compare(const CPTerm& other) const {
	const CPWSumTerm& othercpt = static_cast<const CPWSumTerm&>(other);
	return (_varids <= othercpt._varids) && (_weights < othercpt._weights);
}

bool operator==(const CPBound& a, const CPBound& b) {
	if(a._isvarid == b._isvarid) {
		return a._isvarid ? (a._value._varid == b._value._varid) : (a._value._bound == b._value._bound);
	}
	return false;
}

bool operator<(const CPBound& a, const CPBound& b) {
	if(a._isvarid == b._isvarid) {
		return a._isvarid ? (a._value._varid < b._value._varid) : (a._value._bound < b._value._bound);
	}
	// CPBounds with a number value come before CPBounds with a varid.
	return (a._isvarid < b._isvarid);
}

/*********************************************
	Translate from ground atoms to numbers
*********************************************/

GroundTranslator::~GroundTranslator() {
	// delete TsBodies
	for(map<int,TsBody*>::iterator mapit = _nr2tsbodies.begin(); mapit != _nr2tsbodies.end(); ++mapit)
		delete mapit->second;
	_nr2tsbodies.clear();
	_tsbodies2nr.clear(); // All TsBodies in this map should've been deleted through the other map.
}

int GroundTranslator::translate(unsigned int n, const ElementTuple& args) {
	map<ElementTuple,int>::iterator jt = _table[n].lower_bound(args);
	if(jt != _table[n].end() && jt->first == args) {
		return jt->second;
	}
	else {
		int nr = nextNumber();
		_table[n].insert(jt,pair<ElementTuple,int>(args,nr));
		_backsymbtable[nr] = _symboffsets[n];
		_backargstable[nr] = args;
		return nr;
	}
}

int GroundTranslator::translate(PFSymbol* s, const ElementTuple& args) {
	unsigned int offset = addSymbol(s);
	return translate(offset,args);
}

int GroundTranslator::translate(const vector<int>& clause, bool conj, TsType tstype) {
	int nr = nextNumber();
	PCTsBody* tsbody = new PCTsBody(tstype,clause,conj);
	//_tsbodies2nr.insert(it,pair<TsBody*,int>(tsbody,nr));
	_nr2tsbodies.insert(pair<int,TsBody*>(nr,tsbody));
	return nr;
}

int	GroundTranslator::translate(double bound, char comp, bool strict, AggFunction aggtype, int setnr, TsType tstype) {
	if(comp == '=') {
		vector<int> cl(2);
		cl[0] = translate(bound,'<',false,aggtype,setnr,tstype);
		cl[1] = translate(bound,'>',false,aggtype,setnr,tstype);
		return translate(cl,true,tstype);
	}
	else {
		int nr = nextNumber();
		AggTsBody* tsbody = new AggTsBody(tstype,bound,(comp == '<'),aggtype,setnr);
		if(strict) {
			#warning "This is wrong if floating point weights are allowed!";
			tsbody->_bound = (comp == '<') ? bound + 1 : bound - 1;	
		} 
		else tsbody->_bound = bound;
		//_tsbodies2nr.insert(it,pair<TsBody*,int>(tsbody,nr));
		_nr2tsbodies.insert(pair<int,TsBody*>(nr,tsbody));
		return nr;
	}
}

int GroundTranslator::translate(CPTerm* left, CompType comp, const CPBound& right, TsType tstype) {
	CPTsBody* tsbody = new CPTsBody(tstype,left,comp,right);
	map<TsBody*,int,TsCompare>::iterator it = _tsbodies2nr.lower_bound(tsbody);
	if(it != _tsbodies2nr.end() && *(it->first) == *tsbody) {
		delete tsbody;
		return it->second;
	}
	else {
		int nr = nextNumber();
		_tsbodies2nr.insert(it,pair<TsBody*,int>(tsbody,nr));
		_nr2tsbodies.insert(pair<int,TsBody*>(nr,tsbody));
		return nr;
	}
}

int GroundTranslator::translateSet(const vector<int>& lits, const vector<double>& weights, const vector<double>& trueweights) {
	int setnr;
	if(_freesetnumbers.empty()) {
		TsSet newset;
		setnr = _sets.size();
		_sets.push_back(newset);
		TsSet& tsset = _sets.back();

		tsset._setlits = lits;
		tsset._litweights = weights;
		tsset._trueweights = trueweights;
	}
	else {
		setnr = _freesetnumbers.front();
		_freesetnumbers.pop();
		TsSet& tsset = _sets[setnr];

		tsset._setlits = lits;
		tsset._litweights = weights;
		tsset._trueweights = trueweights;
	}
	return setnr;
}

int GroundTranslator::nextNumber() {
	if(_freenumbers.empty()) {
		int nr = _backsymbtable.size(); 
		_backsymbtable.push_back(0);
		_backargstable.push_back(ElementTuple(0));
		return nr;
	}
	else {
		int nr = _freenumbers.front();
		_freenumbers.pop();
		return nr;
	}
}

unsigned int GroundTranslator::addSymbol(PFSymbol* pfs) {
	for(unsigned int n = 0; n < _symboffsets.size(); ++n)
		if(_symboffsets[n] == pfs) return n;
	_symboffsets.push_back(pfs);
	_table.push_back(map<ElementTuple,int,StrictWeakTupleOrdering>());
	return _symboffsets.size()-1;
}

string GroundTranslator::printAtom(int nr) const {
	stringstream s;
	nr = abs(nr);
	if(nr == _true) return "true";
	else if(nr == _false) return "false";
	if(nr >= int(_backsymbtable.size())) {
		return "error";
	}
	PFSymbol* pfs = symbol(nr);
	if(pfs) {
		s << pfs->to_string();
		if(!(args(nr).empty())) {
			s << "(";
			for(unsigned int c = 0; c < args(nr).size(); ++c) {
				s << args(nr)[c]->to_string();
				if(c !=  args(nr).size()-1) s << ",";
			}
			s << ")";
		}
	}
	else s << "tseitin_" << nr;
	return s.str();
}


/*********************************************
	Translate from ground terms to numbers
*********************************************/

unsigned int GroundTermTranslator::translate(unsigned int offset, const ElementTuple& args) {
	map<ElementTuple,unsigned int>::iterator jt = _table[offset].lower_bound(args);
	if(jt != _table[offset].end() && jt->first == args) {
		return jt->second;
	}
	else {
		unsigned int nr = nextNumber();
		_table[offset].insert(jt,pair<ElementTuple,unsigned int>(args,nr));
		_backfunctable[nr] = _offset2function[offset];
		_backargstable[nr] = args;
		return nr;
	}
}

unsigned int GroundTermTranslator::translate(Function* func, const ElementTuple& args) {
	unsigned int offset = addFunction(func);
	return translate(offset,args);
}

unsigned int GroundTermTranslator::nextNumber() {
	unsigned int nr = _backfunctable.size(); 
	_backfunctable.push_back(0);
	_backargstable.push_back(ElementTuple(0));
	return nr;
}

unsigned int GroundTermTranslator::addFunction(Function* func) {
	map<Function*,unsigned int>::const_iterator found = _function2offset.find(func);
	if(found != _function2offset.end())
		// Simply return number when function is already known
		return found->second;
	else {
		// Add function and number when function is unknown
		unsigned int offset = _offset2function.size();
		_function2offset[func] = offset; 
		_offset2function.push_back(func);
		_table.push_back(map<ElementTuple,unsigned int,StrictWeakTupleOrdering>());
		return offset;	
	}
}

bool GroundTermTranslator::contains(Function* func, const ElementTuple& args) const {
	if(contains(func)) {
		unsigned int offset = getOffset(func);
		map<ElementTuple,unsigned int>::const_iterator jt = _table[offset].lower_bound(args);
		return jt != _table[offset].end() && jt->first == args;
	}
	return false;
}

bool GroundTermTranslator::contains(Function* func)	const {
	return _function2offset.find(func) != _function2offset.end();
}

string GroundTermTranslator::printTerm(unsigned int nr) const {
	stringstream s;
	if(nr >= _backfunctable.size()) {
		return "error";
	}
	Function* func = function(nr);
	s << func->to_string();
	if(!(args(nr).empty())) {
		s << "(";
		for(unsigned int c = 0; c < args(nr).size(); ++c) {
			s << (args(nr))[c]->to_string();
			if(c != args(nr).size()-1) s << ",";
		}
		s << ")";
	}
	return s.str();
}


/*************************************
	Optimized grounding algorithm
*************************************/

bool CopyGrounder::run() const {
	// TODO TODO TODO
	return true;
}

bool TheoryGrounder::run() const {
	if(_verbosity > 0) {
		clog << "Grounding theory " << endl;
		clog << "Components to ground = " << _grounders.size() << endl;
	}
	for(unsigned int n = 0; n < _grounders.size(); ++n) {
		bool b = _grounders[n]->run();
		if(!b) return b;
	}
	return true;
}

bool SentenceGrounder::run() const {
	if(_verbosity > 0) clog << "Grounding sentence " << endl;
	vector<int> cl;
	_subgrounder->run(cl);
	if(cl.empty()) {
		return _conj ? true : false;
	}
	else if(cl.size() == 1) {
		if(cl[0] == _false) {
			_grounding->addEmptyClause();
			return false;
		}
		else if(cl[0] != _true) {
			_grounding->addClause(cl);
			return true;
		}
		else return true;
	}
	else {
		if(_conj) {
			for(unsigned int n = 0; n < cl.size(); ++n)
				_grounding->addUnitClause(cl[n]);
		}
		else {
			_grounding->addClause(cl);
		}
		return true;
	}
}

bool UnivSentGrounder::run() const {
	if(_verbosity > 0) clog << "Grounding a universally quantified sentence " << endl;
	if(_generator->first()) {
		bool b = _subgrounder->run();
		if(!b) {
			_grounding->addEmptyClause();
			return b;
		}
		while(_generator->next()) {
			b = _subgrounder->run();
			if(!b) {
				_grounding->addEmptyClause();
				return b;
			}
		}
	}
	else if(_verbosity > 0) {
		clog << "No instances for this sentence " << endl;
	}
	return true;
}

void FormulaGrounder::setorig(const Formula* f, const map<Variable*,const DomainElement**>& mvd,int verb) {
	_verbosity = verb;
	map<Variable*,Variable*> mvv;
	for(set<Variable*>::const_iterator it = f->freevars().begin(); it != f->freevars().end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),ParseInfo());
		mvv[*it] = v;
		_varmap[v] = mvd.find(*it)->second;
	}
	_origform = f->clone(mvv);
}

void FormulaGrounder::printorig() const {
	clog << "Grounding formula " << _origform->to_string();
	if(not _origform->freevars().empty()) {
		clog << " with instance ";
		for(set<Variable*>::const_iterator it = _origform->freevars().begin(); it != _origform->freevars().end(); ++it) {
			clog << (*it)->to_string() << " = ";
			const DomainElement* e = *(_varmap.find(*it)->second);
			clog << e->to_string() << ' ';
		}
	}
	clog << endl;
}

AtomGrounder::AtomGrounder(GroundTranslator* gt, bool sign, PFSymbol* s,
							const vector<TermGrounder*> sg, InstanceChecker* pic, InstanceChecker* cic,
							const vector<SortTable*>& vst, const GroundingContext& ct) :
	FormulaGrounder(gt,ct), _subtermgrounders(sg), _pchecker(pic), _cchecker(cic),
   	_symbol(gt->addSymbol(s)), _args(sg.size()), _tables(vst), _sign(sign)
	{ _certainvalue = ct._truegen ? _true : _false; }

int AtomGrounder::run() const {
	if(_verbosity > 2) printorig();
	// Run subterm grounders
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		_args[n] = _subtermgrounders[n]->run();
	}
	
	// Checking partial functions
	for(unsigned int n = 0; n < _args.size(); ++n) {
		//TODO: only check positions that can be out of bounds!
		if(!_args[n]) {
			//TODO: produce a warning!
			if(_context._funccontext == PC_BOTH) {
				// TODO: produce an error
			}
			if(_verbosity > 2) {
				clog << "Partial function went out of bounds\n";
				clog << "Result is " << (_context._funccontext != PC_NEGATIVE  ? "true" : "false") << endl;
			}
			return _context._funccontext != PC_NEGATIVE  ? _true : _false;
		}
	}

	// Checking out-of-bounds
	for(unsigned int n = 0; n < _args.size(); ++n) {
		if(!_tables[n]->contains(_args[n])) {
			if(_verbosity > 2) {
				clog << "Term value out of predicate type\n";
				clog << "Result is " << (_sign  ? "false" : "true") << endl;
			}
			return _sign ? _false : _true;
		}
	}

	// Run instance checkers
	if(!(_pchecker->run(_args))) {
		if(_verbosity > 2) {
			clog << "Possible checker failed\n";
			clog << "Result is " << (_certainvalue ? "false" : "true") << endl;
		}
		return _certainvalue ? _false : _true;	// TODO: dit is lelijk
	}
	if(_cchecker->run(_args)) {
		if(_verbosity > 2) {
			clog << "Certain checker succeeded\n";
			clog << "Result is " << _translator->printAtom(_certainvalue) << endl;
		}
		return _certainvalue;
	}

	// Return grounding
	int atom = _translator->translate(_symbol,_args);
	if(!_sign) atom = -atom;
	if(_verbosity > 2) {
		clog << "Result is " << _translator->printAtom(atom) << endl;
	}
	return atom;
}

void AtomGrounder::run(vector<int>& clause) const {
	clause.push_back(run());
}

CPAtomGrounder::CPAtomGrounder(GroundTranslator* gt, GroundTermTranslator* tt, bool sign, Function* func,
							const vector<TermGrounder*> vtg, InstanceChecker* pic, InstanceChecker* cic,
							const vector<SortTable*>& vst, const GroundingContext& ct) :
	AtomGrounder(gt,sign,func,vtg,pic,cic,vst,ct), _termtranslator(tt) { }

int CPAtomGrounder::run() const {
	if(_verbosity > 2) printorig();
	// Run subterm grounders
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		_args[n] = _subtermgrounders[n]->run();
	}
	
	// Checking partial functions
	for(unsigned int n = 0; n < _args.size(); ++n) {
		//TODO: only check positions that can be out of bounds!
		if(!_args[n]) {
			//TODO: produce a warning!
			if(_context._funccontext == PC_BOTH) {
				// TODO: produce an error
			}
			if(_verbosity > 2) {
				clog << "Partial function went out of bounds\n";
				clog << "Result is " << (_context._funccontext != PC_NEGATIVE  ? "true" : "false") << endl;
			}
			return _context._funccontext != PC_NEGATIVE  ? _true : _false;
		}
	}

	// Checking out-of-bounds
	for(unsigned int n = 0; n < _args.size(); ++n) {
		if(!_tables[n]->contains(_args[n])) {
			if(_verbosity > 2) {
				clog << "Term value out of predicate type\n";
				clog << "Result is " << (_sign  ? "false" : "true") << endl;
			}
			return _sign ? _false : _true;
		}
	}

	// Run instance checkers
	if(!(_pchecker->run(_args))) {
		if(_verbosity > 2) {
			clog << "Possible checker failed\n";
			clog << "Result is " << (_certainvalue ? "false" : "true") << endl;
		}
		return _certainvalue ? _false : _true;	// TODO: dit is lelijk
	}
	if(_cchecker->run(_args)) {
		if(_verbosity > 2) {
			clog << "Certain checker succeeded\n";
			clog << "Result is " << _translator->printAtom(_certainvalue) << endl;
		}
		return _certainvalue;
	}

	// Return grounding
	assert(typeid(*(_translator->getSymbol(_symbol))) == typeid(Function)); // by definition...
	Function* func = static_cast<Function*>(_translator->getSymbol(_symbol));
	ElementTuple args = _args; args.pop_back();
	int value = _args.back()->value()._int;
	
	unsigned int varid = _termtranslator->translate(func,args); //FIXME conversion is nasty...
	CPTerm* leftterm = new CPVarTerm(varid);
	CPBound rightbound(false,value);
	int atom = _translator->translate(leftterm,CT_EQ,rightbound,TS_EQ);
	if(!_sign) atom = -atom;
	return atom;
}

int CPFormulaGrounder::run() const {
	const DomainElement* left = _lefttermgrounder->run();
	const DomainElement* right = _righttermgrounder->run();
	if(!left || !right) {
		return _context._funccontext != PC_NEGATIVE  ? _true : _false;
	}
	// TODO??? out-of-bounds check. Can out-of-bounds ever occur on </2, >/2, =/2???
	
	if(left->type() == DET_COMPOUND) {
		unsigned int leftvarid = _termtranslator->translate(left->value()._compound->function(),left->value()._compound->args());
		// TODO: try to use the faster translate function (use addSymbol first)
		CPTerm* leftterm = new CPVarTerm(leftvarid);
		if(right->type() == DET_COMPOUND) {
			unsigned int rightvarid = _termtranslator->translate(right->value()._compound->function(),right->value()._compound->args());
			// TODO: try to use the faster translate function 
			CPBound rightbound(true,rightvarid);
			return _translator->translate(leftterm,_comparator,rightbound,_context._tseitin);
		}	
		else {
			assert(right->type() == DET_INT);
			int rightvalue = right->value()._int;
			CPBound rightbound(false,rightvalue);
			return _translator->translate(leftterm,_comparator,rightbound,_context._tseitin);
		}
	}
	else {
		assert(left->type() == DET_INT);
		int leftvalue = left->value()._int;
		if(right->type() == DET_COMPOUND) {
			unsigned int rightvarid = _termtranslator->translate(right->value()._compound->function(),right->value()._compound->args());
			// TODO: try to use the faster translate function 
			CPTerm* rightterm = new CPVarTerm(rightvarid);
			CPBound leftbound(false,leftvalue);
			return _translator->translate(rightterm,invertcomp(_comparator),leftbound,_context._tseitin);
		}	
		else {
			assert(right->type() == DET_INT);
			int rightvalue = right->value()._int;
			switch(_comparator) {
				case CT_EQ:
					return leftvalue == rightvalue ? _true : _false;
				case CT_NEQ:
					return leftvalue != rightvalue ? _true : _false;
				case CT_LEQ:
					return leftvalue <= rightvalue ? _true : _false;
				case CT_GEQ:
					return leftvalue >= rightvalue ? _true : _false;
				case CT_LT:
					return leftvalue < rightvalue ? _true : _false;
				case CT_GT:	
					return leftvalue > rightvalue ? _true : _false;
				default: assert(false);
			}
		}
	}
	assert(false);
	return 0;
}

void CPFormulaGrounder::run(vector<int>& clause) const {
	clause.push_back(run());
}

/**
 * int AggGrounder::handleDoubleNegation(double boundvalue, int setnr) const
 * DESCRIPTION
 * 		Invert the comparator and the sign of the tseitin when the aggregate is in a doubly negated context. 
 */
int AggGrounder::handleDoubleNegation(double boundvalue, int setnr) const {
	bool newcomp;
	switch(_comp) {
		case '<' : newcomp = '>'; break;
		case '>' : newcomp = '<'; break;
		case '=' : assert(false); break;
		default : assert(false); break;
	}
	TsType tp = _context._tseitin;
	int tseitin = _translator->translate(boundvalue,newcomp,false,_type,setnr,tp);
	return _sign ? -tseitin : tseitin;
}

/**
 * int AggGrounder::finishCard(double truevalue, double boundvalue, int setnr) const
 */
int AggGrounder::finishCard(double truevalue, double boundvalue, int setnr) const {
	int leftvalue = int(boundvalue - truevalue);
	const TsSet& tsset = _translator->groundset(setnr);
	int maxposscard = tsset.size();
	TsType tp = _context._tseitin;
	bool simplify = false;
	bool conj;
	bool negateset;
	switch(_comp) {
		case '=':
			if(leftvalue < 0 || leftvalue > maxposscard) {
				return _sign ? _false : _true;
			}
			else if(leftvalue == 0) {
				simplify = true;
				conj = true;
				negateset = true;
			}
			else if(leftvalue == maxposscard) {
				simplify = true;
				conj = true;
				negateset = false;
			}
			break;
		case '<':
			if(leftvalue < 0) {
				return _sign ? _true : _false;
			}
			else if(leftvalue == 0) {
				simplify = true;
				conj = false;
				negateset = false;
			}
			else if(leftvalue == maxposscard-1) {
				simplify = true;
				conj = true;
				negateset = false;
			}
			else if(leftvalue >= maxposscard) {
				return _sign ? _false : _true;
			}
			break;
		case '>':
			if(leftvalue <= 0) {
				return _sign ? _false : _true;
			}
			else if(leftvalue == 1) {
				simplify = true;
				conj = true;
				negateset = true;
			}
			else if(leftvalue == maxposscard) {
				simplify = true;
				conj = false;
				negateset = true;
			}
			else if(leftvalue > maxposscard) {
				return _sign ? _true : _false;
			}
			break;
		default:
			assert(false);
	}
	if(simplify) {
		if(_doublenegtseitin) {
			if(negateset) {
				int tseitin = _translator->translate(tsset.literals(),!conj,tp);
				return _sign ? -tseitin : tseitin;
			}
			else {
				vector<int> newsetlits(tsset.size());
				for(unsigned int n = 0; n < tsset.size(); ++n) newsetlits[n] = -tsset.literal(n);
				int tseitin = _translator->translate(newsetlits,!conj,tp);
				return _sign ? -tseitin : tseitin;
			}
		}
		else {
			if(negateset) {
				vector<int> newsetlits(tsset.size());
				for(unsigned int n = 0; n < tsset.size(); ++n) newsetlits[n] = -tsset.literal(n);
				int tseitin = _translator->translate(newsetlits,conj,tp);
				return _sign ? tseitin : -tseitin;
			}
			else {
				int tseitin = _translator->translate(tsset.literals(),conj,tp);
				return _sign ? tseitin : -tseitin;
			}
		}
	}
	else {
		if(_doublenegtseitin) return handleDoubleNegation(double(leftvalue),setnr);
		else {
			int tseitin = _translator->translate(double(leftvalue),_comp,true,AGG_CARD,setnr,tp);
			return _sign ? tseitin : -tseitin;
		}
	}
}

/**
 * int AggGrounder::finish(double boundvalue, double newboundvalue, double minpossvalue, double maxpossvalue, int setnr) const
 * DESCRIPTION
 * 		General finish method for grounding of sum, product, minimum and maximum aggregates.
 * 		Checks whether the aggregate will be certainly true or false, based on minimum and maximum possible values and the given bound;
 * 		and creates a tseitin, handling double negation when necessary;
 */
int AggGrounder::finish(double boundvalue, double newboundvalue, double minpossvalue, double maxpossvalue, int setnr) const {
	// Check minimum and maximum possible values against the given bound
	switch(_comp) { //TODO more complicated propagation is possible!
		case '=':
			if(minpossvalue > boundvalue || maxpossvalue < boundvalue)
				return _sign ? _false : _true;
			break;
		case '<':
			if(boundvalue < minpossvalue)
				return _sign ? _true : _false;
			else if(boundvalue >= maxpossvalue)
				return _sign ? _false : _true;
			break;
		case '>':
			if(boundvalue > maxpossvalue)
				return _sign ? _true : _false;
			else if(boundvalue <= minpossvalue)
				return _sign ? _false : _true;
			break;
		default:
			assert(false);
	}
	if(_doublenegtseitin)
		return handleDoubleNegation(newboundvalue,setnr);
	else {
		int tseitin;
		TsType tp = _context._tseitin;
	// FIXME ??? WRONG CODE ???	
	//	if((_type == AGG_MIN) && (_comp == '=')) // Only use lower bound
	//		tseitin = _translator->translate(newboundvalue,'<',false,AGG_MIN,setnr,tp);
	//	else if((_type == AGG_MAX) && (_comp == '=')) // Only use upper bound
	//		tseitin = _translator->translate(newboundvalue,'>',false,AGG_MAX,setnr,tp);
	//	else
			tseitin = _translator->translate(newboundvalue,_comp,true,_type,setnr,tp);
		return _sign ? tseitin : -tseitin;
	}
}

/**
 * int AggGrounder::run() const
 * DESCRIPTION
 * 		Run the aggregate grounder.
 */
int AggGrounder::run() const {
	// Run subgrounders
	int setnr = _setgrounder->run();
	const DomainElement* bound = _boundgrounder->run();

	// Retrieve the set, note that weights might be changed when handling min and max aggregates.
	TsSet& tsset = _translator->groundset(setnr);

	// Retrieve the value of the bound
	double boundvalue = bound->type() == DET_INT ? (double) bound->value()._int : bound->value()._double;

	// Compute the value of the aggregate based on weights of literals that are certainly true.	
	double truevalue = applyAgg(_type,tsset.trueweights());

	// When the set is empty, return an answer based on the current value of the aggregate.
	if(tsset.literals().empty()) {
		bool returnvalue;
		switch(_comp) {
			case '<' : returnvalue = boundvalue < truevalue; break;
			case '>' : returnvalue = boundvalue > truevalue; break;
			case '=' : returnvalue = boundvalue == truevalue; break;
			default: assert(false); returnvalue = true;
		}
		return _sign == returnvalue ? _true : _false;
	}

	// Handle specific aggregates.
	int tseitin;
	double minpossvalue = truevalue;
	double maxpossvalue = truevalue;
	switch(_type) {
		case AGG_CARD: { 
			tseitin = finishCard(truevalue,boundvalue,setnr);
			break;
		}
		case AGG_SUM: {
			// Compute the minimum and maximum possible value of the sum.
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				if(tsset.weight(n) > 0) maxpossvalue += tsset.weight(n);
				else if(tsset.weight(n) < 0) minpossvalue += tsset.weight(n);
			}
			// Finish
			tseitin = finish(boundvalue,(boundvalue-truevalue),minpossvalue,maxpossvalue,setnr);
			break;
		}
		case AGG_PROD: {
			// Compute the minimum and maximum possible value of the product.
			bool containsneg = false;
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				maxpossvalue *= abs(tsset.weight(n));
				if(tsset.weight(n) < 0) containsneg = true;
			}
			if(containsneg) minpossvalue = -maxpossvalue;
			// Finish
			tseitin = finish(boundvalue,(boundvalue/truevalue),minpossvalue,maxpossvalue,setnr);
			break;
		}
		case AGG_MIN: {
			// Compute the minimum possible value of the set.
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				minpossvalue = (tsset.weight(n) < minpossvalue) ? tsset.weight(n) : minpossvalue;
				// Decrease all weights greater than truevalue to truevalue.
				if(tsset.weight(n) > truevalue) tsset.setWeight(n,truevalue);
			}
			// Finish
			tseitin = finish(boundvalue,boundvalue,minpossvalue,maxpossvalue,setnr);
			break;
		}
		case AGG_MAX: {
			// Compute the maximum possible value of the set.
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				maxpossvalue = (tsset.weight(n) > maxpossvalue) ? tsset.weight(n) : maxpossvalue;
				// Increase all weights less than truevalue to truevalue.
				if(tsset.weight(n) < truevalue) tsset.setWeight(n,truevalue);
			}
			// Finish
			tseitin = finish(boundvalue,boundvalue,minpossvalue,maxpossvalue,setnr);
			break;
		}
		default: 
			assert(false);
			tseitin = 0;
	}
	return tseitin;
}

void AggGrounder::run(vector<int>& clause) const {
	clause.push_back(run());	
}

inline bool ClauseGrounder::check1(int l) const {
	return _conj ? l == _false : l == _true;
}

inline bool ClauseGrounder::check2(int l) const {
	return _conj ? l == _true : l == _false;
}

inline int ClauseGrounder::result1() const {
	return (_conj == _sign) ? _false : _true;
}

inline int ClauseGrounder::result2() const {
	return (_conj == _sign) ? _true : _false;
}

int ClauseGrounder::finish(vector<int>& cl) const {
	if(_verbosity > 2) {
		printorig();
	}
	if(cl.empty()) {
		if(_verbosity > 2) {
			clog << "Result = " << _translator->printAtom(result2()) << endl;
		}
		return result2();
	}
	else if(cl.size() == 1) {
		if(_verbosity > 2) {
			clog << "Result = " << (_sign ? _translator->printAtom(cl[0]) : _translator->printAtom(-cl[0])) << endl;
		}
		return _sign ? cl[0] : -cl[0];
	}
	else {
		TsType tp = _context._tseitin;
		if(!_sign) {
			if(tp == TS_IMPL) tp = TS_RIMPL;
			else if(tp == TS_RIMPL) tp = TS_IMPL;
		}
		if(_doublenegtseitin) {
			for(unsigned int n = 0; n < cl.size(); ++n) cl[n] = -cl[n];
			int ts = _translator->translate(cl,!_conj,tp);
			if(_verbosity > 2) {
				clog << "Result = " << (_sign ? "~" : "");
				clog << _translator->printAtom(cl[0]) << ' ';
				for(unsigned int n = 1; n < cl.size(); ++n) { 
					clog << (!_conj ? "& " : "| ") << _translator->printAtom(cl[n]) << ' ';
				}
			}
			return _sign ? -ts : ts;
		}
		else {
			int ts = _translator->translate(cl,_conj,tp);
			if(_verbosity > 2) {
				clog << "Result = " << (_sign ? "" : "~");
				clog << _translator->printAtom(cl[0]) << ' ';
				for(unsigned int n = 1; n < cl.size(); ++n) { 
					clog << (_conj ? "& " : "| ") << _translator->printAtom(cl[n]) << ' ';
				}
			}
			return _sign ? ts : -ts;
		}
	}
}

int BoolGrounder::run() const {
	vector<int> cl;
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		int l = _subgrounders[n]->run();
		if(check1(l)) return result1();
		else if(! check2(l)) cl.push_back(l);
	}
	return finish(cl);
}

void BoolGrounder::run(vector<int>& clause) const {
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		int l = _subgrounders[n]->run();
		if(check1(l)) {
			clause.clear();
			clause.push_back(result1());
			return;
		}
		else if(!check2(l)) clause.push_back(_sign ? l : -l);
	}
}

int QuantGrounder::run() const {
	if(_verbosity > 2) {
		printorig();
	}
	vector<int> cl;
	if(_generator->first()) {
		int l = _subgrounder->run();
		if(check1(l)) {
			if(_verbosity > 2) {
				clog << "Result = " << _translator->printAtom(result1()) << endl;
			}
			return result1();
		}
		else if(! check2(l)) cl.push_back(l);
		while(_generator->next()) {
			l = _subgrounder->run();
			if(check1(l)) {
				if(_verbosity > 2) {
					clog << "Result = " << _translator->printAtom(result1()) << endl;
				}
				return result1();
			}
			else if(! check2(l)) cl.push_back(l);
		}
	}
	return finish(cl);
}

void QuantGrounder::run(vector<int>& clause) const {
	if(_verbosity > 2) {
		printorig();
	}
	if(_generator->first()) {
		int l = _subgrounder->run();
		if(check1(l)) {
			clause.clear();
			clause.push_back(result1());
			if(_verbosity > 2) {
				clog << "Result = " << _translator->printAtom(result1()) << endl;
			}
			return;
		}
		else if(! check2(l)) clause.push_back(_sign ? l : -l);
		while(_generator->next()) {
			l = _subgrounder->run();
			if(check1(l)) {
				clause.clear();
				clause.push_back(result1());
				if(_verbosity > 2) {
					clog << "Result = " << _translator->printAtom(result1()) << endl;
				}
				return;
			}
			else if(! check2(l)) clause.push_back(_sign ? l : -l);
		}
	}
	if(_verbosity > 2) {
		clog << "Result = " << (_sign ? "" : "~");
		if(clause.empty()) {
			clog << (_conj ? "true" : "false") << endl;
		}
		else {
			clog << _translator->printAtom(clause[0]) << ' ';
			for(unsigned int n = 1; n < clause.size(); ++n) {
			   clog << (_conj ? "& " : "| ") << _translator->printAtom(clause[n]) << ' ';
			}
		}
	}
}

int EquivGrounder::run() const {
	// Run subgrounders
	int left = _leftgrounder->run();
	int right = _rightgrounder->run();

	if(left == right) return _sign ? _true : _false;
	else if(left == _true) {
		if(right == _false) return _sign ? _false : _true;
		else return _sign ? right : -right;
	}
	else if(left == _false) {
		if(right == _true) return _sign ? _false : _true;
		else return _sign ? -right : right;
	}
	else if(right == _true) return _sign ? left : -left;
	else if(right == _false) return _sign ? -left : left;
	else {
		GroundClause cl1(2);
		GroundClause cl2(2);
		cl1[0] = left;	cl1[1] = _sign ? -right : right;
		cl2[0] = -left;	cl2[1] = _sign ? right : -right;
		GroundClause tcl(2);
		TsType tp = _context._tseitin;
		tcl[0] = _translator->translate(cl1,false,tp);
		tcl[1] = _translator->translate(cl2,false,tp);
		return _translator->translate(tcl,true,tp);
	}
}

void EquivGrounder::run(vector<int>& clause) const {
	// Run subgrounders
	int left = _leftgrounder->run();
	int right = _rightgrounder->run();

	if(left == right) { clause.push_back(_sign ? _true : _false); return;	}
	else if(left == _true) {
		if(right == _false) { clause.push_back(_sign ? _false : _true); return; }
		else { clause.push_back(_sign ? right : -right); return; }
	}
	else if(left == _false) {
		if(right == _true) { clause.push_back(_sign ? _false : _true); return; }
		else { clause.push_back(_sign ? -right : right); return; }
	}
	else if(right == _true) { clause.push_back(_sign ? left : -left); return; }
	else if(right == _false) { clause.push_back(_sign ? -left : left); return; }
	else {
		GroundClause cl1(2);
		GroundClause cl2(2);
		cl1[0] = left;	cl1[1] = _sign ? -right : right;
		cl2[0] = -left;	cl2[1] = _sign ? right : -right;
		TsType tp = _context._tseitin;
		int ts1 = _translator->translate(cl1,false,tp);
		int ts2 = _translator->translate(cl2,false,tp);
		clause.push_back(ts1); clause.push_back(ts2);
	}
}

void TermGrounder::setorig(const Term* t, const map<Variable*,const DomainElement**>& mvd, int verb) {
	_verbosity = verb;
	map<Variable*,Variable*> mvv;
	for(set<Variable*>::const_iterator it = t->freevars().begin(); it != t->freevars().end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),ParseInfo());
		mvv[*it] = v;
		_varmap[v] = mvd.find(*it)->second;
	}
	_origterm = t->clone(mvv);
}

void TermGrounder::printorig() const {
	clog << "Grounding term " << _origterm->to_string();
	if(not _origterm->freevars().empty()) {
		clog << " with instance ";
		for(set<Variable*>::const_iterator it = _origterm->freevars().begin(); it != _origterm->freevars().end(); ++it) {
			clog << (*it)->to_string() << " = ";
			const DomainElement* e = *(_varmap.find(*it)->second);
			clog << e->to_string() << ' ';
		}
	}
	clog << endl;
}

const DomainElement* VarTermGrounder::run() const {
	return *_value;
}

const DomainElement* FuncTermGrounder::run() const {
	if(_verbosity > 2) { 
		printorig();
	}
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		_args[n] = _subtermgrounders[n]->run();
	}
	const DomainElement* result = (*_functable)[_args];
	if(_verbosity > 2) {
		clog << "Result (by FuncTermGrounder) = " << *result << endl;
	}
	return result;
}

//const DomainElement* CPSumTermGrounder::run() const {
//	if(_verbosity > 2) {
//		printorig();
//	}
//	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
//		_args[n] = _subtermgrounders[n]->run();
//	}
//cerr << "Arguments are ";
//for(ElementTuple::const_iterator it = _args.begin(); it != _args.end(); ++it)
//	cerr << *(*it) << ' ';
//cerr << endl;
//	const DomainElement* result = DomainElementFactory::instance()->create(_function,_args);
//	if(_verbosity > 2) {
//		clog << "Result (by CPSumTermGrounder) = " << *result << endl;
//	}
//	return result;
//}

const DomainElement* ThreeValuedFuncTermGrounder::run() const {
	if(_verbosity > 2) {
		printorig();
	}
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		_args[n] = _subtermgrounders[n]->run();
		if(not _args[n]) return 0;
	}
	if(not _universe.contains(_args)) {
		return 0;
	}
	const DomainElement* result = (*_functable)[_args];
	if(not result) {
		result = DomainElementFactory::instance()->create(_function,_args);
	}
	if(_verbosity > 2) {
		clog << "Result (by ThreeValuedFuncTermGrounder) = " << *result << endl;
	}
	return result;
}

const DomainElement* AggTermGrounder::run() const {
	int setnr = _setgrounder->run();
	const TsSet& tsset = _translator->groundset(setnr);
	assert(tsset.empty());
	double value = applyAgg(_type,tsset.trueweights());
	const DomainElement* result = DomainElementFactory::instance()->create(value);
	if(_verbosity > 2) {
		clog << "Result = " << *result << endl;
	}
	return result;
}

const DomainElement* ThreeValuedAggTermGrounder::run() const {
	//TODO
	assert(false);
	return 0;
}

int EnumSetGrounder::run() const {
	vector<int>	literals;
	vector<double> weights;
	vector<double> trueweights;
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		int l = _subgrounders[n]->run();
		if(l != _false) {
			const DomainElement* d = _subtermgrounders[n]->run();
			double w = d->type() == DET_INT ? (double) d->value()._int : d->value()._double;
			if(l == _true) trueweights.push_back(w);
			else {
				weights.push_back(w);
				literals.push_back(l);
			}
		}
	}
	int s = _translator->translateSet(literals,weights,trueweights);
	return s;
}

int QuantSetGrounder::run() const {
	vector<int> literals;
	vector<double> weights;
	vector<double> trueweights;
	if(_generator->first()) {
		int l = _subgrounder->run();
		if(l != _false) {
			const DomainElement* weight = _weightgrounder->run();
			double w = weight->type() == DET_INT ? (double) weight->value()._int : weight->value()._double;
			if(l == _true) trueweights.push_back(w);
			else {
				weights.push_back(w);
				literals.push_back(l);
			}
		}
		while(_generator->next()) {
			l = _subgrounder->run();
			if(l != _false) {
				const DomainElement* weight = _weightgrounder->run();
				double w = weight->type() == DET_INT ? (double) weight->value()._int : weight->value()._double;
				if(l == _true) trueweights.push_back(w);
				else {
					weights.push_back(w);
					literals.push_back(l);
				}
			}
		}
	}
	int s = _translator->translateSet(literals,weights,trueweights);
	return s;
}

HeadGrounder::HeadGrounder(AbstractGroundTheory* gt, InstanceChecker* pc, InstanceChecker* cc, PFSymbol* s, 
			const vector<TermGrounder*>& sg, const vector<SortTable*>& vst) :
	_grounding(gt), _subtermgrounders(sg), _truechecker(pc), _falsechecker(cc), _symbol(gt->translator()->addSymbol(s)),
	_args(sg.size()), _tables(vst) { }

int HeadGrounder::run() const {
	// Run subterm grounders
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) 
		_args[n] = _subtermgrounders[n]->run();
	
	// Checking partial functions
	for(unsigned int n = 0; n < _args.size(); ++n) {
		//TODO: only check positions that can be out of bounds or ...!
		//TODO: produce a warning!
		if(!_args[n]) return _false;
		if(!_tables[n]->contains(_args[n])) return _false;
	}

	// Run instance checkers and return grounding
	int atom = _grounding->translator()->translate(_symbol,_args);
	if(_truechecker->run(_args)) {
		_grounding->addUnitClause(atom);
	}
	else if(_falsechecker->run(_args)) {
		_grounding->addUnitClause(-atom);
	}
	return atom;
}

bool RuleGrounder::run() const {
	bool conj = _bodygrounder->conjunctive();
	if(_bodygenerator->first()) {	
		vector<int>	body;
		_bodygrounder->run(body);
		bool falsebody = (body.empty() && !conj) || (body.size() == 1 && body[0] == _false);
		if(!falsebody) {
			bool truebody = (body.empty() && conj) || (body.size() == 1 && body[0] == _true);
			if(_headgenerator->first()) {
				int head = _headgrounder->run();
				assert(head != _true);
				if(head != _false) {
					if(truebody) _definition->addTrueRule(head);
					else _definition->addPCRule(head,body,conj,_context._tseitin == TS_RULE);
				}
				while(_headgenerator->next()) {
					head = _headgrounder->run();
					assert(head != _true);
					if(head != _false) {
						if(truebody) _definition->addTrueRule(head);
						else _definition->addPCRule(head,body,conj,_context._tseitin == TS_RULE);
					}
				}
			}
		}
		while(_bodygenerator->next()) {
			body.clear();
			_bodygrounder->run(body);
			bool falsebody = (body.empty() && !conj) || (body.size() == 1 && body[0] == _false);
			if(!falsebody) {
				bool truebody = (body.empty() && conj) || (body.size() == 1 && body[0] == _true);
				if(_headgenerator->first()) {
					int head = _headgrounder->run();
					assert(head != _true);
					if(head != _false) {
						if(truebody) _definition->addTrueRule(head);
						else _definition->addPCRule(head,body,conj,_context._tseitin == TS_RULE);
					}
					while(_headgenerator->next()) {
						head = _headgrounder->run();
						assert(head != _true);
						if(head != _false) {
							if(truebody) _definition->addTrueRule(head);
							else _definition->addPCRule(head,body,conj,_context._tseitin == TS_RULE);
						}
					}
				}
			}
		}
	}
	return true;
}

bool DefinitionGrounder::run() const {
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		bool b = _subgrounders[n]->run();
		if(!b) return false;
	}
	_grounding->addDefinition(_definition);
	return true;
}


/******************************
	GrounderFactory methods
******************************/

/**
 * set<Function*> GrounderFactory::findCPFunctions()
 * DESCRIPTION
 * 		Finds out which functions can be passed to the constraint solver.
 */
set<const Function*> GrounderFactory::findCPFunctions(const AbstractTheory* theory) {
	Vocabulary* vocabulary = theory->vocabulary();
	for(map<string,Function*>::const_iterator funcit = vocabulary->firstfunc(); funcit != vocabulary->lastfunc(); ++funcit) {
		Function* function = funcit->second;
		bool groundtocp = false;
		// Check whether the (user-defined) function's outsort is over integers
		Sort* intsort = *(vocabulary->sort("int")->begin());
		if(function->overloaded()) {
			set<Function*> nonbuiltins = function->nonbuiltins();
			for(set<Function*>::const_iterator nbfit = nonbuiltins.begin(); nbfit != nonbuiltins.end(); ++nbfit) {
				groundtocp = (SortUtils::resolve(function->outsort(),intsort,vocabulary) == intsort);
			}
		} else if(!function->builtin()) {
			groundtocp = (SortUtils::resolve(function->outsort(),intsort,vocabulary) == intsort);
		}
		if(groundtocp) _cpfunctions.insert(function);
	}
	if(_options->groundverbosity() > 1) {
		clog << "Function that could be handled by constraint solver: ";
		for(set<const Function*>::const_iterator it = _cpfunctions.begin(); it != _cpfunctions.end(); ++it) {
			clog << (*it)->to_string() << " ";
		}
		clog << endl;
	}
	return _cpfunctions;
}

bool GrounderFactory::isCPFunction(const PFSymbol* symbol) const { 
	if(typeid(*symbol) == typeid(Function)) {
		const Function* func = static_cast<const Function*>(symbol);
		return _cpfunctions.find(func) != _cpfunctions.end();
	}
	return false;
}


/**
 * bool GrounderFactory::recursive(const Formula*)
 * DESCRIPTION
 * 		Finds out whether a formula contains recursively defined symbols.
 */
bool GrounderFactory::recursive(const Formula* f) {
	for(set<PFSymbol*>::const_iterator it = _context._defined.begin(); it != _context._defined.end(); ++it) {
		if(f->contains(*it)) return true;
	}
	return false;
}

/**
 * void GrounderFactory::InitContext() 
 * DESCRIPTION
 *		Initializes the context of the GrounderFactory before visiting a sentence.
 */
void GrounderFactory::InitContext() {
	_context._truegen		= false;
	_context._funccontext	= PC_POSITIVE;
	_context._monotone		= PC_POSITIVE;
	_context._component		= CC_SENTENCE;
	_context._tseitin		= TS_IMPL;
	_context._defined.clear();
}

void GrounderFactory::AggContext() {
	_context._truegen = false;
	_context._funccontext = PC_POSITIVE;
	_context._tseitin = (_context._tseitin == TS_RULE) ? TS_RULE : TS_EQ;
	_context._component = CC_FORMULA;
}

/**
 *	void GrounderFactory::SaveContext() 
 *	DESCRIPTION
 *		Pushes the current context on a stack 
 */
void GrounderFactory::SaveContext() {
	_contextstack.push(_context);
}

/**
 * void GrounderFactory::RestoreContext()
 * DESCRIPTION
 *		Restores the context to the top of the stack and pops the stack
 */
void GrounderFactory::RestoreContext() {
	_context = _contextstack.top();
	_contextstack.pop();
}

/**
 * void GrounderFactory::DeeperContext(bool sign)
 * DESCRIPTION
 *		Adapts the context to go one level deeper, and inverting some values if sign is negative
 * PARAMETERS
 *		sign	- the sign
 */
void GrounderFactory::DeeperContext(bool sign) {
	// One level deeper
	if(_context._component == CC_SENTENCE) _context._component = CC_FORMULA;
	// Swap positive, truegen and tseitin according to sign
	if(!sign) {

		_context._truegen = !_context._truegen;

		if(_context._funccontext == PC_POSITIVE) _context._funccontext = PC_NEGATIVE;
		else if(_context._funccontext == PC_NEGATIVE) _context._funccontext = PC_POSITIVE;
		if(_context._monotone == PC_POSITIVE) _context._monotone = PC_NEGATIVE;
		else if(_context._monotone == PC_NEGATIVE) _context._monotone = PC_POSITIVE;

		if(_context._tseitin == TS_IMPL) _context._tseitin = TS_RIMPL;
		else if(_context._tseitin == TS_RIMPL) _context._tseitin = TS_IMPL;

	}
}

/**
 * void GrounderFactory::descend(Term* t)
 * DESCRIPTION
 *		Visits a term and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		t	- the visited term
 */
void GrounderFactory::descend(Term* t) {
	SaveContext();
	t->accept(this);
	RestoreContext();
}

/**
 * void GrounderFactory::descend(SetExpr* s)
 * DESCRIPTION
 *		Visits a set and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		s	- the visited set
 */
void GrounderFactory::descend(SetExpr* s) {
	SaveContext();
	s->accept(this);
	RestoreContext();
}

/**
 * void GrounderFactory::descend(Formula* f)
 * DESCRIPTION
 *		Visits a formula and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		f	- the visited formula
 */
void GrounderFactory::descend(Formula* f) {
	SaveContext();
	f->accept(this);
	RestoreContext();
}

/**
 * void GrounderFactory::descend(Rule* r)
 * DESCRIPTION
 *		Visits a rule and ensures the context is restored to the value before the visit.
 * PARAMETERS
 *		r	- the visited rule
 */
void GrounderFactory::descend(Rule* r) {
	SaveContext();
	r->accept(this);
	RestoreContext();
}

/**
 * TopLevelGrounder* GrounderFactory::create(const AbstractTheory* theory)
 * DESCRIPTION
 *		Creates a grounder for the given theory. The grounding produced by that grounder
 *		will be (partially) reduced with respect to the structure _structure of the GrounderFactory.
 *		The produced grounding is not passed to a solver, but stored internally as a EcnfTheory.
 * PARAMETERS
 *		theory	- the theory for which a grounder will be created
 * PRECONDITIONS
 *		The vocabulary of theory is a subset of the vocabulary of the structure of the GrounderFactory.
 * RETURNS
 *		A grounder such that calling run() on it produces a grounding.
 *		This grounding can then be obtained by calling grounding() on the grounder.
 */
TopLevelGrounder* GrounderFactory::create(const AbstractTheory* theory) {
	// Allocate an ecnf theory to be returned by the grounder
	_grounding = new GroundTheory(theory->vocabulary(),_structure->clone());

	// Find function that can be passed to CP solver.
	if(_options->cpsupport()) findCPFunctions(theory);

	// Create the grounder
	theory->accept(this);
	return _toplevelgrounder;
}

/**
 * TopLevelGrounder* GrounderFactory::create(const AbstractTheory* theory, SATSolver* solver)
 * DESCRIPTION
 *		Creates a grounder for the given theory. The grounding produced by that grounder
 *		will be (partially) reduced with respect to the structure _structure of the GrounderFactory.
 *		The produced grounding is directly passed to the given solver. 
 * PARAMETERS
 *		theory	- the theory for which a grounder will be created.
 *		solver	- the solver to which the grounding will be passed.
 * PRECONDITIONS
 *		The vocabulary of theory is a subset of the vocabulary of the structure of the GrounderFactory.
 * RETURNS
 *		A grounder such that calling run() on it produces a grounding.
 *		This grounding can then be obtained by calling grounding() on the grounder.
 *		One or more models of the ground theory can be obtained by calling solve() on
 *		the solver.
 */
TopLevelGrounder* GrounderFactory::create(const AbstractTheory* theory, SATSolver* solver) {
	// Allocate a solver theory
	_grounding = new SolverTheory(theory->vocabulary(),solver,_structure->clone());

	// Find function that can be passed to CP solver.
	if(_options->cpsupport()) findCPFunctions(theory);

	// Create the grounder
	theory->accept(this);
	return _toplevelgrounder;
}

/**
 * void GrounderFactory::visit(const EcnfTheory* ecnf)
 * DESCRIPTION
 *		Creates a grounder for a ground ecnf theory. This grounder returns a (reduced) copy of the ecnf theory.
 * PARAMETERS
 *		ecnf	- the given ground ecnf theory
 * POSTCONDITIONS
 *		_toplevelgrounder is equal to the created grounder.
 */
void GrounderFactory::visit(const GroundTheory* ecnf) {
	_toplevelgrounder = new CopyGrounder(_grounding,ecnf,_options->groundverbosity());	
}

/**
 * void GrounderFactory::visit(const Theory* theory)
 * DESCRIPTION
 *		Creates a grounder for a non-ground theory.
 * PARAMETERS
 *		theory	- the non-ground theory
 * POSTCONDITIONS
 *		_toplevelgrounder is equal to the created grounder
 */
void GrounderFactory::visit(const Theory* theory) {
	// Collect all components (sentences, definitions, and fixpoint definitions) of the theory
	set<TheoryComponent*> tcomps = theory->components();
	vector<TheoryComponent*> components(tcomps.begin(),tcomps.end());

	// Order components the components to optimize the grounding process
	// TODO

	// Create grounders for all components
	vector<TopLevelGrounder*> children(components.size());
	for(unsigned int n = 0; n < components.size(); ++n) {
		InitContext();
		if(_options->groundverbosity() > 0) clog << "Creating a grounder for " << *(components[n]) << endl;
		components[n]->accept(this);
		children[n] = _toplevelgrounder; 
	}

	// Create the grounder
	_toplevelgrounder = new TheoryGrounder(_grounding,children,_options->groundverbosity());
}

/**
 * void GrounderFactory::visit(const PredForm* pf) 
 * DESCRIPTION
 *		Creates a grounder for an atomic formula.
 * PARAMETERS
 *		pf	- the atomic formula
 * PRECONDITIONS
 *		Each free variable that occurs in pf occurs in _varmapping.
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CC_SENTENCE:	_toplevelgrounder
 *			CC_HEAD:		_headgrounder
 *			CC_FORMULA:		_formgrounder
 */
void GrounderFactory::visit(const PredForm* pf) {
	// Move all functions and aggregates that are three-valued according 
	// to _structure outside the atom. To avoid changing the original atom, 
	// we first clone it.
	PredForm* newpf = pf->clone();
	Formula* transpf = FormulaUtils::moveThreeValTerms(newpf,_structure,(_context._funccontext != PC_NEGATIVE),_options->cpsupport(),_cpfunctions);

	if(typeid(*transpf) != typeid(PredForm)) {	// The rewriting changed the atom
		if(_options->groundverbosity() > 1) {
			clog << "Rewritten " << *pf << " to " << *transpf << endl; 
		}
		transpf->accept(this);
	}
	else {	// The rewriting did not change the atom
		PredForm* ptranspf = dynamic_cast<PredForm*>(transpf);
		// Create grounders for the subterms
		bool cpsubterms = false;
		vector<TermGrounder*> subtermgrounders;
		vector<SortTable*>	  argsorttables;
		for(unsigned int n = 0; n < ptranspf->subterms().size(); ++n) {
			descend(ptranspf->subterms()[n]);
			subtermgrounders.push_back(_termgrounder);
			cpsubterms = cpsubterms || _termgrounder->canReturnCPVar();
			argsorttables.push_back(_structure->inter(ptranspf->symbol()->sorts()[n]));
		}
		// Create checkers and grounder
		if(_options->cpsupport() && cpsubterms) {
			if(_context._component == CC_HEAD) {
				notyetimplemented("No CP support for defined predicates yet...");
			}
			else {
				string name = ptranspf->symbol()->name();
				if(typeid(*(ptranspf->symbol())) == typeid(Predicate) && (name == "=/2" || name == "</2" || name == ">/2")) {
					CompType comp;
					if(name == "=/2")
						comp = pf->sign() ? CT_EQ : CT_NEQ;
					else if(name == "</2")
						comp = pf->sign() ? CT_LT : CT_GEQ;
					else if(name == ">/2")
						comp = pf->sign() ? CT_GT : CT_LEQ;
					else {
						assert(false);
						comp = CT_EQ;
					}
					_formgrounder = new CPFormulaGrounder(_grounding->translator(),_grounding->termtranslator(),
											subtermgrounders[0],comp,subtermgrounders[1],_context);
				}
				else if(typeid(*(pf->symbol())) == typeid(Predicate)) {
					// pf is a predicate different from =, < and >
					assert(false);
					// THIS WILL NEVER HAPPEN!! TODO Explain why...
				}
				else {
					assert(typeid(*(pf->symbol())) == typeid(Function));
					// pf is a function: F(x1,..,xn,xo) which is same as F(x1,..,xn) = xo
					assert(false);
					// THIS WILL NEVER HAPPEN!! TODO Explain why...
				}
				if(_context._component == CC_SENTENCE) 
					_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_options->groundverbosity());
			}
		}
		else {
			PredInter* inter = _structure->inter(ptranspf->symbol());
			CheckerFactory checkfactory;
			if(_context._component == CC_HEAD) {
				// Create instance checkers
				InstanceChecker* truech = checkfactory.create(inter,true,true);
				InstanceChecker* falsech = checkfactory.create(inter,false,true);
				// Create the grounder
				_headgrounder = new HeadGrounder(_grounding,truech,falsech,ptranspf->symbol(),subtermgrounders,argsorttables);
			}
			else {
				// Create instance checkers
				InstanceChecker* possch;
				InstanceChecker* certainch;
				if(_context._truegen == ptranspf->sign()) {	
					possch = checkfactory.create(inter,false,false);
					certainch = checkfactory.create(inter,true,true);
				}
				else {	
					possch = checkfactory.create(inter,true,false);
					certainch = checkfactory.create(inter,false,true);
				}
				// Create the grounder
				if(_options->cpsupport() && (typeid(*(ptranspf->symbol())) == typeid(Function)) && isCPFunction(ptranspf->symbol())) { 
					Function* func = static_cast<Function*>(ptranspf->symbol());
					_formgrounder = new CPAtomGrounder(_grounding->translator(),_grounding->termtranslator(),ptranspf->sign(),func,
											subtermgrounders,possch,certainch,argsorttables,_context);
				} 
				else {
					_formgrounder = new AtomGrounder(_grounding->translator(),ptranspf->sign(),ptranspf->symbol(),
											subtermgrounders,possch,certainch,argsorttables,_context);
				}
				_formgrounder->setorig(ptranspf,_varmapping,_options->groundverbosity());
				if(_context._component == CC_SENTENCE) 
					_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_options->groundverbosity());
			}
		}
	}
	transpf->recursiveDelete();
}

/**
 * void GrounderFactory::visit(const BoolForm* bf)
 * DESCRIPTION
 *		Creates a grounder for a conjunction or disjunction of formulas
 * PARAMETERS
 *		bf	- the conjunction or disjunction
 * PRECONDITIONS
 *		Each free variable that occurs in bf occurs in _varmapping.
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CC_SENTENCE:	_toplevelgrounder
 *			CC_FORMULA:		_formgrounder
 *			CC_HEAD is not possible
 */
void GrounderFactory::visit(const BoolForm* bf) {

	// Handle a top-level conjunction without creating tseitin atoms
	if(_context._component == CC_SENTENCE && (bf->conj() == bf->sign())) {
		// If bf is a negated disjunction, push the negation one level deeper.
		// Take a clone to avoid changing bf;
		BoolForm* newbf = bf->clone();
		if(!(newbf->conj())) {
			newbf->conj(true);
			newbf->swapsign();
			for(vector<Formula*>::const_iterator it = newbf->subformulas().begin(); it != newbf->subformulas().end(); ++it)
				(*it)->swapsign();
		}

		// Visit the subformulas
		vector<TopLevelGrounder*> sub;
		for(vector<Formula*>::const_iterator it = newbf->subformulas().begin(); it != newbf->subformulas().end(); ++it) {
			descend(*it);
			sub.push_back(_toplevelgrounder);
		}
		newbf->recursiveDelete();
		_toplevelgrounder = new TheoryGrounder(_grounding,sub,_options->groundverbosity());
	}
	else {	// Formula bf is not a top-level conjunction

		// Create grounders for subformulas
		SaveContext();
		DeeperContext(bf->sign());
		vector<FormulaGrounder*> sub;
		for(vector<Formula*>::const_iterator it = bf->subformulas().begin(); it != bf->subformulas().end(); ++it) {
			descend(*it);
			sub.push_back(_formgrounder);
		}
		RestoreContext();

		// Create grounder
		SaveContext();
		if(recursive(bf)) _context._tseitin = TS_RULE;
		_formgrounder = new BoolGrounder(_grounding->translator(),sub,bf->sign(),bf->conj(),_context);
		RestoreContext();
		_formgrounder->setorig(bf,_varmapping,_options->groundverbosity());
		if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_options->groundverbosity());

	}
}

/**
 * void GrounderFactory::visit(const QuantForm* qf)
 * DESCRIPTION
 *		Creates a grounder for a quantified formula
 * PARAMETERS
 *		qf	- the quantified formula
 * PRECONDITIONS
 *		Each free variable that occurs in qf occurs in _varmapping.
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CC_SENTENCE:	_toplevelgrounder
 *			CC_FORMULA:		_formgrounder
 *			CC_HEAD is not possible
 */
void GrounderFactory::visit(const QuantForm* qf) {
	// Create instance generator
	vector<const DomainElement**> vars;
	vector<SortTable*>	tables;
	for(std::set<Variable*>::const_iterator it = qf->quantvars().begin(); it != qf->quantvars().end(); ++it) {
		const DomainElement** d = new const DomainElement*();
		_varmapping[*it] = d;
		vars.push_back(d);
		SortTable* st = _structure->inter((*it)->sort());
		if(!st->finite()) {
			cerr << "Warning: infinite grounding of formula ";
			if(qf->pi().original()) {
				cerr << *(qf->pi().original());
				cerr << "\n   internal representation: ";
			}
			cerr << *qf << endl;
		}
		tables.push_back(st);
	}
	GeneratorFactory gf;
	InstGenerator* gen = gf.create(vars,tables);

	// Handle top-level universal quantifiers efficiently
	if(_context._component == CC_SENTENCE && (qf->sign() == qf->univ())) {
		Formula* newsub = qf->subf()->clone();
		if(!(qf->univ())) newsub->swapsign();
		descend(newsub);
		newsub->recursiveDelete();
		_toplevelgrounder = new UnivSentGrounder(_grounding,_toplevelgrounder,gen,_options->groundverbosity());
	}
	else {
		// Create grounder for subformula
		SaveContext();
		DeeperContext(qf->sign());
		_context._truegen = !(qf->univ()); 
		descend(qf->subf());
		RestoreContext();

		// Create the grounder
		SaveContext();
		if(recursive(qf)) _context._tseitin = TS_RULE;
		_formgrounder = new QuantGrounder(_grounding->translator(),_formgrounder,qf->sign(),qf->univ(),gen,_context);
		RestoreContext();
		_formgrounder->setorig(qf,_varmapping,_options->groundverbosity());
		if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_options->groundverbosity());
	}
}

/**
 * void GrounderFactory::visit(const EquivForm* ef)
 * DESCRIPTION
 *		Creates a grounder for an equivalence.
 * PARAMETERS
 *		ef	- the equivalence
 * PRECONDITIONS
 *		Each free variable that occurs in ef occurs in _varmapping.
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CC_SENTENCE:	_toplevelgrounder
 *			CC_FORMULA:		_formgrounder
 *			CC_HEAD is not possible
 */
void GrounderFactory::visit(const EquivForm* ef) {
	// Create grounders for the subformulas
	SaveContext();
	DeeperContext(ef->sign());
	_context._funccontext = PC_BOTH;
	_context._monotone = PC_BOTH;
	_context._tseitin = TS_EQ; 

	descend(ef->left());
	FormulaGrounder* leftg = _formgrounder;
	descend(ef->right());
	FormulaGrounder* rightg = _formgrounder;
	RestoreContext();

	// Create the grounder
	SaveContext();
	if(recursive(ef)) _context._tseitin = TS_RULE;
	_formgrounder = new EquivGrounder(_grounding->translator(),leftg,rightg,ef->sign(),_context);
	RestoreContext();
	if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true,_options->groundverbosity());
}

/**
 * void GrounderFactory::visit(const AggForm* af)
 * DESCRIPTION
 * 		Creates a grounder for an aggregate.
 */
void GrounderFactory::visit(const AggForm* af) {
	AggForm* newaf = af->clone();
	Formula* transaf = FormulaUtils::moveThreeValTerms(newaf,_structure,(_context._funccontext != PC_NEGATIVE),_options->cpsupport(),_cpfunctions);

	if(typeid(*transaf) != typeid(AggForm)) {	// The rewriting changed the atom
		transaf->accept(this);
	}
	else {	// The rewriting did not change the atom
		AggForm* atransaf = dynamic_cast<AggForm*>(transaf);
		// Create grounder for the bound
		descend(atransaf->left());
		TermGrounder* boundgr = _termgrounder;
	
		// Create grounder for the set
		SaveContext();
		if(recursive(atransaf)) assert(FormulaUtils::monotone(atransaf) || FormulaUtils::antimonotone(atransaf));
		DeeperContext(!FormulaUtils::antimonotone(atransaf));
		descend(atransaf->right()->set());
		SetGrounder* setgr = _setgrounder;
		RestoreContext();
	
		// Create aggregate grounder
		SaveContext();
		if(recursive(atransaf)) _context._tseitin = TS_RULE;
		// TODO: change AggGrounder so that it accepts a CompType...
		char cmp; bool sgn = atransaf->sign();
		switch(atransaf->comp()) {
			case CT_EQ: cmp = '='; break;
			case CT_NEQ: cmp = '='; sgn = !sgn; break;
			case CT_LT: cmp = '<'; break;
			case CT_LEQ: cmp = '>'; sgn = !sgn; break;
			case CT_GT: cmp = '>'; break;
			case CT_GEQ: cmp = '<'; sgn = !sgn; break;
			default: assert(false); cmp = '=';
		}
		_formgrounder = new AggGrounder(_grounding->translator(),_context,atransaf->right()->function(),setgr,boundgr,cmp,sgn);
		RestoreContext();
		if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true,_options->groundverbosity());
	}
	transaf->recursiveDelete();
}

/**
 * void GrounderFactory::visit(const EqChainForm* ef)
 * DESCRIPTION
 * 		Creates a grounder for an equation chain.
 */
void GrounderFactory::visit(const EqChainForm* ef) {
	Formula* f = ef->clone();
	f = FormulaUtils::remove_eqchains(f,_grounding->vocabulary());
	f->accept(this);
	f->recursiveDelete();
}

/**
 * void GrounderFactory::visit(const VarTerm* t)
 * DESCRIPTION
 * 		Creates a grounder for a variable term.
 */
void GrounderFactory::visit(const VarTerm* t) {
	assert(_varmapping.find(t->var()) != _varmapping.end());
	_termgrounder = new VarTermGrounder(_varmapping.find(t->var())->second);
	_termgrounder->setorig(t,_varmapping,_options->groundverbosity());
}

/**
 * void GrounderFactory::visit(const DomainTerm* t)
 * DESCRIPTION
 * 		Creates a grounder for a domain term.
 */
void GrounderFactory::visit(const DomainTerm* t) {
	_termgrounder = new DomTermGrounder(t->value());
	_termgrounder->setorig(t,_varmapping,_options->groundverbosity());
}

/**
 * void GrounderFactory::visit(const FuncTerm* t)
 * DESCRIPTION
 * 		Creates a grounder for a function term.
 */
void GrounderFactory::visit(const FuncTerm* t) {
	// Create grounders for subterms
	vector<TermGrounder*> subtermgrounders;
	for(vector<Term*>::const_iterator it = t->subterms().begin(); it != t->subterms().end(); ++it) {
		(*it)->accept(this);
		if(_termgrounder) subtermgrounders.push_back(_termgrounder);
	}

	// Create term grounder
	Function* function = t->function();
//	if(_options->cpsupport() && function->name() == "+/2") {
//		//FIXME Probably not always correct TODO Check whether subtermgrounders can return CPVars? Is that enough?
//		FuncTable* ftable = _structure->inter(function)->functable();
//		assert(ftable);
//		_termgrounder = new CPSumTermGrounder(subtermgrounders,function,ftable);
//	} else 
	if(_structure->inter(function)->approxtwovalued()) {
		FuncTable* ftable = _structure->inter(function)->functable();
		assert(ftable);
		_termgrounder = new FuncTermGrounder(subtermgrounders,ftable);
	}
	else {
		Universe universe = _structure->universe(function);
		FuncTable* ftable = new FuncTable(new EnumeratedInternalFuncTable(),universe); 
		vector<SortTable*> domtabs = universe.tables(); domtabs.pop_back();
		Universe domain(domtabs);
		if(domain.approxfinite()) {
			const PredTable* ptable = _structure->inter(function)->graphinter()->ct();
			for(TableIterator it = ptable->begin(); it.hasNext(); ++it) {
				ftable->add(*it);
			}
		}
		else {
			assert(false);
			// This should never happen.. FuncTerm should have been moved.
		}

		_termgrounder = new ThreeValuedFuncTermGrounder(subtermgrounders,function,ftable,universe);
	}
	_termgrounder->setorig(t,_varmapping,_options->groundverbosity());
}

/**
 * void GrounderFactory::visit(const AggTerm* at)
 * DESCRIPTION
 * 		Creates a grounder for a aggregate term.
 */
void GrounderFactory::visit(const AggTerm* t) {
	// Create set grounder
	t->set()->accept(this);

	// Create term grounder
	//if(SetUtils::approxTwoValued(t->set(),_structure) || not _options->cpsupport())
	_termgrounder = new AggTermGrounder(_grounding->translator(),t->function(),_setgrounder);
	//else //TODO
	//	_termgrounder = new ThreeValuedAggTermGrounder(_grounding->translator(),t->function(),_setgrounder);
	_termgrounder->setorig(t,_varmapping,_options->groundverbosity());
}

/**
 * void GrounderFactory::visit(const EnumSetExpr* s)
 * DESCRIPTION
 * 		Creates a grounder for an enumarated set.
 */
void GrounderFactory::visit(const EnumSetExpr* s) {
	// Create grounders for formulas and weights
	vector<FormulaGrounder*> subgr;
	vector<TermGrounder*> subtgr;
	SaveContext();
	AggContext();
	for(unsigned int n = 0; n < s->subformulas().size(); ++n) {
		descend(s->subformulas()[n]);
		subgr.push_back(_formgrounder);
		descend(s->subterms()[n]);
		subtgr.push_back(_termgrounder);
	}
	RestoreContext();

	// Create set grounder
	_setgrounder = new EnumSetGrounder(_grounding->translator(),subgr,subtgr);
}

/**
 * void GrounderFactory::visit(const QuantSetExpr* s)
 * DESCRIPTION
 * 		Creates a grounder for a quantified set.
 */
void GrounderFactory::visit(const QuantSetExpr* s) {
	// Create instance generator
	InstGenerator* gen = 0;
	GeneratorNode* node = 0;
	for(set<Variable*>::const_iterator it = s->quantvars().begin(); it != s->quantvars().end(); ++it) {
		const DomainElement** d = new const DomainElement*();
		_varmapping[*it] = d;
		SortTable* st = _structure->inter((*it)->sort());
		assert(st->finite());	// TODO: produce an error message
		SortInstGenerator* tig = new SortInstGenerator(st,d);
		if(s->quantvars().size() == 1) {
			gen = tig;
			break;
		}
		else if(it == s->quantvars().begin()) node = new LeafGeneratorNode(tig);
		else node = new OneChildGeneratorNode(tig,node);
	}
	if(!gen) gen = new TreeInstGenerator(node);
	
	// Create grounder for subformula
	SaveContext();
	AggContext();
	descend(s->subformulas()[0]);
	FormulaGrounder* sub = _formgrounder;
	RestoreContext();

	// Create grounder for weight
	descend(s->subterms()[0]);
	TermGrounder* wg = _termgrounder;

	// Create grounder	
	_setgrounder = new QuantSetGrounder(_grounding->translator(),sub,gen,wg);
}

/**
 * void GrounderFactory::visit(const Definition* def)
 * DESCRIPTION
 * 		Creates a grounder for a definition.
 */
void GrounderFactory::visit(const Definition* def) {
	// Create new ground definition
	_definition = new GroundDefinition(_grounding->translator());

	// Store defined predicates
	for(set<PFSymbol*>::const_iterator it = def->defsymbols().begin(); it != def->defsymbols().end(); ++it) {
		_context._defined.insert(*it);
	}
	
	// Create rule grounders
	vector<RuleGrounder*> subgrounders;
	for(vector<Rule*>::const_iterator it = def->rules().begin(); it != def->rules().end(); ++it) {
		descend(*it);
		subgrounders.push_back(_rulegrounder);
	}
	
	// Create definition grounder
	_toplevelgrounder = new DefinitionGrounder(_grounding,_definition,subgrounders,_options->groundverbosity());

	_context._defined.clear();
}

/**
 * void GrounderFactory::visit(const Rule* rule)
 * DESCRIPTION
 * 		Creates a grounder for a definitional rule.
 */
void GrounderFactory::visit(const Rule* rule) {
	// Split the quantified variables in two categories: 
	//		1. the variables that only occur in the head
	//		2. the variables that occur in the body (and possibly in the head)
	vector<Variable*>	headvars;
	vector<Variable*>	bodyvars;
	for(set<Variable*>::const_iterator it = rule->quantvars().begin(); it != rule->quantvars().end(); ++it) {
		if(rule->body()->contains(*it)) {
			bodyvars.push_back(*it);
		}
		else {
			headvars.push_back(*it);
		}
	}

	// Create head instance generator
	InstGenerator* headgen = 0;
	GeneratorNode* hnode = 0;
	for(unsigned int n = 0; n < headvars.size(); ++n) {
		const DomainElement** d = new const DomainElement*();
		_varmapping[headvars[n]] = d;
		SortTable* st = _structure->inter(headvars[n]->sort());
		assert(st->finite());	// TODO: produce an error message
		SortInstGenerator* sig = new SortInstGenerator(st,d);
		if(headvars.size() == 1) {
			headgen = sig;
			break;
		}
		else if(n == 0) hnode = new LeafGeneratorNode(sig);
		else hnode = new OneChildGeneratorNode(sig,hnode);
	}
	if(!headgen) headgen = new TreeInstGenerator(hnode);
	
	// Create body instance generator
	InstGenerator* bodygen = 0;
	GeneratorNode* bnode = 0;
	for(unsigned int n = 0; n < bodyvars.size(); ++n) {
		const DomainElement** d = new const DomainElement*();
		_varmapping[bodyvars[n]] = d;
		SortTable* st = _structure->inter(bodyvars[n]->sort());
		assert(st->finite());	// TODO: produce an error message
		SortInstGenerator* sig = new SortInstGenerator(st,d);
		if(bodyvars.size() == 1) {
			bodygen = sig;
			break;
		}
		else if(n == 0) bnode = new LeafGeneratorNode(sig);
		else bnode = new OneChildGeneratorNode(sig,bnode);
	}
	if(!bodygen) bodygen = new TreeInstGenerator(bnode);
	
	// Create head grounder
	SaveContext();
	_context._component = CC_HEAD;
	descend(rule->head());
	HeadGrounder* headgr = _headgrounder;
	RestoreContext();

	// Create body grounder
	SaveContext();
	_context._funccontext = PC_NEGATIVE;		// minimize truth value of rule bodies
	_context._monotone = PC_POSITIVE;
	_context._truegen = true;				// body instance generator corresponds to an existential quantifier
	_context._component = CC_FORMULA;
	_context._tseitin = TS_EQ;
	descend(rule->body());
	FormulaGrounder* bodygr = _formgrounder;
	RestoreContext();

	// Create rule grounder
	SaveContext();
	if(recursive(rule->body())) _context._tseitin = TS_RULE;
	_rulegrounder = new RuleGrounder(_definition,headgr,bodygr,headgen,bodygen,_context);
	RestoreContext();

}

/**************
	Visitor
**************/

void TheoryVisitor::visit(const CPVarTerm*) {
	// TODO
}

void TheoryVisitor::visit(const CPWSumTerm*) {
	// TODO
}

void TheoryVisitor::visit(const CPSumTerm*) {
	// TODO
}

void TheoryVisitor::visit(const CPReification*) {
	// TODO
}
