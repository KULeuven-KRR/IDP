/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef GROUNDTRANSLATOR_HPP_
#define GROUNDTRANSLATOR_HPP_

#include "Utils.hpp"
#include "IncludeComponents.hpp"

#include <unordered_map>
#include <bits/functional_hash.h>

class LazyRuleGrounder;
class TsSet;
class CPTerm;
class LazyGroundingManager;
class CPBound;
class ResidualAndFreeInst;
class TsSet;

struct HashTuple {
	size_t operator()(const ElementTuple& tuple) const;
};

//typedef std::map<ElementTuple, Lit, Compare<ElementTuple> > Tuple2AtomMap;
typedef std::unordered_map<ElementTuple, Lit, HashTuple> Tuple2AtomMap;
typedef std::map<TsBody*, Lit, Compare<TsBody> > Ts2Atom;

struct SymbolAndAtomMap {
	PFSymbol* symbol;
	Tuple2AtomMap tuple2atom;

	SymbolAndAtomMap(PFSymbol* symbol)
			: symbol(symbol) {
	}
};

enum class AtomType {
	INPUT, TSEITINWITHSUBFORMULA, LONETSEITIN
};

struct SymbolAndTuple {
	PFSymbol* symbol;
	ElementTuple tuple;

	SymbolAndTuple() {
	}
	SymbolAndTuple(PFSymbol* symbol, const ElementTuple& tuple)
			: symbol(symbol), tuple(tuple) {
	}
};

/**
 * Translator stores:
 * 		for a tseitin atom, what its interpretation is
 * 		for an input atom, what symbol it refers to and what elementtuple
 * 		for an atom which is neither, should not store anything, except that it is not stored.
 */

class GroundTranslator {
private:
	std::vector<SymbolAndAtomMap> symbols; // Each symbol added to the translated is associated a unique number, the index into this vector, at which the symbol is also stored

	std::vector<AtomType> atomtype;
	std::vector<SymbolAndTuple*> atom2Tuple; // Pointers manager by the translator!
	std::vector<tspair> atom2TsBody; // Pointers manager by the translator!

	std::map<unsigned int, std::vector<LazyRuleGrounder*> > symbol2rulegrounder; // map a symbol to the rulegrounders in which the symbol occurs as a head

	std::queue<int> _freenumbers; // keeps atom numbers that were freed and can be used again
	std::queue<int> _freesetnumbers; // keeps set numbers that were freed and can be used again

	// TODO pointer
	std::vector<TsSet> _sets; // keeps mapping between Set numbers and sets

	Lit addTseitinBody(TsBody* body);
	Lit nextNumber(AtomType type);

public:
	GroundTranslator();
	~GroundTranslator();

	Lit translate(unsigned int, const ElementTuple&);
	Lit translate(const std::vector<int>& cl, bool conj, TsType tp);
	Lit translate(const Lit& head, const std::vector<Lit>& clause, bool conj, TsType tstype);
	Lit translate(double bound, CompType comp, AggFunction aggtype, int setnr, TsType tstype);
	Lit translate(PFSymbol*, const ElementTuple&);
	Lit translate(CPTerm*, CompType, const CPBound&, TsType);
	Lit translateSet(const std::vector<int>&, const std::vector<double>&, const std::vector<double>&);
	void translate(LazyGroundingManager const* const lazygrounder, ResidualAndFreeInst* instance, TsType type);

	void notifyDefined(PFSymbol* pfs, LazyRuleGrounder* const grounder);

	unsigned int addSymbol(PFSymbol* pfs);

	bool isStored(Lit atom) const {
		return atom > 0 && atomtype.size() > (unsigned int) atom;
	}
	AtomType getType(Lit atom) const {
		return atomtype[atom];
	}

	bool isInputAtom(int atom) const {
		return isStored(atom) && getType(atom) == AtomType::INPUT;
	}
	PFSymbol* getSymbol(int atom) const {
		Assert(isInputAtom(atom) && atom2Tuple[atom]->symbol!=NULL);
		return atom2Tuple[atom]->symbol;
	}
	const ElementTuple& getArgs(int atom) const {
		Assert(isInputAtom(atom) && atom2Tuple[atom]->symbol!=NULL);
		return atom2Tuple[atom]->tuple;
	}

	bool isTseitinWithSubformula(int atom) const {
		return isStored(atom) && getType(atom) == AtomType::TSEITINWITHSUBFORMULA;
	}
	TsBody* getTsBody(int atom) const {
		Assert(isTseitinWithSubformula(atom));
		return atom2TsBody[atom].second;
	}

	int createNewUninterpretedNumber() {
		return nextNumber(AtomType::LONETSEITIN);
	}

	bool isSet(int setID) const {
		return _sets.size() > (unsigned int) setID;
	}

	//TODO: when _sets contains pointers instead of objects, this should return a TsSet*
	const TsSet groundset(int setID) const {
		Assert(isSet(setID));
		return _sets[setID];
	}

	bool isManagingSymbol(unsigned int n) const {
		return symbols.size() > n;
	}
	unsigned int nbManagedSymbols() const {
		return symbols.size();
	}
	PFSymbol* getManagedSymbol(unsigned int n) const {
		Assert(isManagingSymbol(n));
		return symbols[n].symbol;
	}
	const Tuple2AtomMap& getTuples(unsigned int n) const {
		Assert(isManagingSymbol(n));
		return symbols[n].tuple2atom;
	}

	std::string printLit(const Lit& atom) const;
};

#endif /* GROUNDTRANSLATOR_HPP_ */
