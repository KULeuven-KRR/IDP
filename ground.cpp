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
#include <cstdlib>
#include <utility> // for relational operators (namespace rel_ops)

#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "ecnf.hpp"
#include "options.hpp"
#include "generator.hpp"
#include "checker.hpp"
#include "common.hpp"
#include "GeneralUtils.hpp"
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

#include "fobdd.hpp"
#include "symbolicstructure.hpp"

using namespace std;
using namespace rel_ops;

double MCPA = 1;	// TODO: constant currently used when pruning bdds. Should be made context dependent

bool TsBody::operator==(const TsBody& body) const{
	if(typeid(*this)!=typeid(body)){
		return false;
	}
	return type()==body.type();
}
bool TsBody::operator<(const TsBody& body) const{
	if(typeid(*this).before(typeid(body))){
		return true;
	}else if(typeid(body).before(typeid(*this))){
		return false;
	}else if(type()<body.type()){
		return true;
	}else {
		return false;
	}
}

bool AggTsBody::operator==(const TsBody& body) const{
	if(not (*this==body)){
		return false;
	}
	auto rhs = dynamic_cast<const AggTsBody&>(body);
	return bound()==rhs.bound() && setnr()==rhs.setnr() && lower()==rhs.lower() && aggtype()==rhs.aggtype();
}
bool AggTsBody::operator<(const TsBody& body) const{
	if(TsBody::operator<(body)){
		return true;
	}else if(TsBody::operator>(body)){
		return false;
	}
	auto rhs = dynamic_cast<const AggTsBody&>(body);
	if(bound()<rhs.bound()){
		return true;
	}else if(bound()>rhs.bound()){
		return false;
	}
	if(lower()<rhs.lower()){
		return true;
	}else if(lower()>rhs.lower()){
		return false;
	}
	if(lower()<rhs.lower()){
		return true;
	}else if(lower()>rhs.lower()){
		return false;
	}
	if(aggtype()<rhs.aggtype()){
		return true;
	}
	return false;
}
bool PCTsBody::operator==(const TsBody& other) const{
	if(not TsBody::operator==(other)){
		return false;
	}
	auto rhs = dynamic_cast<const PCTsBody&>(other);
	return body()==rhs.body() && conj()==rhs.conj();
}
bool PCTsBody::operator<(const TsBody& other) const{
	if(TsBody::operator<(other)){
		return true;
	}else if(TsBody::operator>(other)){
		return false;
	}
	auto rhs = dynamic_cast<const PCTsBody&>(other);
	if(conj()<rhs.conj()){
		return true;
	}else if(conj()>rhs.conj()){
		return false;
	}
	if(body()<rhs.body()){
		return true;
	}
	return false;
}
bool CPTsBody::operator==(const TsBody& body) const{
	if(not TsBody::operator==(body)){
		return false;
	}
	auto rhs = dynamic_cast<const CPTsBody&>(body);
	return comp()==rhs.comp() && left()==rhs.left() && right()==rhs.right();
}
bool CPTsBody::operator<(const TsBody& body) const{
	if(TsBody::operator<(body)){
		return true;
	}else if(TsBody::operator>(body)){
		return false;
	}
	auto rhs = dynamic_cast<const CPTsBody&>(body);
	if(comp()<rhs.comp()){
		return true;
	}else if(comp()>rhs.comp()){
		return false;
	}
	if(left()<rhs.left()){
		return true;
	}else if(left()>rhs.left()){
		return false;
	}
	if(right()<rhs.right()){
		return true;
	}
	return false;
}
bool LazyTsBody::operator==(const TsBody& body) const{
	if(not TsBody::operator==(body)){
		return false;
	}
	auto rhs = dynamic_cast<const LazyTsBody&>(body);
	return id_==rhs.id_ && grounder_==rhs.grounder_ && (*inst)==(*rhs.inst);
}
bool LazyTsBody::operator<(const TsBody& body) const{
	if(TsBody::operator<(body)){
		return true;
	}else if(TsBody::operator>(body)){
		return false;
	}
	auto rhs = dynamic_cast<const LazyTsBody&>(body);
	if(id_<rhs.id_){
		return true;
	}else if(id_>rhs.id_){
		return false;
	}
	if(grounder_<rhs.grounder_){
		return true;
	}else if(grounder_>rhs.grounder_){
		return false;
	}
	if((*inst)==(*rhs.inst)){
		return true;
	}
	return false;
}

bool CPTerm::operator==(const CPTerm& body) const{
	return typeid(*this)==typeid(body);
}
bool CPTerm::operator<(const CPTerm& body) const{
	return typeid(*this).before(typeid(body));
}
bool CPVarTerm::operator==(const CPTerm& body) const{
	if(not CPTerm::operator==(body)){
		return false;
	}
	auto rhs = dynamic_cast<const CPVarTerm&>(body);
	return _varid==rhs._varid;
}
bool CPVarTerm::operator<(const CPTerm& body) const{
	if(CPTerm::operator<(body)){
		return true;
	}else if(CPTerm::operator>(body)){
		return false;
	}
	auto rhs = dynamic_cast<const CPVarTerm&>(body);
	if(_varid<rhs._varid){
		return true;
	}
	return false;
}
bool CPSumTerm::operator==(const CPTerm& body) const{
	if(not CPTerm::operator==(body)){
		return false;
	}
	auto rhs = dynamic_cast<const CPSumTerm&>(body);
	return _varids==rhs._varids;
}
bool CPSumTerm::operator<(const CPTerm& body) const{
	if(CPTerm::operator<(body)){
		return true;
	}else if(CPTerm::operator>(body)){
		return false;
	}
	auto rhs = dynamic_cast<const CPSumTerm&>(body);
	if(_varids<rhs._varids){
		return true;
	}
	return false;
}
bool CPWSumTerm::operator==(const CPTerm& body) const{
	if(not CPTerm::operator==(body)){
		return false;
	}
	auto rhs = dynamic_cast<const CPWSumTerm&>(body);
	return _varids==rhs._varids;
}
bool CPWSumTerm::operator<(const CPTerm& body) const{
	if(CPTerm::operator<(body)){
		return true;
	}else if(CPTerm::operator>(body)){
		return false;
	}
	auto rhs = dynamic_cast<const CPWSumTerm&>(body);
	if(_varids<rhs._varids){
		return true;
	}else if(_varids>rhs._varids){
		return false;
	}
	if(_weights<rhs._weights){
		return true;
	}
	return false;
}

bool CPBound::operator==(const CPBound& rhs) const{
	if(_isvarid!=rhs._isvarid){
		return false;
	}
	if(_isvarid){
		return _varid==rhs._varid;
	}else{
		return _bound==rhs._bound;
	}
}
bool CPBound::operator<(const CPBound& rhs) const{
	if(_isvarid<rhs._isvarid){
		return true;
	}else if(_isvarid>rhs._isvarid){
		return false;
	}
	if(_isvarid){
		return _varid<rhs._varid;
	}else{
		return _bound<rhs._bound;
	}
}


/*********************************************
	Translate from ground atoms to numbers
*********************************************/

GroundTranslator::~GroundTranslator() {
	deleteList<SymbolAndTuple>(atom2Tuple);
	for(auto i = atom2TsBody.begin(); i < atom2TsBody.end(); ++i){
		delete((*i).second);
	}
}

Lit GroundTranslator::translate(unsigned int n, const ElementTuple& args) {
	Lit lit = 0;
	auto jt = symbols[n].tuple2atom.lower_bound(args);
	if(jt != symbols[n].tuple2atom.end() && jt->first == args) {
		lit = jt->second;
	} else {
		lit = nextNumber(AtomType::INPUT);
		symbols[n].tuple2atom.insert(jt, Tuple2Atom(args,lit));
		atom2Tuple[lit] = new SymbolAndTuple(symbols[n].symbol, args);

		// FIXME expensive operation to do so often!
		auto rulesit = symbol2rulegrounder.find(n);
		if(rulesit!=symbol2rulegrounder.end() && rulesit->second.size()>0){
			(*rulesit->second.begin())->notify(lit, args, rulesit->second);
		}
	}

	return lit;
}

Lit GroundTranslator::translate(PFSymbol* s, const ElementTuple& args) {
	unsigned int offset = addSymbol(s);
	return translate(offset,args);
}

Lit GroundTranslator::translate(const vector<Lit>& clause, bool conj, TsType tstype) {
	int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	return translate(nr, clause, conj, tstype);
}

Lit GroundTranslator::translate(const Lit& head, const vector<Lit>& clause, bool conj, TsType tstype) {
	PCTsBody* tsbody = new PCTsBody(tstype,clause,conj);
	atom2TsBody[head] = tspair(head,tsbody);
	return head;
}

// Adds a tseitin body only if it does not yet exist. TODO why does this seem only relevant for CP Terms?
Lit GroundTranslator::addTseitinBody(TsBody* tsbody){
// FIXME optimization: check whether the same comparison has already been added and reuse the tseitin.
/*	auto it = _tsbodies2nr.lower_bound(tsbody);

	if(it != _tsbodies2nr.end() && *(it->first) == *tsbody) { // Already exists
		delete tsbody;
		return it->second;
	}*/

	int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	atom2TsBody[nr] = tspair(nr, tsbody);
	return nr;
}

void GroundTranslator::notifyDefined(PFSymbol* pfs, LazyRuleGrounder* const grounder){
	int symbolnumber = addSymbol(pfs);
	auto it = symbol2rulegrounder.find(symbolnumber);
	if(symbol2rulegrounder.find(symbolnumber)==symbol2rulegrounder.end()){
		it = symbol2rulegrounder.insert(pair<uint, std::vector<LazyRuleGrounder*> >(symbolnumber, {})).first;
	}
	for(auto grounderit = it->second.begin(); grounderit<it->second.end(); ++grounderit){
		if(grounder==*grounderit){
			return;
		}
	}
	it->second.push_back(grounder);
}

void GroundTranslator::translate(LazyQuantGrounder const* const lazygrounder, ResidualAndFreeInst* instance, TsType tstype) {
	instance->residual = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	LazyTsBody* tsbody = new LazyTsBody(lazygrounder->id(), lazygrounder, instance, tstype);
	atom2TsBody[instance->residual] = tspair(instance->residual, tsbody);
}

Lit	GroundTranslator::translate(double bound, CompType comp, bool strict, AggFunction aggtype, int setnr, TsType tstype) {
	if(comp == CompType::EQ) {
		vector<int> cl(2);
		cl[0] = translate(bound,CompType::LT,false,aggtype,setnr,tstype);
		cl[1] = translate(bound,CompType::GT,false,aggtype,setnr,tstype);
		return translate(cl,true,tstype);
	}
	else {
		Lit head = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
		AggTsBody* tsbody = new AggTsBody(tstype,bound,(comp == CompType::LT),aggtype,setnr);
		if(strict) {
			#warning "This is wrong if floating point weights are allowed!";
			tsbody->setBound((comp == CompType::LT) ? bound + 1 : bound - 1);
		}else{
			tsbody->setBound(bound);
		}
		atom2TsBody[head] = tspair(head,tsbody);
		return head;
	}
}

Lit GroundTranslator::translate(CPTerm* left, CompType comp, const CPBound& right, TsType tstype) {
	CPTsBody* tsbody = new CPTsBody(tstype,left,comp,right);
	// FIXME optimization: check whether the same comparison has already been added and reuse the tseitin.
	// => this should be generalized to sharing detection!
/*	auto it = lower_bound(atom2TsBody.begin(), atom2TsBody.end(), tspair(0,tsbody), compareTsPair);
	if(it != atom2TsBody.end() && (*it).second == *tsbody) {
		delete tsbody;
		return (*it).first;
	}
	else {*/
		int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
		atom2TsBody[nr] = tspair(nr, tsbody);
		return nr;
	//}
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

Lit GroundTranslator::nextNumber(AtomType type) {
	if(_freenumbers.empty()) {
		Lit atom = atomtype.size();
		atom2TsBody.push_back(tspair(atom,(TsBody*)NULL));
		atom2Tuple.push_back(NULL);
		atomtype.push_back(type);
		return atom;
	} else {
		int nr = _freenumbers.front();
		_freenumbers.pop();
		return nr;
	}
}

unsigned int GroundTranslator::addSymbol(PFSymbol* pfs) {
	for(unsigned int n = 0; n < symbols.size(); ++n){
		if(symbols[n].symbol == pfs){
			return n;
		}
	}
	symbols.push_back(SymbolAndAtomMap(pfs));
	return symbols.size()-1;
}

string GroundTranslator::printAtom(const Lit& atom, bool longnames) const {
	stringstream s;
	uint nr = atom;
	if(nr == _true) { return "true"; }
	if(nr == _false) { return "false"; }
	if(not isStored(nr)) {
		return "error";
	}

	switch(atomtype[nr]){
		case AtomType::INPUT: {
			PFSymbol* pfs = getSymbol(nr);
			s << pfs->toString(longnames);
			auto tuples = getArgs(nr);
			if(not tuples.empty()) {
				s << "(";
				bool begin = true;
				for(auto i = tuples.begin(); i!=tuples.end(); ++i){
					if(not begin){ s <<", "; }
					begin = false;
					s << (*i)->toString();
				}
				s << ")";
			}
			break;
		}
		case AtomType::TSEITINWITHSUBFORMULA:
			s << "tseitin_" << nr;
			break;
		case AtomType::LONETSEITIN:
			s << "tseitin_" << nr;
			break;
	}
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

string GroundTermTranslator::printTerm(const VarId& varid, bool longnames) const {
	stringstream s;
	if(varid >= _varid2function.size()) { return "error"; }
	const Function* func = function(varid);
	if(func) {
		s << func->toString(longnames);
		if(not args(varid).empty()) {
			s << "(";
			for(auto gtit = args(varid).begin(); gtit != args(varid).end(); ++gtit) {
				if((*gtit)._isvarid) {
					s << printTerm((*gtit)._varid,longnames);
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

GrounderFactory::GrounderFactory(AbstractStructure* structure, Options* opts, SymbolicStructure* symstructure)
	: _structure(structure), _symstructure(symstructure), _options(opts), 
		_verbosity(opts->getValue(IntType::GROUNDVERBOSITY)), _cpsupport(opts->getValue(BoolType::CPSUPPORT)),
		_longnames(opts->getValue(BoolType::LONGNAMES)) {

	// Create a symbolic structure if no such structure is given
	if(_symstructure == NULL) {
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
//	for(auto predit = vocabulary->firstpred(); predit != vocabulary->lastpred(); ++predit) {
//		Predicate* predicate = predit->second;
//		if(VocabularyUtils::isComparisonPredicate(predicate)) {
//			_cpsymbols.insert(predicate);
//		}
//	}
	for(auto funcit = vocabulary->firstFunc(); funcit != vocabulary->lastFunc(); ++funcit) {
		Function* function = funcit->second;
		bool passtocp = false;
		// Check whether the (user-defined) function's outsort is over integers
		Sort* intsort = *(vocabulary->sort("int")->begin());
		if(function->overloaded()) {
			set<Function*> nonbuiltins = function->nonbuiltins();
			for(auto nbfit = nonbuiltins.begin(); nbfit != nonbuiltins.end(); ++nbfit) {
				passtocp = (SortUtils::resolve(function->outsort(),intsort,vocabulary) == intsort);
			}
		} else if(not function->builtin()) {
			passtocp = (SortUtils::resolve(function->outsort(),intsort,vocabulary) == intsort);
		}
		if(passtocp) { _cpsymbols.insert(function); }
	}
	if(_verbosity > 1) {
		clog << "User-defined symbols that can be handled by the constraint solver: ";
		for(auto it = _cpsymbols.begin(); it != _cpsymbols.end(); ++it) {
			clog << (*it)->toString(false) << " "; // TODO longnames?
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
	for(auto it = _context._defined.begin(); it != _context._defined.end(); ++it) {
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
	_context._funccontext	= Context::POSITIVE;
	_context._monotone		= Context::POSITIVE;
	_context._component		= CC_SENTENCE;
	_context._tseitin		= _options->getValue(MODELCOUNTEQUIVALENCE)?TsType::EQ:TsType::IMPL;
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
	if(_context._component == CC_SENTENCE) { _context._component = CC_FORMULA; }
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
	Formula* transpf = FormulaUtils::moveThreeValuedTerms(newpf,_structure,_context._funccontext,_cpsupport,_cpsymbols);
	transpf = FormulaUtils::removeEqChains(transpf);
	if(not _cpsupport) { 
		transpf = FormulaUtils::graphFunctions(transpf); 
	}

	if(typeid(*transpf) != typeid(PredForm)) {	// The rewriting changed the atom
		if(_verbosity > 1) {
			clog << "Rewritten "; pf->put(clog,_longnames);
			clog << " to "; transpf->put(clog,_longnames);
		   	clog << "\n"; 
		}
		transpf->accept(this);
	}
	else {	// The rewriting did not change the atom
		PredForm* newpf = dynamic_cast<PredForm*>(transpf);
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
			_formgrounder->setOrig(newpf,varmapping(),_verbosity);
			if(_context._component == CC_SENTENCE) { 
				_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_verbosity);
			}
		}
		else {
			PredInter* inter = _structure->inter(newpf->symbol());
			CheckerFactory checkfactory;
			if(_context._component == CC_HEAD) {
				// Create instance checkers
				InstanceChecker* truech = checkfactory.create(inter,CERTAIN_TRUE);
				InstanceChecker* falsech = checkfactory.create(inter,CERTAIN_FALSE);
				// Create the grounder
				_headgrounder = new HeadGrounder(_grounding,truech,falsech,newpf->symbol(),subtermgrounders,argsorttables);
			}
			else {
				// Create instance checkers
				vector<Sort*> checksorts;
				vector<const DomElemContainer*> checkargs;
				vector<SortTable*> tables;
				for(auto it = newpf->subterms().begin(); it != newpf->subterms().end(); ++it) {
					checksorts.push_back((*it)->sort());
					checkargs.push_back(new const DomElemContainer());
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
				//FIXME restore: PredTable* posstable = new PredTable(new BDDInternalPredTable(possbdd,_symstructure->manager(),fovars,_structure),Universe(tables));
				//FIXME restore: PredTable* certtable = new PredTable(new BDDInternalPredTable(certbdd,_symstructure->manager(),fovars,_structure),Universe(tables));
				PredTable* posstable = new PredTable(new FullInternalPredTable(),Universe(tables));
				PredTable* certtable = new PredTable(new InverseInternalPredTable(new FullInternalPredTable()),Universe(tables));
				InstGenerator* possch = GeneratorFactory::create(posstable,vector<bool>(checkargs.size(),true),checkargs,Universe(tables));
				InstGenerator* certainch = GeneratorFactory::create(certtable,vector<bool>(checkargs.size(),true),checkargs,Universe(tables));
				// Create the grounder
// FIXME verify use of newpf and transpf
				_formgrounder = new AtomGrounder(_grounding->translator(),newpf->sign(),newpf->symbol(),
										subtermgrounders,checkargs, possch,certainch,inter, argsorttables,_context);
				_formgrounder->setOrig(newpf,varmapping(),_verbosity);
				if(_context._component == CC_SENTENCE) { 
					_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_verbosity);
				}
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
		if(not newbf->conj()) {
			newbf->conj(true);
			newbf->negate();
			for(auto it = newbf->subformulas().begin(); it != newbf->subformulas().end(); ++it) {
				(*it)->negate();
			}
		}
		// Visit the subformulas
		vector<TopLevelGrounder*> sub;
		for(auto it = newbf->subformulas().begin(); it != newbf->subformulas().end(); ++it) {
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
		for(auto it = bf->subformulas().begin(); it != bf->subformulas().end(); ++it) {
			descend(*it);
			sub.push_back(_formgrounder);
		}
		RestoreContext();

		// Create grounder
		SaveContext();
		if(recursive(bf)) { _context._tseitin = TsType::RULE; }
		_formgrounder = new BoolGrounder(_grounding->translator(),sub,bf->sign(),bf->conj(),_context);
		RestoreContext();
		_formgrounder->setOrig(bf,_varmapping,_verbosity);
		if(_context._component == CC_SENTENCE) { 
			_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,false,_verbosity);
		}
	}
}

const DomElemContainer* GrounderFactory::createVarMapping(Variable * const var){
	const DomElemContainer* d = new DomElemContainer();
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
	Formula* clonedformula = qf->subformula()->clone();
	Formula* movedformula = FormulaUtils::moveThreeValuedTerms(clonedformula,_structure,_context._funccontext);
	movedformula = FormulaUtils::removeEqChains(movedformula);
	movedformula = FormulaUtils::graphFunctions(movedformula);
	const FOBDD* generatorbdd = _symstructure->evaluate(movedformula,(qf->isUniv() ? QT_PF : QT_PT));
	const FOBDD* checkerbdd = _symstructure->evaluate(movedformula,(qf->isUniv() ? QT_CF : QT_CT));

	vector<const DomElemContainer*> vars;
	vector<SortTable*>	tables;

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

// TODO verify and combine with createVarMapAndG
	//	auto gen = createVarMapAndGenerator(qf->quantVars());
	vector<Variable*> fovars;
	vector<Variable*> optivars;
	vector<bool> pattern;
	for(auto it = movedformula->freeVars().begin(); it != movedformula->freeVars().end(); ++it) {
		if(qf->quantVars().find(*it) == qf->quantVars().end()) {
			assert(_varmapping.find(*it) != _varmapping.end());
			vars.push_back(_varmapping[*it]);
			pattern.push_back(true);
		}
		else {
			const DomElemContainer* d = new const DomElemContainer();
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
	InstGenerator* gen = GeneratorFactory::create(gentable,pattern,vars,Universe(tables));
	InstGenerator* check = GeneratorFactory::create(checktable,vector<bool>(vars.size(),true),vars,Universe(tables));

	// Handle top-level universal quantifiers efficiently
	if(_context._component == CC_SENTENCE && qf->isUnivWithSign()) {
		Formula* newsub = qf->subformula()->clone();
		if(not qf->isUniv()) { newsub->negate(); }
		descend(newsub);
		newsub->recursiveDelete();
		_toplevelgrounder = new UnivSentGrounder(_grounding,_toplevelgrounder,gen,_verbosity);
	}
	else {
		// Create grounder for subformula
		SaveContext();
		DeeperContext(qf->sign());
		_context._truegen = !(qf->isUniv());
		descend(qf->subformula());
		RestoreContext();

		// Create the grounder
		SaveContext();
		if(recursive(qf)) { _context._tseitin = TsType::RULE; }

		bool canlazyground = false;
		if(not qf->isUniv() && _context._monotone==Context::POSITIVE && _context._tseitin==TsType::IMPL){
			canlazyground = true;
		}

		// FIXME better under-approximation of what to lazily ground
		if(_options->getValue(BoolType::GROUNDLAZILY) && canlazyground && typeid(*_grounding)==typeid(SolverTheory)){
			_formgrounder = new LazyQuantGrounder(qf->freeVars(), dynamic_cast<SolverTheory*>(_grounding),_grounding->translator(),_formgrounder,qf->sign(),qf->quant(),gen, check,_context);
		}else{
			_formgrounder = new QuantGrounder(_grounding->translator(),_formgrounder,qf->sign(),qf->quant(),gen,check,_context);
		}

		RestoreContext();
		_formgrounder->setOrig(qf,_varmapping,_verbosity);
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
	if(recursive(ef)){
		_context._tseitin = TsType::RULE;
	}
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
	AggForm* newaf = af->clone();
	Formula* transaf = FormulaUtils::moveThreeValuedTerms(newaf,_structure,_context._funccontext,_cpsupport,_cpsymbols);

	if(typeid(*transaf) != typeid(AggForm)) {	// The rewriting changed the atom
		if(_verbosity > 1) {
			clog << "Rewritten "; af->put(clog,_longnames);
			clog << " to "; transaf->put(clog,_longnames);
		   	clog << "\n"; 
		}
		transaf->accept(this);
	}
	else {	// The rewriting did not change the atom
		AggForm* newaf = dynamic_cast<AggForm*>(transaf);
		// Create grounder for the bound
		descend(newaf->left());
		TermGrounder* boundgr = _termgrounder;
	
		// Create grounder for the set
		SaveContext();
		if(recursive(newaf)){
			assert(FormulaUtils::isMonotone(newaf) || FormulaUtils::isAntimonotone(newaf));
		}
		DeeperContext((not FormulaUtils::isAntimonotone(newaf))?SIGN::POS:SIGN::NEG);
		descend(newaf->right()->set());
		SetGrounder* setgr = _setgrounder;
		RestoreContext();

// FIXME check correctnes of sign and comp combination!	
		// Create aggregate grounder
		SaveContext();
		if(recursive(newaf)){
			_context._tseitin = TsType::RULE;
		}
		if(isNeg(newaf->sign())) {
			if(_context._tseitin == TsType::IMPL) { _context._tseitin = TsType::RIMPL; }
			else if(_context._tseitin == TsType::RIMPL) { _context._tseitin = TsType::IMPL; }
		}
		_formgrounder = new AggGrounder(_grounding->translator(),_context,newaf->right()->function(),setgr,boundgr,newaf->comp(),newaf->sign());
		RestoreContext();
		if(_context._component == CC_SENTENCE) {
			_toplevelgrounder = new SentenceGrounder(_grounding,_formgrounder,true,_verbosity);
		}
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
	assert(varmapping().find(t->var()) != varmapping().end());
	_termgrounder = new VarTermGrounder(varmapping().find(t->var())->second);
	_termgrounder->setOrig(t,varmapping(),_verbosity);
}

/**
 * void GrounderFactory::visit(const DomainTerm* t)
 * DESCRIPTION
 * 		Creates a grounder for a domain term.
 */
void GrounderFactory::visit(const DomainTerm* t) {
	_termgrounder = new DomTermGrounder(t->value());
	_termgrounder->setOrig(t,varmapping(),_verbosity);
}

/**
 * void GrounderFactory::visit(const FuncTerm* t)
 * DESCRIPTION
 * 		Creates a grounder for a function term.
 */
void GrounderFactory::visit(const FuncTerm* t) {
	// Create grounders for subterms
	vector<TermGrounder*> subtermgrounders;
	for(auto it = t->subterms().begin(); it != t->subterms().end(); ++it) {
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
	_termgrounder->setOrig(t,varmapping(),_verbosity);
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
	_termgrounder->setOrig(t,varmapping(),_verbosity);
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
	// TODO Move three-valued terms in the set expression
	//SetExpr* transqs = TermUtils::moveThreeValuedTerms(qs->clone(),_structure,_context._funccontext,_cpsupport,_cpsymbols);
	//QuantSetExpr* newqs = dynamic_cast<QuantSetExpr*>(transqs);

	//if(_verbosity > 1) {
	//	clog << "Rewritten "; qs->put(clog,_longnames);
	//	clog << " to "; newqs->put(clog,_longnames);
	//   	clog << "\n"; 
	//}

	// Create instance generator
	vector<SortTable*> vst;
	Formula* clonedformula = qs->subformulas()[0]->clone();
	Formula* movedformula = FormulaUtils::moveThreeValuedTerms(clonedformula,_structure,Context::POSITIVE);
	movedformula = FormulaUtils::removeEqChains(movedformula);
	movedformula = FormulaUtils::graphFunctions(movedformula);
	const FOBDD* generatorbdd = _symstructure->evaluate(movedformula,QT_PT);
	const FOBDD* checkerbdd = _symstructure->evaluate(movedformula,QT_CT);
	vector<const DomElemContainer*> vars;
// FIXME	auto gen = createVarMapAndGenerator(qs->quantVars());
	vector<Variable*> fovars;
	vector<Variable*> optivars;
	vector<SortTable*> tables;
	vector<bool> pattern;
	for(auto it = movedformula->freeVars().begin(); it != movedformula->freeVars().end(); ++it) {
		if(qs->quantVars().find(*it) == qs->quantVars().end()) {
			vars.push_back(_varmapping[*it]);
			pattern.push_back(true);
		}
		else {
			const DomElemContainer* d = new const DomElemContainer();
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
	InstGenerator* gen = GeneratorFactory::create(gentable,pattern,vars,Universe(tables));
	InstGenerator* check = GeneratorFactory::create(checktable,vector<bool>(vars.size(),true),vars,Universe(tables));

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
	for(auto it = def->defsymbols().begin(); it != def->defsymbols().end(); ++it) {
		_context._defined.insert(*it);
	}
	
	// Create rule grounders
	vector<RuleGrounder*> subgrounders;
	for(auto it = def->rules().begin(); it != def->rules().end(); ++it) {
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
	vector<const DomElemContainer*> hvars;
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
	// FIXME Move all three-valued terms outside the head
	// TODO for lazygroundrules, need a generator for all variables NOT occurring in the head!

	InstGenerator *headgen = NULL, *bodygen = NULL;

	if(_options->getValue(BoolType::GROUNDLAZILY)){ // FIXME check we also have the correct groundtheory!
		// for lazy ground rules, need a generator which generates bodies given a head, so only vars not occurring in the head!
		varlist bodyvars;
		for(auto it = rule->quantVars().begin(); it != rule->quantVars().end(); ++it) {
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
		for(auto it = rule->quantVars().begin(); it != rule->quantVars().end(); ++it) {
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
	if(_options->getValue(BoolType::GROUNDLAZILY)){
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
