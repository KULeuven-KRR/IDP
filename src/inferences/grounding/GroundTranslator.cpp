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
#include "grounders/LazyFormulaGrounders.hpp"
#include "grounders/DefinitionGrounders.hpp"

using namespace std;

GroundTranslator::GroundTranslator()
		: 	atomtype(1, AtomType::LONETSEITIN),
			_sets(1) {
	atom2Tuple.push_back(NULL);
	atom2TsBody.push_back(tspair { 0, (TsBody*) NULL });
}

GroundTranslator::~GroundTranslator() {
	for (auto i = atom2Tuple.cbegin(); i != atom2Tuple.cend(); ++i) {
		if (*i != NULL) {
			delete (*i);
		}
	}
	atom2Tuple.clear();
	for (auto i = atom2TsBody.cbegin(); i < atom2TsBody.cend(); ++i) {
		delete ((*i).second);
	}
}

Lit GroundTranslator::translate(SymbolOffset symbolID, const ElementTuple& args) {
	Lit lit = 0;
	//auto jt = symbols[n].tuple2atom.lower_bound(args);
	//if (jt != symbols[n].tuple2atom.cend() && jt->first == args) {
	auto& symbolinfo = symbols[symbolID];
	auto jt = symbolinfo.tuple2atom.find(args);
	if (jt != symbolinfo.tuple2atom.cend()) {
		lit = jt->second;
	} else {
		lit = nextNumber(AtomType::INPUT);
		symbolinfo.tuple2atom.insert(jt, Tuple2Atom { args, lit });
		if (symbolinfo.tuple2atom.size() == 1) {
			newsymbols.push(symbolID);
		}
		atom2Tuple[lit] = new SymbolAndTuple(symbolinfo.symbol, args);

		// NOTE: when getting here, a new literal was created, so have to check whether any lazy bounds are watching its symbol
		if (not symbolinfo.assocGrounders.empty()) {
			symbolinfo.assocGrounders.front()->notify(lit, args, symbolinfo.assocGrounders); // First part gets the grounding
		}
	}

	//clog <<toString(symbols[n].symbol) <<toString(args) <<" maps to " <<lit <<nt();

	return lit;
}

// TODO expensive!
int GroundTranslator::getSymbol(PFSymbol* pfs) const {
	for (size_t n = 0; n < symbols.size(); ++n) {
		if (symbols[n].symbol == pfs) {
			return n;
		}
	}
	return -1;
}

SymbolOffset GroundTranslator::addSymbol(PFSymbol* pfs) {
	auto n = getSymbol(pfs);
	if (n == -1) {
		symbols.push_back(SymbolInfo(pfs));
		return symbols.size() - 1;
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

bool GroundTranslator::canBeDelayedOn(PFSymbol* pfs, Context context, int id) const {
	auto symbolID = getSymbol(pfs);
	if (symbolID == -1) { // there is no such symbol yet
		return true;
	}
	auto& grounders = symbols[symbolID].assocGrounders;
	if (grounders.empty()) {
		return true;
	}
	for (auto i = grounders.cbegin(); i < grounders.cend(); ++i) {
		if (context == Context::BOTH) { // If unknown-delay, can only delay if in same DEFINITION
			if (id == -1 || (*i)->getID() != id) {
				return false;
			}
		} else if ((*i)->getContext() != context) { // If true(false)-delay, can delay if we do not find any false(true) or unknown delay
			return false;
		}
	}
	return true;
}

void GroundTranslator::notifyDelay(PFSymbol* pfs, DelayGrounder* const grounder) {
	Assert(grounder!=NULL);
	//clog <<"Notified that symbol " <<toString(pfs) <<" is defined on id " <<grounder->getID() <<".\n";
	auto symbolID = addSymbol(pfs);
	auto& grounders = symbols[symbolID].assocGrounders;
#ifndef NDEBUG
	Assert(canBeDelayedOn(pfs, grounder->getContext(), grounder->getID()));
	for (auto i = grounders.cbegin(); i < grounders.cend(); ++i) {
		Assert(grounder != *i);
	}
#endif
	grounders.push_back(grounder);

	// TODO in this way, we will add func constraints for all functions, might not be necessary!
	newsymbols.push(symbolID); // NOTE: For defined functions, should add the func constraint anyway, because it is not guaranteed to have a model!
}

Lit GroundTranslator::translate(LazyStoredInstantiation* instance, TsType tstype) {
	auto tseitin = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	//clog <<"Adding lazy tseitin" <<instance->residual <<nt();
	auto tsbody = new LazyTsBody(instance, tstype);
	atom2TsBody[tseitin] = tspair(tseitin, tsbody);
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
		atom2TsBody[head] = tspair(head, tsbody);
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
	auto it = cpset.find(tsbody);
	if (it != cpset.cend()) {
		delete tsbody;
		if (it->first->comp() != comp) { // NOTE: OPTIMIZATION! = and ~= map to the same tsbody etc. => look at ecnf.cpp:compEqThroughNeg
			return -it->second;
		} else {
			return it->second;
		}
	} else {
		int nr = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
		atom2TsBody[nr] = tspair(nr, tsbody);
		cpset[tsbody] = nr;
		return nr;
	}
}

SetId GroundTranslator::translateSet(const litlist& lits, const weightlist& weights, const weightlist& trueweights, const varidlist& varids) {
	SetId setnr;
	if (_freesetnumbers.empty()) {
		TsSet newset;
		setnr = _sets.size();
		_sets.push_back(newset);
	} else {
		setnr = _freesetnumbers.front();
		_freesetnumbers.pop();
	}
	TsSet& tsset = _sets[setnr];
	tsset._setlits = lits;
	tsset._litweights = weights;
	tsset._trueweights = trueweights;
	tsset._varids = varids;
	return setnr;
}

Lit GroundTranslator::nextNumber(AtomType type) {
	if (_freenumbers.empty()) {
		Lit nr = atomtype.size();
		atom2TsBody.push_back(tspair { nr, (TsBody*) NULL });
		atom2Tuple.push_back(NULL);
		atomtype.push_back(type);
		return nr;
	} else {
		Lit nr = _freenumbers.front();
		_freenumbers.pop();
		return nr;
	}
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
	case AtomType::TSEITINWITHSUBFORMULA:
		s << "tseitin_" << nr;
		break;
	case AtomType::LONETSEITIN:
		s << "tseitin_" << nr;
		break;
	}
	return s.str();
}
