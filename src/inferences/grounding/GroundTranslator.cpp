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
#include "ecnf.hpp"
#include "structure.hpp"
#include "vocabulary.hpp"
#include "inferences/grounding/grounders/LazyQuantGrounder.hpp"
#include "inferences/grounding/grounders/DefinitionGrounders.hpp"

using namespace std;

GroundTranslator::GroundTranslator(const AbstractStructure* structure)
		: structure(structure), nextnumber(1), _sets(1) {
}

GroundTranslator::~GroundTranslator() {
	for (auto i = atom2Tuple.cbegin(); i != atom2Tuple.cend(); ++i) {
		if (*i != NULL) {
			delete (*i);
		}
	}
	atom2Tuple.clear();
	for (auto i = atom2TsBody.cbegin(); i < atom2TsBody.cend(); ++i) {
		if (*i != NULL) {
			delete (*i);
		}
	}
}

Lit GroundTranslator::translate(unsigned int n, const ElementTuple& args) {
	Lit lit = 0;
	// TODO can we save the largest tuple seen till now? NOTE: would only help for creation, not retrieval!
	auto& symbolmap = symbolswithmapping[n];
	if(symbolmap.finite){
		Assert(false);
	}
	const auto jt = symbolmap.tuple2atom.lower_bound(args);
	if (jt != symbolmap.tuple2atom.cend() && jt->first == args) { // second part is again a arity time check
		lit = jt->second;
	} else {
		lit = nextNumber(AtomType::INPUT);
		symbolmap.tuple2atom.insert(jt, { args, lit });
		atom2Tuple[lit] = new SymbolAndTuple(symbolmap.symbol, args);

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
	atom2TsBody[head] = tsbody;
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
	atom2TsBody[nr] = tsbody;
	return nr;
}

void GroundTranslator::notifyDefined(PFSymbol* pfs, LazyRuleGrounder* const grounder) {
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

void GroundTranslator::translate(LazyQuantGrounder const* const lazygrounder, ResidualAndFreeInst* instance, TsType tstype) {
	instance->residual = nextNumber(AtomType::TSEITINWITHSUBFORMULA);
	LazyTsBody* tsbody = new LazyTsBody(lazygrounder->id(), lazygrounder, instance, tstype);
	atom2TsBody[instance->residual] = tsbody;
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
		atom2TsBody[head] = tsbody;
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
	atom2TsBody[nr] = tsbody;
	return nr;
	//}
}

int GroundTranslator::translateSet(const vector<int>& lits, const vector<double>& weights, const vector<double>& trueweights) {
	int setnr;
	TsSet newset;
	setnr = _sets.size();
	_sets.push_back(newset);
	TsSet& tsset = _sets.back();
	tsset._setlits = lits;
	tsset._litweights = weights;
	tsset._trueweights = trueweights;
	return setnr;
}

Lit GroundTranslator::nextNumber(AtomType type) {
	Lit atom = nextnumber++;
	atom2TsBody.resize(atom, NULL);
	atom2Tuple.resize(atom, NULL);
	atomtype.resize(atom, AtomType::UNASSIGNED);
	atomtype[atom] = type;
	return atom;
}

unsigned int GroundTranslator::addSymbol(PFSymbol* pfs) {
	for (unsigned int n = 0; n < symbolswithmapping.size(); ++n) {
		if (symbolswithmapping[n].symbol == pfs) {
			return n;
		}
	}
	auto universe = structure->universe(pfs);
	if(universe.finite() && false){ // Cannot work unless we can transform any domainelement to an index into its sort
		vector<int> sizes;
		for(auto i=universe.tables().cbegin(); i<universe.tables().cend(); ++i){
			sizes.push_back((*i)->size()._size * (sizes.size()>0?sizes.back():1));
		}
		int size = universe.size()._size;
		Assert(size==sizes.back());
		atom2Tuple.resize(nextnumber+size, NULL);
		atomtype.resize(nextnumber+size, AtomType::UNASSIGNED);
		symbolswithmapping.push_back(SymbolAndAtomMap(pfs, nextnumber, sizes));
		nextnumber += size;
	}else{
		symbolswithmapping.push_back(SymbolAndAtomMap(pfs));
	}

	return symbolswithmapping.size() - 1;
}

string GroundTranslator::printLit(const Lit& lit) const {
	if(not isStored(lit)){
		return "non-existing literal";
	}
	int nr = lit;
	if (nr == _true) {
		return "true";
	}
	if (nr == _false) {
		return "false";
	}
	stringstream s;
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
	case AtomType::UNASSIGNED: // NOTE: will not occur
		break;
	}
	return s.str();
}
