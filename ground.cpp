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
#include "grounders/FormulaGrounders.hpp"
#include "grounders/TermGrounders.hpp"
#include "grounders/SetGrounders.hpp"
#include "grounders/DefinitionGrounders.hpp"
#include "grounders/LazyQuantGrounder.hpp"

using namespace std;
using namespace rel_ops;


/****************************************
	Comparison operators for TsBodies
****************************************/

bool operator==(const TsBody& a, const TsBody& b) {
	if(typeid(a) != typeid(b))
		return false;
	else
		return (int)a.type() == (int)b.type();
}

bool operator<(const TsBody& a, const TsBody& b) {
	// Order of TsBodies of different types is defined by lexical order of there typeids.
	if(typeid(a) != typeid(b)){
		return (typeid(a).name() < typeid(b).name());
	}else{
		return (int)a.type() < (int)b.type();
	}
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
		return a._isvarid ? (a._varid == b._varid) : (a._bound == b._bound);
	}
	return false;
}

bool operator<(const CPBound& a, const CPBound& b) {
	if(a._isvarid == b._isvarid) {
		return a._isvarid ? (a._varid < b._varid) : (a._bound < b._bound);
	}
	// CPBounds with a number value come before CPBounds with a CP variable identifier.
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

Lit GroundTranslator::translate(unsigned int n, const ElementTuple& args) {
	auto jt = _table[n].lower_bound(args);

	Lit lit;
	if(jt != _table[n].end() && jt->first == args) {
		lit = jt->second;
	} else {
		lit = nextNumber();
		_table[n].insert(jt,pair<ElementTuple,int>(args,lit));
		_backsymbtable[lit] = _symboffsets[n];
		_backargstable[lit] = args;

		// FIXME expensive operation to do so often!
		auto ruleit = symbol2rulegrounder.find(n);
		if(ruleit!=symbol2rulegrounder.end()){
			ruleit->second->notify(lit, args);
		}
	}

	return lit;
}

Lit GroundTranslator::translate(PFSymbol* s, const ElementTuple& args) {
	unsigned int offset = addSymbol(s);
	return translate(offset,args);
}

Lit GroundTranslator::translate(const vector<Lit>& clause, bool conj, TsType tstype) {
	int nr = nextNumber();
	return translate(nr, clause, conj, tstype);
}

Lit GroundTranslator::translate(const Lit& head, const vector<Lit>& clause, bool conj, TsType tstype) {
	PCTsBody* tsbody = new PCTsBody(tstype,clause,conj);
	_nr2tsbodies.insert(pair<int,TsBody*>(head,tsbody));
	return head;
}

// Adds a tseitin body only if it does not yet exist. TODO why does this seem only relevant for CP Terms?
Lit GroundTranslator::addTseitinBody(TsBody* tsbody){
	auto it = _tsbodies2nr.lower_bound(tsbody);

	if(it != _tsbodies2nr.end() && *(it->first) == *tsbody) { // Already exists
		delete tsbody;
		return it->second;
	}

	int nr = nextNumber();
	_tsbodies2nr.insert(it,pair<TsBody*,int>(tsbody,nr));
	_nr2tsbodies.insert(pair<int,TsBody*>(nr,tsbody));
	return nr;
}

void GroundTranslator::notifyDefined(PFSymbol* pfs, LazyRuleGrounder* const grounder){
	assert(symbol2rulegrounder.find(addSymbol(pfs))==symbol2rulegrounder.end());
	symbol2rulegrounder.insert(pair<uint, LazyRuleGrounder*>(addSymbol(pfs), grounder));
}

void GroundTranslator::translate(LazyQuantGrounder const* const lazygrounder, ResidualAndFreeInst* instance, TsType tstype) {
	instance->residual = nextNumber();
	LazyTsBody* tsbody = new LazyTsBody(lazygrounder->id(), lazygrounder, instance, tstype);
	_nr2tsbodies.insert(pair<int,TsBody*>(instance->residual,tsbody));
}

Lit	GroundTranslator::translate(double bound, char comp, bool strict, AggFunction aggtype, int setnr, TsType tstype) {
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
		_nr2tsbodies.insert(pair<int,TsBody*>(nr,tsbody));
		return nr;
	}
}

Lit GroundTranslator::translate(CPTerm* left, CompType comp, const CPBound& right, TsType tstype) {
	CPTsBody* tsbody = new CPTsBody(tstype,left,comp,right);
	return addTseitinBody(tsbody);
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
		_backsymbtable.push_back(NULL);
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
	if(nr == _true) return "true";
	else if(nr == _false) return "false";
	if(nr >= int(_backsymbtable.size())) {
		return "error";
	}
	PFSymbol* pfs = atom2symbol(nr);
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
	CPTsBody* cprelation = new CPTsBody(TsType::EQ,cpterm,CompType::EQ,bound);
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
	CPTsBody* cprelation = new CPTsBody(TsType::EQ,cpterm,CompType::EQ,bound);
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
	if(found != _function2offset.end())
		// Simply return number when function is already known
		return found->second;
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
	if(varid >= _varid2function.size()) {
		return "error";
	}
	const Function* func = function(varid);
	if(func) {
		s << func->to_string();
		if(not args(varid).empty()) {
			s << "(";
			for(vector<GroundTerm>::const_iterator gtit = args(varid).begin(); gtit != args(varid).end(); ++gtit) {
				if((*gtit)._isvarid) {
					s << printTerm((*gtit)._varid);
				} else {
					s << (*gtit)._domelement->to_string();
				}
				if(gtit != args(varid).end()-1) s << ",";
			}
			s << ")";
		}
	} else s << "var_" << varid;
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
		if(!b){
			return b;
		}
	}
	_grounding->closeTheory();
	return true;
}

bool SentenceGrounder::run() const {
	if(_verbosity > 1) clog << "Grounding sentence " << "\n";
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
			_grounding->add(cl);
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
			_grounding->add(cl);
		}
		return true;
	}
}

bool UnivSentGrounder::run() const {
	if(_verbosity > 1) clog << "Grounding a universally quantified sentence " << "\n";
	if(not _generator->first()) {
		if(_verbosity > 1){
			clog << "No instances for this sentence " << "\n";
		}
		return true;
	}
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
	return true;
}


/******************************
	GrounderFactory methods
******************************/

GrounderFactory::GrounderFactory(AbstractStructure* structure, Options* opts)
	: _structure(structure), _options(opts), _verbosity(opts->groundverbosity()), _cpsupport(opts->cpsupport()) {
}

set<const PFSymbol*> GrounderFactory::findCPSymbols(const AbstractTheory* theory) {
	Vocabulary* vocabulary = theory->vocabulary();
//	for(map<string,Predicate*>::const_iterator predit = vocabulary->firstpred(); predit != vocabulary->lastpred(); ++predit) {
//		Predicate* predicate = predit->second;
//		if(VocabularyUtils::isComparisonPredicate(predicate)) {
//			_cpsymbols.insert(predicate);
//		}
//	}
	for(map<string,Function*>::const_iterator funcit = vocabulary->firstfunc(); funcit != vocabulary->lastfunc(); ++funcit) {
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
		if(passtocp) _cpsymbols.insert(function);
	}
	if(_verbosity > 1) {
		clog << "User-defined symbols that can be handled by the constraint solver: ";
		for(set<const PFSymbol*>::const_iterator it = _cpsymbols.begin(); it != _cpsymbols.end(); ++it) {
			clog << (*it)->to_string() << " ";
		}
		clog << "\n";
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
	_context._funccontext	= Context::POSITIVE;
	_context._monotone		= Context::POSITIVE;
	_context._component		= CC_SENTENCE;
	_context._tseitin		= TsType::IMPL;
	_context._defined.clear();
}

void GrounderFactory::AggContext() {
	_context._truegen = false;
	_context._funccontext = Context::POSITIVE;
	_context._tseitin = (_context._tseitin == TsType::RULE) ? TsType::RULE : TsType::EQ;
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
void GrounderFactory::DeeperContext(SIGN sign) {
	// One level deeper
	if(_context._component == CC_SENTENCE) _context._component = CC_FORMULA;
	// Swap positive, truegen and tseitin according to sign
	if(isNeg(sign)) {
		_context._truegen = !_context._truegen;

		if(_context._funccontext == Context::POSITIVE) _context._funccontext = Context::NEGATIVE;
		else if(_context._funccontext == Context::NEGATIVE) _context._funccontext = Context::POSITIVE;
		if(_context._monotone == Context::POSITIVE) _context._monotone = Context::NEGATIVE;
		else if(_context._monotone == Context::NEGATIVE) _context._monotone = Context::POSITIVE;

		if(_context._tseitin == TsType::IMPL) _context._tseitin = TsType::RIMPL;
		else if(_context._tseitin == TsType::RIMPL) _context._tseitin = TsType::IMPL;

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

	// Find function that can be passed to CP solver.
	if(_cpsupport){
		findCPSymbols(theory);
	}

	// Create the grounder
	theory->accept(this);
	return _toplevelgrounder;
}

// TODO comment
TopLevelGrounder* GrounderFactory::create(const AbstractTheory* theory, InteractivePrintMonitor* monitor, Options* opts) {
	GroundTheory<PrintGroundPolicy>* groundtheory = new GroundTheory<PrintGroundPolicy>(_structure->clone());
	groundtheory->initialize(monitor, groundtheory->structure(), groundtheory->translator(), groundtheory->termtranslator(), opts);
	_grounding = groundtheory;

	// Find function that can be passed to CP solver.
	if(_cpsupport){
		findCPSymbols(theory);
	}

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
	if(_cpsupport){
		findCPSymbols(theory);
	}

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
	for(unsigned int n = 0; n < components.size(); ++n) {
		InitContext();
		if(_verbosity > 0) clog << "Creating a grounder for " << *(components[n]) << "\n";
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
 *		Each free variable that occurs in pf occurs in varmapping().
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
	Formula* transpf = FormulaUtils::moveThreeValuedTerms(newpf,_structure,(_context._funccontext != Context::NEGATIVE),_cpsupport,_cpsymbols);

	if(typeid(*transpf) != typeid(PredForm)) {	// The rewriting changed the atom
		if(_verbosity > 1) {
			clog << "Rewritten " << *pf << " to " << *transpf << "\n"; 
		}
		transpf->accept(this);
	}
	else {	// The rewriting did not change the atom
		PredForm* ptranspf = dynamic_cast<PredForm*>(transpf);
		// Create grounders for the subterms
		vector<TermGrounder*> subtermgrounders;
		vector<SortTable*>	  argsorttables;
		for(unsigned int n = 0; n < ptranspf->subterms().size(); ++n) {
			descend(ptranspf->subterms()[n]);
			subtermgrounders.push_back(_termgrounder);
			argsorttables.push_back(_structure->inter(ptranspf->symbol()->sorts()[n]));
		}
		// Create checkers and grounder
		if(_cpsupport && VocabularyUtils::isComparisonPredicate(ptranspf->symbol())) {
			string name = ptranspf->symbol()->name();
			CompType comp;
			if(name == "=/2")
				comp = isPos(pf->sign()) ? CompType::EQ : CompType::NEQ;
			else if(name == "</2")
				comp = isPos(pf->sign()) ? CompType::LT : CompType::GEQ;
			else if(name == ">/2")
				comp = isPos(pf->sign()) ? CompType::GT : CompType::LEQ;
			else {
				assert(false);
				comp = CompType::EQ;
			}
			_formgrounder = new ComparisonGrounder(_grounding->translator(),_grounding->termtranslator(),
									subtermgrounders[0],comp,subtermgrounders[1],_context);
			_formgrounder->setorig(ptranspf,varmapping(),_verbosity);
			if(_context._component == CC_SENTENCE) { 
				_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_verbosity);
			}
		}
		else {
			PredInter* inter = _structure->inter(ptranspf->symbol());
			CheckerFactory checkfactory;
			if(_context._component == CC_HEAD) {
				// Create instance checkers
				InstanceChecker* truech = checkfactory.create(inter,CERTAIN_TRUE);
				InstanceChecker* falsech = checkfactory.create(inter,CERTAIN_FALSE);
				// Create the grounder
				_headgrounder = new HeadGrounder(_grounding,truech,falsech,ptranspf->symbol(),subtermgrounders,argsorttables);
			}
			else {
				// Create instance checkers
				InstanceChecker* possch;
				InstanceChecker* certainch;
				if(_context._truegen == isPos(ptranspf->sign())) {
					possch = checkfactory.create(inter,POSS_TRUE);
					certainch = checkfactory.create(inter,CERTAIN_TRUE);
				} else {
					possch = checkfactory.create(inter,POSS_FALSE);
					certainch = checkfactory.create(inter,CERTAIN_FALSE);
				}
				// Create the grounder
				_formgrounder = new AtomGrounder(_grounding->translator(),ptranspf->sign(),ptranspf->symbol(),
										subtermgrounders,possch,certainch,argsorttables,_context);
				_formgrounder->setorig(ptranspf,varmapping(),_verbosity);
				if(_context._component == CC_SENTENCE) { 
					_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_verbosity);
				}
//XXX Old code XXX
//				if(_cpsupport && (typeid(*(ptranspf->symbol())) == typeid(Function)) && isCPFunction(ptranspf->symbol())) { 
//					Function* func = static_cast<Function*>(ptranspf->symbol());
//					_formgrounder = new CPAtomGrounder(_grounding->translator(),_grounding->termtranslator(),ptranspf->sign(),func,
//											subtermgrounders,possch,certainch,argsorttables,_context);
//				} 
//				else {
//					_formgrounder = new AtomGrounder(_grounding->translator(),ptranspf->sign(),ptranspf->symbol(),
//											subtermgrounders,possch,certainch,argsorttables,_context);
//				}
//XXX
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
 *		Each free variable that occurs in bf occurs in varmapping().
 * POSTCONDITIONS
 *		According to _context, the created grounder is assigned to
 *			CC_SENTENCE:	_toplevelgrounder
 *			CC_FORMULA:		_formgrounder
 *			CC_HEAD is not possible
 */
void GrounderFactory::visit(const BoolForm* bf) {
	// Handle a top-level conjunction without creating tseitin atoms
	if(_context._component == CC_SENTENCE && bf->isConjWithSign()) {
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
		if(recursive(bf)) _context._tseitin = TsType::RULE;
		_formgrounder = new BoolGrounder(_grounding->translator(),sub,bf->sign(),bf->conj(),_context);
		RestoreContext();
		_formgrounder->setorig(bf,varmapping(),_verbosity);
		if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_verbosity);

	}
}

const DomainElement** GrounderFactory::createVarMapping(Variable * const var){
	const DomainElement** d = new const DomainElement*();
	assert(varmapping().find(var)==varmapping().end());
	_varmapping[var] = d;
	return d;
}

/**
 * void GrounderFactory::visit(const QuantForm* qf)
 * DESCRIPTION
 *		Creates a grounder for a quantified formula
 * PARAMETERS
 *		qf	- the quantified formula
 * PRECONDITIONS
 *		Each free variable that occurs in qf occurs in varmapping().
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
	auto gen = createVarMapAndGenerator(qf->quantvars());

	// Check for infinite grounding
	for(auto it=tables.begin(); it<tables.end(); ++it){
		if(not (*it)->finite()) {
			cerr << "Warning: infinite grounding of formula ";
			if(qf->pi().original()) {
				cerr << *(qf->pi().original());
				cerr << "\n   internal representation: ";
			}
			cerr << *qf << "\n";
		}
	}

	// Handle top-level universal quantifiers efficiently
	if(_context._component == CC_SENTENCE && qf->isUnivWithSign()) {
		Formula* newsub = qf->subf()->clone();
		if(not qf->isUniv()){
			newsub->swapsign();
		}
		descend(newsub);
		newsub->recursiveDelete();
		_toplevelgrounder = new UnivSentGrounder(_grounding,_toplevelgrounder,gen,_verbosity);
	}
	else {
		// Create grounder for subformula
		SaveContext();
		DeeperContext(qf->sign());
		_context._truegen = not qf->isUniv();
		descend(qf->subf());
		RestoreContext();

		// Create the grounder
		SaveContext();
		if(recursive(qf)) _context._tseitin = TsType::RULE;

		bool canlazyground = false;
		if(not qf->isUniv() && _context._monotone==Context::POSITIVE && _context._tseitin==TsType::IMPL){
			canlazyground = true;
		}

		// FIXME better under-approximation of what to lazily ground
		if(_options->groundlazily() && canlazyground && typeid(*_grounding)==typeid(SolverTheory)){
			_formgrounder = new LazyQuantGrounder(qf->freevars(), dynamic_cast<SolverTheory*>(_grounding),_grounding->translator(),_formgrounder,qf->sign(),qf->quant(),gen,_context);
		}else{
			_formgrounder = new QuantGrounder(_grounding->translator(),_formgrounder,qf->sign(),qf->quant(),gen,_context);
		}

		RestoreContext();
		_formgrounder->setorig(qf,varmapping(),_verbosity);
		if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_verbosity);
	}
}

/**
 * void GrounderFactory::visit(const EquivForm* ef)
 * DESCRIPTION
 *		Creates a grounder for an equivalence.
 * PARAMETERS
 *		ef	- the equivalence
 * PRECONDITIONS
 *		Each free variable that occurs in ef occurs in varmapping().
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
	_context._funccontext = Context::BOTH;
	_context._monotone = Context::BOTH;
	_context._tseitin = TsType::EQ; 

	descend(ef->left());
	FormulaGrounder* leftg = _formgrounder;
	descend(ef->right());
	FormulaGrounder* rightg = _formgrounder;
	RestoreContext();

	// Create the grounder
	SaveContext();
	if(recursive(ef)) _context._tseitin = TsType::RULE;
	_formgrounder = new EquivGrounder(_grounding->translator(),leftg,rightg,ef->sign(),_context);
	RestoreContext();
	if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true,_verbosity);
}

/**
 * void GrounderFactory::visit(const AggForm* af)
 * DESCRIPTION
 * 		Creates a grounder for an aggregate.
 */
void GrounderFactory::visit(const AggForm* af) {
	AggForm* newaf = af->clone();
	Formula* transaf = FormulaUtils::moveThreeValuedTerms(newaf,_structure,(_context._funccontext != Context::NEGATIVE),_cpsupport,_cpsymbols);

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
		DeeperContext((not FormulaUtils::antimonotone(atransaf))?SIGN::POS:SIGN::NEG);
		descend(atransaf->right()->set());
		SetGrounder* setgr = _setgrounder;
		RestoreContext();
	
		// Create aggregate grounder
		SaveContext();
		if(recursive(atransaf)) _context._tseitin = TsType::RULE;
		_formgrounder = new AggGrounder(_grounding->translator(),_context,atransaf->right()->function(),setgr,boundgr,atransaf->comp(),atransaf->sign());
		RestoreContext();
		if(_context._component == CC_SENTENCE) _toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true,_verbosity);
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
	assert(varmapping().find(t->var()) != varmapping().end());
	_termgrounder = new VarTermGrounder(varmapping().find(t->var())->second);
	_termgrounder->setorig(t,varmapping(),_verbosity);
}

/**
 * void GrounderFactory::visit(const DomainTerm* t)
 * DESCRIPTION
 * 		Creates a grounder for a domain term.
 */
void GrounderFactory::visit(const DomainTerm* t) {
	_termgrounder = new DomTermGrounder(t->value());
	_termgrounder->setorig(t,varmapping(),_verbosity);
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
	FuncTable* ftable = _structure->inter(function)->functable();
	SortTable* domain = _structure->inter(function->outsort());
	if(_cpsupport && FuncUtils::isIntSum(function,_structure->vocabulary())) {
		_termgrounder = new SumTermGrounder(_grounding,_grounding->termtranslator(),ftable,domain,subtermgrounders[0],subtermgrounders[1]);
	} else {
		_termgrounder = new FuncTermGrounder(_grounding->termtranslator(),function,ftable,domain,subtermgrounders);
	}
	_termgrounder->setorig(t,varmapping(),_verbosity);
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
	_termgrounder->setorig(t,varmapping(),_verbosity);
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
	vector<SortTable*> vst;
	vector<const DomainElement**> vars;
	auto gen = createVarMapAndGenerator(s->quantvars());
	
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

template<class VarList>
InstGenerator* GrounderFactory::createVarMapAndGenerator(const VarList& vars){
	vector<SortTable*> hvst;
	vector<const DomainElement**> hvars;
	for(auto it=vars.begin(); it!=vars.end(); ++it) {
		auto domelem = createVarMapping(*it);
		hvars.push_back(domelem);
		auto sorttable = structure()->inter((*it)->sort());
		hvst.push_back(sorttable);
	}
	GeneratorFactory gf;
	return gf.create(hvars,hvst);
}

/**
 * void GrounderFactory::visit(const Rule* rule)
 * DESCRIPTION
 * 		Creates a grounder for a definitional rule.
 */
void GrounderFactory::visit(const Rule* rule) {
	// TODO for lazygroundrules, need a generator for all variables NOT occurring in the head!

	InstGenerator *headgen = NULL, *bodygen = NULL;

	if(_options->groundlazily()){ // FIXME check we also have the correct groundtheory!
		// for lazy ground rules, need a generator which generates bodies given a head, so only vars not occurring in the head!
		varlist bodyvars;
		for(auto it = rule->quantvars().begin(); it != rule->quantvars().end(); ++it) {
			if(not rule->head()->contains(*it)) {
				bodyvars.push_back(*it);
			}else{
				createVarMapping(*it);
			}
		}

		bodygen = createVarMapAndGenerator(bodyvars);
	}else{
		// Split the quantified variables in two categories:
		//		1. the variables that only occur in the head
		//		2. the variables that occur in the body (and possibly in the head)

		varlist	headvars;
		varlist	bodyvars;
		for(auto it = rule->quantvars().begin(); it != rule->quantvars().end(); ++it) {
			if(rule->body()->contains(*it)) {
				bodyvars.push_back(*it);
			}
			else {
				headvars.push_back(*it);
			}
		}

		headgen = createVarMapAndGenerator(headvars);
		bodygen = createVarMapAndGenerator(bodyvars);
	}

	// Create head grounder
	SaveContext();
	_context._component = CC_HEAD;
	descend(rule->head());
	HeadGrounder* headgr = _headgrounder;
	RestoreContext();

	// Create body grounder
	SaveContext();
	_context._funccontext = Context::NEGATIVE;		// minimize truth value of rule bodies
	_context._monotone = Context::POSITIVE;
	_context._truegen = true;				// body instance generator corresponds to an existential quantifier
	_context._component = CC_FORMULA;
	_context._tseitin = TsType::EQ;
	descend(rule->body());
	FormulaGrounder* bodygr = _formgrounder;
	RestoreContext();

	// Create rule grounder
	SaveContext();
	if(recursive(rule->body())) _context._tseitin = TsType::RULE;
	if(_options->groundlazily()){
		_rulegrounder = new LazyRuleGrounder(headgr,bodygr,bodygen,_context);
	}else{
		_rulegrounder = new RuleGrounder(headgr,bodygr,headgen,bodygen,_context);
	}
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

void LazyTsBody::notifyTheoryOccurence(){
	grounder_->notifyTheoryOccurence(inst);
}
