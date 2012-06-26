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
#include "structure/HashElementTuple.hpp"

class DelayGrounder;
class TsSet;
class CPTerm;
class CPBound;
class LazyStoredInstantiation;
class TsSet;
class Function;
class AbstractStructure;
class GroundTerm;
class CPTsBody;
class SortTable;
class CPTerm;

typedef size_t SymbolOffset;

//typedef std::map<ElementTuple, Lit, Compare<ElementTuple> > Tuple2AtomMap;
typedef std::unordered_map<ElementTuple, Lit, HashTuple> Tuple2AtomMap;
typedef std::map<TsBody*, Lit, Compare<TsBody> > Ts2Atom;

typedef size_t SymbolOffset;

struct SymbolInfo {
	PFSymbol* symbol;
	Tuple2AtomMap tuple2atom;
	std::vector<DelayGrounder*> assocGrounders;

	SymbolInfo(PFSymbol* symbol)
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

struct CompareTs {
	bool operator()(CPTsBody* left, CPTsBody* right);
};

class GroundTranslator {
private:
	std::queue<int> newsymbols;
	std::vector<SymbolInfo> symbols; // Each symbol added to the translated is associated a unique number, the index into this vector, at which the symbol is also stored

	std::vector<AtomType> atomtype;
	std::vector<SymbolAndTuple*> atom2Tuple; // Pointers manager by the translator!
	std::vector<tspair> atom2TsBody; // Pointers manager by the translator!

	std::queue<int> _freenumbers; // keeps atom numbers that were freed and can be used again
	std::queue<int> _freesetnumbers; // keeps set numbers that were freed and can be used again

	// TODO pointer
	std::vector<TsSet> _sets; // keeps mapping between Set numbers and sets

	std::map<CPTsBody*, Lit, CompareTs> cpset;

	Vocabulary* _vocabulary;
	AbstractStructure* _structure;

	std::vector<std::map<std::vector<GroundTerm>, VarId> > _functerm2varid_table; //!< map function term to CP variable identifier
	std::vector<Function*> _varid2function; //!< map CP varid to the symbol of its corresponding term
	std::vector<std::vector<GroundTerm> > _varid2args; //!< map CP varid to the terms of its corresponding term

	std::vector<Function*> _offset2function;
	std::map<Function*, SymbolOffset> _function2offset;

	std::map<VarId, CPTsBody*> _varid2cprelation;

	std::vector<SortTable*> _varid2domain;

	Lit nextNumber(AtomType type);
	VarId nextNumber();

	int getSymbol(PFSymbol* pfs) const;

public:
	GroundTranslator(AbstractStructure* structure);
	~GroundTranslator();

	// NOTE: used to add func constraints as soon as possible
	int getNextNewSymbol(){
		auto i = newsymbols.front();
		newsymbols.pop();
		return i;
	}
	bool hasNewSymbols() const {
		return newsymbols.size();
	}

	// Translate into propositional variables
	Lit translate(SymbolOffset, const ElementTuple&);
	Lit translate(const litlist& cl, bool conj, TsType tp);
	Lit translate(const Lit& head, const litlist& clause, bool conj, TsType tstype);
	Lit translate(Weight bound, CompType comp, AggFunction aggtype, SetId setnr, TsType tstype);
	Lit translate(PFSymbol*, const ElementTuple&);
	Lit translate(CPTerm*, CompType, const CPBound&, TsType);
	Lit translateSet(const litlist&, const weightlist&, const weightlist&, const varidlist&);
	Lit translate(LazyStoredInstantiation* instance, TsType type);

	// Translate into finite domain variables
	VarId translate(SymbolOffset offset, const std::vector<GroundTerm>&);
	VarId translate(Function*, const std::vector<GroundTerm>&);
	VarId translate(CPTerm*, SortTable*);
	VarId translate(const DomainElement*);

	/*
	 * @precon: defid==-1 if a FORMULA will be delayed
	 * @precon: context==POS if pfs occurs monotonously, ==NEG if anti-..., otherwise BOTH
	 * Returns true iff delaying pfs in the given context cannot violate satisfiability because of existing watches
	 */
	bool canBeDelayedOn(PFSymbol* pfs, Context context, int defid) const;
	/**
	 * Same preconditions as canBeDelayedOn
	 * Notifies the translator that the given symbol is delayed in the given context with the given grounder.
	 */
	void notifyDelay(PFSymbol* pfs, DelayGrounder* const grounder);

	SymbolOffset addSymbol(PFSymbol* pfs);

	bool isStored(Lit atom) const {
		return atom > 0 && atomtype.size() > (unsigned int) atom;
	}
	AtomType getType(Lit atom) const {
		return atomtype[atom];
	}

	bool isInputAtom(int atom) const {
		return isStored(atom) && getType(atom) == AtomType::INPUT;
	}
	PFSymbol* getSymbol(Lit atom) const {
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

	bool isSet(SetId setID) const {
		return _sets.size() > (size_t) setID;
	}

	//TODO: when _sets contains pointers instead of objects, this should return a TsSet*
	const TsSet groundset(SetId setID) const {
		Assert(isSet(setID));
		return _sets[setID];
	}

	bool isManagingSymbol(SymbolOffset n) const {
		return symbols.size() > n;
	}
	size_t nbManagedSymbols() const {
		return symbols.size();
	}
	PFSymbol* getManagedSymbol(SymbolOffset n) const {
		Assert(isManagingSymbol(n));
		return symbols[n].symbol;
	}
	const Tuple2AtomMap& getTuples(SymbolOffset n) const {
		Assert(isManagingSymbol(n));
		return symbols[n].tuple2atom;
	}

	SymbolOffset addFunction(Function*);

	// Methods for translating variable identifiers to terms
	Function* function(const VarId& varid) const {
		return _varid2function[varid];
	}
	const std::vector<GroundTerm>& args(const VarId& varid) const {
		return _varid2args.at(varid);
	}
	CPTsBody* cprelation(const VarId& varid) const {
		return _varid2cprelation.find(varid)->second;
	}
	SortTable* domain(const VarId& varid) const {
		return _varid2domain[varid];
	}

	size_t nrOffsets() const {
		return _offset2function.size();
	}
	SymbolOffset getOffset(Function* func) const {
		return _function2offset.at(func);
	}
	const Function* getFunction(SymbolOffset offset) const {
		return _offset2function[offset];
	}
	bool hasFunction(Function* function) const{
		return _function2offset.find(function)!=_function2offset.cend();
	}

	std::string print(Lit atom);
	std::string printLit(const Lit& atom) const;
	std::string printTerm(const VarId&) const;
};

#endif /* GROUNDTRANSLATOR_HPP_ */
