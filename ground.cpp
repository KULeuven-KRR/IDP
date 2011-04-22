/************************************
	ground.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "ground.hpp"

#include <typeinfo>
#include <iostream>
#include <sstream>
#include <limits>
#include <cmath>

#include "element.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "ecnf.hpp"
#include "options.hpp"
#include "generator.hpp"
#include "checker.hpp"

using namespace std;

/** The two built-in literals 'true' and 'false' **/
int _true = numeric_limits<int>::max();
int _false = 0;

extern CLOptions _cloptions;

/**********************************************
	Translate from ground atoms to numbers
**********************************************/

int GroundTranslator::translate(unsigned int n, const vector<domelement>& args) {
	map<vector<domelement>,int>::iterator jt = _table[n].lower_bound(args);
	if(jt != _table[n].end() && jt->first == args) {
		return jt->second;
	}
	else {
		int nr = nextNumber();
		_table[n].insert(jt,pair<vector<domelement>,int>(args,nr));
		_backsymbtable[nr] = _symboffsets[n];
		_backargstable[nr] = args;
		return nr;
	}
}

int GroundTranslator::translate(PFSymbol* s, const vector<TypedElement>& args) {
	unsigned int offset = addSymbol(s);
	vector<domelement> newargs(args.size());
	for(unsigned int n = 0; n < args.size(); ++n) {
		newargs[n] = CPPointer(args[n]);
	}
	return translate(offset,newargs);
}

int GroundTranslator::translate(const vector<int>& clause, bool conj, TsType tstype) {
	int nr = nextNumber();
	PCTsBody* tsbody = new PCTsBody(tstype,clause,conj);
	_tsbodies[nr] = tsbody;
	return nr;
}

int	GroundTranslator::translate(double bound, char comp, bool strict, AggType aggtype, int setnr, TsType tstype) {
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
		_tsbodies[nr] = tsbody;
		return nr;
	}
}

int GroundTranslator::translate(CPTerm* left, CompType comp, const CPBound& right, TsType tstype) {
	int nr = nextNumber();
	CPTsBody* tb = new CPTsBody(tstype,left,comp,right);
	_tsbodies[nr] = tb;
	return nr;
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
		_backargstable.push_back(vector<domelement>(0));
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
	_table.push_back(map<vector<domelement>,int>());
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
				s << ElementUtil::ElementToString((args(nr))[c]);
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

unsigned int GroundTermTranslator::translate(unsigned int offset, const vector<domelement>& args) {
	map<vector<domelement>,unsigned int>::iterator jt = _table[offset].lower_bound(args);
	if(jt != _table[offset].end() && jt->first == args) {
		return jt->second;
	}
	else {
		unsigned int nr = nextNumber();
		_table[offset].insert(jt,pair<vector<domelement>,unsigned int>(args,nr));
		_backfunctable[nr] = _offset2function[offset];
		_backargstable[nr] = args;
		return nr;
	}
}

unsigned int GroundTermTranslator::translate(Function* func, const vector<TypedElement>& args) {
	unsigned int offset = addFunction(func);
	vector<domelement> newargs(args.size());
	for(unsigned int n = 0; n < args.size(); ++n) {
		newargs[n] = CPPointer(args[n]);
	}
	return translate(offset,newargs);
}

unsigned int GroundTermTranslator::nextNumber() {
	unsigned int nr = _backfunctable.size(); 
	_backfunctable.push_back(0);
	_backargstable.push_back(vector<domelement>(0));
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
		_table.push_back(map<vector<domelement>,unsigned int>());
		return offset;	
	}
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
			s << ElementUtil::ElementToString((args(nr))[c]);
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
	for(unsigned int n = 0; n < _grounders.size(); ++n) {
		bool b = _grounders[n]->run();
		if(!b) return b;
	}
	return true;
}

bool SentenceGrounder::run() const {
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
	return true;
}

#ifndef NDEBUG
void FormulaGrounder::setorig(const Formula* f, const map<Variable*,domelement*>& mvd) {
	map<Variable*,Variable*> mvv;
	for(unsigned int n = 0; n < f->nrFvars(); ++n) {
		Variable* v = new Variable(f->fvar(n)->name(),f->fvar(n)->sort(),ParseInfo());
		mvv[f->fvar(n)] = v;
		_varmap[v] = mvd.find(f->fvar(n))->second;
	}
	_origform = f->clone(mvv);
}

void FormulaGrounder::printorig() const {
	cerr << "Grounding formula " << _origform->to_string() << " with instance ";
	for(unsigned int n = 0; n < _origform->nrFvars(); ++n) {
		cerr << _origform->fvar(n)->to_string() << " = ";
		Element e; e._compound = *(_varmap.find(_origform->fvar(n))->second);
		cerr << ElementUtil::ElementToString(e,ELCOMPOUND) << ' ';
	}
	cerr << endl;
}
#endif

AtomGrounder::AtomGrounder(GroundTranslator* gt, bool sign, PFSymbol* s,
							const vector<TermGrounder*> sg, InstanceChecker* pic, InstanceChecker* cic,
							const vector<SortTable*>& vst, const GroundingContext& ct) :
	FormulaGrounder(gt,ct), _subtermgrounders(sg), _pchecker(pic), _cchecker(cic), _symbol(gt->addSymbol(s)), _args(sg.size()), _tables(vst), _sign(sign)
	{ _certainvalue = ct._truegen ? _true : _false; }

int AtomGrounder::run() const {
	// Run subterm grounders
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) 
		_args[n] = _subtermgrounders[n]->run();
	
	// Checking partial functions
	for(unsigned int n = 0; n < _args.size(); ++n) {
		//TODO: only check positions that can be out of bounds!
		if(!ElementUtil::exists(_args[n])) {
			//TODO: produce a warning!
			if(_context._funccontext == PC_BOTH) {
				// TODO: produce an error
			}
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Partial function went out of bounds\n";
				cerr << "Result is " << (_context._funccontext != PC_NEGATIVE  ? "true" : "false") << endl;
			}
#endif
			return _context._funccontext != PC_NEGATIVE  ? _true : _false;
		}
	}

	// Checking out-of-bounds
	for(unsigned int n = 0; n < _args.size(); ++n) {
		if(!_tables[n]->contains(_args[n])) {
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Term value out of predicate type\n";
				cerr << "Result is " << (_sign  ? "false" : "true") << endl;
			}
#endif
			return _sign ? _false : _true;
		}
	}

	// Run instance checkers and return grounding
	if(!(_pchecker->run(_args))) {
#ifndef NDEBUG
		if(_cloptions._verbose) {
			printorig();
			cerr << "Possible checker failed\n";
			cerr << "Result is " << (_certainvalue ? "false" : "true") << endl;
		}
#endif
		return _certainvalue ? _false : _true;	// TODO: dit is lelijk
	}
	if(_cchecker->run(_args)) {
#ifndef NDEBUG
		if(_cloptions._verbose) {
			printorig();
			cerr << "Certain checker succeeded\n";
			cerr << "Result is " << _translator->printAtom(_certainvalue) << endl;
		}
#endif
		return _certainvalue;
	}
	else {
		int atom = _translator->translate(_symbol,_args);
		if(!_sign) atom = -atom;
#ifndef NDEBUG
		if(_cloptions._verbose) {
			printorig();
			cerr << "Result is " << _translator->printAtom(atom) << endl;
		}
#endif
		return atom;
	}
}

void AtomGrounder::run(vector<int>& clause) const {
	clause.push_back(run());
#ifndef NDEBUG
if(_cloptions._verbose) {
	printorig();
	cerr << "Result = " << _translator->printAtom(clause.back()) << endl;
}
#endif
}

CompType invertcpcomp(CompType ct) {
	switch(ct) {
		case CT_EQ: return CT_EQ;
		case CT_NEQ: return CT_NEQ;
		case CT_LEQ: return CT_GEQ;
		case CT_GEQ: return CT_LEQ;
		case CT_LT: return CT_GT;
		case CT_GT: return CT_LT;
		default: assert(false);
	}
}

int CPGrounder::run() const {
	domelement left = _lefttermgrounder->run();
	domelement right = _righttermgrounder->run();
	if(!ElementUtil::exists(left) || !ElementUtil::exists(right)) {
		return _context._funccontext != PC_NEGATIVE  ? _true : _false;
	}
	// TODO??? out-of-bounds check. Can out-of-bounds ever occur on </2, >/2, =/2???
	
	if(left->_function) {
		unsigned int leftvarid = _termtranslator->translate(left->_function,left->_args);	// TODO: try to use the faster translate function 
		CPTerm* leftterm = new CPVarTerm(leftvarid);
		if(right->_function) {
			unsigned int rightvarid = _termtranslator->translate(right->_function,right->_args);	// TODO: try to use the faster translate function 
			CPBound rightbound(true,rightvarid);
			return _translator->translate(leftterm,_comparator,rightbound,_context._tseitin);
		}	
		else {
			assert(right->_args[0]._type == ELINT);
			int rightvalue = right->_args[0]._element._int;
			CPBound rightbound(false,rightvalue);
			return _translator->translate(leftterm,_comparator,rightbound,_context._tseitin);
		}
	}
	else {
		assert(left->_args[0]._type == ELINT);
		int leftvalue = left->_args[0]._element._int;
		if(right->_function) {
			unsigned int rightvarid = _termtranslator->translate(right->_function,right->_args);	// TODO: try to use the faster translate function 
			CPTerm* rightterm = new CPVarTerm(rightvarid);
			CPBound leftbound(false,leftvalue);
			return _translator->translate(rightterm,invertcpcomp(_comparator),leftbound,_context._tseitin);
		}	
		else {
			assert(right->_args[0]._type == ELINT);
			int rightvalue = right->_args[0]._element._int;
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
}

void CPGrounder::run(vector<int>& clause) const {
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
			int tseitin = _translator->translate(double(leftvalue),_comp,true,AGGCARD,setnr,tp);
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
	}
	if(_doublenegtseitin)
		return handleDoubleNegation(newboundvalue,setnr);
	else {
		int tseitin;
		TsType tp = _context._tseitin;
		if((_type == AGGMIN) && (_comp == '=')) // Only use lower bound
			tseitin = _translator->translate(newboundvalue,'<',false,AGGMIN,setnr,tp);
		else if((_type == AGGMAX) && (_comp == '=')) // Only use upper bound
			tseitin = _translator->translate(newboundvalue,'>',false,AGGMAX,setnr,tp);
		else
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
	domelement bound = _boundgrounder->run();

	// Retrieve the set, note that weights might be changed when handling min and max aggregates.
	TsSet& tsset = _translator->groundset(setnr);

	// Retrieve the value of the bound
	double boundvalue = ElementUtil::convert(bound->_args[0],ELDOUBLE)._double;

	// Compute the value of the aggregate based on weights of literals that are certainly true.	
	double truevalue = AggUtils::compute(_type,tsset.trueweights());

	// When the set is empty, return an answer based on the current value of the aggregate.
	if(tsset.literals().empty()) {
		bool returnvalue;
		switch(_comp) {
			case '<' : returnvalue = boundvalue < truevalue; break;
			case '>' : returnvalue = boundvalue > truevalue; break;
			case '=' : returnvalue = boundvalue == truevalue; break;
			default: assert(false);
		}
		return _sign == returnvalue ? _true : _false;
	}

	// Handle specific aggregates.
	int tseitin;
	double minpossvalue = truevalue;
	double maxpossvalue = truevalue;
	switch(_type) {
		case AGGCARD: { 
			tseitin = finishCard(truevalue,boundvalue,setnr);
			break;
		}
		case AGGSUM: {
			// Compute the minimum and maximum possible value of the sum.
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				if(tsset.weight(n) > 0) maxpossvalue += tsset.weight(n);
				else if(tsset.weight(n) < 0) minpossvalue += tsset.weight(n);
			}
			// Finish
			tseitin = finish(boundvalue,(boundvalue-truevalue),minpossvalue,maxpossvalue,setnr);
			break;
		}
		case AGGPROD: {
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
		case AGGMIN: {
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
		case AGGMAX: {
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
	if(cl.empty()) {
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Result = " << _translator->printAtom(result2()) << endl;
			}
#endif
		return result2();
	}
	else if(cl.size() == 1) {
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Result = " << (_sign ? _translator->printAtom(cl[0]) : _translator->printAtom(-cl[0])) << endl;
			}
#endif
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
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Result = " << (_sign ? "" : "~");
				cerr << _translator->printAtom(cl[0]) << ' ';
				for(unsigned int n = 1; n < cl.size(); ++n) cerr << (!_conj ? "& " : "| ") << _translator->printAtom(cl[n]) << ' ';
			}
#endif
			return _sign ? -ts : ts;
		}
		else {
			int ts = _translator->translate(cl,_conj,tp);
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Result = " << (_sign ? "" : "~");
				cerr << _translator->printAtom(cl[0]) << ' ';
				for(unsigned int n = 1; n < cl.size(); ++n) cerr << (_conj ? "& " : "| ") << _translator->printAtom(cl[n]) << ' ';
			}
#endif
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
	vector<int> cl;
	if(_generator->first()) {
		int l = _subgrounder->run();
		if(check1(l)) {
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Result = " << _translator->printAtom(result1()) << endl;
			}
#endif
			return result1();
		}
		else if(! check2(l)) cl.push_back(l);
		while(_generator->next()) {
			l = _subgrounder->run();
			if(check1(l)) {
#ifndef NDEBUG
				if(_cloptions._verbose) {
					printorig();
					cerr << "Result = " << _translator->printAtom(result1()) << endl;
				}
#endif
				return result1();
			}
			else if(! check2(l)) cl.push_back(l);
		}
	}
	return finish(cl);
}

void QuantGrounder::run(vector<int>& clause) const {
	if(_generator->first()) {
		int l = _subgrounder->run();
		if(check1(l)) {
			clause.clear();
			clause.push_back(result1());
#ifndef NDEBUG
			if(_cloptions._verbose) {
				printorig();
				cerr << "Result = " << _translator->printAtom(result1()) << endl;
			}
#endif
			return;
		}
		else if(! check2(l)) clause.push_back(_sign ? l : -l);
		while(_generator->next()) {
			l = _subgrounder->run();
			if(check1(l)) {
				clause.clear();
				clause.push_back(result1());
#ifndef NDEBUG
				if(_cloptions._verbose) {
					printorig();
					cerr << "Result = " << _translator->printAtom(result1()) << endl;
				}
#endif
				return;
			}
			else if(! check2(l)) clause.push_back(_sign ? l : -l);
		}
	}
#ifndef NDEBUG
	if(_cloptions._verbose) {
		printorig();
		cerr << "Result = " << (_sign ? "" : "~");
		if(clause.empty()) cerr << (_conj ? "true" : "false") << endl;
		else {
			cerr << _translator->printAtom(clause[0]) << ' ';
			for(unsigned int n = 1; n < clause.size(); ++n) cerr << (_conj ? "& " : "| ") << _translator->printAtom(clause[n]) << ' ';
		}
	}
#endif
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

#ifndef NDEBUG
void TermGrounder::setorig(const Term* t, const map<Variable*,domelement*>& mvd) {
	map<Variable*,Variable*> mvv;
	for(unsigned int n = 0; n < t->nrFvars(); ++n) {
		Variable* v = new Variable(t->fvar(n)->name(),t->fvar(n)->sort(),ParseInfo());
		mvv[t->fvar(n)] = v;
		_varmap[v] = mvd.find(t->fvar(n))->second;
	}
	_origterm = t->clone(mvv);
}

void TermGrounder::printorig() const {
	cerr << "Grounding term " << _origterm->to_string() << " with instance ";
	for(unsigned int n = 0; n < _origterm->nrFvars(); ++n) {
		cerr << _origterm->fvar(n)->to_string() << " = ";
		Element e; e._compound = *(_varmap.find(_origterm->fvar(n))->second);
		cerr << ElementUtil::ElementToString(e,ELCOMPOUND) << ' ';
	}
	cerr << endl;
}
#endif 

domelement VarTermGrounder::run() const {
	return *_value;
}

domelement FuncTermGrounder::run() const {
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		_args[n] = _subtermgrounders[n]->run();
	}
	domelement result = (*_function)[_args];
#ifndef NDEBUG
	if(_cloptions._verbose) {
		printorig();
		Element e; e._compound = result;
		cerr << "Result is " << ElementUtil::ElementToString(e,ELCOMPOUND) << endl;
	}
#endif
	return result;
}

domelement ThreeValuedFuncTermGrounder::run() const {
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		_args[n] = _subtermgrounders[n]->run();
		if(!ElementUtil::exists(_args[n]) || !_tables[n]->contains(_args[n])) return 0;
	}
	domelement result = (*_functable)[_args];
	if(ElementUtil::exists(result)) {
#ifndef NDEBUG
		if(_cloptions._verbose) {
			printorig();
			Element e; e._compound = result;
			cerr << "Result is " << ElementUtil::ElementToString(e,ELCOMPOUND) << endl;
		}
#endif
		return result;
	}
	else {
#ifndef NDEBUG
		if(_cloptions._verbose) {
			printorig();
			Element e; e._compound = CPPointer(_function,_args);
			cerr << "Result is " << ElementUtil::ElementToString(e,ELCOMPOUND) << endl;
		}
#endif
		return CPPointer(_function,_args);
	}
}

domelement AggTermGrounder::run() const {
	int setnr = _setgrounder->run();
	const TsSet& tsset = _translator->groundset(setnr);
	assert(tsset.empty());
	double value = AggUtils::compute(_type,tsset.trueweights());
	Element e;
	if(isInt(value)) {
		e._int = int(value);
#ifndef NDEBUG
		if(_cloptions._verbose) {
			printorig();
			cerr << "Result is " << ElementUtil::ElementToString(e,ELINT) << endl;
		}
#endif
		return CPPointer(e,ELINT);
	}
	else {
		e._double = value;
#ifndef NDEBUG
		if(_cloptions._verbose) {
			printorig();
			cerr << "Result is " << ElementUtil::ElementToString(e,ELDOUBLE) << endl;
		}
#endif
		return CPPointer(e,ELDOUBLE);
	}
}

domelement ThreeValuedAggTermGrounder::run() const {
	//TODO
	assert(false);
}

int EnumSetGrounder::run() const {
	vector<int>	literals;
	vector<double> weights;
	vector<double> trueweights;
	for(unsigned int n = 0; n < _subgrounders.size(); ++n) {
		int l = _subgrounders[n]->run();
		if(l != _false) {
			domelement d = _subtermgrounders[n]->run();
			Element e; e._compound = d;
			Element w = ElementUtil::convert(e,ELCOMPOUND,ELDOUBLE);
			if(l == _true) trueweights.push_back(w._double);
			else {
				weights.push_back(w._double);
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
			Element e; e._compound = *_weight;
			Element w = ElementUtil::convert(e,ELCOMPOUND,ELDOUBLE);
			if(l == _true) trueweights.push_back(w._double);
			else {
				weights.push_back(w._double);
				literals.push_back(l);
			}
		}
		while(_generator->next()) {
			l = _subgrounder->run();
			if(l != _false) {
				Element e; e._compound = *_weight;
				Element w = ElementUtil::convert(e,ELCOMPOUND,ELDOUBLE);
				if(l == _true) trueweights.push_back(w._double);
				else {
					weights.push_back(w._double);
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
		if(!ElementUtil::exists(_args[n])) return _false;
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
			vector<Function*> nonbuiltins = function->nonbuiltins();
			for(vector<Function*>::const_iterator nbfit = nonbuiltins.begin(); nbfit != nonbuiltins.end(); ++nbfit) {
				groundtocp = (SortUtils::resolve(function->outsort(),intsort,vocabulary) == intsort);
			}
		} else if(!function->builtin()) {
			groundtocp = (SortUtils::resolve(function->outsort(),intsort,vocabulary) == intsort);
		}
		if(groundtocp) _cpfunctions.insert(function);
	}
	return _cpfunctions;
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
	if(_cpsupport) findCPFunctions(theory);

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
	if(_cpsupport) findCPFunctions(theory);

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
	_toplevelgrounder = new CopyGrounder(_grounding,ecnf);	
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
	vector<TheoryComponent*> components(theory->nrComponents());
	for(unsigned int n = 0; n < theory->nrComponents(); ++n) {
		components[n] = theory->component(n);
	}

	// Order components the components to optimize the grounding process
	// TODO

	// Create grounders for all components
	vector<TopLevelGrounder*> children(components.size());
	for(unsigned int n = 0; n < components.size(); ++n) {
		InitContext();
		components[n]->accept(this);
		children[n] = _toplevelgrounder; 
	}

	// Create the grounder
	_toplevelgrounder = new TheoryGrounder(_grounding,children);
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
	Formula* transpf = FormulaUtils::moveThreeValTerms(newpf,_structure,(_context._funccontext != PC_NEGATIVE),_cpsupport,_cpfunctions);

	if(newpf != transpf) {	// The rewriting changed the atom
		//delete(newpf); TODO: produces a segfault??
		transpf->accept(this);
	}
	else {	// The rewriting did not change the atom
		// Create grounders for the subterms
		bool cpsubterms = false;
		vector<TermGrounder*> subtermgrounders;
		vector<SortTable*>	  argsorttables;
		for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
			descend(pf->subterm(n));
			subtermgrounders.push_back(_termgrounder);
			cpsubterms = cpsubterms || _termgrounder->canReturnCPVar();
			argsorttables.push_back(_structure->inter(pf->symb()->sort(n)));
		}
		// Create checkers and grounder
		if(cpsubterms) {
			if(_context._component == CC_HEAD) {
				assert(false);
				//TODO
			}
			else {
				if(pf->symb()->ispred()) {
					CompType comp;
					if(pf->symb()->name() == "=/2") {
						comp = pf->sign() ? CT_EQ : CT_NEQ;
					}
					else if(pf->symb()->name() == "</2") {
						comp = pf->sign() ? CT_LT : CT_GEQ;
					}
					else if(pf->symb()->name() == ">/2") {
						comp = pf->sign() ? CT_GT : CT_LEQ;
					}
					else assert(false);
					_formgrounder = new CPGrounder(_grounding->translator(),_grounding->termtranslator(),
											subtermgrounders[0],comp,subtermgrounders[1],_context);
				}
				else {
					assert(false);
					//CompType comp = pf->sign() ? CT_EQ : CT_NEQ;
					//TermGrounder* righttermgrounder = subtermgrounders.back();
					//subtermgrounders.pop_back();
					//TermGrounder* lefttermgrounder;
					//TODO construct lefttermgrounder
				}
				if(_context._component == CC_SENTENCE) 
					_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false);
			}
		}
		else {
			PredInter* inter = _structure->inter(pf->symb());
			CheckerFactory checkfactory;
			if(_context._component == CC_HEAD) {
				InstanceChecker* truech = checkfactory.create(inter,true,true);
				InstanceChecker* falsech = checkfactory.create(inter,false,true);
				_headgrounder = new HeadGrounder(_grounding,truech,falsech,pf->symb(),subtermgrounders,argsorttables);
			}
			else {
				InstanceChecker* possch;
				InstanceChecker* certainch;
				if(_context._truegen == pf->sign()) {	
					possch = checkfactory.create(inter,false,false);
					certainch = checkfactory.create(inter,true,true);
				}
				else {	
					possch = checkfactory.create(inter,true,false);
					certainch = checkfactory.create(inter,false,true);
				}
		
				// Create the grounder
				_formgrounder = new AtomGrounder(_grounding->translator(),pf->sign(),pf->symb(),
									 subtermgrounders,possch,certainch,argsorttables,_context);
#ifndef NDEBUG
				_formgrounder->setorig(pf,_varmapping);
#endif
				if(_context._component == CC_SENTENCE) 
					_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false);
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
			for(unsigned int n = 0; n < newbf->nrSubforms(); ++n) newbf->subform(n)->swapsign();
		}

		// Visit the subformulas
		vector<TopLevelGrounder*> sub(newbf->nrSubforms());
		for(unsigned int n = 0; n < newbf->nrSubforms(); ++n) {
			descend(newbf->subform(n));
			sub[n] = _toplevelgrounder;
		}
		newbf->recursiveDelete();
		_toplevelgrounder = new TheoryGrounder(_grounding,sub);
	}
	else {	// Formula bf is not a top-level conjunction

		// Create grounders for subformulas
		SaveContext();
		DeeperContext(bf->sign());
		vector<FormulaGrounder*> sub(bf->nrSubforms());
		for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
			descend(bf->subform(n));
			sub[n] = _formgrounder;
		}
		RestoreContext();

		// Create grounder
		SaveContext();
		if(recursive(bf)) _context._tseitin = TS_RULE;
		_formgrounder = new BoolGrounder(_grounding->translator(),sub,bf->sign(),bf->conj(),_context);
		RestoreContext();
#ifndef NDEBUG
		_formgrounder->setorig(bf,_varmapping);
#endif	
		if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false);

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
	vector<domelement*> vars;
	vector<SortTable*>	tables;
	for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
		domelement* d = new domelement();
		_varmapping[qf->qvar(n)] = d;
		vars.push_back(d);
		SortTable* st = _structure->inter(qf->qvar(n)->sort());
		assert(st->finite());	// TODO: produce an error message
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
		_toplevelgrounder = new UnivSentGrounder(_grounding,_toplevelgrounder,gen);
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
#ifndef NDEBUG
		_formgrounder->setorig(qf,_varmapping);
#endif	
		if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false);
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
	if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true);
}

/**
 * void GrounderFactory::visit(const AggForm* af)
 * DESCRIPTION
 * 		Creates a grounder for an aggregate.
 */
void GrounderFactory::visit(const AggForm* af) {
	AggForm* newaf = af->clone();
	Formula* transaf = FormulaUtils::moveThreeValTerms(newaf,_structure,(_context._funccontext != PC_NEGATIVE),_cpsupport,_cpfunctions);

	if(newaf != transaf) {	// The rewriting changed the atom
		//delete(newaf); TODO: produces a segfault??
		transaf->accept(this);
	}
	else {	// The rewriting did not change the atom
		// Create grounder for the bound
		descend(af->left());
		TermGrounder* boundgr = _termgrounder;
	
		// Create grounder for the set
		SaveContext();
		if(recursive(af)) assert(FormulaUtils::monotone(af) || FormulaUtils::antimonotone(af));
		DeeperContext(!FormulaUtils::antimonotone(af));
		descend(af->right()->set());
		SetGrounder* setgr = _setgrounder;
		RestoreContext();
	
		// Create aggregate grounder
		SaveContext();
		if(recursive(af)) _context._tseitin = TS_RULE;
		_formgrounder = new AggGrounder(_grounding->translator(),_context,af->right()->type(),setgr,boundgr,af->comp(),af->sign());
		RestoreContext();
		if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true);
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
#ifndef NDEBUG
	_termgrounder->setorig(t,_varmapping);
#endif
}

/**
 * void GrounderFactory::visit(const DomainTerm* t)
 * DESCRIPTION
 * 		Creates a grounder for a domain term.
 */
void GrounderFactory::visit(const DomainTerm* t) {
	_termgrounder = new DomTermGrounder(CPPointer(t->value(),t->type()));
#ifndef NDEBUG
	_termgrounder->setorig(t,_varmapping);
#endif
}

/**
 * void GrounderFactory::visit(const FuncTerm* t)
 * DESCRIPTION
 * 		Creates a grounder for a function term.
 */
void GrounderFactory::visit(const FuncTerm* t) {
	// Create grounders for subterms
	vector<TermGrounder*> sub;
	for(unsigned int n = 0; n < t->nrSubterms(); ++n) {
		t->subterm(n)->accept(this);
		if(_termgrounder) sub.push_back(_termgrounder);
	}

	// Create term grounder
	Function* func = t->func();
	if(_structure->inter(func)->fasttwovalued() || not _cpsupport) {
		FuncTable* ft = _structure->inter(func)->functable();
		_termgrounder = new FuncTermGrounder(sub,ft);
	}
	else {
		vector<SortTable*> vst;
		for(unsigned int n = 0; n < func->arity(); ++n) {
			vst.push_back(_structure->inter(func->sort(n)));
		}
		FuncTable* ft; 
		PredInter* pinter = _structure->inter(func)->predinter();
		if(pinter->ct() && typeid(*(pinter->ctpf())) == typeid(FinitePredTable)) {
			ft = new FiniteFuncTable(dynamic_cast<FinitePredTable*>(pinter->ctpf()));
		}
		else {
			vector<ElementType> vet(func->nrSorts(),ELINT);
			ft = new FiniteFuncTable(new FinitePredTable(vet));
		}
		_termgrounder = new ThreeValuedFuncTermGrounder(sub,func,ft,vst);
	}
#ifndef NDEBUG
	_termgrounder->setorig(t,_varmapping);
#endif
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
	if(SetUtils::isTwoValued(t->set(),_structure) || not _cpsupport)
		_termgrounder = new AggTermGrounder(_grounding->translator(),t->type(),_setgrounder);
	else //TODO
		_termgrounder = new ThreeValuedAggTermGrounder(_grounding->translator(),t->type(),_setgrounder);
#ifndef NDEBUG
	_termgrounder->setorig(t,_varmapping);
#endif
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
	for(unsigned int n = 0; n < s->nrSubforms(); ++n) {
		descend(s->subform(n));
		subgr.push_back(_formgrounder);
		descend(s->subterm(n));
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
	for(unsigned int n = 0; n < s->nrQvars(); ++n) {
		domelement* d = new domelement();
		_varmapping[s->qvar(n)] = d;
		SortTable* st = _structure->inter(s->qvar(n)->sort());
		assert(st->finite());	// TODO: produce an error message
		SortInstGenerator* tig = new SortInstGenerator(st,d);
		if(s->nrQvars() == 1) {
			gen = tig;
			break;
		}
		else if(n == 0)	node = new LeafGeneratorNode(tig);
		else node = new OneChildGeneratorNode(tig,node);
	}
	if(!gen) gen = new TreeInstGenerator(node);
	
	// Create grounder for subformula
	SaveContext();
	AggContext();
	descend(s->subf());
	FormulaGrounder* sub = _formgrounder;
	RestoreContext();

	// Create grounder	
	_setgrounder = new QuantSetGrounder(_grounding->translator(),sub,gen,_varmapping[s->qvar(0)]);
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
	for(unsigned int m = 0; m < def->nrDefsyms(); ++m) {
		_context._defined.insert(def->defsym(m));
	}
	
	// Create rule grounders
	vector<RuleGrounder*> subgrounders;
	for(unsigned int n = 0; n < def->nrRules(); ++n) {
		descend(def->rule(n));
		subgrounders.push_back(_rulegrounder);
	}
	
	// Create definition grounder
	_toplevelgrounder = new DefinitionGrounder(_grounding,_definition,subgrounders);

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
	for(unsigned int n = 0; n < rule->nrQvars(); ++n) {
		if(rule->body()->contains(rule->qvar(n))) {
			bodyvars.push_back(rule->qvar(n));
		}
		else {
			headvars.push_back(rule->qvar(n));
		}
	}

	// Create head instance generator
	InstGenerator* headgen = 0;
	GeneratorNode* hnode = 0;
	for(unsigned int n = 0; n < headvars.size(); ++n) {
		domelement* d = new domelement();
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
		domelement* d = new domelement();
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
