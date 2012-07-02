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
class CPBound;
class LazyStoredInstantiation;
class TsSet;
class Function;
class AbstractStructure;
class GroundTerm;
class CPTsBody;
class SortTable;

struct SymbolOffset{
	int offset;
	bool functionlist;

	SymbolOffset(int offset, bool function):offset(offset), functionlist(function){}

};

typedef std::unordered_map<ElementTuple, Lit, HashTuple> Tuple2AtomMap;
typedef std::map<TsBody*, Lit, Compare<TsBody> > Ts2Atom;

struct SymbolInfo {
	PFSymbol* symbol;
	Tuple2AtomMap tuple2atom;
	std::vector<DelayGrounder*> assocGrounders;

	SymbolInfo(PFSymbol* symbol)
			: symbol(symbol) {
	}
};

struct FunctionInfo {
	Function* symbol;
	std::map<std::vector<GroundTerm>, VarId> term2var;

	FunctionInfo(Function* symbol)
			: symbol(symbol) {
	}
};

enum class AtomType {
	INPUT, TSEITINWITHSUBFORMULA, LONETSEITIN, CPGRAPHEQ
};

typedef std::pair<PFSymbol*, ElementTuple> stpair;
typedef std::pair<Function*, std::vector<GroundTerm> > ftpair;

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
	AbstractStructure* _structure;

	// PROPOSITIONAL SYMBOLS
	// SymbolID 2 Symbol + Tuple2Tseitin + grounders
	std::vector<SymbolInfo> symbols;

	// Tseitin 2 atomtype
	// Tseitin 2 tuple
	// Tseitin 2 meaning (if applicable according to type)
	std::vector<AtomType> atomtype;
	std::vector<stpair*> atom2Tuple; 		// Owns pointers!
	std::vector<TsBody*> atom2TsBody; 		// Owns pointers!

	Lit nextNumber(AtomType type);
	SymbolOffset getSymbol(PFSymbol* pfs) const;

	// GROUND TERMS
	// FunctionID 2 Function + term2var
	std::vector<FunctionInfo> functions;

	// Var 2 meaning
	std::vector<ftpair*> var2Tuple; 		// Owns pointers!
	std::vector<CPTsBody*> var2CTsBody;
	std::vector<SortTable*> var2domain;

	std::map<CPTsBody*, Lit, CompareTs> cpset; // Used to detect identical Cpterms, is not used in any other way!

	VarId nextNumber();
	VarId translate(SymbolOffset offset, const std::vector<GroundTerm>&);

	// SETS
	// SetID 2 set
	std::vector<TsSet> _sets;

public:
	GroundTranslator(AbstractStructure* structure);
	~GroundTranslator();

	Vocabulary* vocabulary() const;

	SymbolOffset addSymbol(PFSymbol* pfs);

	// Translate into propositional variables
	Lit translate(SymbolOffset offset, const ElementTuple&);
	Lit translate(const litlist& cl, bool conj, TsType tp);
	Lit translate(const Lit& head, const litlist& clause, bool conj, TsType tstype);
	Lit translate(Weight bound, CompType comp, AggFunction aggtype, SetId setnr, TsType tstype);
	Lit translate(PFSymbol*, const ElementTuple&);
	Lit translate(CPTerm*, CompType, const CPBound&, TsType);
	Lit translate(LazyStoredInstantiation* instance, TsType type);

	VarId translateTerm(Function*, const std::vector<GroundTerm>&);
	VarId translateTerm(CPTerm*, SortTable*);
	VarId translateTerm(const DomainElement*);

	SetId translateSet(const litlist&, const weightlist&, const weightlist&, const varidlist&);

	// PROPOSITIONAL ATOMS
	bool isStored(Lit atom) const {
		return atom > 0 && atomtype.size() > (unsigned int) atom;
	}
	AtomType getType(Lit atom) const {
		return atomtype[atom];
	}
	bool isInputAtom(int atom) const {
		return isStored(atom) && (getType(atom) == AtomType::INPUT || getType(atom) == AtomType::CPGRAPHEQ);
	}
	bool isTseitinWithSubformula(int atom) const {
		return isStored(atom) && (getType(atom) == AtomType::TSEITINWITHSUBFORMULA || getType(atom) == AtomType::CPGRAPHEQ);
	}
	PFSymbol* getSymbol(Lit atom) const {
		Assert(isInputAtom(atom) && atom2Tuple[atom]->first!=NULL);
		return atom2Tuple[atom]->first;
	}
	const ElementTuple& getArgs(Lit atom) const {
		Assert(isInputAtom(atom) && atom2Tuple[atom]->first!=NULL);
		return atom2Tuple[atom]->second;
	}
	TsBody* getTsBody(Lit atom) const {
		Assert(isTseitinWithSubformula(atom));
		return atom2TsBody[atom];
	}
	int createNewUninterpretedNumber() {
		return nextNumber(AtomType::LONETSEITIN);
	}

	// GROUND TERMS

	// Methods for translating variable identifiers to terms
	Function* getFunction(const VarId& varid) const {
		return var2Tuple.at(varid.id)->first;
	}
	const std::vector<GroundTerm>& args(const VarId& varid) const {
		return var2Tuple.at(varid.id)->second;
	}
	CPTsBody* cprelation(const VarId& varid) const {
		return var2CTsBody.at(varid.id);
	}
	SortTable* domain(const VarId& varid) const {
		return var2domain.at(varid.id);
	}

	// SETS
	bool isSet(SetId setID) const {
		return _sets.size() > (size_t) setID.id;
	}
	const TsSet groundset(SetId setID) const {
		Assert(isSet(setID));
		return _sets[setID.id];
	}

	// DELAYS

	/*
	 * @precon: defid==-1 if a FORMULA will be delayed
	 * @precon: context==POS if pfs occurs monotonously, ==NEG if anti-..., otherwise BOTH
	 * Returns true iff delaying pfs in the given context cannot violate satisfiability because of existing watches
	 */
	bool canBeDelayedOn(PFSymbol* pfs, Context context, DefId defid) const;
	/**
	 * Same preconditions as canBeDelayedOn
	 * Notifies the translator that the given symbol is delayed in the given context with the given grounder.
	 */
	void notifyDelay(PFSymbol* pfs, DelayGrounder* const grounder);


	// PRINTING

	std::string print(Lit atom);
	std::string printLit(const Lit& atom) const;
	std::string printTerm(const VarId&) const;
};

#endif /* GROUNDTRANSLATOR_HPP_ */
