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
#include "grounders/LazyQuantGrounder.hpp"
#include "grounders/DefinitionGrounders.hpp"

using namespace std;

inline size_t HashTuple::operator()(const ElementTuple& tuple) const {
	size_t seed = 1;
	for (auto i = tuple.cbegin(); i < tuple.cend(); ++i) {
		switch ((*i)->type()) {
		case DomainElementType::DET_INT:
			seed += (*i)->value()._int;
			break;
		case DomainElementType::DET_DOUBLE:
			seed += (*i)->value()._double;
			break;
		case DomainElementType::DET_STRING:
			seed += reinterpret_cast<size_t>((*i)->value()._string);
			break;
		case DomainElementType::DET_COMPOUND:
			seed += reinterpret_cast<size_t>((*i)->value()._compound);
			break;
		}
	}
	return seed % 104729;
}

GroundTranslator::GroundTranslator()
		: atomtype(1, AtomType::LONETSEITIN), _sets(1) {
	atom2Tuple.push_back(NULL);
	atom2TsBody.push_back(std::pair<Lit, TsBody*> { 0, (TsBody*) NULL });
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

Lit GroundTranslator::translate(unsigned int n, const ElementTuple& args) {
	Lit lit = 0;
	//auto jt = symbols[n].tuple2atom.lower_bound(args);
	//if (jt != symbols[n].tuple2atom.cend() && jt->first == args) {
	auto jt = symbols[n].tuple2atom.find(args);
	if (jt != symbols[n].tuple2atom.cend()) {
		lit = jt->second;
	} else {
		lit = nextNumber(AtomType::INPUT);
		symbols[n].tuple2atom.insert(jt, Tuple2Atom { args, lit });
		if(symbols[n].tuple2atom.size()==1){
			newsymbols.push(n);
		}
		atom2Tuple[lit] = new SymbolAndTuple(symbols[n].symbol, args);

		// FIXME expensive operation to do so often!
		auto rulesit = symbol2rulegrounder.find(n);
		if (rulesit != symbol2rulegrounder.cend() && rulesit->second.size() > 0) {
			(*rulesit->second.cbegin())->notify(lit, args, rulesit->second);
		}
	}

	//clog <<toString(symbols[n].symbol) <<toString(args) <<" maps to " <<lit <<nt();

	return lit;
}

unsigned int GroundTranslator::addSymbol(PFSymbol* pfs) {
	// TODO expensive!
	for (unsigned int n = 0; n < symbols.size(); ++n) {
		if (symbols[n].symbol == pfs) {
			return n;
		}
	}

	symbols.push_back(SymbolAndAtomMap(pfs));
	return symbols.size() - 1;
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

void GroundTranslator::notifyDefined(PFSymbol* pfs, LazyRuleGrounder* const grounder) {
	//cerr <<"Notified that symbol " <<toString(pfs) <<" is defined\n";
	int symbolnumber = addSymbol(pfs);
	auto it = symbol2rulegrounder.find(symbolnumber);
	if (symbol2rulegrounder.find(symbolnumber) == symbol2rulegrounder.cend()) {
		it = symbol2rulegrounder.insert(pair<unsigned int, std::vector<LazyRuleGrounder*> >(symbolnumber, { })).first;
	}
	for (auto grounderit = it->second.cbegin(); grounderit < it->second.cend(); ++grounderit) {
		Assert(grounder != *grounderit);
	}
	it->second.push_back(grounder);
}

void GroundTranslator::translate(LazyGroundingManager const* const lazygrounder, ResidualAndFreeInst* instance, TsType tstype) {
	instance->residual = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	//cerr <<"Adding lazy tseitin" <<instance->residual <<nt();
	LazyTsBody* tsbody = new LazyTsBody(lazygrounder, instance, tstype);
	atom2TsBody[instance->residual] = tspair(instance->residual, tsbody);
}

Lit GroundTranslator::translate(double bound, CompType comp, AggFunction aggtype, int setnr, TsType tstype) {
	if (comp == CompType::EQ) {
		vector<int> cl(2);
		cl[0] = translate(bound, CompType::LEQ, aggtype, setnr, tstype);
		cl[1] = translate(bound, CompType::GEQ, aggtype, setnr, tstype);
		return translate(cl, true, tstype);
	} else if (comp == CompType::NEQ) {
		vector<int> cl(2);
		cl[0] = translate(bound, CompType::GT, aggtype, setnr, tstype);
		cl[1] = translate(bound, CompType::LT, aggtype, setnr, tstype);
		return translate(cl, false, tstype);
	} else {
		Lit head = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
		AggTsBody* tsbody = new AggTsBody(tstype, bound, (comp == CompType::LEQ || comp == CompType::GT), aggtype, setnr);
		atom2TsBody[head] = tspair(head, tsbody);
		return (comp == CompType::LT || comp == CompType::GT) ? -head : head;
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
		atom2TsBody.push_back(tspair { atom, (TsBody*) NULL });
		atom2Tuple.push_back(NULL);
		atomtype.push_back(type);
		return atom;
	} else {
		int nr = _freenumbers.front();
		_freenumbers.pop();
		return nr;
	}
}

string GroundTranslator::printLit(const Lit& lit) const {
	stringstream s;
	int nr = lit;
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
