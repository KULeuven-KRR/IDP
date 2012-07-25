/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "GroundTranslator.hpp"
#include "IncludeComponents.hpp"
//#include "grounders/LazyFormulaGrounders.hpp"
#include "grounders/DefinitionGrounders.hpp"
#include "utils/CPUtils.hpp"
#include "utils/ListUtils.hpp"

using namespace std;

GroundTranslator::GroundTranslator(AbstractStructure* structure)
		: _structure(structure) {

	// Literal 0 is not allowed!
	atomtype.push_back(AtomType::LONETSEITIN);
	atom2Tuple.push_back(NULL);
	atom2TsBody.push_back((TsBody*) NULL);
}

GroundTranslator::~GroundTranslator() {
	deleteList<TsBody>(atom2TsBody);
	deleteList<CPTsBody>(var2CTsBody);
	deleteList<stpair>(atom2Tuple);
	deleteList<ftpair>(var2Tuple);
}

Lit GroundTranslator::translate(SymbolOffset symboloffset, const ElementTuple& args) {
	if (symboloffset.functionlist) {
		std::vector<GroundTerm> terms;
		for (auto i = args.cbegin(); i < args.cend(); ++i) {
			terms.push_back(*i);
		}
		auto image = terms.back();
		Assert(image._domelement->type()==DomainElementType::DET_INT);
		// Otherwise, cannot be a cp-able function
		auto bound = image._domelement->value()._int;
		terms.pop_back();
		auto lit = translate(new CPVarTerm(translate(symboloffset, terms)), CompType::EQ, CPBound(bound), TsType::EQ); // Fixme TSType?
		atom2Tuple[lit]->first = functions[symboloffset.offset].symbol;
		atom2Tuple[lit]->second = args;
		atomtype[lit] = AtomType::CPGRAPHEQ;
		return lit;
	}
	auto symbolID = symboloffset.offset;
	Lit lit = 0;
	auto& symbolinfo = symbols[symbolID];
	auto jt = symbolinfo.tuple2atom.find(args);
	if (jt != symbolinfo.tuple2atom.cend()) {
		lit = jt->second;
	} else {
		lit = nextNumber(AtomType::INPUT);
		symbolinfo.tuple2atom.insert(jt, Tuple2Atom { args, lit });
		atom2Tuple[lit]->first = symbolinfo.symbol;
		atom2Tuple[lit]->second = args;

		// TODO think that this is not necessary
		// NOTE: when getting here, a new literal was created, so have to check whether any lazy bounds are watching its symbol
		//if (not symbolinfo.assocGrounders.empty()) {
			//symbolinfo.assocGrounders.front()->notify(lit, args, symbolinfo.assocGrounders); // First part gets the grounding
		//}
		// ENDTODO
	}

	return lit;
}

Vocabulary* GroundTranslator::vocabulary() const {
	return _structure == NULL ? NULL : _structure->vocabulary();
}

// TODO expensive!
SymbolOffset GroundTranslator::getSymbol(PFSymbol* pfs) const {
	if (pfs->isFunction()) {
		auto function = dynamic_cast<Function*>(pfs);
		if (function != NULL && getOption(CPSUPPORT) && CPSupport::eligibleForCP(function, vocabulary())) {
			for (size_t n = 0; n < functions.size(); ++n) {
				if (functions[n].symbol == pfs) {
					return SymbolOffset(n, true);
				}
			}
		}
	}
	for (size_t n = 0; n < symbols.size(); ++n) {
		if (symbols[n].symbol == pfs) {
			return SymbolOffset(n, false);
		}
	}
	return SymbolOffset(-1, false);
}

SymbolOffset GroundTranslator::addSymbol(PFSymbol* pfs) {
	auto n = getSymbol(pfs);
	if (n.offset == -1) {
		if (pfs->isFunction()) {
			auto function = dynamic_cast<Function*>(pfs);
			if (function != NULL && getOption(CPSUPPORT) && CPSupport::eligibleForCP(function, vocabulary())) {
				functions.push_back(FunctionInfo(function));
				return SymbolOffset(functions.size() - 1, true);
			}
		}
		symbols.push_back(SymbolInfo(pfs));
		return SymbolOffset(symbols.size() - 1, false);

	} else {
		return n;
	}
}

Lit GroundTranslator::translate(PFSymbol* s, const ElementTuple& args) {
	SymbolOffset offset = addSymbol(s);
	return translate(offset, args);
}

Lit GroundTranslator::translate(const litlist& clause, bool conj, TsType tstype) {
	int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	return translate(nr, clause, conj, tstype);
}

Lit GroundTranslator::translate(const Lit& head, const litlist& clause, bool conj, TsType tstype) {
	auto tsbody = new PCTsBody(tstype, clause, conj);
	atom2TsBody[head] = tsbody;
	return head;
}

bool GroundTranslator::canBeDelayedOn(PFSymbol* pfs, Context context, DefId id) const {
	auto symboloffset = getSymbol(pfs);
	Assert(not symboloffset.functionlist);
	auto symbolId = symboloffset.offset;
	if (symbolId == -1) { // there is no such symbol yet
		return true;
	}
	auto& grounders = symbols[symbolId].assocGrounders;
	if (grounders.empty()) {
		return true;
	}
	throw notyetimplemented("Checking allowed delays");
/*	for (auto i = grounders.cbegin(); i < grounders.cend(); ++i) {
		if (context == Context::BOTH) { // If unknown-delay, can only delay if in same DEFINITION
			if (id == -1 || (*i)->getID() != id) {
				return false;
			}
		} else if ((*i)->getContext() != context) { // If true(false)-delay, can delay if we do not find any false(true) or unknown delay
			return false;
		}
	}
	return true;*/
}

void GroundTranslator::notifyDelay(PFSymbol* pfs, DelayGrounder* const grounder) {
	Assert(grounder!=NULL);
	//clog <<"Notified that symbol " <<toString(pfs) <<" is defined on id " <<grounder->getID() <<".\n";
	throw notyetimplemented("Notifying of delays");
/*	auto symbolID = addSymbol(pfs);
	Assert(not symbolID.functionlist);
	auto& grounders = symbols[symbolID.offset].assocGrounders;
#ifndef NDEBUG
	Assert(canBeDelayedOn(pfs, grounder->getContext(), grounder->getID()));
	for (auto i = grounders.cbegin(); i < grounders.cend(); ++i) {
		Assert(grounder != *i);
	}
#endif
	grounders.push_back(grounder);*/
}

Lit GroundTranslator::translate(LazyInstantiation* instance, TsType tstype) {
	auto tseitin = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	//clog <<"Adding lazy tseitin" <<instance->residual <<nt();
	auto tsbody = new LazyTsBody(instance, tstype);
	atom2TsBody[tseitin] = tsbody;
	return tseitin;
}

Lit GroundTranslator::translate(double bound, CompType comp, AggFunction aggtype, SetId setnr, TsType tstype) {
	if (comp == CompType::EQ) {
		auto l = translate(bound, CompType::LEQ, aggtype, setnr, tstype);
		auto l2 = translate(bound, CompType::GEQ, aggtype, setnr, tstype);
		return translate( { l, l2 }, true, tstype);
	} else if (comp == CompType::NEQ) {
		auto l = translate(bound, CompType::GT, aggtype, setnr, tstype);
		auto l2 = translate(bound, CompType::LT, aggtype, setnr, tstype);
		return translate( { l, l2 }, false, tstype);
	} else {
		auto head = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
		if (comp == CompType::LT) {
			bound += 1;
			comp = CompType::LEQ;
		} else if (comp == CompType::GT) {
			bound -= 1;
			comp = CompType::GEQ;
		}
		MAssert(comp==CompType::LEQ || comp==CompType::GEQ);
		auto tsbody = new AggTsBody(tstype, bound, comp == CompType::LEQ, aggtype, setnr);
		atom2TsBody[head] = tsbody;
		return head;
	}
}

bool CompareTs::operator()(CPTsBody* left, CPTsBody* right) {
//	cerr << "Comparing " << toString(left) << " with " << toString(right) << "\n";
	if (left == NULL) {
		if (right == NULL) {
			return false;
		}
		return true;
	} else if (right == NULL) {
		return false;
	}
	return *left < *right;
}

Lit GroundTranslator::translate(CPTerm* left, CompType comp, const CPBound& right, TsType tstype) {
	auto tsbody = new CPTsBody(tstype, left, comp, right);
	// TODO => this should be generalized to sharing detection!
//	cerr <<"Searching for " <<toString(left) <<toString(comp) <<toString(right) <<"in \n";
//	for(auto i=cpset.cbegin(); i!=cpset.cend(); ++i){
//		cerr <<toString(i->first) <<"\n";
//	}
	auto it = cpset.find(tsbody);
	if (it != cpset.cend()) {
//		cerr <<"Found: literal " <<it->second <<"\n";
		delete tsbody;
		if (it->first->comp() != comp) { // NOTE: OPTIMIZATION! = and ~= map to the same tsbody etc. => look at ecnf.cpp:compEqThroughNeg
			return -it->second;
		} else {
			return it->second;
		}
	} else {
//		cerr <<"Adding new\n";
		int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
		atom2TsBody[nr] = tsbody;
		cpset[tsbody] = nr;
		return nr;
	}
}

//// Adds a tseitin body only if it does not yet exist. TODO why does this seem only relevant for CP Terms?
//Lit GroundTranslator::addTseitinBody(TsBody* tsbody) {
//// TODO optimization: check whether the same comparison has already been added and reuse the tseitin.
//	auto it = _tsbodies2nr.lower_bound(tsbody);
//	if (it != _tsbodies2nr.cend() && *(it->first) == *tsbody) { // Already exists
//		delete tsbody;
//		return it->second;
//	}
//	int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
//	atom2TsBody[nr] = tspair(nr, tsbody);
//	return nr;
//}

SetId GroundTranslator::translateSet(const litlist& lits, const weightlist& weights, const weightlist& trueweights, const varidlist& varids) {
	TsSet tsset;
	tsset._setlits = lits;
	tsset._litweights = weights;
	tsset._trueweights = trueweights;
	tsset._varids = varids;
	auto setnr = _sets.size();
	_sets.push_back(tsset);
	return setnr;
}

Lit GroundTranslator::nextNumber(AtomType type) {
	Lit nr = atomtype.size();
	atom2TsBody.push_back(NULL);
	atom2Tuple.push_back(new stpair());
	atomtype.push_back(type);
	return nr;
}

VarId GroundTranslator::translate(SymbolOffset offset, const vector<GroundTerm>& args) {
	Assert(offset.functionlist);
	auto& info = functions[offset.offset];
	auto it = info.term2var.lower_bound(args);
	if (it != info.term2var.cend() && it->first == args) {
		return it->second;
	} else {
		VarId varid = nextNumber();
		info.term2var.insert(it, pair<vector<GroundTerm>, VarId> { args, varid });
		var2Tuple[varid.id]->first = info.symbol;
		var2Tuple[varid.id]->second = args;
		var2domain[varid.id] = _structure->inter(info.symbol->outsort());
		return varid;
	}
}

VarId GroundTranslator::translateTerm(Function* function, const vector<GroundTerm>& args) {
	Assert(CPSupport::eligibleForCP(function, vocabulary()));
	auto offset = addSymbol(function);
	return translate(offset, args);
}

VarId GroundTranslator::translateTerm(CPTerm* cpterm, SortTable* domain) {
	auto varid = nextNumber();
	CPBound bound(varid);
	auto cprelation = new CPTsBody(TsType::EQ, cpterm, CompType::EQ, bound);
	var2CTsBody[varid.id] = cprelation;
	var2domain[varid.id] = domain;
	return varid;
}

VarId GroundTranslator::translateTerm(const DomainElement* element) {
	auto varid = nextNumber();
	// Create a new CP variable term
	auto cpterm = new CPVarTerm(varid);
	// Create a new CP bound based on the domain element
	Assert(element->type() == DET_INT);
	CPBound bound(element->value()._int);
	// Add a new CP constraint
	auto cprelation = new CPTsBody(TsType::EQ, cpterm, CompType::EQ, bound);
	var2CTsBody[varid.id] = cprelation;
	// Add a new domain containing only the given domain element
	auto domain = TableUtils::createSortTable();
	domain->add(element);
	var2domain[varid.id] = domain;
	// Return the new variable identifier
	return varid;
}

VarId GroundTranslator::nextNumber() {
	VarId id;
	id.id = var2Tuple.size();
	var2Tuple.push_back(new ftpair());
	var2CTsBody.push_back(NULL);
	var2domain.push_back(NULL);
	return id;
}

string GroundTranslator::printTerm(const VarId& varid) const {
	if (varid.id >= var2Tuple.size()) {
		return "error";
	}
	auto func = getFunction(varid);
	if (func == NULL) {
		stringstream s;
		s << "var_" << varid;
		return s.str();
	}
	stringstream s;
	s << toString(func);
	if (not args(varid).empty()) {
		s << "(";
		for (auto gtit = args(varid).cbegin(); gtit != args(varid).cend(); ++gtit) {
			if ((*gtit).isVariable) {
				s << printTerm((*gtit)._varid);
			} else {
				s << toString((*gtit)._domelement);
			}
			if (gtit != args(varid).cend() - 1) {
				s << ",";
			}
		}
		s << ")";
	}
	return s.str();
}

string GroundTranslator::print(Lit lit) {
	return printLit(lit);
}

string GroundTranslator::printLit(const Lit& lit) const {
	stringstream s;
	Lit nr = lit;
	if (nr == _true) {
		return "true";
	}
	if (nr == _false) {
		return "false";
	}
	if (nr < 0) {
		s << "~";
		nr = -nr;
	}
	if (not isStored(nr)) {
		return "error";
	}

	switch (atomtype[nr]) {
	case AtomType::INPUT: {
		PFSymbol* pfs = getSymbol(nr);
		s << toString(pfs);
		auto tuples = getArgs(nr);
		if (not tuples.empty()) {
			s << "(";
			bool begin = true;
			for (auto i = tuples.cbegin(); i != tuples.cend(); ++i) {
				if (not begin) {
					s << ", ";
				}
				begin = false;
				s << toString(*i);
			}
			s << ")";
		}
		break;
	}
	case AtomType::CPGRAPHEQ:
	case AtomType::TSEITINWITHSUBFORMULA:
		s << "tseitin_" << nr;
		break;
	case AtomType::LONETSEITIN:
		s << "tseitin_" << nr;
		break;
	}
	return s.str();
}
