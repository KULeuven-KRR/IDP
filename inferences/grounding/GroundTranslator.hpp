#ifndef GROUNDTRANSLATOR_HPP_
#define GROUNDTRANSLATOR_HPP_

#include "inferences/grounding/Utils.hpp"
#include "ecnf.hpp"
#include "structure.hpp"

class LazyRuleGrounder;
class TsSet;
class CPTerm;
class LazyQuantGrounder;
class CPBound;
class ResidualAndFreeInst;
class TsSet;

typedef std::map<ElementTuple, Lit, Compare<ElementTuple> > Tuple2AtomMap;
typedef std::map<TsBody*, Lit, Compare<TsBody> > Ts2Atom;

struct SymbolAndAtomMap {
	PFSymbol* symbol;
	Tuple2AtomMap tuple2atom;

	SymbolAndAtomMap(PFSymbol* symbol) :
			symbol(symbol) {
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
	SymbolAndTuple(PFSymbol* symbol, const ElementTuple& tuple) :
			symbol(symbol), tuple(tuple) {
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

	std::map<uint, std::vector<LazyRuleGrounder*> > symbol2rulegrounder; // map a symbol to the rulegrounders in which the symbol occurs as a head

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
	Lit translate(double bound, CompType comp, bool strict, AggFunction aggtype, int setnr, TsType tstype);
	Lit translate(PFSymbol*, const ElementTuple&);
	Lit translate(CPTerm*, CompType, const CPBound&, TsType);
	Lit translateSet(const std::vector<int>&, const std::vector<double>&, const std::vector<double>&);
	void translate(LazyQuantGrounder const* const lazygrounder, ResidualAndFreeInst* instance, TsType type);

	void notifyDefined(PFSymbol* pfs, LazyRuleGrounder* const grounder);

	unsigned int addSymbol(PFSymbol* pfs);

	bool isStored(Lit atom) const {
		return atom > 0 && atomtype.size() > (uint)atom;
	}
	AtomType getType(Lit atom) const {
		return atomtype[atom];
	}

	bool isInputAtom(int atom) const {
		return isStored(atom) && getType(atom) == AtomType::INPUT;
	}
	PFSymbol* getSymbol(int atom) const {
		assert(isInputAtom(atom) && atom2Tuple[atom]->symbol!=NULL);
		return atom2Tuple[atom]->symbol;
	}
	const ElementTuple& getArgs(int atom) const {
		assert(isInputAtom(atom) && atom2Tuple[atom]->symbol!=NULL);
		return atom2Tuple[atom]->tuple;
	}

	bool isTseitinWithSubformula(int atom) const {
		return isStored(atom) && getType(atom) == AtomType::TSEITINWITHSUBFORMULA;
	}
	TsBody* getTsBody(int atom) const {
		assert(isTseitinWithSubformula(atom));
		return atom2TsBody[atom].second;
	}

	int createNewUninterpretedNumber() {
		return nextNumber(AtomType::LONETSEITIN);
	}

	bool isSet(int setID) const {
		return _sets.size() > (uint)setID;
	}

	const TsSet& groundset(int setID) const {
		assert(isSet(setID));
		return _sets[setID];
	}
	TsSet& groundset(int setID) {
		assert(isSet(setID));
		return _sets[setID];
	}

	bool isManagingSymbol(uint n) const {
		return symbols.size() > n;
	}
	unsigned int nbManagedSymbols() const {
		return symbols.size();
	}
	PFSymbol* getManagedSymbol(uint n) const {
		assert(isManagingSymbol(n));
		return symbols[n].symbol;
	}
	const Tuple2AtomMap& getTuples(uint n) const {
		assert(isManagingSymbol(n));
		return symbols[n].tuple2atom;
	}

	std::string printAtom(const Lit& atom, bool longnames) const;
};

#endif /* GROUNDTRANSLATOR_HPP_ */
