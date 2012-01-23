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

// TODO all code should be reformatted to only use int domainelements. Only in that case can we use a more efficient ground translator for finite sorts (as
// we then can constant time derive an index from a domainelement.
// In the background, there should be a transformation possible from int domainelements to the original ones, given their sort.
// But this then leads to problems concerning calling lua code etc?

#include "inferences/grounding/Utils.hpp"
#include "ecnf.hpp"
#include "structure.hpp"

#include <unordered_map>
#include <bits/functional_hash.h>

class LazyRuleGrounder;
class TsSet;
class CPTerm;
class LazyQuantGrounder;
class CPBound;
class ResidualAndFreeInst;
class TsSet;

enum class AtomType {
	INPUT, TSEITINWITHSUBFORMULA, LONETSEITIN, UNASSIGNED
};

struct SymbolAndTuple {
	PFSymbol* symbol;
	ElementTuple tuple;

	SymbolAndTuple()
			: symbol(NULL) {
	}
	SymbolAndTuple(PFSymbol* symbol, const ElementTuple& tuple)
			: symbol(symbol), tuple(tuple) {
	}
};

struct HashTuple {
	size_t operator()(const ElementTuple& tuple) const {
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
		return seed;
	}
};

typedef std::map<ElementTuple, Lit, Compare<ElementTuple> > Tuple2AtomMap;

struct SymbolAndAtomMap {
	PFSymbol* symbol;
	bool finite;
	int startnumber;
	std::vector<int> multsizes;
	Tuple2AtomMap tuple2atom;

	SymbolAndAtomMap(PFSymbol* symbol)
			: symbol(symbol), finite(false) {
	}
	SymbolAndAtomMap(PFSymbol* symbol, int startnumber, std::vector<int> sizes)
			: symbol(symbol), finite(true), startnumber(startnumber), multsizes(sizes) {
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
	const AbstractStructure* structure;
public:
	GroundTranslator(const AbstractStructure* structure);
	~GroundTranslator();

	// Tuple management
private:
	int nextnumber;

	// Each symbol added to the translated is associated a unique number, the index into this vector, at which the symbol is also stored
	std::vector<SymbolAndAtomMap> symbolswithmapping;

	std::vector<AtomType> atomtype; // For each atomnumber, stores what type it is.

	std::vector<SymbolAndTuple*> atom2Tuple; // Used for mapping back from atomnumbers, by INDEX! Pointers managed by the translator!

	Lit nextNumber(AtomType type);

	bool isManagingSymbol(unsigned int n) const {
		return symbolswithmapping.size() > n;
	}
public:
	Lit translate(unsigned int, const ElementTuple&);
	Lit translate(PFSymbol*, const ElementTuple&);

	bool isStored(Lit atomnumber) const {
		return atomnumber > 0 && atomtype.size() > (unsigned int) atomnumber && atomtype[atomnumber]!=AtomType::UNASSIGNED;
	}
	AtomType getType(Lit atomnumber) const {
		Assert(atomtype.size()>atomnumber);
		return atomtype[atomnumber];
	}
	bool isInputAtom(Lit atomnumber) const {
		return isStored(atomnumber) && getType(atomnumber) == AtomType::INPUT;
	}

	// Symbols
	unsigned int getSymbol(PFSymbol* pfs) const {
		for (unsigned int n = 0; n < symbolswithmapping.size(); ++n) {
			if (symbolswithmapping[n].symbol == pfs) {
				return n;
			}
		}
	}
	unsigned int addSymbol(PFSymbol* pfs);
	unsigned int nbManagedSymbols() const {
		return symbolswithmapping.size();
	}
	PFSymbol* getManagedSymbol(unsigned int symbolnumber) const {
		Assert(isManagingSymbol(symbolnumber));
		return symbolswithmapping[symbolnumber].symbol;
	}
	const Tuple2AtomMap& getTuples(unsigned int symbolnumber) const {
		Assert(isManagingSymbol(symbolnumber));
		return symbolswithmapping[symbolnumber].tuple2atom;
	}

	// Backtranslation
	PFSymbol* getSymbol(Lit atomnumber) const {
		Assert(isInputAtom(atomnumber) && atom2Tuple[atomnumber]->symbol!=NULL);
		return atom2Tuple[atomnumber]->symbol;
	}
	const ElementTuple& getArgs(Lit atomnumber) const {
		Assert(isInputAtom(atomnumber) && atom2Tuple[atomnumber]->symbol!=NULL);
		return atom2Tuple[atomnumber]->tuple;
	}

	// Tseitin management
private:
	std::vector<TsBody*> atom2TsBody; // Pointers managed by the translator!
	std::map<unsigned int, std::vector<LazyRuleGrounder*> > symbol2rulegrounder; // map a symbol to the rulegrounders in which the symbol occurs as a head

	Lit addTseitinBody(TsBody* body);
public:
	Lit translate(const std::vector<int>& cl, bool conj, TsType tp);
	Lit translate(const Lit& head, const std::vector<Lit>& clause, bool conj, TsType tstype);
	Lit translate(double bound, CompType comp, AggFunction aggtype, int setnr, TsType tstype);
	Lit translate(CPTerm*, CompType, const CPBound&, TsType);
	void translate(LazyQuantGrounder const* const lazygrounder, ResidualAndFreeInst* instance, TsType type);

	void notifyDefined(PFSymbol* pfs, LazyRuleGrounder* const grounder);

	bool isTseitinWithSubformula(int atom) const {
		return isStored(atom) && getType(atom) == AtomType::TSEITINWITHSUBFORMULA;
	}
	TsBody* getTsBody(int atom) const {
		Assert(isTseitinWithSubformula(atom));
		return atom2TsBody[atom];
	}

	int createNewUninterpretedNumber() {
		return nextNumber(AtomType::LONETSEITIN);
	}

	// Set management
private:
	// TODO pointer
	std::vector<TsSet> _sets; // keeps mapping between Set numbers and sets
public:
	Lit translateSet(const std::vector<int>&, const std::vector<double>&, const std::vector<double>&);

	bool isSet(int setID) const {
		return _sets.size() > (unsigned int) setID;
	}

	//TODO: when _sets contains pointers instead of objects, this should return a TsSet*
	const TsSet groundset(int setID) const {
		Assert(isSet(setID));
		return _sets[setID];
	}

	// Printing

	std::string printLit(const Lit& atom) const;
};

#endif /* GROUNDTRANSLATOR_HPP_ */
