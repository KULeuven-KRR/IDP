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
#include "generators/GeneratorFactory.hpp"
#include "generators/InstGenerator.hpp"
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

#include "generators/BasicGenerators.hpp"
#include "generators/TableGenerator.hpp"

#include "utils/TheoryUtils.hpp"

#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "symbolicstructure.hpp"

using namespace std;
using namespace rel_ops;

GenType operator not(GenType orig) {
	switch (orig) {
		case GenType::CANMAKEFALSE:
			return GenType::CANMAKETRUE;
		case GenType::CANMAKETRUE:
			return GenType::CANMAKEFALSE;
	}
}

double MCPA = 1; // TODO: constant currently used when pruning bdds. Should be made context dependent

bool TsBody::operator==(const TsBody& body) const {
	if (typeid(*this) != typeid(body)) {
		return false;
	}
	return type() == body.type();
}

bool TsBody::operator<(const TsBody& body) const {
	if (typeid(*this).before(typeid(body))) {
		return true;
	} else if (typeid(body).before(typeid(*this))) {
		return false;
	} else if (type() < body.type()) {
		return true;
	} else {
		return false;
	}
}

bool AggTsBody::operator==(const TsBody& body) const {
	if (not (*this == body)) {
		return false;
	}
	auto rhs = dynamic_cast<const AggTsBody&>(body);
	return bound() == rhs.bound() && setnr() == rhs.setnr() && lower() == rhs.lower() && aggtype() == rhs.aggtype();
}
bool AggTsBody::operator<(const TsBody& body) const {
	if (TsBody::operator<(body)) {
		return true;
	} else if (TsBody::operator>(body)) {
		return false;
	}
	auto rhs = dynamic_cast<const AggTsBody&>(body);
	if (bound() < rhs.bound()) {
		return true;
	} else if (bound() > rhs.bound()) {
		return false;
	}
	if (lower() < rhs.lower()) {
		return true;
	} else if (lower() > rhs.lower()) {
		return false;
	}
	if (lower() < rhs.lower()) {
		return true;
	} else if (lower() > rhs.lower()) {
		return false;
	}
	if (aggtype() < rhs.aggtype()) {
		return true;
	}
	return false;
}
bool PCTsBody::operator==(const TsBody& other) const {
	if (not TsBody::operator==(other)) {
		return false;
	}
	auto rhs = dynamic_cast<const PCTsBody&>(other);
	return body() == rhs.body() && conj() == rhs.conj();
}
bool PCTsBody::operator<(const TsBody& other) const {
	if (TsBody::operator<(other)) {
		return true;
	} else if (TsBody::operator>(other)) {
		return false;
	}
	auto rhs = dynamic_cast<const PCTsBody&>(other);
	if (conj() < rhs.conj()) {
		return true;
	} else if (conj() > rhs.conj()) {
		return false;
	}
	if (body() < rhs.body()) {
		return true;
	}
	return false;
}

bool CPTsBody::operator==(const TsBody& body) const {
	if (not TsBody::operator==(body)) {
		return false;
	}
	auto rhs = dynamic_cast<const CPTsBody&>(body);
	return comp() == rhs.comp() && left() == rhs.left() && right() == rhs.right();
}
bool CPTsBody::operator<(const TsBody& body) const {
	if (TsBody::operator<(body)) {
		return true;
	} else if (TsBody::operator>(body)) {
		return false;
	}
	auto rhs = dynamic_cast<const CPTsBody&>(body);
	if (comp() < rhs.comp()) {
		return true;
	} else if (comp() > rhs.comp()) {
		return false;
	}
	if (left() < rhs.left()) {
		return true;
	} else if (left() > rhs.left()) {
		return false;
	}
	if (right() < rhs.right()) {
		return true;
	}
	return false;
}
/*bool LazyTsBody::operator==(const TsBody& body) const {
	if (not TsBody::operator==(body)) {
		return false;
	}
	auto rhs = dynamic_cast<const LazyTsBody&>(body);
	return id_ == rhs.id_ && grounder_ == rhs.grounder_ && (*inst) == (*rhs.inst);
}*/
/*bool LazyTsBody::operator<(const TsBody& body) const {
	if (TsBody::operator<(body)) {
		return true;
	} else if (TsBody::operator>(body)) {
		return false;
	}
	auto rhs = dynamic_cast<const LazyTsBody&>(body);
	if (id_ < rhs.id_) {
		return true;
	} else if (id_ > rhs.id_) {
		return false;
	}
	if (grounder_ < rhs.grounder_) {
		return true;
	} else if (grounder_ > rhs.grounder_) {
		return false;
	}
	if ((*inst) == (*rhs.inst)) {
		return true;
	}
	return false;
}*/

bool CPTerm::operator==(const CPTerm& body) const {
	return typeid(*this) == typeid(body);
}

bool CPTerm::operator<(const CPTerm& body) const {
	return typeid(*this).before(typeid(body));
}

bool CPVarTerm::operator==(const CPTerm& body) const {
	if (not CPTerm::operator==(body)) {
		return false;
	}
	auto rhs = dynamic_cast<const CPVarTerm&>(body);
	return _varid == rhs._varid;
}
bool CPVarTerm::operator<(const CPTerm& body) const {
	if (CPTerm::operator<(body)) {
		return true;
	} else if (CPTerm::operator>(body)) {
		return false;
	}
	auto rhs = dynamic_cast<const CPVarTerm&>(body);
	if (_varid < rhs._varid) {
		return true;
	}
	return false;
}
bool CPSumTerm::operator==(const CPTerm& body) const {
	if (not CPTerm::operator==(body)) {
		return false;
	}
	auto rhs = dynamic_cast<const CPSumTerm&>(body);
	return _varids == rhs._varids;
}
bool CPSumTerm::operator<(const CPTerm& body) const {
	if (CPTerm::operator<(body)) {
		return true;
	} else if (CPTerm::operator>(body)) {
		return false;
	}
	auto rhs = dynamic_cast<const CPSumTerm&>(body);
	if (_varids < rhs._varids) {
		return true;
	}
	return false;
}
bool CPWSumTerm::operator==(const CPTerm& body) const {
	if (not CPTerm::operator==(body)) {
		return false;
	}
	auto rhs = dynamic_cast<const CPWSumTerm&>(body);
	return _varids == rhs._varids;
}
bool CPWSumTerm::operator<(const CPTerm& body) const {
	if (CPTerm::operator<(body)) {
		return true;
	} else if (CPTerm::operator>(body)) {
		return false;
	}
	auto rhs = dynamic_cast<const CPWSumTerm&>(body);
	if (_varids < rhs._varids) {
		return true;
	} else if (_varids > rhs._varids) {
		return false;
	}
	if (_weights < rhs._weights) {
		return true;
	}
	return false;
}

bool CPBound::operator==(const CPBound& rhs) const {
	if (_isvarid != rhs._isvarid) {
		return false;
	}
	if (_isvarid) {
		return _varid == rhs._varid;
	} else {
		return _bound == rhs._bound;
	}
}
bool CPBound::operator<(const CPBound& rhs) const {
	if (_isvarid < rhs._isvarid) {
		return true;
	} else if (_isvarid > rhs._isvarid) {
		return false;
	}
	if (_isvarid) {
		return _varid < rhs._varid;
	} else {
		return _bound < rhs._bound;
	}
}

/*********************************************
	Translate from ground atoms to numbers
*********************************************/

GroundTranslator::~GroundTranslator() {
	deleteList<SymbolAndTuple>(atom2Tuple);
	for (auto i = atom2TsBody.cbegin(); i < atom2TsBody.cend(); ++i) {
		delete ((*i).second);
	}
}

Lit GroundTranslator::translate(unsigned int n, const ElementTuple& args) {
	Lit lit = 0;
	auto jt = symbols[n].tuple2atom.lower_bound(args);
	if (jt != symbols[n].tuple2atom.cend() && jt->first == args) {
		lit = jt->second;
	} else {
		lit = nextNumber(AtomType::INPUT);
		symbols[n].tuple2atom.insert(jt, Tuple2Atom(args, lit));
		atom2Tuple[lit] = new SymbolAndTuple(symbols[n].symbol, args);

		// FIXME expensive operation to do so often!
		auto rulesit = symbol2rulegrounder.find(n);
		if (rulesit != symbol2rulegrounder.cend() && rulesit->second.size() > 0) {
			(*rulesit->second.cbegin())->notify(lit, args, rulesit->second);
		}
	}

	return lit;
}

Lit GroundTranslator::translate(PFSymbol* s, const ElementTuple& args) {
	unsigned int offset = addSymbol(s);
	return translate(offset, args);
}

Lit GroundTranslator::translate(const vector<Lit>& clause, bool conj, TsType tstype) {
	int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	return translate(nr, clause, conj, tstype);
}

Lit GroundTranslator::translate(const Lit& head, const vector<Lit>& clause, bool conj, TsType tstype) {
	PCTsBody* tsbody = new PCTsBody(tstype, clause, conj);
	atom2TsBody[head] = tspair(head, tsbody);
	return head;
}

// Adds a tseitin body only if it does not yet exist. TODO why does this seem only relevant for CP Terms?
Lit GroundTranslator::addTseitinBody(TsBody* tsbody) {
// FIXME optimization: check whether the same comparison has already been added and reuse the tseitin.
	/*	auto it = _tsbodies2nr.lower_bound(tsbody);

	 if(it != _tsbodies2nr.cend() && *(it->first) == *tsbody) { // Already exists
	 delete tsbody;
	 return it->second;
	 }*/

	int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	atom2TsBody[nr] = tspair(nr, tsbody);
	return nr;
}

void GroundTranslator::notifyDefined(PFSymbol* pfs, LazyRuleGrounder* const grounder) {
	int symbolnumber = addSymbol(pfs);
	auto it = symbol2rulegrounder.find(symbolnumber);
	if (symbol2rulegrounder.find(symbolnumber) == symbol2rulegrounder.cend()) {
		it = symbol2rulegrounder.insert(pair<uint, std::vector<LazyRuleGrounder*> >(symbolnumber, { })).first;
	}
	for (auto grounderit = it->second.cbegin(); grounderit < it->second.cend(); ++grounderit) {
		if (grounder == *grounderit) {
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

Lit GroundTranslator::translate(double bound, CompType comp, bool strict, AggFunction aggtype, int setnr, TsType tstype) {
	if (comp == CompType::EQ) {
		vector<int> cl(2);
		cl[0] = translate(bound, CompType::LT, false, aggtype, setnr, tstype);
		cl[1] = translate(bound, CompType::GT, false, aggtype, setnr, tstype);
		return translate(cl, true, tstype);
	} else {
		Lit head = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
		AggTsBody* tsbody = new AggTsBody(tstype, bound, (comp == CompType::LT), aggtype, setnr);
		if (strict) {
#warning "This is wrong if floating point weights are allowed!";
			tsbody->setBound((comp == CompType::LT) ? bound + 1 : bound - 1);
		} else {
			tsbody->setBound(bound);
		}
		atom2TsBody[head] = tspair(head, tsbody);
		return head;
	}
}

Lit GroundTranslator::translate(CPTerm* left, CompType comp, const CPBound& right, TsType tstype) {
	CPTsBody* tsbody = new CPTsBody(tstype, left, comp, right);
	// FIXME optimization: check whether the same comparison has already been added and reuse the tseitin.
	// => this should be generalized to sharing detection!
	/*	auto it = lower_bound(atom2TsBody.cbegin(), atom2TsBody.cend(), tspair(0,tsbody), compareTsPair);
	 if(it != atom2TsBody.cend() && (*it).second == *tsbody) {
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
	if (_freesetnumbers.empty()) {
		TsSet newset;
		setnr = _sets.size();
		_sets.push_back(newset);
		TsSet& tsset = _sets.back();

		tsset._setlits = lits;
		tsset._litweights = weights;
		tsset._trueweights = trueweights;
	} else {
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
	if (_freenumbers.empty()) {
		Lit atom = atomtype.size();
		atom2TsBody.push_back(tspair(atom, (TsBody*) NULL));
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
	for (unsigned int n = 0; n < symbols.size(); ++n) {
		if (symbols[n].symbol == pfs) {
			return n;
		}
	}
	symbols.push_back(SymbolAndAtomMap(pfs));
	return symbols.size() - 1;
}

string GroundTranslator::printAtom(const Lit& atom, bool longnames) const {
	stringstream s;
	uint nr = atom;
	if (nr == _true) {
		return "true";
	}
	if (nr == _false) {
		return "false";
	}
	if (not isStored(nr)) {
		return "error";
	}

	switch (atomtype[nr]) {
		case AtomType::INPUT: {
			PFSymbol* pfs = getSymbol(nr);
			s << pfs->toString(longnames);
			auto tuples = getArgs(nr);
			if (not tuples.empty()) {
				s << "(";
				bool begin = true;
				for (auto i = tuples.cbegin(); i != tuples.cend(); ++i) {
					if (not begin) {
						s << ", ";
					}
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
	if (a.isVariable == b.isVariable) {
		return a.isVariable ? (a._varid == b._varid) : (a._domelement == b._domelement);
	}
	return false;
}

bool operator<(const GroundTerm& a, const GroundTerm& b) {
	if (a.isVariable == b.isVariable) {
		return a.isVariable ? (a._varid < b._varid) : (a._domelement < b._domelement);
	}
	// GroundTerms with a domain element come before GroundTerms with a CP variable identifier.
	return (a.isVariable < b.isVariable);
}

VarId GroundTermTranslator::translate(size_t offset, const vector<GroundTerm>& args) {
	map<vector<GroundTerm>, VarId>::iterator it = _functerm2varid_table[offset].lower_bound(args);
	if (it != _functerm2varid_table[offset].cend() && it->first == args) {
		return it->second;
	} else {
		VarId varid = nextNumber();
		_functerm2varid_table[offset].insert(it, pair<vector<GroundTerm>, VarId>(args, varid));
		_varid2function[varid] = _offset2function[offset];
		_varid2args[varid] = args;
		_varid2domain[varid] = _structure->inter(_offset2function[offset]->outsort());
		return varid;
	}
}

VarId GroundTermTranslator::translate(Function* function, const vector<GroundTerm>& args) {
	size_t offset = addFunction(function);
	return translate(offset, args);
}

VarId GroundTermTranslator::translate(CPTerm* cpterm, SortTable* domain) {
	VarId varid = nextNumber();
	CPBound bound(varid);
	CPTsBody* cprelation = new CPTsBody(TsType::EQ, cpterm, CompType::EQ, bound);
	_varid2cprelation.insert(pair<VarId, CPTsBody*>(varid, cprelation));
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
	CPTsBody* cprelation = new CPTsBody(TsType::EQ, cpterm, CompType::EQ, bound);
	_varid2cprelation.insert(pair<VarId, CPTsBody*>(varid, cprelation));
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
	map<Function*, size_t>::const_iterator found = _function2offset.find(func);
	if (found != _function2offset.cend()) {
		// Simply return number when function is already known
		return found->second;
	} else {
		// Add function and number when function is unknown
		size_t offset = _offset2function.size();
		_function2offset[func] = offset;
		_offset2function.push_back(func);
		_functerm2varid_table.push_back(map<vector<GroundTerm>, VarId>());
		return offset;
	}
}

string GroundTermTranslator::printTerm(const VarId& varid, bool longnames) const {
	stringstream s;
	if (varid >= _varid2function.size()) {
		return "error";
	}
	const Function* func = function(varid);
	if (func) {
		s << func->toString(longnames);
		if (not args(varid).empty()) {
			s << "(";
			for (auto gtit = args(varid).cbegin(); gtit != args(varid).cend(); ++gtit) {
				if ((*gtit).isVariable) {
					s << printTerm((*gtit)._varid, longnames);
				} else {
					s << (*gtit)._domelement->toString();
				}
				if (gtit != args(varid).cend() - 1) {
					s << ",";
				}
			}
			s << ")";
		}
	} else {
		s << "var_" << varid;
	}
	return s.str();
}

/******************************
	GrounderFactory methods
******************************/

GrounderFactory::GrounderFactory(AbstractStructure* structure, Options* opts, SymbolicStructure* symstructure) :
		_structure(structure), _symstructure(symstructure), _options(opts), _verbosity(opts->getValue(IntType::GROUNDVERBOSITY)), _cpsupport(
				opts->getValue(BoolType::CPSUPPORT)), _longnames(opts->getValue(BoolType::LONGNAMES)) {

	// Create a symbolic structure if no such structure is given
	if (_symstructure == NULL) {
		FOBDDManager* manager = new FOBDDManager();
		std::map<PFSymbol*, const FOBDD*> ctbounds;
		std::map<PFSymbol*, const FOBDD*> cfbounds;
		std::map<PFSymbol*, std::vector<const FOBDDVariable*> > vars;
		Vocabulary* vocabulary = structure->vocabulary();
		for (auto it = vocabulary->firstPred(); it != vocabulary->lastPred(); ++it) {
			set<Predicate*> sp = it->second->nonbuiltins();
			for (auto jt = sp.cbegin(); jt != sp.cend(); ++jt) {
				PredInter* pinter = structure->inter(*jt);
				if (not pinter->approxTwoValued()) {
					vector<Variable*> pvars = VarUtils::makeNewVariables((*jt)->sorts());
					vector<const FOBDDVariable*> pbddvars(pvars.size());
					vector<const FOBDDArgument*> pbddargs(pvars.size());
					for (size_t n = 0; n < pvars.size(); ++n) {
						const FOBDDVariable* bddvar = manager->getVariable(pvars[n]);
						pbddvars[n] = bddvar;
						pbddargs[n] = bddvar;
					}
					vars[*jt] = pbddvars;
					const FOBDDKernel* ctkernel = manager->getAtomKernel(*jt, AtomKernelType::AKT_CT, pbddargs);
					const FOBDDKernel* cfkernel = manager->getAtomKernel(*jt, AtomKernelType::AKT_CF, pbddargs);
					ctbounds[*jt] = manager->getBDD(ctkernel, manager->truebdd(), manager->falsebdd());
					cfbounds[*jt] = manager->getBDD(cfkernel, manager->truebdd(), manager->falsebdd());
				}
			}
		}
		for (auto it = vocabulary->firstFunc(); it != vocabulary->lastFunc(); ++it) {
			set<Function*> sf = it->second->nonbuiltins();
			for (auto jt = sf.cbegin(); jt != sf.cend(); ++jt) {
				PredInter* pinter = structure->inter(*jt)->graphInter();
				if (not pinter->approxTwoValued()) {
					vector<Variable*> pvars = VarUtils::makeNewVariables((*jt)->sorts());
					vector<const FOBDDVariable*> pbddvars(pvars.size());
					vector<const FOBDDArgument*> pbddargs(pvars.size());
					for (size_t n = 0; n < pvars.size(); ++n) {
						const FOBDDVariable* bddvar = manager->getVariable(pvars[n]);
						pbddvars[n] = bddvar;
						pbddargs[n] = bddvar;
					}
					vars[*jt] = pbddvars;
					const FOBDDKernel* ctkernel = manager->getAtomKernel(*jt, AtomKernelType::AKT_CT, pbddargs);
					const FOBDDKernel* cfkernel = manager->getAtomKernel(*jt, AtomKernelType::AKT_CF, pbddargs);
					ctbounds[*jt] = manager->getBDD(ctkernel, manager->truebdd(), manager->falsebdd());
					cfbounds[*jt] = manager->getBDD(cfkernel, manager->truebdd(), manager->falsebdd());
				}
			}
		}
		_symstructure = new SymbolicStructure(manager, ctbounds, cfbounds, vars);
	}

	if (_verbosity > 2) {
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
	for (auto funcit = vocabulary->firstFunc(); funcit != vocabulary->lastFunc(); ++funcit) {
		Function* function = funcit->second;
		bool passtocp = false;
		// Check whether the (user-defined) function's outsort is over integers
		Sort* intsort = *(vocabulary->sort("int")->begin());
		if (function->overloaded()) {
			set<Function*> nonbuiltins = function->nonbuiltins();
			for (auto nbfit = nonbuiltins.cbegin(); nbfit != nonbuiltins.cend(); ++nbfit) {
				passtocp = (SortUtils::resolve(function->outsort(), intsort, vocabulary) == intsort);
			}
		} else if (not function->builtin()) {
			passtocp = (SortUtils::resolve(function->outsort(), intsort, vocabulary) == intsort);
		}
		if (passtocp) {
			_cpsymbols.insert(function);
		}
	}
	if (_verbosity > 1) {
		clog << "User-defined symbols that can be handled by the constraint solver: ";
		for (auto it = _cpsymbols.cbegin(); it != _cpsymbols.cend(); ++it) {
			clog << (*it)->toString(false) << " "; // TODO longnames?
		}
		clog << "\n";
	}
	return _cpsymbols;
}

bool GrounderFactory::isCPSymbol(const PFSymbol* symbol) const {
	return VocabularyUtils::isComparisonPredicate(symbol) || (_cpsymbols.find(symbol) != _cpsymbols.cend());
}

/**
 * bool GrounderFactory::recursive(const Formula*)
 * DESCRIPTION
 * 		Finds out whether a formula contains recursively defined symbols.
 */
bool GrounderFactory::recursive(const Formula* f) {
	for (auto it = _context._defined.cbegin(); it != _context._defined.cend(); ++it) {
		if (f->contains(*it)) {
			return true;
		}
	}
	return false;
}

/**
 * void GrounderFactory::InitContext()
 * DESCRIPTION
 *		Initializes the context of the GrounderFactory before visiting a sentence.
 */
void GrounderFactory::InitContext() {
	_context.gentype = GenType::CANMAKEFALSE;
	_context._funccontext = Context::POSITIVE;
	_context._monotone = Context::POSITIVE;
	_context._component = CompContext::SENTENCE;
	_context._tseitin = _options->getValue(MODELCOUNTEQUIVALENCE) ? TsType::EQ : TsType::IMPL;
	_context._defined.clear();
	_context._conjunctivePathFromRoot = true; // NOTE: default true: needs to be set to false in each visit in grounderfactory in which it is no longer the case
	_context._conjPathUntilNode = true;
}

void GrounderFactory::AggContext() {
	_context.gentype = GenType::CANMAKEFALSE;
	_context._funccontext = Context::POSITIVE;
	_context._tseitin = (_context._tseitin == TsType::RULE) ? TsType::RULE : TsType::EQ;
	_context._component = CompContext::FORMULA;
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
	if (_context._component == CompContext::SENTENCE) {
		_context._component = CompContext::FORMULA;
	}

	// If the parent was no longer conjunctive, the new node also won't be
	if(not _context._conjPathUntilNode){
		_context._conjunctivePathFromRoot = false;
	}

	// Swap positive, truegen and tseitin according to sign
	if (isNeg(sign)) {
		_context.gentype = not _context.gentype;

		if (_context._funccontext == Context::POSITIVE)
			_context._funccontext = Context::NEGATIVE;
		else if (_context._funccontext == Context::NEGATIVE)
			_context._funccontext = Context::POSITIVE;
		if (_context._monotone == Context::POSITIVE)
			_context._monotone = Context::NEGATIVE;
		else if (_context._monotone == Context::NEGATIVE)
			_context._monotone = Context::POSITIVE;

		if (_context._tseitin == TsType::IMPL)
			_context._tseitin = TsType::RIMPL;
		else if (_context._tseitin == TsType::RIMPL)
			_context._tseitin = TsType::IMPL;

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
Grounder* GrounderFactory::create(const AbstractTheory* theory) {
	// Allocate an ecnf theory to be returned by the grounder
	GroundTheory<GroundPolicy>* groundtheory = new GroundTheory<GroundPolicy>(theory->vocabulary(), _structure->clone());
	_grounding = groundtheory;

	// Find functions that can be passed to CP solver.
	if (_cpsupport) {
		findCPSymbols(theory);
	}

	// Create the grounder
	theory->accept(this);
	return _topgrounder;
}

// TODO comment
Grounder* GrounderFactory::create(const AbstractTheory* theory, InteractivePrintMonitor* monitor, Options* opts) {
	GroundTheory<PrintGroundPolicy>* groundtheory = new GroundTheory<PrintGroundPolicy>(_structure->clone());
	groundtheory->initialize(monitor, groundtheory->structure(), groundtheory->translator(), groundtheory->termtranslator(), opts);
	_grounding = groundtheory;

	// Find functions that can be passed to CP solver.
	if (_cpsupport) {
		findCPSymbols(theory);
	}

	// Create the grounder
	theory->accept(this);
	return _topgrounder;
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
Grounder* GrounderFactory::create(const AbstractTheory* theory, SATSolver* solver) {
	// Allocate a solver theory
	GroundTheory<SolverPolicy>* groundtheory = new GroundTheory<SolverPolicy>(theory->vocabulary(), _structure->clone());
	groundtheory->initialize(solver, _verbosity, groundtheory->termtranslator());
	_grounding = groundtheory;

	// Find function that can be passed to CP solver.
	if (_cpsupport) {
		findCPSymbols(theory);
	}

	// Create the grounder
	theory->accept(this);
	return _topgrounder;
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
	vector<TheoryComponent*> components(tcomps.cbegin(), tcomps.cend());

	// Order components the components to optimize the grounding process
	// TODO

	InitContext();

	// Create grounders for all components
	vector<Grounder*> children(components.size());
	for (size_t n = 0; n < components.size(); ++n) {
		if (_verbosity > 0) {
			clog << "Creating a grounder for ";
			components[n]->put(clog, _longnames);
			clog << "\n";
		}
		components[n]->accept(this);
		children[n] = _topgrounder;
	}

	_topgrounder = new BoolGrounder(_grounding, children, SIGN::POS, true, _context);
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
 *			CompContext::SENTENCE:	_toplevelgrounder
 *			CompContext::HEAD:		_headgrounder
 *			CompContext::FORMULA:		_formgrounder
 */
void GrounderFactory::visit(const PredForm* pf) {
	_context._conjunctivePathFromRoot = false;

	// Move all functions and aggregates that are three-valued according
	// to _structure outside the atom. To avoid changing the original atom,
	// we first clone it.
	if (_verbosity > 3) {
		clog << pf->toString() << " (Original)" <<"\n";
	}
	// FIXME verkeerde type afgeleid voor vergelijkingen a=b (zou bvb range die beide omvat moeten zijn, is nu niet het geval).
	// FIXME aggregaten moeten correct worden herschreven als ze niet tweewaardig zijn
	Formula* transpf = FormulaUtils::unnestThreeValuedTerms(pf->clone(), _structure, _context._funccontext, _cpsupport, _cpsymbols);
	if (_verbosity > 3) {
		clog << transpf->toString() << " (3-valued terms moved)" <<"\n";
	}
	transpf = FormulaUtils::splitComparisonChains(transpf, NULL);
	if (_verbosity > 3) {
		clog << transpf->toString() << " (Comparison chains split)" <<"\n";
	}
	if (not _cpsupport) { // TODO Check not present in quantgrounder
		// NOTE: Graph aggregates before graphing functions! Ambiguity in (FuncTerm = AggTerm).
		transpf = FormulaUtils::graphAggregates(transpf); // FIXME where does this all have to be added
		transpf = FormulaUtils::graphFunctions(transpf);
		if (_verbosity > 3) {
			clog << transpf->toString() << " (Aggregates and functions graphed)" <<"\n";
		}
	}

	if (not sametypeid<PredForm>(*transpf)) { // The rewriting changed the atom
		if (_verbosity > 1) {
			clog << "Rewritten " << pf->toString() << " to " << transpf->toString() << "\n";
		}
		transpf->accept(this);
		transpf->recursiveDelete();
		return;
	}

	PredForm* newpf = dynamic_cast<PredForm*>(transpf);

	// Create grounders for the subterms
	vector<TermGrounder*> subtermgrounders;
	vector<SortTable*> argsorttables;
	for (auto n = 0; n < newpf->subterms().size(); ++n) {
		//if (_context._conjunctivePathFromRoot) { // FIXME
		//	_context._conjunctivePathFromRoot = true;
		//}
		descend(newpf->subterms()[n]);
		subtermgrounders.push_back(_termgrounder);
		argsorttables.push_back(_structure->inter(newpf->symbol()->sorts()[n]));
	}

	// Create checkers and grounder
	if (_cpsupport && VocabularyUtils::isComparisonPredicate(newpf->symbol())) {
		string name = newpf->symbol()->name();
		CompType comp;
		if (name == "=/2") {
			comp = isPos(pf->sign()) ? CompType::EQ : CompType::NEQ;
		} else if (name == "</2") {
			comp = isPos(pf->sign()) ? CompType::LT : CompType::GEQ;
		} else {
			assert(name == ">/2");
			comp = isPos(pf->sign()) ? CompType::GT : CompType::LEQ;
		}

		_formgrounder = new ComparisonGrounder(_grounding, _grounding->termtranslator(), subtermgrounders[0], comp, subtermgrounders[1], _context);
		_formgrounder->setOrig(newpf, varmapping(), _verbosity);
		if (_context._component == CompContext::SENTENCE) { // TODO Refactor outside?
			_topgrounder = _formgrounder;
		}
		return;
	}

	if (_context._component == CompContext::HEAD) {
		PredInter* inter = _structure->inter(newpf->symbol());
		CheckerFactory checkfactory;
		InstanceChecker* truech = checkfactory.create(inter, TruthType::CERTAIN_TRUE);
		InstanceChecker* falsech = checkfactory.create(inter, TruthType::CERTAIN_FALSE);
		_headgrounder = new HeadGrounder(_grounding, truech, falsech, newpf->symbol(), subtermgrounders, argsorttables);
		return;
	}

	// Grounding basic predform

	// Create instance checkers
	vector<Sort*> checksorts;
	vector<const DomElemContainer*> checkargs; // Set by grounder, then used by checkers
	vector<SortTable*> tables;
	// NOTE: order is important!
	for (auto it = newpf->subterms().cbegin(); it != newpf->subterms().cend(); ++it) {
		checksorts.push_back((*it)->sort());
		checkargs.push_back(new const DomElemContainer());
		tables.push_back(_structure->inter((*it)->sort()));
	}

	// TODO enable bdds after debugging
/*	vector<Variable*> fovars = VarUtils::makeNewVariables(checksorts);
	 vector<Term*> foterms = TermUtils::makeNewVarTerms(fovars);
	 PredForm* checkpf = new PredForm(newpf->sign(),newpf->symbol(),foterms,FormulaParseInfo());
	 const FOBDD* possbdd;
	 const FOBDD* certbdd;
	 if(_context.gentype == GenType::CANMAKETRUE) { //TOdo refactor
		 possbdd = _symstructure->evaluate(checkpf,TruthType::POSS_TRUE);
		 certbdd = _symstructure->evaluate(checkpf,TruthType::CERTAIN_TRUE);
	 } else {
		 possbdd = _symstructure->evaluate(checkpf,TruthType::POSS_FALSE);
		 certbdd = _symstructure->evaluate(checkpf,TruthType::CERTAIN_FALSE);
	 }

	 PredTable* posstable = new PredTable(new BDDInternalPredTable(possbdd,_symstructure->manager(),fovars,_structure),Universe(tables));
	 PredTable* certtable = new PredTable(new BDDInternalPredTable(certbdd,_symstructure->manager(),fovars,_structure),Universe(tables));*/
	PredTable* posstable = new PredTable(new FullInternalPredTable(), Universe(tables));
	PredTable* certtable = new PredTable(new InverseInternalPredTable(new FullInternalPredTable()), Universe(tables));
	/*cerr <<"Certainly table: \n";
	certtable->print(std::cerr);
	cerr <<"\nPossible table: \n";
	posstable->print(std::cerr);
	cerr <<"\n";*/
	InstChecker* possch = GeneratorFactory::create(posstable, vector<Pattern>(checkargs.size(), Pattern::INPUT), checkargs, Universe(tables));
	InstChecker* certainch = GeneratorFactory::create(certtable, vector<Pattern>(checkargs.size(), Pattern::INPUT), checkargs, Universe(tables));

	_formgrounder = new AtomGrounder(_grounding, newpf->sign(), newpf->symbol(), subtermgrounders, checkargs, possch, certainch,
			_structure->inter(newpf->symbol()), argsorttables, _context);

	_formgrounder->setOrig(newpf, varmapping(), _verbosity);
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = _formgrounder;
	}
	newpf->recursiveDelete();
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
 *			CompContext::SENTENCE:	_toplevelgrounder
 *			CompContext::FORMULA:		_formgrounder
 *			CompContext::HEAD is not possible
 */
void GrounderFactory::visit(const BoolForm* bf) {
	// Handle a top-level conjunction without creating tseitin atoms
	if (_context._component == CompContext::SENTENCE && bf->isConjWithSign()) {  // FIXME resolve in clean way with conjpathtoroot
		// If bf is a negated disjunction, push the negation one level deeper.
		// Take a clone to avoid changing bf;
		BoolForm* newbf = bf->clone();
		if (not newbf->conj()) {
			newbf->conj(true);
			newbf->negate();
			for (auto it = newbf->subformulas().cbegin(); it != newbf->subformulas().cend(); ++it) {
				(*it)->negate();
			}
		}
		// Visit the subformulas
		vector<Grounder*> sub;
		for (auto it = newbf->subformulas().cbegin(); it != newbf->subformulas().cend(); ++it) {
			descend(*it);
			sub.push_back(_topgrounder);
		}
		newbf->recursiveDelete();
		_topgrounder = new BoolGrounder(_grounding, sub, SIGN::POS, true, _context);
	} else { // Formula bf is not a top-level conjunction
		// Create grounders for subformulas
		SaveContext();
		if(not bf->isConjWithSign()){
			_context._conjPathUntilNode = false;
		}
		DeeperContext(bf->sign());
		vector<Grounder*> sub;
		for (auto it = bf->subformulas().cbegin(); it != bf->subformulas().cend(); ++it) {
			descend(*it);
			sub.push_back(_formgrounder);
			//TODO: here we could check for true/false formulas.  Useful?
		}
		RestoreContext();

		// Create grounder
		SaveContext();
		if(not bf->isConjWithSign()){
			_context._conjPathUntilNode = false;
		}
		if (recursive(bf)) {
			_context._tseitin = TsType::RULE;
		}
		_formgrounder = new BoolGrounder(_grounding, sub, bf->sign(), bf->conj(), _context);
		RestoreContext();
		_formgrounder->setOrig(bf, _varmapping, _verbosity);
		if (_context._component == CompContext::SENTENCE) {
			_topgrounder = _formgrounder;
		}
	}
}

const DomElemContainer* GrounderFactory::createVarMapping(Variable * const var) {
	const DomElemContainer* d = new DomElemContainer();
	assert(varmapping().find(var)==varmapping().cend());
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
 *			CompContext::SENTENCE:	_toplevelgrounder
 *			CompContext::FORMULA:		_formgrounder
 *			CompContext::HEAD is not possible
 */
void GrounderFactory::visit(const QuantForm* qf) {
	// TODO guarantee that e.g. no more double negations exist? => FLAGS bijhouden van wat er met de theorie gebeurd

	// Create instance generator
	Formula* newsubformula = qf->subformula()->clone();
	newsubformula = FormulaUtils::unnestThreeValuedTerms(newsubformula, _structure, _context._funccontext);
	newsubformula = FormulaUtils::splitComparisonChains(newsubformula, NULL);
	newsubformula = FormulaUtils::graphFunctions(newsubformula);

	// NOTE: if the checker return valid, then the value of the formula can be decided from the value of the checked instantiation
	//	for universal: checker valid => formula false, for existential: checker valid => formula true

	// !x phi(x) => generate all x possibly false
	// !x phi(x) => check for x certainly false
	// FIXME SUBFORMULA got cloned, not the formula itself! REVIEW CODE!

	GenAndChecker gc = createVarsAndGenerators(newsubformula, qf, qf->isUniv() ? TruthType::POSS_FALSE : TruthType::POSS_TRUE,
			qf->isUniv() ? TruthType::CERTAIN_FALSE : TruthType::CERTAIN_TRUE);

	// Create grounder for subformula
	SaveContext();
	if(not qf->isUnivWithSign()){
		_context._conjPathUntilNode = false;
	}
	DeeperContext(qf->sign());
	_context.gentype = qf->isUniv() ? GenType::CANMAKEFALSE : GenType::CANMAKETRUE;
	descend(qf->subformula());
	RestoreContext();

	// Create the grounder
	SaveContext();
	if (recursive(qf)) {
		_context._tseitin = TsType::RULE;
	}
	if(not qf->isUnivWithSign()){
		_context._conjPathUntilNode = false;
	}

	bool canlazyground = false;
	if (not qf->isUniv() && _context._monotone == Context::POSITIVE && _context._tseitin == TsType::IMPL) {
		canlazyground = true;
	}

	// FIXME add better under-approximation of what to lazily ground
	if (_options->getValue(BoolType::GROUNDLAZILY) && canlazyground && typeid(*_grounding) == typeid(SolverTheory)) {
		_formgrounder = new LazyQuantGrounder(qf->freeVars(), dynamic_cast<SolverTheory*>(_grounding), _formgrounder,
				qf->sign(), qf->quant(), gc._generator, gc._checker, _context);
	} else {
		_formgrounder = new QuantGrounder(_grounding, _formgrounder, qf->sign(), qf->quant(), gc._generator, gc._checker, _context);
	}
	RestoreContext();

	_formgrounder->setOrig(qf, _varmapping, _verbosity);

	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = _formgrounder;
	}

	newsubformula->recursiveDelete();
}

const FOBDD* GrounderFactory::improve_generator(const FOBDD* bdd, const vector<Variable*>& fovars, double mcpa) {
	FOBDDManager* manager = _symstructure->manager();

	/*cerr << "improving\n";
	 manager->put(cerr,bdd);
	 set<Variable*> sv(fovars.cbegin(),fovars.cend());
	 set<const FOBDDVariable*> sfv = manager->getVariables(sv);
	 set<const FOBDDDeBruijnIndex*> id;
	 cerr << "current cost = " << manager->estimatedCostAll(bdd,sfv,id,_structure) << endl;
	 */
	// 1. Optimize the query
	FOBDDManager optimizemanager;
	const FOBDD* copybdd = optimizemanager.getBDD(bdd, manager);
	set<const FOBDDVariable*> copyvars;
	set<const FOBDDDeBruijnIndex*> indices;
	for (auto it = fovars.cbegin(); it != fovars.cend(); ++it) {
		copyvars.insert(optimizemanager.getVariable(*it));
	}
	optimizemanager.optimizequery(copybdd, copyvars, indices, _structure);
	/*
	 cerr << "optimized version\n";
	 optimizemanager.put(cerr,copybdd);
	 sfv = optimizemanager.getVariables(sv);
	 cerr << "cost is now: " << optimizemanager.estimatedCostAll(copybdd,sfv,id,_structure) << endl;
	 */

	// 2. Remove certain leaves
	const FOBDD* pruned = optimizemanager.make_more_true(copybdd, copyvars, indices, _structure, mcpa);
	/*
	 cerr << "pruned version\n";
	 optimizemanager.put(cerr,pruned);
	 */

	// 3. Replace result
	return manager->getBDD(pruned, &optimizemanager);
}

const FOBDD* GrounderFactory::improve_checker(const FOBDD* bdd, double mcpa) {
	FOBDDManager* manager = _symstructure->manager();

	// 1. Optimize the query
	FOBDDManager optimizemanager;
	const FOBDD* copybdd = optimizemanager.getBDD(bdd, manager);
	set<const FOBDDVariable*> copyvars;
	set<const FOBDDDeBruijnIndex*> indices;
	optimizemanager.optimizequery(copybdd, copyvars, indices, _structure);

	// 2. Remove certain leaves
	const FOBDD* pruned = optimizemanager.make_more_false(copybdd, copyvars, indices, _structure, mcpa);

	// 3. Replace result
	return manager->getBDD(pruned, &optimizemanager);
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
 *			CompContext::SENTENCE:	_toplevelgrounder
 *			CompContext::FORMULA:		_formgrounder
 *			CompContext::HEAD is not possible
 */
void GrounderFactory::visit(const EquivForm* ef) {
	_context._conjunctivePathFromRoot = false;

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
	if (recursive(ef)) {
		_context._tseitin = TsType::RULE;
	}
	_formgrounder = new EquivGrounder(_grounding, leftg, rightg, ef->sign(), _context);
	RestoreContext();
	if (_context._component == CompContext::SENTENCE) {
		_topgrounder = _formgrounder;
	}
}

void GrounderFactory::visit(const AggForm* af) {
	_context._conjunctivePathFromRoot = false;

	AggForm* newaf = af->clone();
	Formula* transaf = FormulaUtils::unnestThreeValuedTerms(newaf, _structure, _context._funccontext, _cpsupport, _cpsymbols);

	if (typeid(*transaf) != typeid(AggForm)) { // The rewriting changed the atom
		if (_verbosity > 1) {
			clog << "Rewritten ";
			af->put(clog, _longnames);
			clog << " to ";
			transaf->put(clog, _longnames);
			clog << "\n";
		}
		transaf->accept(this);
	} else { // The rewriting did not change the atom
		AggForm* newaf = dynamic_cast<AggForm*>(transaf);
		// Create grounder for the bound
		descend(newaf->left());
		TermGrounder* boundgr = _termgrounder;

		// Create grounder for the set
		SaveContext();
		if (recursive(newaf)) {
			assert(FormulaUtils::isMonotone(newaf) || FormulaUtils::isAntimonotone(newaf));
		}
		DeeperContext((not FormulaUtils::isAntimonotone(newaf)) ? SIGN::POS : SIGN::NEG);
		descend(newaf->right()->set());
		SetGrounder* setgr = _setgrounder;
		RestoreContext();

		// Create aggregate grounder
		SaveContext();
		if (recursive(newaf)) {
			_context._tseitin = TsType::RULE;
		}
		if (isNeg(newaf->sign())) {
			if (_context._tseitin == TsType::IMPL) {
				_context._tseitin = TsType::RIMPL;
			} else if (_context._tseitin == TsType::RIMPL) {
				_context._tseitin = TsType::IMPL;
			}
		}
		_formgrounder = new AggGrounder(_grounding, _context, newaf->right()->function(), setgr, boundgr, newaf->comp(), newaf->sign());
		RestoreContext();
		if (_context._component == CompContext::SENTENCE) {
			_topgrounder = _formgrounder;
		}
	}
	transaf->recursiveDelete();
}

void GrounderFactory::visit(const EqChainForm* ef) {
	_context._conjunctivePathFromRoot = false;

	Formula* f = ef->clone();
	f = FormulaUtils::splitComparisonChains(f, _grounding->vocabulary());
	f->accept(this);
	f->recursiveDelete();
}

void GrounderFactory::visit(const VarTerm* t) {
	_context._conjunctivePathFromRoot = false;

	assert(varmapping().find(t->var()) != varmapping().cend());
	_termgrounder = new VarTermGrounder(varmapping().find(t->var())->second);
	_termgrounder->setOrig(t, varmapping(), _verbosity);
}

void GrounderFactory::visit(const DomainTerm* t) {
	_context._conjunctivePathFromRoot = false;

	_termgrounder = new DomTermGrounder(t->value());
	_termgrounder->setOrig(t, varmapping(), _verbosity);
}

/**
 * void GrounderFactory::visit(const FuncTerm* t)
 * DESCRIPTION
 * 		Creates a grounder for a function term.
 */
void GrounderFactory::visit(const FuncTerm* t) {
	_context._conjunctivePathFromRoot = false;

	// Create grounders for subterms
	vector<TermGrounder*> subtermgrounders;
	for (auto it = t->subterms().cbegin(); it != t->subterms().cend(); ++it) {
		(*it)->accept(this);
		if (_termgrounder)
			subtermgrounders.push_back(_termgrounder);
	}

	// Create term grounder
	Function* function = t->function();
	FuncTable* ftable = _structure->inter(function)->funcTable();
	SortTable* domain = _structure->inter(function->outsort());
	if (_cpsupport && FuncUtils::isIntSum(function, _structure->vocabulary())) {
		if (function->name() == "-/2") {
			_termgrounder = new SumTermGrounder(_grounding, _grounding->termtranslator(), ftable, domain, subtermgrounders[0], subtermgrounders[1],
					ST_MINUS);
		} else {
			_termgrounder = new SumTermGrounder(_grounding, _grounding->termtranslator(), ftable, domain, subtermgrounders[0], subtermgrounders[1]);
		}
	} else {
		_termgrounder = new FuncTermGrounder(_grounding->termtranslator(), function, ftable, domain, subtermgrounders);
	}
	_termgrounder->setOrig(t, varmapping(), _verbosity);
}

/**
 * void GrounderFactory::visit(const AggTerm* at)
 * DESCRIPTION
 * 		Creates a grounder for a aggregate term.
 */
void GrounderFactory::visit(const AggTerm* t) {
	_context._conjunctivePathFromRoot = false;

	// Create set grounder
	t->set()->accept(this);

	// Create term grounder
	_termgrounder = new AggTermGrounder(_grounding->translator(), t->function(), _setgrounder);
	_termgrounder->setOrig(t, varmapping(), _verbosity);
}

/**
 * void GrounderFactory::visit(const EnumSetExpr* s)
 * DESCRIPTION
 * 		Creates a grounder for an enumarated set.
 */
void GrounderFactory::visit(const EnumSetExpr* s) {
	_context._conjunctivePathFromRoot = false;

	// Create grounders for formulas and weights
	vector<FormulaGrounder*> subfgr;
	vector<TermGrounder*> subtgr;
	SaveContext();
	AggContext();
	for (size_t n = 0; n < s->subformulas().size(); ++n) {
		descend(s->subformulas()[n]);
		subfgr.push_back(_formgrounder);
		descend(s->subterms()[n]);
		subtgr.push_back(_termgrounder);
	}
	RestoreContext();

	// Create set grounder
	_setgrounder = new EnumSetGrounder(_grounding->translator(), subfgr, subtgr);
}

// TODO verify
template<typename OrigConstruct>
GrounderFactory::GenAndChecker GrounderFactory::createVarsAndGenerators(Formula* subformula, OrigConstruct* orig, TruthType generatortype,
		TruthType checkertype) {
	vector<const DomElemContainer*> vars;
	vector<SortTable*> tables;
	vector<Variable*> fovars, quantfovars;
	vector<Pattern> pattern;

	for(auto it = subformula->freeVars().cbegin(); it != subformula->freeVars().cend(); ++it) {
		if(orig->quantVars().find(*it) == orig->quantVars().cend()) { // It is a free var of the quantified formula
			assert(_varmapping.find(*it) != _varmapping.cend()); // So should already have a varmapping
			vars.push_back(_varmapping[*it]);
			pattern.push_back(Pattern::INPUT);
		}else { // It is a var quantified in the orig formula, so should create a new varmapping for it
			const DomElemContainer* d = new const DomElemContainer();
			_varmapping[*it] = d;
			vars.push_back(d);
			pattern.push_back(Pattern::OUTPUT);
			quantfovars.push_back(*it);
		}
		fovars.push_back(*it);
		SortTable* st = _structure->inter((*it)->sort());
		tables.push_back(st);
	}

	// Check for infinite grounding
	for (auto it = tables.cbegin(); it < tables.cend(); ++it) {
		if (not (*it)->finite()) {
			Warning::possiblyInfiniteGrounding(orig->pi().original() != NULL ? orig->pi().original()->toString() : "", orig->toString());
		}
	}

	// FIXME => unsafe to have to pass in fovars explicitly (order is never checked?)
/*	const FOBDD* generatorbdd = _symstructure->evaluate(subformula, generatortype); // !x phi(x) => generate all x possibly false
	const FOBDD* checkerbdd = _symstructure->evaluate(subformula, checkertype); // !x phi(x) => check for x certainly false
	// FIXME checker is incorrect
	cerr <<"Generator bdd: \n";
	_symstructure->manager()->put(std::cerr, generatorbdd);
	cerr <<"\nChecker bdd: \n";
	_symstructure->manager()->put(std::cerr, generatorbdd);
	cerr <<"\n";
	generatorbdd = improve_generator(generatorbdd, quantfovars, MCPA);
	checkerbdd = improve_checker(checkerbdd, MCPA);
	cerr <<"Improved generator bdd: \n";
	_symstructure->manager()->put(std::cerr, generatorbdd);
	cerr <<"\nImproved checker bdd: \n";
	_symstructure->manager()->put(std::cerr, generatorbdd);
	cerr <<"\n";
	PredTable* gentable = new PredTable(new BDDInternalPredTable(generatorbdd, _symstructure->manager(), fovars, _structure), Universe(tables));
	PredTable* checktable = new PredTable(new BDDInternalPredTable(checkerbdd, _symstructure->manager(), fovars, _structure), Universe(tables));
*/
	PredTable* gentable = new PredTable(new FullInternalPredTable(), Universe(tables));
	PredTable* checktable = new PredTable(new InverseInternalPredTable(new FullInternalPredTable), Universe(tables));
	/*cerr <<"Generator table: \n";
	gentable->print(std::cerr);
	cerr <<"\nChecker table: \n";
	checktable->print(std::cerr);
	cerr <<"\n";*/
	InstGenerator* gen = GeneratorFactory::create(gentable, pattern, vars, Universe(tables));
	InstChecker* check = GeneratorFactory::create(checktable, vector<Pattern>(vars.size(), Pattern::INPUT), vars, Universe(tables));
	return GenAndChecker(gen, check);
}

/**
 * void GrounderFactory::visit(const QuantSetExpr* s)
 * DESCRIPTION
 * 		Creates a grounder for a quantified set.
 */
void GrounderFactory::visit(const QuantSetExpr* origqs) {
	_context._conjunctivePathFromRoot = false;

	// Move three-valued terms in the set expression
	auto transqs = TermUtils::moveThreeValuedTerms(origqs->clone(),_structure,_context._funccontext,_cpsupport,_cpsymbols);
	if(not sametypeid<QuantSetExpr>(*transqs)){
		if(_verbosity > 1) {
			clog << "Rewritten "; origqs->put(clog,_longnames);
			clog << " to "; transqs->put(clog,_longnames);
		   	clog << "\n";
		}
		transqs->accept(this);
		return;
	}

	auto newqs = dynamic_cast<QuantSetExpr*>(transqs);
	Formula* clonedformula = newqs->subformulas()[0]->clone();
	Formula* newsubformula = FormulaUtils::unnestThreeValuedTerms(clonedformula, _structure, Context::POSITIVE);
	newsubformula = FormulaUtils::splitComparisonChains(newsubformula, NULL);
	newsubformula = FormulaUtils::graphFunctions(newsubformula);

	// NOTE: generator generates possibly true instances, checker checks the certainly true ones
	GenAndChecker gc = createVarsAndGenerators(newsubformula, newqs, TruthType::POSS_TRUE, TruthType::CERTAIN_TRUE);

	// Create grounder for subformula
	SaveContext();
	AggContext();
	descend(newqs->subformulas()[0]);
	FormulaGrounder* subgr = _formgrounder;
	RestoreContext();

	// Create grounder for weight
	descend(newqs->subterms()[0]);
	TermGrounder* wgr = _termgrounder;

	// Create grounder
	_setgrounder = new QuantSetGrounder(_grounding->translator(), subgr, gc._generator, gc._checker, wgr);
}

/**
 * void GrounderFactory::visit(const Definition* def)
 * DESCRIPTION
 * 		Creates a grounder for a definition.
 */
void GrounderFactory::visit(const Definition* def) {
	_context._conjunctivePathFromRoot = false;

	// Store defined predicates
	for (auto it = def->defsymbols().cbegin(); it != def->defsymbols().cend(); ++it) {
		_context._defined.insert(*it);
	}

	// Create rule grounders
	vector<RuleGrounder*> subgrounders;
	for (auto it = def->rules().cbegin(); it != def->rules().cend(); ++it) {
		descend(*it);
		subgrounders.push_back(_rulegrounder);
	}

	_topgrounder = new DefinitionGrounder(_grounding, subgrounders, _context);

	_context._defined.clear();
}

template<class VarList>
InstGenerator* GrounderFactory::createVarMapAndGenerator(const VarList& vars) {
	vector<SortTable*> hvst;
	vector<const DomElemContainer*> hvars;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		auto domelem = createVarMapping(*it);
		hvars.push_back(domelem);
		auto sorttable = structure()->inter((*it)->sort());
		hvst.push_back(sorttable);
	}
	GeneratorFactory gf;
	return gf.create(hvars, hvst);
}

/**
 * void GrounderFactory::visit(const Rule* rule)
 * DESCRIPTION
 * 		Creates a grounder for a definitional rule.
 */
void GrounderFactory::visit(const Rule* rule) {
	_context._conjunctivePathFromRoot = false;

	// FIXME Move all three-valued terms outside the head
	// TODO for lazygroundrules, we need a generator for all variables NOT occurring in the head!

	InstGenerator *headgen = NULL, *bodygen = NULL;

	if (_options->getValue(BoolType::GROUNDLAZILY)) {
		assert(sametypeid<SolverTheory>(*_grounding));
		// TODO resolve this in a clean way
		// for lazy ground rules, need a generator which generates bodies given a head, so only vars not occurring in the head!
		varlist bodyvars;
		for (auto it = rule->quantVars().cbegin(); it != rule->quantVars().cend(); ++it) {
			if (not rule->head()->contains(*it)) {
				bodyvars.push_back(*it);
			} else {
				createVarMapping(*it);
			}
		}

		bodygen = createVarMapAndGenerator(bodyvars);
	} else {
		// Split the quantified variables in two categories:
		//		1. the variables that only occur in the head
		//		2. the variables that occur in the body (and possibly in the head)

		varlist headvars;
		varlist bodyvars;
		for (auto it = rule->quantVars().cbegin(); it != rule->quantVars().cend(); ++it) {
			if (rule->body()->contains(*it)) {
				bodyvars.push_back(*it);
			} else {
				headvars.push_back(*it);
			}
		}

		headgen = createVarMapAndGenerator(headvars);
		bodygen = createVarMapAndGenerator(bodyvars);
	}

	// Create head grounder
	SaveContext();
	_context._component = CompContext::HEAD;
	descend(rule->head());
	HeadGrounder* headgr = _headgrounder;
	RestoreContext();

	// Create body grounder
	SaveContext();
	_context._funccontext = Context::NEGATIVE; // minimize truth value of rule bodies
	_context._monotone = Context::POSITIVE;
	_context.gentype = GenType::CANMAKETRUE; // body instance generator corresponds to an existential quantifier
	_context._component = CompContext::FORMULA;
	_context._tseitin = TsType::EQ;
	descend(rule->body());
	FormulaGrounder* bodygr = _formgrounder;
	RestoreContext();

	// Create rule grounder
	SaveContext();
	if (recursive(rule->body()))
		_context._tseitin = TsType::RULE;
	if (_options->getValue(BoolType::GROUNDLAZILY)) {
		_rulegrounder = new LazyRuleGrounder(headgr, bodygr, bodygen, _context);
	} else {
		_rulegrounder = new RuleGrounder(headgr, bodygr, headgen, bodygen, _context);
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

void LazyTsBody::notifyTheoryOccurence() {
	grounder_->notifyTheoryOccurence(inst);
}
