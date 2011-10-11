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
#include "monitors/interactiveprintmonitor.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "groundtheories/GroundPolicy.hpp"
#include "groundtheories/PrintGroundPolicy.hpp"
#include "fobdd.hpp"
#include "symbolicstructure.hpp"

using namespace std;
using namespace rel_ops;

/** The two built-in literals 'true' and 'false' **/
int _true = numeric_limits<int>::max();
int _false = 0;

double MCPA = 1;	// TODO: constant currently used when pruning bdds. Should be made context dependent

/****************************************
	Comparison operators for TsBodies
****************************************/

bool operator==(const TsBody& a, const TsBody& b) {
	if(typeid(a) != typeid(b)) { return false; }
	else { return a.equal(b); }
}

bool operator<(const TsBody& a, const TsBody& b) {
	// Order of TsBodies of different types is defined by lexical order of there typeids.
	if(typeid(a) != typeid(b)) { return (typeid(a).name() < typeid(b).name()); }
	else { return a.compare(b); }
}	

bool TsBody::equal(const TsBody& other) const {
	return _type == other.type();
}

bool TsBody::compare(const TsBody& other) const {
	return _type < other.type();
}

bool CPTsBody::equal(const TsBody& other) const {
	if(not TsBody::equal(other)) { return false; }
	const CPTsBody& othercpt = static_cast<const CPTsBody&>(other);
	return (*_left == *othercpt.left()) && (_comp == othercpt.comp()) && (_right == othercpt.right());
}

bool CPTsBody::compare(const TsBody& other) const {
	if(TsBody::compare(other)) { return true; }
	else if(TsBody::equal(other)) {
		const CPTsBody& othercpt = static_cast<const CPTsBody&>(other);
		if(*_left < *othercpt.left()) { 
			return true;
		} else if(*_left == *othercpt.left()) {
			if(_comp < othercpt.comp()) {
				return true;
			} else if(_comp == othercpt.comp()) {
				if(_right < othercpt.right()) {
					return true;
				}
			}
		}
	}
	return false;
}

/** Comparing CP terms and bounds **/

bool operator==(const CPTerm& a, const CPTerm& b) {
	if(typeid(a) != typeid(b)) { return false; }
	else { return a.equal(b); }
}

bool operator<(const CPTerm& a, const CPTerm& b) {
	if(typeid(a) != typeid(b)) { return (typeid(a).name() < typeid(b).name()); }
	else { return a.compare(b); }
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
		return (a._isvarid ? (a._varid == b._varid) : (a._bound == b._bound));
	}
	return false;
}

bool operator<(const CPBound& a, const CPBound& b) {
	if(a._isvarid == b._isvarid) {
		return (a._isvarid ? (a._varid < b._varid) : (a._bound < b._bound));
	}
	// CPBounds with a number value come before CPBounds with a CP variable identifier.
	return (a._isvarid < b._isvarid);
}


/*********************************************
	Translate from ground atoms to numbers
*********************************************/

GroundTranslator::~GroundTranslator() {
	// delete TsBodies
	for(map<int,TsBody*>::iterator mapit = _nr2tsbodies.begin(); mapit != _nr2tsbodies.end(); ++mapit) {
		delete mapit->second;
	}
	_nr2tsbodies.clear();
	_tsbodies2nr.clear(); // All TsBodies in this map should've been deleted through the other map.
}

int GroundTranslator::translate(unsigned int n, const ElementTuple& args) {
	map<ElementTuple,int,StrictWeakTupleOrdering>::iterator jt = _table[n].lower_bound(args);
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
		else { tsbody->_bound = bound; }
		//_tsbodies2nr.insert(it,pair<TsBody*,int>(tsbody,nr));
		_nr2tsbodies.insert(pair<int,TsBody*>(nr,tsbody));
		return nr;
	}
}

int GroundTranslator::translate(CPTerm* left, CompType comp, const CPBound& right, TsType tstype) {
	CPTsBody* tsbody = new CPTsBody(tstype,left,comp,right);
	map<TsBody*,int,StrictWeakTsBodyOrdering>::iterator it = _tsbodies2nr.lower_bound(tsbody);
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
	for(unsigned int n = 0; n < _symboffsets.size(); ++n){
		if(_symboffsets[n] == pfs){
			return n;
		}
	}
	_symboffsets.push_back(pfs);
	_table.push_back(map<ElementTuple,int,StrictWeakTupleOrdering>());
	return _symboffsets.size()-1;
}

string GroundTranslator::printAtom(int nr) const {
	stringstream s;
	nr = abs(nr);
	if(nr == _true) { return "true"; }
	if(nr == _false) { return "false"; }
	if(nr >= int(_backsymbtable.size())) { return "error"; }
	PFSymbol* pfs = atom2symbol(nr);
	if(pfs) {
		s << pfs->toString();
		if(not args(nr).empty()) {
			s << "(";
			for(size_t c = 0; c < args(nr).size(); ++c) {
				s << args(nr)[c]->toString();
				if(c !=  args(nr).size()-1) { s << ","; }
			}
			s << ")";
		}
	}
	else { s << "tseitin_" << nr; }
	return s.str();
}


/*********************************************
	Translate from ground terms to numbers
*********************************************/

bool operator==(const GroundTerm& a, const GroundTerm& b) {
	if(a._isvarid == b._isvarid) {
		return a._isvarid ? (a._varid == b._varid) : (a._domelement == b._domelement);
	}
	return false;
}

bool operator<(const GroundTerm& a, const GroundTerm& b) {
	if(a._isvarid == b._isvarid) {
		return a._isvarid ? (a._varid < b._varid) : (a._domelement < b._domelement);
	}
	// GroundTerms with a domain element come before GroundTerms with a CP variable identifier.
	return (a._isvarid < b._isvarid);
}

VarId GroundTermTranslator::translate(size_t offset, const vector<GroundTerm>& args) {
	map<vector<GroundTerm>,VarId>::iterator it = _functerm2varid_table[offset].lower_bound(args);
	if(it != _functerm2varid_table[offset].end() && it->first == args) {
		return it->second;
	}
	else {
		VarId varid = nextNumber();
		_functerm2varid_table[offset].insert(it,pair<vector<GroundTerm>,VarId>(args,varid));
		_varid2function[varid] = _offset2function[offset];
		_varid2args[varid] = args;
		_varid2domain[varid] = _structure->inter(_offset2function[offset]->outsort());
		return varid;
	}
}

VarId GroundTermTranslator::translate(Function* function, const vector<GroundTerm>& args) {
	size_t offset = addFunction(function);
	return translate(offset,args);
}

VarId GroundTermTranslator::translate(CPTerm* cpterm, SortTable* domain) {
	VarId varid = nextNumber();
	CPBound bound(varid);
	CPTsBody* cprelation = new CPTsBody(TS_EQ,cpterm,CT_EQ,bound);
	_varid2cprelation.insert(pair<VarId,CPTsBody*>(varid,cprelation));
	_varid2domain[varid] = domain;
	return varid;
}

VarId GroundTermTranslator::translate(const DomainElement* element) {
	VarId varid = nextNumber();
	// Create a new CP variable term
	CPVarTerm* cpterm = new CPVarTerm(varid);
	// Create a new CP bound based on the domain element
	assert(element->type() == DET_INT);
	CPBound bound(element->value()._int);
	// Add a new CP constraint
	CPTsBody* cprelation = new CPTsBody(TS_EQ,cpterm,CT_EQ,bound);
	_varid2cprelation.insert(pair<VarId,CPTsBody*>(varid,cprelation));
	// Add a new domain containing only the given domain element
	SortTable* domain = new SortTable(new EnumeratedInternalSortTable());
	domain->add(element);
	_varid2domain[varid] = domain;
	// Return the new variable identifier
	return varid;
}

size_t GroundTermTranslator::nextNumber() {
	size_t nr = _varid2function.size(); 
	_varid2function.push_back(0);
	_varid2args.push_back(vector<GroundTerm>(0));
	_varid2domain.push_back(0);
	return nr;
}

size_t GroundTermTranslator::addFunction(Function* func) {
	map<Function*,size_t>::const_iterator found = _function2offset.find(func);
	if(found != _function2offset.end()) {
		// Simply return number when function is already known
		return found->second;
	}
	else {
		// Add function and number when function is unknown
		size_t offset = _offset2function.size();
		_function2offset[func] = offset; 
		_offset2function.push_back(func);
		_functerm2varid_table.push_back(map<vector<GroundTerm>,VarId>());
		return offset;	
	}
}

string GroundTermTranslator::printTerm(const VarId& varid) const {
	stringstream s;
	if(varid >= _varid2function.size()) { return "error"; }
	const Function* func = function(varid);
	if(func) {
		s << func->toString();
		if(not args(varid).empty()) {
			s << "(";
			for(vector<GroundTerm>::const_iterator gtit = args(varid).begin(); gtit != args(varid).end(); ++gtit) {
				if((*gtit)._isvarid) {
					s << printTerm((*gtit)._varid);
				} else {
					s << (*gtit)._domelement->toString();
				}
				if(gtit != args(varid).end()-1) { s << ","; }
			}
			s << ")";
		}
	} else { s << "var_" << varid; }
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
	_grounding->startTheory();
	if(_verbosity > 0) {
		clog << "Grounding theory " << "\n";
		clog << "Components to ground = " << _grounders.size() << "\n";
	}
	for(unsigned int n = 0; n < _grounders.size(); ++n) {
		bool b = _grounders[n]->run();
		if(not b) {
			return b;
		}
	}
	_grounding->closeTheory();
	return true;
}

bool SentenceGrounder::run() const {
	if(_verbosity > 1) { clog << "Grounding sentence " << "\n"; }
	vector<int> cl;
	_subgrounder->run(cl);
	if(cl.empty()) {
		return (_conj ? true : false);
	}
	else if(cl.size() == 1) {
		if(cl[0] == _false) {
			_grounding->addEmptyClause();
			return false;
		}
		else if(cl[0] != _true) {
			_grounding->add(cl);
			return true;
		}
		else { return true; }
	}
	else {
		if(_conj) {
			for(size_t n = 0; n < cl.size(); ++n) {
				_grounding->addUnitClause(cl[n]);
			}
		}
		else {
			_grounding->add(cl);
		}
		return true;
	}
}

bool UnivSentGrounder::run() const {
	if(_verbosity > 1) { clog << "Grounding a universally quantified sentence " << "\n"; }
	if(_generator->first()) {
		bool b = _subgrounder->run();
		if(not b) {
			_grounding->addEmptyClause();
			return b;
		}
		while(_generator->next()) {
			b = _subgrounder->run();
			if(not b) {
				_grounding->addEmptyClause();
				return b;
			}
		}
	}
	else if(_verbosity > 1) {
		clog << "No instances for this sentence " << "\n";
	}
	return true;
}

void FormulaGrounder::setOrig(const Formula* f, const map<Variable*, const DomainElement**>& mvd, int verb) {
	_verbosity = verb;
	map<Variable*,Variable*> mvv;
	for(set<Variable*>::const_iterator it = f->freeVars().begin(); it != f->freeVars().end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),ParseInfo());
		mvv[*it] = v;
		_varmap[v] = mvd.find(*it)->second;
	}
	_origform = f->clone(mvv);
}

void FormulaGrounder::printOrig() const {
	clog << "Grounding formula " << _origform->toString();
	if(not _origform->freeVars().empty()) {
		clog << " with instance ";
		for(set<Variable*>::const_iterator it = _origform->freeVars().begin(); it != _origform->freeVars().end(); ++it) {
			clog << (*it)->toString() << " = ";
			const DomainElement* e = *(_varmap.find(*it)->second);
			clog << e->toString() << ' ';
		}
	}
	clog << "\n";
}

AtomGrounder::AtomGrounder(GroundTranslator* gt, bool sign, PFSymbol* s,
							const vector<TermGrounder*> sg, const vector<const DomainElement**>& checkargs,
							InstGenerator* pic, InstGenerator* cic, PredInter* inter,
							const vector<SortTable*>& vst, const GroundingContext& ct)
	: FormulaGrounder(gt,ct), _subtermgrounders(sg), _pchecker(pic), _cchecker(cic), _symbol(gt->addSymbol(s)),
	_tables(vst), _sign(sign), _checkargs(checkargs), _inter(inter)
	{ _certainvalue = (ct._truegen ? _true : _false); }

int AtomGrounder::run() const {
	if(_verbosity > 2) { printOrig(); }

	// Run subterm grounders
	bool alldomelts = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for(size_t n = 0; n < _subtermgrounders.size(); ++n) {
		groundsubterms[n] = _subtermgrounders[n]->run();
		if(groundsubterms[n]._isvarid) {
			alldomelts = false;
		} else {
			args[n] = groundsubterms[n]._domelement;
		}
	}

	// Checking partial functions
	for(size_t n = 0; n < args.size(); ++n) {
		//TODO: only check positions that can be out of bounds!
		if(not groundsubterms[n]._isvarid && not args[n]) {
			//TODO: produce a warning!
			if(_context._funccontext == PC_BOTH) {
				// TODO: produce an error
			}
			if(_verbosity > 2) {
				clog << "Partial function went out of bounds\n";
				clog << "Result is " << (_context._funccontext != PC_NEGATIVE  ? "true" : "false") << "\n";
			}
			return (_context._funccontext != PC_NEGATIVE  ? _true : _false);
		}
	}

	// Checking out-of-bounds
	for(size_t n = 0; n < args.size(); ++n) {
		if(not groundsubterms[n]._isvarid && not _tables[n]->contains(args[n])) {
			if(_verbosity > 2) {
				clog << "Term value out of predicate type\n";
				clog << "Result is " << (_sign  ? "false" : "true") << "\n";
			}
			return (_sign ? _false : _true);
		}
	}

	// Run instance checkers
	if(alldomelts) {
		for(size_t n = 0; n < args.size(); ++n) {
			*(_checkargs[n]) = args[n];
		}
		if(not _pchecker->first()) {
			if(_verbosity > 2) {
				clog << "Possible checker failed\n";
				clog << "Result is " << (_certainvalue ? "false" : "true") << "\n";
			}
			return _certainvalue ? _false : _true;	// TODO: dit is lelijk
		}
		if(_cchecker->first()) {
			if(_verbosity > 2) {
				clog << "Certain checker succeeded\n";
				clog << "Result is " << _translator->printAtom(_certainvalue) << "\n";
			}
			return _certainvalue;
		}
		if(_inter->isTrue(args)) { return _sign ? _true : _false; }
		if(_inter->isFalse(args)) { return _sign ? _false : _true; }
	}

	// Return grounding
	if(alldomelts) {
		int atom = _translator->translate(_symbol,args);
		if(not _sign) { atom = -atom; }
		if(_verbosity > 2) { clog << "Result is " << _translator->printAtom(atom) << "\n"; }
		return atom;
	}
	else {
		assert(false);
	}
}

void AtomGrounder::run(vector<int>& clause) const {
	clause.push_back(run());
}

int ComparisonGrounder::run() const {
	const GroundTerm& left = _lefttermgrounder->run();
	const GroundTerm& right = _righttermgrounder->run();

	//XXX Is following check necessary??
	if((not left._domelement && not left._varid) || (not right._domelement && not right._varid)) {
		return _context._funccontext != PC_NEGATIVE  ? _true : _false;
	}

	//TODO??? out-of-bounds check. Can out-of-bounds ever occur on </2, >/2, =/2???
	
	if(left._isvarid) {
		CPTerm* leftterm = new CPVarTerm(left._varid);
		if(right._isvarid) {
			CPBound rightbound(right._varid);
			return _translator->translate(leftterm,_comparator,rightbound,TS_EQ); //TODO use _context._tseitin?
		}	
		else {
			assert(not right._isvarid);
			int rightvalue = right._domelement->value()._int;
			CPBound rightbound(rightvalue);
			return _translator->translate(leftterm,_comparator,rightbound,TS_EQ); //TODO use _context._tseitin?
		}
	}
	else {
		assert(not left._isvarid);
		int leftvalue = left._domelement->value()._int;
		if(right._isvarid) {
			CPTerm* rightterm = new CPVarTerm(right._varid);
			CPBound leftbound(leftvalue);
			return _translator->translate(rightterm,invertComp(_comparator),leftbound,TS_EQ); //TODO use _context._tseitin?
		}	
		else {
			assert(not right._isvarid);
			int rightvalue = right._domelement->value()._int;
			switch(_comparator) {
				case CT_EQ: { return leftvalue == rightvalue ? _true : _false; }
				case CT_NEQ: { return leftvalue != rightvalue ? _true : _false; }
				case CT_LEQ: { return leftvalue <= rightvalue ? _true : _false; }
				case CT_GEQ: { return leftvalue >= rightvalue ? _true : _false; }
				case CT_LT: { return leftvalue < rightvalue ? _true : _false; }
				case CT_GT:	{ return leftvalue > rightvalue ? _true : _false; }
				default: assert(false);
			}
		}
	}
	assert(false);
	return 0;
}

void ComparisonGrounder::run(vector<int>& clause) const {
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
	if(not _sign) {
		if(tp == TS_IMPL) { tp = TS_RIMPL; }
		else if(tp == TS_RIMPL) { tp = TS_IMPL; }
	}
	if(simplify) {
		if(_doublenegtseitin) {
			if(negateset) {
				int tseitin = _translator->translate(tsset.literals(),!conj,tp);
				return _sign ? -tseitin : tseitin;
			}
			else {
				vector<int> newsetlits(tsset.size());
				for(size_t n = 0; n < tsset.size(); ++n) {
					newsetlits[n] = -tsset.literal(n);
				}
				int tseitin = _translator->translate(newsetlits,!conj,tp);
				return _sign ? -tseitin : tseitin;
			}
		}
		else {
			if(negateset) {
				vector<int> newsetlits(tsset.size());
				for(size_t n = 0; n < tsset.size(); ++n) {
					newsetlits[n] = -tsset.literal(n);
				}
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
		if(_doublenegtseitin) { return handleDoubleNegation(double(leftvalue),setnr); }
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
			if(minpossvalue > boundvalue || maxpossvalue < boundvalue) {
				return _sign ? _false : _true;
			}
			break;
		case '<':
			if(boundvalue < minpossvalue) {
				return _sign ? _true : _false;
			} else if(boundvalue >= maxpossvalue) {
				return _sign ? _false : _true;
			}
			break;
		case '>':
			if(boundvalue > maxpossvalue) {
				return _sign ? _true : _false;
			} else if(boundvalue <= minpossvalue) {
				return _sign ? _false : _true;
			}
			break;
		default:
			assert(false);
	}
	if(_doublenegtseitin) {
		return handleDoubleNegation(newboundvalue,setnr);
	} else {
		int tseitin;
		TsType tp = _context._tseitin;
		if(not _sign) {
			if(tp == TS_IMPL) { tp = TS_RIMPL; }
			else if(tp == TS_RIMPL) { tp = TS_IMPL; }
		}
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
	const GroundTerm& groundbound = _boundgrounder->run();
	assert(not groundbound._isvarid); //TODO
	const DomainElement* bound = groundbound._domelement;

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
			for(size_t n = 0; n < tsset.size(); ++n) {
				if(tsset.weight(n) > 0) { maxpossvalue += tsset.weight(n); }
				else if(tsset.weight(n) < 0) { minpossvalue += tsset.weight(n); }
			}
			// Finish
			tseitin = finish(boundvalue,(boundvalue-truevalue),minpossvalue,maxpossvalue,setnr);
			break;
		}
		case AGG_PROD: {
			// Compute the minimum and maximum possible value of the product.
			bool containsneg = false;
			for(size_t n = 0; n < tsset.size(); ++n) {
				maxpossvalue *= abs(tsset.weight(n));
				if(tsset.weight(n) < 0) { containsneg = true; }
			}
			if(containsneg) { minpossvalue = -maxpossvalue; }
			// Finish
			tseitin = finish(boundvalue,(boundvalue/truevalue),minpossvalue,maxpossvalue,setnr);
			break;
		}
		case AGG_MIN: {
			// Compute the minimum possible value of the set.
			for(size_t n = 0; n < tsset.size(); ++n) {
				minpossvalue = (tsset.weight(n) < minpossvalue) ? tsset.weight(n) : minpossvalue;
				// Decrease all weights greater than truevalue to truevalue.
				if(tsset.weight(n) > truevalue) { tsset.setWeight(n,truevalue); }
			}
			// Finish
			tseitin = finish(boundvalue,boundvalue,minpossvalue,maxpossvalue,setnr);
			break;
		}
		case AGG_MAX: {
			// Compute the maximum possible value of the set.
			for(size_t n = 0; n < tsset.size(); ++n) {
				maxpossvalue = (tsset.weight(n) > maxpossvalue) ? tsset.weight(n) : maxpossvalue;
				// Increase all weights less than truevalue to truevalue.
				if(tsset.weight(n) < truevalue) { tsset.setWeight(n,truevalue); }
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
	return _conj ? (l == _false) : (l == _true);
}

inline bool ClauseGrounder::check2(int l) const {
	return _conj ? (l == _true) : (l == _false);
}

inline int ClauseGrounder::result1() const {
	return (_conj == _sign) ? _false : _true;
}

inline int ClauseGrounder::result2() const {
	return (_conj == _sign) ? _true : _false;
}

int ClauseGrounder::finish(vector<int>& cl) const {
	if(_verbosity > 2) { printOrig(); }
	if(cl.empty()) {
		if(_verbosity > 2) { clog << "Result = " << _translator->printAtom(result2()) << "\n"; }
		return result2();
	}
	else if(cl.size() == 1) {
		if(_verbosity > 2) { clog << "Result = " << (_sign ? _translator->printAtom(cl[0]) : _translator->printAtom(-cl[0])) << "\n"; }
		return _sign ? cl[0] : -cl[0];
	}
	else {
		TsType tp = _context._tseitin;
		if(not _sign) {
			if(tp == TS_IMPL) { tp = TS_RIMPL; }
			else if(tp == TS_RIMPL) { tp = TS_IMPL; }
		}
		if(_doublenegtseitin) {
			for(size_t n = 0; n < cl.size(); ++n) { cl[n] = -cl[n]; }
			int ts = _translator->translate(cl,!_conj,tp);
			if(_verbosity > 2) {
				clog << "Result = " << (_sign ? "~" : "");
				clog << _translator->printAtom(cl[0]) << ' ';
				for(size_t n = 1; n < cl.size(); ++n) { 
					clog << (not _conj ? "& " : "| ") << _translator->printAtom(cl[n]) << ' ';
				}
				clog << '\n';
			}
			return _sign ? -ts : ts;
		}
		else {
			int ts = _translator->translate(cl,_conj,tp);
			if(_verbosity > 2) {
				clog << "Result = " << (_sign ? "" : "~");
				clog << _translator->printAtom(cl[0]) << ' ';
				for(size_t n = 1; n < cl.size(); ++n) { 
					clog << (_conj ? "& " : "| ") << _translator->printAtom(cl[n]) << ' ';
				}
				clog << '\n';
			}
			return _sign ? ts : -ts;
		}
	}
}

int BoolGrounder::run() const {
	vector<int> cl;
	for(size_t n = 0; n < _subgrounders.size(); ++n) {
		int l = _subgrounders[n]->run();
		if(check1(l)) { return result1(); }
		else if(not check2(l)) { cl.push_back(l); }
	}
	return finish(cl);
}

void BoolGrounder::run(vector<int>& clause) const {
	for(size_t n = 0; n < _subgrounders.size(); ++n) {
		int l = _subgrounders[n]->run();
		if(check1(l)) {
			clause.clear();
			clause.push_back(result1());
			return;
		}
		else if(not check2(l)) { clause.push_back(_sign ? l : -l); }
	}
}

int QuantGrounder::run() const {
	if(_verbosity > 2) { printOrig(); }
	vector<int> cl;
	if(_generator->first()) {
		if(_checker->first()) {
			if(_verbosity > 2) { clog << "Result = " << _translator->printAtom(result1()) << "\n"; }
			return result1();
		}
		int l = _subgrounder->run();
		if(check1(l)) {
			if(_verbosity > 2) { clog << "Result = " << _translator->printAtom(result1()) << "\n"; }
			return result1();
		}
		else if(not check2(l)) { cl.push_back(l); }
		while(_generator->next()) {
			if(_checker->first()) {
				if(_verbosity > 2) { clog << "Result = " << _translator->printAtom(result1()) << "\n"; }
				return result1();
			}
			l = _subgrounder->run();
			if(check1(l)) {
				if(_verbosity > 2) { clog << "Result = " << _translator->printAtom(result1()) << "\n"; }
				return result1();
			}
			else if(not check2(l)) { cl.push_back(l); }
		}
	}
	return finish(cl);
}

void QuantGrounder::run(vector<int>& clause) const {
	if(_verbosity > 2) { printOrig(); }
	if(_generator->first()) {
		int l = _subgrounder->run();
		if(check1(l)) {
			clause.clear();
			clause.push_back(result1());
			if(_verbosity > 2) { clog << "Result = " << _translator->printAtom(result1()) << "\n"; }
			return;
		}
		else if(not check2(l)) { clause.push_back(_sign ? l : -l); }
		while(_generator->next()) {
			l = _subgrounder->run();
			if(check1(l)) {
				clause.clear();
				clause.push_back(result1());
				if(_verbosity > 2) { clog << "Result = " << _translator->printAtom(result1()) << "\n"; }
				return;
			}
			else if(not check2(l)) { clause.push_back(_sign ? l : -l); }
		}
	}
	if(_verbosity > 2) {
		clog << "Result = " << (_sign ? "" : "~");
		if(clause.empty()) {
			clog << (_conj ? "true" : "false") << "\n";
		}
		else {
			clog << _translator->printAtom(clause[0]) << ' ';
			for(size_t n = 1; n < clause.size(); ++n) {
			   clog << (_conj ? "& " : "| ") << _translator->printAtom(clause[n]) << ' ';
			}
		}
	}
}

int EquivGrounder::run() const {
	// Run subgrounders
	int left = _leftgrounder->run();
	int right = _rightgrounder->run();

	if(left == right) { return _sign ? _true : _false; }
	else if(left == _true) {
		if(right == _false) { return _sign ? _false : _true; }
		else { return _sign ? right : -right; }
	}
	else if(left == _false) {
		if(right == _true) { return _sign ? _false : _true; }
		else { return _sign ? -right : right; }
	}
	else if(right == _true) { return _sign ? left : -left; }
	else if(right == _false) { return _sign ? -left : left; }
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

void TermGrounder::setOrig(const Term* t, const map<Variable*,const DomainElement**>& mvd, int verb) {
	_verbosity = verb;
	map<Variable*,Variable*> mvv;
	for(set<Variable*>::const_iterator it = t->freeVars().begin(); it != t->freeVars().end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),ParseInfo());
		mvv[*it] = v;
		_varmap[v] = mvd.find(*it)->second;
	}
	_origterm = t->clone(mvv);
}

void TermGrounder::printOrig() const {
	clog << "Grounding term " << _origterm->toString();
	if(not _origterm->freeVars().empty()) {
		clog << " with instance ";
		for(set<Variable*>::const_iterator it = _origterm->freeVars().begin(); it != _origterm->freeVars().end(); ++it) {
			clog << (*it)->toString() << " = ";
			const DomainElement* e = *(_varmap.find(*it)->second);
			clog << e->toString() << ' ';
		}
	}
	clog << "\n";
}

GroundTerm DomTermGrounder::run() const {
	//SortTable* _domain = new SortTable(new EnumeratedInternalSortTable());
	//_domain->add(_value);
	return GroundTerm(_value);
}

GroundTerm VarTermGrounder::run() const {
	//TODO Set domain?
	return GroundTerm(*_value);
}

GroundTerm FuncTermGrounder::run() const {
	if(_verbosity > 2) { printOrig(); }
	bool calculable = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for(size_t n = 0; n < _subtermgrounders.size(); ++n) {
		groundsubterms[n] = _subtermgrounders[n]->run();
		if(groundsubterms[n]._isvarid) {
			calculable = false;
		} else {
			args[n] = groundsubterms[n]._domelement;
		}
	}
	if(calculable && _functable) { // All ground subterms are domain elements!
		const DomainElement* result = (*_functable)[args];
		if(result) {
			if(_verbosity > 2) {
				clog << "Result = " << *result << "\n";
			}
			return GroundTerm(result);
		}
	}
	// assert(isCPSymbol(_function->symbol())) && some of the ground subterms are CP terms.
	VarId varid = _termtranslator->translate(_function,groundsubterms);
	if(_verbosity > 2) {
		clog << "Result = " << _termtranslator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}

CPTerm* createSumTerm(SumType type, const VarId& left, const VarId& right) {
	if(type == ST_MINUS) {
		vector<VarId> varids(2); //XXX vector<VarId> varids[] = { left, right }; ?
		varids[0] = left;
		varids[1] = right;
		vector<int> weights(2);
		weights[0] = 1;
		weights[1] = -1;
		return new CPWSumTerm(varids,weights);
	} else {
		return new CPSumTerm(left,right);
	}
}

GroundTerm SumTermGrounder::run() const {
	if(_verbosity > 2) { 
		printOrig();
	}
	// Run subtermgrounders
	const GroundTerm& left = _lefttermgrounder->run();
	const GroundTerm& right = _righttermgrounder->run();
	SortTable* leftdomain = _lefttermgrounder->getDomain();
	SortTable* rightdomain = _righttermgrounder->getDomain();

	// Compute domain for the sum term
	if(not _domain || not _domain->approxFinite()) {
		if(not left._isvarid) {
			leftdomain = new SortTable(new EnumeratedInternalSortTable());
			leftdomain->add(left._domelement);
		}
		if(not right._isvarid) {
			rightdomain = new SortTable(new EnumeratedInternalSortTable());
			rightdomain->add(right._domelement);
		}
		if(leftdomain && rightdomain && leftdomain->approxFinite() && rightdomain->approxFinite()) {
			int leftmin = leftdomain->first()->value()._int;
			int rightmin = rightdomain->first()->value()._int;
			int leftmax = leftdomain->last()->value()._int;
			int rightmax = rightdomain->last()->value()._int;
			int min, max; 
			if(_type == ST_PLUS) {
				min = leftmin + rightmin;
				max = leftmax + rightmax;
			} else if (_type == ST_MINUS) {
				min = leftmin - rightmin;
				max = leftmax - rightmax;
			}
			if(max < min) { swap(min,max); }
			_domain = new SortTable(new IntRangeInternalSortTable(min,max));
		} else {
			if(leftdomain && not leftdomain->approxFinite()) {
				cerr << "Left domain is infinite..." << endl;
			}
			if(rightdomain && not rightdomain->approxFinite()) {
				cerr << "Right domain is infinite..." << endl;
			}
			//TODO one of the domains is unknown or infinite...
			//TODO one case when left or right is a domain element!
			//assert(false);
		}
	}

	VarId varid;
	if(left._isvarid) {
		if(right._isvarid) {
			CPTerm* sumterm = createSumTerm(_type,left._varid,right._varid);
			varid = _termtranslator->translate(sumterm,_domain);
		} else {
			assert(not right._isvarid);
			VarId rightvarid = _termtranslator->translate(right._domelement);
			// Create tseitin
			CPTsBody* cpelement = _termtranslator->cprelation(rightvarid);
			int tseitin = _grounding->translator()->translate(cpelement->left(),cpelement->comp(),cpelement->right(),TS_EQ);
			_grounding->addUnitClause(tseitin);
			// Create cp sum term
			CPTerm* sumterm = createSumTerm(_type,left._varid,rightvarid);
			varid = _termtranslator->translate(sumterm,_domain);
		}
	} else {
		assert(not left._isvarid);
		if(right._isvarid) {
			VarId leftvarid = _termtranslator->translate(left._domelement);
			// Create tseitin
			CPTsBody* cpelement = _termtranslator->cprelation(leftvarid);
			int tseitin = _grounding->translator()->translate(cpelement->left(),cpelement->comp(),cpelement->right(),TS_EQ);
			_grounding->addUnitClause(tseitin);
			// Create cp sum term
			CPTerm* sumterm = createSumTerm(_type,leftvarid,right._varid);
			varid = _termtranslator->translate(sumterm,_domain);
		} else { // Both subterms are domain elements, so lookup the result in the function table.
			assert(not right._isvarid);
			assert(_functable);
			ElementTuple args(2); args[0] = left._domelement; args[1] = right._domelement;
			const DomainElement* result = (*_functable)[args];
			assert(result);
			if(_verbosity > 2) {
				clog << "Result = " << *result << "\n";
			}
			return GroundTerm(result);
		}
	}

	// Return result
	if(_verbosity > 2) {
		clog << "Result = " << _termtranslator->printTerm(varid) << "\n";
	}
	return GroundTerm(varid);
}

GroundTerm AggTermGrounder::run() const {
//TODO Should this grounder return a VarId in some cases?
	int setnr = _setgrounder->run();
	const TsSet& tsset = _translator->groundset(setnr);
	assert(tsset.empty());
	double value = applyAgg(_type,tsset.trueweights());
	const DomainElement* result = DomainElementFactory::instance()->create(value);
	if(_verbosity > 2) {
		clog << "Result = " << *result << "\n";
	}
	return GroundTerm(result);
}

int EnumSetGrounder::run() const {
	vector<int>	literals;
	vector<double> weights;
	vector<double> trueweights;
	for(size_t n = 0; n < _subgrounders.size(); ++n) {
		int l = _subgrounders[n]->run();
		if(l != _false) {
			const GroundTerm& groundweight = _subtermgrounders[n]->run();
			assert(not groundweight._isvarid);
			const DomainElement* d = groundweight._domelement;
			double w = (d->type() == DET_INT ? (double) d->value()._int : d->value()._double);
			if(l == _true) {
				trueweights.push_back(w);
			} else {
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
		int l;
		do {
			if(_checker->first()) l = _true;
			else l = _subgrounder->run();
			if(l != _false) {
				const GroundTerm& groundweight = _weightgrounder->run();
				assert(not groundweight._isvarid);
				const DomainElement* weight = groundweight._domelement;
				double w = (weight->type() == DET_INT ? (double) weight->value()._int : weight->value()._double);
				if(l == _true) {
					trueweights.push_back(w);
				} else {
					weights.push_back(w);
					literals.push_back(l);
				}
			}
		} while(_generator->next());
	}
	int s = _translator->translateSet(literals,weights,trueweights);
	return s;
}

HeadGrounder::HeadGrounder(AbstractGroundTheory* gt, InstanceChecker* pc, InstanceChecker* cc, PFSymbol* s, 
		const vector<TermGrounder*>& sg, const vector<SortTable*>& vst)
	: _grounding(gt), _subtermgrounders(sg), _truechecker(pc), _falsechecker(cc), _symbol(gt->translator()->addSymbol(s)), _tables(vst) {
}

int HeadGrounder::run() const {
	// Run subterm grounders
	bool alldomelts = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for(size_t n = 0; n < _subtermgrounders.size(); ++n) {
		groundsubterms[n] = _subtermgrounders[n]->run();
		if(groundsubterms[n]._isvarid) {
			alldomelts = false;
		} else {
			args[n] = groundsubterms[n]._domelement;
		}
	}
	//XXX All subterm grounders should return domain elements.
	assert(alldomelts);
	
	// Checking partial functions
	for(size_t n = 0; n < args.size(); ++n) {
		//TODO: only check positions that can be out of bounds or ...!
		//TODO: produce a warning!
		if(not args[n]) { return _false; }
		if(not _tables[n]->contains(args[n])) { return _false; }
	}

	// Run instance checkers and return grounding
	int atom = _grounding->translator()->translate(_symbol,args);
	if(_truechecker->run(args)) {
		_grounding->addUnitClause(atom);
	}
	else if(_falsechecker->run(args)) {
		_grounding->addUnitClause(-atom);
	}
	return atom;
}

void RuleGrounder::addTrueRule(GroundDefinition* const grounddefinition, int head) const {
	addPCRule(grounddefinition, head,vector<int>(0),true,false);
}

void RuleGrounder::addFalseRule(GroundDefinition* const grounddefinition, int head) const {
	addPCRule(grounddefinition, head,vector<int>(0),false,false);
}

void RuleGrounder::addPCRule(GroundDefinition* grounddefinition, int head, const vector<int>& body, bool conj, bool recursive) const {
	grounddefinition->addPCRule(head, body, conj, recursive);
}

void RuleGrounder::addAggRule(GroundDefinition* grounddefinition, int head, int setnr, AggFunction aggtype, bool lower, double bound, bool recursive) const {
	grounddefinition->addAggRule(head, setnr, aggtype, lower, bound, recursive);
}

bool RuleGrounder::run(unsigned int defid, GroundDefinition* grounddefinition) const {
	bool conj = _bodygrounder->conjunctive();
	if(_bodygenerator->first()) {	
		vector<int>	body;
		do {
			body.clear();
			_bodygrounder->run(body);
			bool falsebody = (body.empty() && !conj) || (body.size() == 1 && body[0] == _false);
			if(not falsebody) {
				bool truebody = (body.empty() && conj) || (body.size() == 1 && body[0] == _true);
				if(_headgenerator->first()) {
					do{
						int head = _headgrounder->run();
						assert(head != _true);
						if(head != _false) {
							if(truebody) {
								addTrueRule(grounddefinition, head);
							} else {
								addPCRule(grounddefinition, head,body,conj,_context._tseitin == TS_RULE);
							}
						}
					} while(_headgenerator->next());
				}
			}
		} while(_bodygenerator->next());
	}
	return true;
}

unsigned int DefinitionGrounder::_currentdefnb = 1;

DefinitionGrounder::DefinitionGrounder(AbstractGroundTheory* gt, std::vector<RuleGrounder*> subgr,int verb)
		: TopLevelGrounder(gt,verb), _defnb(_currentdefnb++), _subgrounders(subgr), _grounddefinition(new GroundDefinition(_defnb, gt->translator())) {
}

bool DefinitionGrounder::run() const {
	for(size_t n = 0; n < _subgrounders.size(); ++n) {
		bool b = _subgrounders[n]->run(id(), _grounddefinition);
		if(not b) {
			return false;
		}
	}
	return true;
}


/******************************
	GrounderFactory methods
******************************/

GrounderFactory::GrounderFactory(AbstractStructure* structure, Options* opts, SymbolicStructure* symstructure)
	: _structure(structure), _symstructure(symstructure), _options(opts), _verbosity(opts->groundverbosity()),
		_cpsupport(opts->cpsupport()), _longnames(opts->longnames()) {

	// Create a symbolic structure if no such structure is given
	if(not _symstructure) {
		FOBDDManager* manager = new FOBDDManager();
		std::map<PFSymbol*,const FOBDD*> ctbounds;
		std::map<PFSymbol*,const FOBDD*> cfbounds;
		std::map<PFSymbol*,std::vector<const FOBDDVariable*> > vars;
		Vocabulary* vocabulary = structure->vocabulary();
		for(auto it = vocabulary->firstPred(); it != vocabulary->lastPred(); ++it) {
			set<Predicate*> sp = it->second->nonbuiltins();
			for(auto jt = sp.begin(); jt != sp.end(); ++jt) {
				PredInter* pinter = structure->inter(*jt);
				if(not pinter->approxTwoValued()) {
					vector<Variable*> pvars = VarUtils::makeNewVariables((*jt)->sorts());
					vector<const FOBDDVariable*> pbddvars(pvars.size());
					vector<const FOBDDArgument*> pbddargs(pvars.size());
					for(size_t n = 0; n < pvars.size(); ++n) {
						const FOBDDVariable* bddvar = manager->getVariable(pvars[n]);
						pbddvars[n] = bddvar; pbddargs[n] = bddvar;
					}
					vars[*jt] = pbddvars;
					const FOBDDKernel* ctkernel = manager->getAtomKernel(*jt,AKT_CT,pbddargs);
					const FOBDDKernel* cfkernel = manager->getAtomKernel(*jt,AKT_CF,pbddargs);
					ctbounds[*jt] = manager->getBDD(ctkernel,manager->truebdd(),manager->falsebdd());
					cfbounds[*jt] = manager->getBDD(cfkernel,manager->truebdd(),manager->falsebdd());
				}
			}
		}
		for(auto it = vocabulary->firstFunc(); it != vocabulary->lastFunc(); ++it) {
			set<Function*> sf = it->second->nonbuiltins();
			for(auto jt = sf.begin(); jt != sf.end(); ++jt) {
				PredInter* pinter = structure->inter(*jt)->graphInter();
				if(not pinter->approxTwoValued()) {
					vector<Variable*> pvars = VarUtils::makeNewVariables((*jt)->sorts());
					vector<const FOBDDVariable*> pbddvars(pvars.size());
					vector<const FOBDDArgument*> pbddargs(pvars.size());
					for(size_t n = 0; n < pvars.size(); ++n) {
						const FOBDDVariable* bddvar = manager->getVariable(pvars[n]);
						pbddvars[n] = bddvar; pbddargs[n] = bddvar;
					}
					vars[*jt] = pbddvars;
					const FOBDDKernel* ctkernel = manager->getAtomKernel(*jt,AKT_CT,pbddargs);
					const FOBDDKernel* cfkernel = manager->getAtomKernel(*jt,AKT_CF,pbddargs);
					ctbounds[*jt] = manager->getBDD(ctkernel,manager->truebdd(),manager->falsebdd());
					cfbounds[*jt] = manager->getBDD(cfkernel,manager->truebdd(),manager->falsebdd());
				}
			}
		}
		_symstructure = new SymbolicStructure(manager,ctbounds,cfbounds,vars);
	}

	if(_verbosity > 2) {
		clog << "Using the following symbolic structure to ground: " << endl;
		_symstructure->put(clog);
	}

}

set<const PFSymbol*> GrounderFactory::findCPSymbols(const AbstractTheory* theory) {
	Vocabulary* vocabulary = theory->vocabulary();
//	for(map<string,Predicate*>::const_iterator predit = vocabulary->firstpred(); predit != vocabulary->lastpred(); ++predit) {
//		Predicate* predicate = predit->second;
//		if(VocabularyUtils::isComparisonPredicate(predicate)) {
//			_cpsymbols.insert(predicate);
//		}
//	}
	for(map<string,Function*>::const_iterator funcit = vocabulary->firstFunc(); funcit != vocabulary->lastFunc(); ++funcit) {
		Function* function = funcit->second;
		bool passtocp = false;
		// Check whether the (user-defined) function's outsort is over integers
		Sort* intsort = *(vocabulary->sort("int")->begin());
		if(function->overloaded()) {
			set<Function*> nonbuiltins = function->nonbuiltins();
			for(set<Function*>::const_iterator nbfit = nonbuiltins.begin(); nbfit != nonbuiltins.end(); ++nbfit) {
				passtocp = (SortUtils::resolve(function->outsort(),intsort,vocabulary) == intsort);
			}
		} else if(not function->builtin()) {
			passtocp = (SortUtils::resolve(function->outsort(),intsort,vocabulary) == intsort);
		}
		if(passtocp) { _cpsymbols.insert(function); }
	}
	if(_verbosity > 1) {
		clog << "User-defined symbols that can be handled by the constraint solver: ";
		for(set<const PFSymbol*>::const_iterator it = _cpsymbols.begin(); it != _cpsymbols.end(); ++it) {
			(*it)->put(clog,_longnames);
			clog << ' ';
		}
		clog << '\n';
	}
	return _cpsymbols;
}

bool GrounderFactory::isCPSymbol(const PFSymbol* symbol) const {
	return VocabularyUtils::isComparisonPredicate(symbol) || (_cpsymbols.find(symbol) != _cpsymbols.end());
}

/**
 * bool GrounderFactory::recursive(const Formula*)
 * DESCRIPTION
 * 		Finds out whether a formula contains recursively defined symbols.
 */
bool GrounderFactory::recursive(const Formula* f) {
	for(set<PFSymbol*>::const_iterator it = _context._defined.begin(); it != _context._defined.end(); ++it) {
		if(f->contains(*it)) { return true; }
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
	_context._truegen 		= false;
	_context._funccontext 	= PC_POSITIVE;
	_context._tseitin 		= (_context._tseitin == TS_RULE) ? TS_RULE : TS_EQ;
	_context._component 	= CC_FORMULA;
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
	if(_context._component == CC_SENTENCE) { _context._component = CC_FORMULA; }
	// Swap positive, truegen and tseitin according to sign
	if(not sign) {
		_context._truegen = !_context._truegen;

		if(_context._funccontext == PC_POSITIVE) { _context._funccontext = PC_NEGATIVE; }
		else if(_context._funccontext == PC_NEGATIVE) { _context._funccontext = PC_POSITIVE; }

		if(_context._monotone == PC_POSITIVE) { _context._monotone = PC_NEGATIVE; }
		else if(_context._monotone == PC_NEGATIVE) { _context._monotone = PC_POSITIVE; }

		if(_context._tseitin == TS_IMPL) { _context._tseitin = TS_RIMPL; }
		else if(_context._tseitin == TS_RIMPL) { _context._tseitin = TS_IMPL; }
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
	GroundTheory<GroundPolicy>* groundtheory = new GroundTheory<GroundPolicy>(theory->vocabulary(),_structure->clone());
	_grounding = groundtheory;

	// Find functions that can be passed to CP solver.
	if(_cpsupport) { findCPSymbols(theory); }

	// Create the grounder
	theory->accept(this);
	return _toplevelgrounder;
}

// TODO comment
TopLevelGrounder* GrounderFactory::create(const AbstractTheory* theory, InteractivePrintMonitor* monitor, Options* opts) {
	GroundTheory<PrintGroundPolicy>* groundtheory = new GroundTheory<PrintGroundPolicy>(_structure->clone());
	groundtheory->initialize(monitor, groundtheory->structure(), groundtheory->translator(), groundtheory->termtranslator(), opts);
	_grounding = groundtheory;

	// Find functions that can be passed to CP solver.
	if(_cpsupport) { findCPSymbols(theory); }

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
	GroundTheory<SolverPolicy>* groundtheory = new GroundTheory<SolverPolicy>(theory->vocabulary(), _structure->clone());
	groundtheory->initialize(solver, _verbosity, groundtheory->termtranslator());
	_grounding = groundtheory;

	// Find function that can be passed to CP solver.
	if(_cpsupport) { findCPSymbols(theory); }

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
void GrounderFactory::visit(const AbstractGroundTheory* ecnf) {
	_toplevelgrounder = new CopyGrounder(_grounding,ecnf,_verbosity);	
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
	for(size_t n = 0; n < components.size(); ++n) {
		InitContext();
		if(_verbosity > 0) {
			clog << "Creating a grounder for ";
			components[n]->put(clog,_longnames);
			clog << "\n";
		}
		components[n]->accept(this);
		children[n] = _toplevelgrounder; 
	}

	// Create the grounder
	_toplevelgrounder = new TheoryGrounder(_grounding,children,_verbosity);
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
	PredForm* clonedformula = pf->clone();
	Formula* movedformula = FormulaUtils::moveThreeValuedTerms(clonedformula,_structure,_context._funccontext,_cpsupport,_cpsymbols);
	movedformula = FormulaUtils::removeEqChains(movedformula);
	if(not _cpsupport) { movedformula = FormulaUtils::graphFunctions(movedformula); }

	if(typeid(*movedformula) != typeid(PredForm)) {	// The rewriting changed the atom
		if(_verbosity > 1) {
			clog << "Rewritten "; pf->put(clog,_longnames);
			clog << " to "; movedformula->put(clog,_longnames);
		   	clog << "\n"; 
		}
		movedformula->accept(this);
	}
	else {	// The rewriting did not change the atom
		PredForm* newpf = dynamic_cast<PredForm*>(movedformula);
		// Create grounders for the subterms
		vector<TermGrounder*> subtermgrounders;
		vector<SortTable*>	  argsorttables;
		for(size_t n = 0; n < newpf->subterms().size(); ++n) {
			descend(newpf->subterms()[n]);
			subtermgrounders.push_back(_termgrounder);
			argsorttables.push_back(_structure->inter(newpf->symbol()->sorts()[n]));
		}
		// Create checkers and grounder
		if(_cpsupport && VocabularyUtils::isComparisonPredicate(newpf->symbol())) {
			string name = newpf->symbol()->name();
			CompType comp;
			if(name == "=/2") { comp = (pf->sign() ? CT_EQ : CT_NEQ); }
			else if(name == "</2") { comp = (pf->sign() ? CT_LT : CT_GEQ); }
			else if(name == ">/2") { comp = (pf->sign() ? CT_GT : CT_LEQ); }
			else { assert(false); comp = CT_EQ; }
			_formgrounder = new ComparisonGrounder(_grounding->translator(),_grounding->termtranslator(),
									subtermgrounders[0],comp,subtermgrounders[1],_context);
			_formgrounder->setOrig(newpf,_varmapping,_verbosity);
			if(_context._component == CC_SENTENCE) { 
				_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_verbosity);
			}
		}
		else {
			PredInter* inter = _structure->inter(newpf->symbol());
			CheckerFactory checkfactory;
			if(_context._component == CC_HEAD) {
				// Create instance checkers
				InstanceChecker* truech = checkfactory.create(inter,true,true);
				InstanceChecker* falsech = checkfactory.create(inter,false,true);
				// Create the grounder
				_headgrounder = new HeadGrounder(_grounding,truech,falsech,newpf->symbol(),subtermgrounders,argsorttables);
			}
			else {
				// Create instance checkers
				vector<Sort*> checksorts;
				vector<const DomainElement**> checkargs;
				vector<SortTable*> tables;
				for(auto it = newpf->subterms().begin(); it != newpf->subterms().end(); ++it) {
					checksorts.push_back((*it)->sort());
					checkargs.push_back(new const DomainElement*());
					tables.push_back(_structure->inter((*it)->sort()));
				}
				vector<Variable*> fovars = VarUtils::makeNewVariables(checksorts);
				vector<Term*> foterms = TermUtils::makeNewVarTerms(fovars);
				PredForm* checkpf = new PredForm(newpf->sign(),newpf->symbol(),foterms,FormulaParseInfo());
				const FOBDD* possbdd;
				const FOBDD* certbdd;
				if(_context._truegen) {	
					possbdd = _symstructure->evaluate(checkpf,QT_PT);
					certbdd = _symstructure->evaluate(checkpf,QT_CT); 
				}
				else {	
					possbdd = _symstructure->evaluate(checkpf,QT_PF);
					certbdd = _symstructure->evaluate(checkpf,QT_CF); 
				}
				PredTable* posstable = new PredTable(new BDDInternalPredTable(possbdd,_symstructure->manager(),fovars,_structure),Universe(tables));
				PredTable* certtable = new PredTable(new BDDInternalPredTable(certbdd,_symstructure->manager(),fovars,_structure),Universe(tables));
				GeneratorFactory gf;
				InstGenerator* possch = gf.create(posstable,vector<bool>(checkargs.size(),true),checkargs,Universe(tables));
				InstGenerator* certainch = gf.create(certtable,vector<bool>(checkargs.size(),true),checkargs,Universe(tables));
				// Create the grounder
				_formgrounder = new AtomGrounder(_grounding->translator(),newpf->sign(),newpf->symbol(),
										subtermgrounders,checkargs,possch,certainch,inter,argsorttables,_context);
				_formgrounder->setOrig(newpf,_varmapping,_verbosity);
				if(_context._component == CC_SENTENCE) { 
					_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_verbosity);
				}
			}
		}
	}
	movedformula->recursiveDelete();
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
		if(not newbf->conj()) {
			newbf->conj(true);
			newbf->negate();
			for(vector<Formula*>::const_iterator it = newbf->subformulas().begin(); it != newbf->subformulas().end(); ++it) {
				(*it)->negate();
			}
		}
		// Visit the subformulas
		vector<TopLevelGrounder*> sub;
		for(vector<Formula*>::const_iterator it = newbf->subformulas().begin(); it != newbf->subformulas().end(); ++it) {
			descend(*it);
			sub.push_back(_toplevelgrounder);
		}
		newbf->recursiveDelete();
		_toplevelgrounder = new TheoryGrounder(_grounding,sub,_verbosity);
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
		if(recursive(bf)) { _context._tseitin = TS_RULE; }
		_formgrounder = new BoolGrounder(_grounding->translator(),sub,bf->sign(),bf->conj(),_context);
		RestoreContext();
		_formgrounder->setOrig(bf,_varmapping,_verbosity);
		if(_context._component == CC_SENTENCE) { 
			_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_verbosity);
		}
	}
}

const FOBDD* GrounderFactory::improve_generator(const FOBDD* bdd, const vector<Variable*>& fovars, double mcpa) {
	FOBDDManager* manager = _symstructure->manager();

/*cerr << "improving\n";
manager->put(cerr,bdd);
set<Variable*> sv(fovars.begin(),fovars.end());
set<const FOBDDVariable*> sfv = manager->getVariables(sv);
set<const FOBDDDeBruijnIndex*> id;
cerr << "current cost = " << manager->estimatedCostAll(bdd,sfv,id,_structure) << endl;
*/
	// 1. Optimize the query
	FOBDDManager optimizemanager;
	const FOBDD* copybdd = optimizemanager.getBDD(bdd,manager);
	set<const FOBDDVariable*> copyvars;
	set<const FOBDDDeBruijnIndex*> indices;
	for(auto it = fovars.begin(); it != fovars.end(); ++it) {
		copyvars.insert(optimizemanager.getVariable(*it));
	}
	optimizemanager.optimizequery(copybdd,copyvars,indices,_structure);
/*
cerr << "optimized version\n";
optimizemanager.put(cerr,copybdd);
sfv = optimizemanager.getVariables(sv);
cerr << "cost is now: " << optimizemanager.estimatedCostAll(copybdd,sfv,id,_structure) << endl;
*/

	// 2. Remove certain leaves
	const FOBDD* pruned = optimizemanager.make_more_true(copybdd,copyvars,indices,_structure,mcpa);
/*
cerr << "pruned version\n";
optimizemanager.put(cerr,pruned);
*/

	// 3. Replace result
	return manager->getBDD(pruned,&optimizemanager);
}

const FOBDD* GrounderFactory::improve_checker(const FOBDD* bdd, double mcpa) {
	FOBDDManager* manager = _symstructure->manager();

	// 1. Optimize the query
	FOBDDManager optimizemanager;
	const FOBDD* copybdd = optimizemanager.getBDD(bdd,manager);
	set<const FOBDDVariable*> copyvars;
	set<const FOBDDDeBruijnIndex*> indices;
	optimizemanager.optimizequery(copybdd,copyvars,indices,_structure);

	// 2. Remove certain leaves
	const FOBDD* pruned = optimizemanager.make_more_false(copybdd,copyvars,indices,_structure,mcpa);

	// 3. Replace result
	return manager->getBDD(pruned,&optimizemanager);
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
//cerr << "visit of " << *qf << endl;
	// Create instance generator
	Formula* clonedformula = qf->subformula()->clone();
	Formula* movedformula = FormulaUtils::moveThreeValuedTerms(clonedformula,_structure,_context._funccontext);
	movedformula = FormulaUtils::removeEqChains(movedformula);
	movedformula = FormulaUtils::graphFunctions(movedformula);
//cerr << "body translated to " << *movedformula << endl;
	const FOBDD* generatorbdd = _symstructure->evaluate(movedformula,(qf->univ() ? QT_PF : QT_PT));
	const FOBDD* checkerbdd = _symstructure->evaluate(movedformula,(qf->univ() ? QT_CF : QT_CT));
//cerr << "generatorbdd:\n";
//_symstructure->manager()->put(cerr,generatorbdd);
//cerr << "checkerbdd:\n";
//_symstructure->manager()->put(cerr,checkerbdd);
	vector<const DomainElement**> vars;
	vector<Variable*> fovars;
	vector<Variable*> optivars;
	vector<SortTable*> tables;
	vector<bool> pattern;
	for(std::set<Variable*>::const_iterator it = movedformula->freeVars().begin(); it != movedformula->freeVars().end(); ++it) {
		if(qf->quantVars().find(*it) == qf->quantVars().end()) {
			assert(_varmapping.find(*it) != _varmapping.end());
			vars.push_back(_varmapping[*it]);
			pattern.push_back(true);
		}
		else {
			const DomainElement** d = new const DomainElement*();
			_varmapping[*it] = d;
			vars.push_back(d);
			pattern.push_back(false);
			optivars.push_back(*it);
		}
		fovars.push_back(*it);
		SortTable* st = _structure->inter((*it)->sort());
		tables.push_back(st);
	}
	generatorbdd = improve_generator(generatorbdd,optivars,MCPA);
	checkerbdd = improve_checker(checkerbdd,MCPA);
//cerr << "improved generatorbdd:\n";
//_symstructure->manager()->put(cerr,generatorbdd);
//cerr << "improved checkerbdd:\n";
//_symstructure->manager()->put(cerr,checkerbdd);
	PredTable* gentable = new PredTable(new BDDInternalPredTable(generatorbdd,_symstructure->manager(),fovars,_structure),Universe(tables));
	PredTable* checktable = new PredTable(new BDDInternalPredTable(checkerbdd,_symstructure->manager(),fovars,_structure),Universe(tables));
	GeneratorFactory gf;
	InstGenerator* gen = gf.create(gentable,pattern,vars,Universe(tables));
	InstGenerator* check = gf.create(checktable,vector<bool>(vars.size(),true),vars,Universe(tables));

	// Handle top-level universal quantifiers efficiently
	if(_context._component == CC_SENTENCE && (qf->sign() == qf->univ())) {
		Formula* newsub = qf->subformula()->clone();
		if(not qf->univ()) { newsub->negate(); }
		descend(newsub);
		newsub->recursiveDelete();
		_toplevelgrounder = new UnivSentGrounder(_grounding,_toplevelgrounder,gen,_verbosity);
	}
	else {
		// Create grounder for subformula
		SaveContext();
		DeeperContext(qf->sign());
		_context._truegen = !(qf->univ()); 
		descend(qf->subformula());
		RestoreContext();

		// Create the grounder
		SaveContext();
		if(recursive(qf)) { _context._tseitin = TS_RULE; }
		_formgrounder = new QuantGrounder(_grounding->translator(),_formgrounder,qf->sign(),qf->univ(),gen,check,_context);
		RestoreContext();
		_formgrounder->setOrig(qf,_varmapping,_verbosity);
		if(_context._component == CC_SENTENCE) {
			_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_verbosity);
		}
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
	if(recursive(ef)) { _context._tseitin = TS_RULE; }
	_formgrounder = new EquivGrounder(_grounding->translator(),leftg,rightg,ef->sign(),_context);
	RestoreContext();
	if(_context._component == CC_SENTENCE) {
		_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true,_verbosity);
	}
}

/**
 * void GrounderFactory::visit(const AggForm* af)
 * DESCRIPTION
 * 		Creates a grounder for an aggregate.
 */
void GrounderFactory::visit(const AggForm* af) {
	AggForm* clonedformula = af->clone();
	Formula* movedformula = FormulaUtils::moveThreeValuedTerms(clonedformula,_structure,_context._funccontext,_cpsupport,_cpsymbols);
	//movedformula = FormulaUtils::removeEqChains(movedformula);
	//movedformula = FormulaUtils::graphFunctions(movedformula);

	if(typeid(*movedformula) != typeid(AggForm)) {	// The rewriting changed the atom
		if(_verbosity > 1) {
			clog << "Rewritten "; af->put(clog,_longnames);
			clog << " to "; movedformula->put(clog,_longnames);
		   	clog << "\n"; 
		}
		movedformula->accept(this);
	}
	else {	// The rewriting did not change the atom
		AggForm* newaf = dynamic_cast<AggForm*>(movedformula);
		// Create grounder for the bound
		descend(newaf->left());
		TermGrounder* boundgr = _termgrounder;
	
		// Create grounder for the set
		SaveContext();
		if(recursive(newaf)) { assert(FormulaUtils::isMonotone(newaf) || FormulaUtils::isAntimonotone(newaf)); }
		DeeperContext(not FormulaUtils::isAntimonotone(newaf));
		descend(newaf->right()->set());
		SetGrounder* setgr = _setgrounder;
		RestoreContext();
	
		// Create aggregate grounder
		SaveContext();
		if(recursive(newaf)) _context._tseitin = TS_RULE;
		// TODO: change AggGrounder so that it accepts a CompType...
		char cmp; bool sgn = newaf->sign();
		switch(newaf->comp()) {
			case CT_EQ: cmp = '='; break;
			case CT_NEQ: cmp = '='; sgn = !sgn; break;
			case CT_LT: cmp = '<'; break;
			case CT_LEQ: cmp = '>'; sgn = !sgn; break;
			case CT_GT: cmp = '>'; break;
			case CT_GEQ: cmp = '<'; sgn = !sgn; break;
			default: assert(false); cmp = '=';
		}
		if(not sgn) {
			if(_context._tseitin == TS_IMPL) { _context._tseitin = TS_RIMPL; }
			else if(_context._tseitin == TS_RIMPL) { _context._tseitin = TS_IMPL; }
		}
		_formgrounder = new AggGrounder(_grounding->translator(),_context,newaf->right()->function(),setgr,boundgr,cmp,sgn);
		RestoreContext();
		if(_context._component == CC_SENTENCE) {
			_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true,_verbosity);
		}
	}
	movedformula->recursiveDelete();
}

/**
 * void GrounderFactory::visit(const EqChainForm* ef)
 * DESCRIPTION
 * 		Creates a grounder for an equation chain.
 */
void GrounderFactory::visit(const EqChainForm* ef) {
	Formula* f = ef->clone();
	f = FormulaUtils::removeEqChains(f,_grounding->vocabulary());
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
	_termgrounder->setOrig(t,_varmapping,_verbosity);
}

/**
 * void GrounderFactory::visit(const DomainTerm* t)
 * DESCRIPTION
 * 		Creates a grounder for a domain term.
 */
void GrounderFactory::visit(const DomainTerm* t) {
	_termgrounder = new DomTermGrounder(t->value());
	_termgrounder->setOrig(t,_varmapping,_verbosity);
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
	FuncTable* ftable = _structure->inter(function)->funcTable();
	SortTable* domain = _structure->inter(function->outsort());
	if(_cpsupport && FuncUtils::isIntSum(function,_structure->vocabulary())) {
		if(function->name() == "-/2") {
			_termgrounder = new SumTermGrounder(_grounding,_grounding->termtranslator(),ftable,domain,subtermgrounders[0],subtermgrounders[1],ST_MINUS);
		} else {
			_termgrounder = new SumTermGrounder(_grounding,_grounding->termtranslator(),ftable,domain,subtermgrounders[0],subtermgrounders[1]);
		}
	} else {
		_termgrounder = new FuncTermGrounder(_grounding->termtranslator(),function,ftable,domain,subtermgrounders);
	}
	_termgrounder->setOrig(t,_varmapping,_verbosity);
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
	_termgrounder = new AggTermGrounder(_grounding->translator(),t->function(),_setgrounder);
	_termgrounder->setOrig(t,_varmapping,_verbosity);
}

/**
 * void GrounderFactory::visit(const EnumSetExpr* s)
 * DESCRIPTION
 * 		Creates a grounder for an enumarated set.
 */
void GrounderFactory::visit(const EnumSetExpr* s) {
	// Create grounders for formulas and weights
	vector<FormulaGrounder*> subfgr;
	vector<TermGrounder*> subtgr;
	SaveContext();
	AggContext();
	for(size_t n = 0; n < s->subformulas().size(); ++n) {
		descend(s->subformulas()[n]);
		subfgr.push_back(_formgrounder);
		descend(s->subterms()[n]);
		subtgr.push_back(_termgrounder);
	}
	RestoreContext();

	// Create set grounder
	_setgrounder = new EnumSetGrounder(_grounding->translator(),subfgr,subtgr);
}

/**
 * void GrounderFactory::visit(const QuantSetExpr* s)
 * DESCRIPTION
 * 		Creates a grounder for a quantified set.
 */
void GrounderFactory::visit(const QuantSetExpr* qs) {
	// Move three-valued terms in the set expression
	//SetExpr* transqs = TermUtils::moveThreeValuedTerms(qs->clone(),_structure,_context._funccontext,_cpsupport,_cpsymbols);
	//QuantSetExpr* newqs = dynamic_cast<QuantSetExpr*>(transqs);

	//if(_verbosity > 1) {
	//	clog << "Rewritten "; qs->put(clog,_longnames);
	//	clog << " to "; newqs->put(clog,_longnames);
	//   	clog << "\n"; 
	//}

	// Create instance generator
	Formula* clonedformula = qs->subformulas()[0]->clone();
	Formula* movedformula = FormulaUtils::moveThreeValuedTerms(clonedformula,_structure,PC_POSITIVE);
	movedformula = FormulaUtils::removeEqChains(movedformula);
	movedformula = FormulaUtils::graphFunctions(movedformula);
	const FOBDD* generatorbdd = _symstructure->evaluate(movedformula,QT_PT);
	const FOBDD* checkerbdd = _symstructure->evaluate(movedformula,QT_CT);
	vector<const DomainElement**> vars;
	vector<Variable*> fovars;
	vector<Variable*> optivars;
	vector<SortTable*> tables;
	vector<bool> pattern;
	for(std::set<Variable*>::const_iterator it = movedformula->freeVars().begin(); it != movedformula->freeVars().end(); ++it) {
		if(qs->quantVars().find(*it) == qs->quantVars().end()) {
			vars.push_back(_varmapping[*it]);
			pattern.push_back(true);
		}
		else {
			const DomainElement** d = new const DomainElement*();
			_varmapping[*it] = d;
			vars.push_back(d);
			pattern.push_back(false);
			optivars.push_back(*it);
		}
		fovars.push_back(*it);
		SortTable* st = _structure->inter((*it)->sort());
		tables.push_back(st);
	}
	generatorbdd = improve_generator(generatorbdd,optivars,MCPA);
	checkerbdd = improve_checker(checkerbdd,MCPA);
	PredTable* gentable = new PredTable(new BDDInternalPredTable(generatorbdd,_symstructure->manager(),fovars,_structure),Universe(tables));
	PredTable* checktable = new PredTable(new BDDInternalPredTable(checkerbdd,_symstructure->manager(),fovars,_structure),Universe(tables));
	GeneratorFactory gf;
	InstGenerator* gen = gf.create(gentable,pattern,vars,Universe(tables));
	InstGenerator* check = gf.create(checktable,vector<bool>(vars.size(),true),vars,Universe(tables));

	// Create grounder for subformula
	SaveContext();
	AggContext();
	descend(qs->subformulas()[0]);
	FormulaGrounder* subgr = _formgrounder;
	RestoreContext();

	// Create grounder for weight
	descend(qs->subterms()[0]);
	TermGrounder* wgr = _termgrounder;

	// Create grounder	
	_setgrounder = new QuantSetGrounder(_grounding->translator(),subgr,gen,check,wgr);
}

/**
 * void GrounderFactory::visit(const Definition* def)
 * DESCRIPTION
 * 		Creates a grounder for a definition.
 */
void GrounderFactory::visit(const Definition* def) {
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
	_toplevelgrounder = new DefinitionGrounder(_grounding,subgrounders,_verbosity);

	_context._defined.clear();
}

/**
 * void GrounderFactory::visit(const Rule* rule)
 * DESCRIPTION
 * 		Creates a grounder for a definitional rule.
 */
void GrounderFactory::visit(const Rule* rule) {
	// FIXME Move all three-valued terms outside the head

	// Split the quantified variables in two categories: 
	//		1. the variables that only occur in the head
	//		2. the variables that occur in the body (and possibly in the head)
	vector<Variable*>	headvars;
	vector<Variable*>	bodyvars;
	for(set<Variable*>::const_iterator it = rule->quantVars().begin(); it != rule->quantVars().end(); ++it) {
		if(rule->body()->contains(*it)) {
			bodyvars.push_back(*it);
		}
		else {
			headvars.push_back(*it);
		}
	}

	// Create head instance generator
	vector<SortTable*> hvst;
	vector<const DomainElement**> hvars;
	for(unsigned int n = 0; n < headvars.size(); ++n) {
		const DomainElement** d = new const DomainElement*();
		_varmapping[headvars[n]] = d;
		hvst.push_back(_structure->inter((headvars[n])->sort()));
		hvars.push_back(d);
	}
	GeneratorFactory gf;
	InstGenerator* headgen = gf.create(hvars,hvst);
	
	// Create body instance generator
	vector<SortTable*> bvst;
	vector<const DomainElement**> bvars;
	for(unsigned int n = 0; n < bodyvars.size(); ++n) {
		const DomainElement** d = new const DomainElement*();
		_varmapping[bodyvars[n]] = d;
		bvst.push_back(_structure->inter((bodyvars[n])->sort()));
		bvars.push_back(d);
	}
	InstGenerator* bodygen = gf.create(bvars,bvst);
	
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
	_rulegrounder = new RuleGrounder(headgr,bodygr,headgen,bodygen,_context);
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
