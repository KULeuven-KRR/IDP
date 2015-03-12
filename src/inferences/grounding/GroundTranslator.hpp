/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#ifndef GROUNDTRANSLATOR_HPP_
#define GROUNDTRANSLATOR_HPP_

#include "GroundUtils.hpp"
#include "IncludeComponents.hpp"

#include <unordered_map>
#include <bits/functional_hash.h>
#include "structure/HashElementTuple.hpp"

class DelayGrounder;
class TsSet;
struct CPBound;
struct LazyInstantiation;
class TsSet;
class Function;
class Structure;
struct GroundTerm;
class CPTsBody;
class SortTable;

struct SymbolOffset {
	int offset;
	bool functionlist;

	SymbolOffset(int offset, bool function)
			: 	offset(offset),
				functionlist(function) {
	}

};

typedef std::unordered_map<ElementTuple, Lit, HashTuple> Tuple2AtomMap;
typedef std::map<TsBody*, Lit, Compare<TsBody> > Ts2Atom;

#include "generators/InstGenerator.hpp" // TODO temporary (for PATTERN usage)
class DomElemContainer;

struct CheckerInfo {
	InstChecker *ctchecker, *ptchecker;
	std::vector<const DomElemContainer*> containers; // The containers used to construct the checkers, which should be fully instantiated before checking
	PredInter* inter;

	CheckerInfo(PFSymbol* symbol, StructureInfo structure);
	~CheckerInfo();
};

struct SymbolInfo {
	PFSymbol* symbol;
	Tuple2AtomMap tuple2atom;
	CheckerInfo* checkers;

	std::map<std::vector<GroundTerm>, Lit > lazyatoms2lit;

	SymbolInfo(PFSymbol* symbol, StructureInfo structure);
	~SymbolInfo();
};

struct FunctionInfo {
	Function* symbol;
	std::map<std::vector<GroundTerm>, VarId> term2var;
	InstGenerator *truerangegenerator, *falserangegenerator;
	CheckerInfo* checkers;

	std::map<std::vector<GroundTerm>, Lit > lazyatoms2lit;

	FunctionInfo(Function* symbol, StructureInfo structure);
	~FunctionInfo();
};

enum class AtomType {
	INPUT,
	TSEITINWITHSUBFORMULA,
	LONETSEITIN,
	CPGRAPHEQ
};

typedef std::pair<PFSymbol*, ElementTuple> stpair;
typedef std::pair<Function*, std::vector<GroundTerm> > ftpair;

template<class T>
struct CPCompare {
	bool operator()(T* left, T* right){
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
};

/**
 * Translator stores:
 * 		for a tseitin atom, what its interpretation is
 * 		for an input atom, what symbol it refers to and what elementtuple
 * 		for an atom which is neither, should not store anything, except that it is not stored.
 */
class LazyGroundingManager;

class GroundTranslator {
private:
	StructureInfo _structure;
	AbstractGroundTheory* _grounding; //!< The ground theory that will be produce
	LazyGroundingManager* _groundingmanager;

	// PROPOSITIONAL SYMBOLS
	// SymbolID 2 Symbol + Tuple2Tseitin + grounders
	std::vector<SymbolInfo*> symbols;

	// Tseitin 2 atomtype
	// Tseitin 2 tuple
	// Tseitin 2 meaning (if applicable according to type)
	std::vector<AtomType> atomtype;
	std::vector<stpair*> atom2Tuple; // Owns pointers!
	std::vector<TsBody*> atom2TsBody; // Owns pointers! // Important: gets deleted the moment it is added to the ground theory!!! (except for cp, where it is needed later for sharing detection)

	Lit _trueLit;
	Lit nextNumber(AtomType type);
	SymbolOffset getSymbol(PFSymbol* pfs) const;

	// GROUND TERMS
	// FunctionID 2 Function + term2var
	std::vector<FunctionInfo*> functions;

	// Var 2 meaning
	std::vector<ftpair*> var2Tuple; // Owns pointers!
	std::vector<CPTsBody*> var2CTsBody;
	std::vector<SortTable*> var2domain;
	std::vector<Lit> var2partial;
	std::map<int, VarId> storedTerms; // Tabling of terms which are equal to a domain element

	// Maps used to detect identical CP terms and atoms
	std::map<CPTsBody*, Lit, CPCompare<CPTsBody> > cpset; // Used to detect identical Cpterms, is not used in any other way!
	std::map<CPTerm*, std::map<SortTable*, VarId>, CPCompare<CPTerm> > cp2id; // Used to detect identical Cpterms, is not used in any other way!

	VarId nextVarNumber(SortTable* domain = NULL);

	// SETS
	// SetID 2 set
	int maxquantsetid;
	std::vector<TsSet> _sets;
	std::map<int, std::map<ElementTuple, SetId, Compare<ElementTuple> > > _freevar2set;

	std::map<std::vector<GroundTerm>, Lit >& getTerm2Lits(PFSymbol* symbol);
	void createImplications(Lit newhead, const std::map<std::vector<GroundTerm>, Lit >& term2lits, const std::vector<GroundTerm>& terms, bool recursive);

public:
	GroundTranslator(StructureInfo structure, AbstractGroundTheory* grounding);
	void initialize();

	Structure* getConcreteStructure() const {
		return _structure.concrstructure;
	}
	std::shared_ptr<GenerateBDDAccordingToBounds> getSymbolicStructure() {
		return _structure.symstructure;
	}

	~GroundTranslator();

	void addMonitor(LazyGroundingManager* manager){
		_groundingmanager = manager;
	}

	Vocabulary* vocabulary() const;

	SymbolOffset addSymbol(PFSymbol* pfs);

	// Translate into propositional variables
private:
	Lit getLiteral(SymbolOffset offset, const ElementTuple&);
public:
	Lit conjunction (const Lit l, const Lit r, TsType tstype); //Returns the conjunction of l and r
	Lit reify(const litlist& cl, bool conj, TsType tp);
	Lit reify(Weight bound, CompType comp, AggFunction aggtype, SetId setnr, TsType tstype);
	Lit reify(CPTerm*, CompType, const CPBound&, TsType);
	Lit reify(LazyInstantiation* instance, TsType type);

	VarId translateTerm(Function*, const std::vector<GroundTerm>&);
	VarId translateTerm(SymbolOffset offset, const std::vector<GroundTerm>&);
	VarId translateTerm(CPTerm*, SortTable*);
	VarId translateTerm(const DomainElement*);

	TruthValue checkApplication(const DomainElement* domelem, SortTable* predtable, SortTable* termtable, Context funccontext, SIGN sign, const Formula* origFormula = NULL); //The original formula only serves for good error messages

	Lit translateNonReduced(PFSymbol* symbol, const ElementTuple& args);
	Lit translateReduced(PFSymbol* symbol, const ElementTuple& args, bool recursive);
	Lit translateReduced(const SymbolOffset& offset, const ElementTuple& args, bool recursivecontext);
private:
	std::map<bool, std::map<bool, std::map<int, std::map<ElementTuple, Lit> > > > knownlits;
	Lit translate(const SymbolOffset& offset, const ElementTuple&, bool reduced);
public:

	void addKnown(VarId id);

	bool hasSymbol(PFSymbol* symbol) const {
		return getSymbol(symbol).offset!=-1;
	}

	Tuple2AtomMap emptymap;
	const Tuple2AtomMap& getIntroducedLiteralsFor(PFSymbol* symbol) const{
		if(not hasSymbol(symbol)){
			return emptymap;
		}
		auto offset = getSymbol(symbol);
		Assert(offset.offset!=-1);
		if(offset.functionlist){
			throw notyetimplemented("Lazy grounding with support for function symbols in the grounding");
		}else{
			return symbols[offset.offset]->tuple2atom;
		}
	}

	Lit trueLit() const{
		return _trueLit;
	}
	Lit falseLit() const{
		return -trueLit();
	}

	Lit addLazyElement(PFSymbol* symbol, const std::vector<GroundTerm>& terms, bool recursive);

	// PROPOSITIONAL ATOMS
	bool isStored(int atom) const {
		return atom > 0 && atomtype.size() > (unsigned int) atom;
	}
	AtomType getType(int atom) const {
		return atomtype[atom];
	}
	bool isInputAtom(int atom) const {
		return isStored(atom) && (getType(atom) == AtomType::INPUT || getType(atom) == AtomType::CPGRAPHEQ);
	}
	bool isTseitinWithSubformula(int atom) const {
		return isStored(atom) && (getType(atom) == AtomType::TSEITINWITHSUBFORMULA || getType(atom) == AtomType::CPGRAPHEQ);
	}
	PFSymbol* getSymbol(int atom) const {
		Assert(isInputAtom(atom) && atom2Tuple[atom]->first!=NULL);
		return atom2Tuple[atom]->first;
	}
	const ElementTuple& getArgs(int atom) const {
		Assert(isInputAtom(atom) && atom2Tuple[atom]->first!=NULL);
		return atom2Tuple[atom]->second;
	}

	TsBody* getTsBody(Lit atom) const {
		Assert(isTseitinWithSubformula(atom));
		return atom2TsBody[atom];
	}
	void removeTsBody(Lit atom);
	int createNewUninterpretedNumber() {
		return nextNumber(AtomType::LONETSEITIN);
	}
	VarId createNewVarIdNumber(SortTable* domain = NULL) {
		return nextVarNumber(domain);
	}

	// GROUND TERMS

	bool hasVarIdMapping(const VarId& varid) const {
		return var2Tuple.size() > varid.id && var2Tuple.at(varid.id) != NULL;
	}

	// Methods for translating variable identifiers to terms
	Function* getFunction(const VarId& varid) const {
		Assert(hasVarIdMapping(varid));
		Assert(var2Tuple.at(varid.id)->first!=NULL);
		return var2Tuple.at(varid.id)->first;
	}
	const std::vector<GroundTerm>& getArgs(const VarId& varid) const {
		Assert(hasVarIdMapping(varid));
		return var2Tuple.at(varid.id)->second;
	}
	CPTsBody* cprelation(const VarId& varid) const {
		return var2CTsBody.at(varid.id);
	}
	Lit getNonDenoting(const VarId& varid) const;
	SortTable* domain(const VarId& varid) const {
		return var2domain.at(varid.id);
	}

	// SETS
	int createNewQuantSetId(){
		return maxquantsetid++;
	}
	SetId getPossibleSet(int id, const ElementTuple& freevar_inst) const;
	bool isSet(SetId setID) const;
	const TsSet groundset(SetId setID) const;
	SetId translateSet(const litlist& lits, const weightlist& posweights, const weightlist& negweights, const termlist& terms){
		return translateSet(createNewQuantSetId(), {}, lits, posweights, negweights, terms);
	}
	SetId translateSet(int id, const ElementTuple& freevar_inst, const litlist&, const weightlist&, const weightlist&, const termlist&);

	// PRINTING

	std::string printLit(const Lit& atom) const;
	std::string printTerm(const GroundTerm&) const;
};

#endif /* GROUNDTRANSLATOR_HPP_ */
