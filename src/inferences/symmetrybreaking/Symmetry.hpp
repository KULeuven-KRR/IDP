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

#pragma once

#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <ostream>

#include "visitors/TheoryVisitor.hpp"

class DomainElement;
class Structure;
class AbstractTheory;
class AbstractGroundTheory;
class Sort;
class PFSymbol;
class Predicate;
class Function;
class Variable;
class OccurrencesCounter;

class InterchangeabilitySet;

// This class functions as representative of a set of interchangeable domain elements

class ElementOccurrence {
private:
	InterchangeabilitySet* ics;
public:
	const DomainElement* domel;
	size_t hash;


	ElementOccurrence(InterchangeabilitySet* intset, const DomainElement* de);

	~ElementOccurrence() {
	}

	bool isEqualTo(const ElementOccurrence& other) const;
};

struct ElOcHash {
	size_t operator()(std::shared_ptr<ElementOccurrence> eloc) const {
		return eloc->hash;
	}
};

struct ElOcEqual {
	bool operator()(std::shared_ptr<ElementOccurrence> a, std::shared_ptr<ElementOccurrence> b) const {
		return a->isEqualTo(*b);
	}
};

class ArgPosSet {
public:
  std::map<PFSymbol*,std::set<unsigned int> > argPositions; // ordered because of easy literal ordering when breaking symmetry
  std::set<PFSymbol*> symbols;
  
  void addArgPos(PFSymbol* symb, unsigned int arg);
  bool hasArgPos(PFSymbol* symb, unsigned int arg);
  void print(std::ostream& ostr);
};

class Symmetry {
private:
  ArgPosSet argpositions;
  std::map<const DomainElement*, const DomainElement*> image;
  
  // returns true iff the tuple was transformed to a different tuple
  bool transformToSymmetric(std::vector<const DomainElement*>& tuple, std::set<unsigned int>& positions);
  void getUsefulAtoms(std::unordered_map<unsigned int, unsigned int>& groundSym, std::vector<unsigned int>& groundOrder, std::vector<int>& out_orig, std::vector<int>& out_sym);
  void getGroundSymmetry(AbstractGroundTheory* gt, Structure* struc, std::unordered_map<unsigned int, unsigned int>& groundSym, std::vector<unsigned int>& groundOrder);
  
public:
  Symmetry(const ArgPosSet& ap);
  void addImage(const DomainElement* first, const DomainElement* second);
  void print(std::ostream& ostr);
  
  void addBreakingClauses(AbstractGroundTheory* gt, Structure* struc, bool nbModelsEquivalent);
};

class InterchangeabilityGroup {
	
private:
	ArgPosSet symbolargs;
	std::set<const DomainElement*> elements;
  
  void getSymmetricLiterals(AbstractGroundTheory* gt, Structure* struc, const DomainElement* smaller, const DomainElement* bigger, std::vector<int>& out, std::vector<int>& sym_out) const;
	
public:
	InterchangeabilityGroup(std::vector<const DomainElement*>& domels, std::vector<PFSymbol*>& symbs3val, 
		ArgPosSet& symbargs);
	void print(std::ostream& ostr);

	unsigned int getNrSwaps();
	bool hasSymbArg(PFSymbol* symb, unsigned int arg);
  
  void addBreakingClauses(AbstractGroundTheory* gt, Structure* struc, bool nbModelsEquivalent) const;
  void getGoodGenerators(std::vector<Symmetry*>& out) const;
};

class InterchangeabilitySet {
private:
	std::unordered_set<const Sort*> sorts;
	std::unordered_map<std::shared_ptr<ElementOccurrence>, std::vector<const DomainElement*>*, ElOcHash, ElOcEqual> partition;

public:
  std::unordered_set<const DomainElement*> occursAsConstant;
	const Structure* _struct;
	ArgPosSet symbolargs; // symbols with corresponding arguments

	InterchangeabilitySet(const Structure* s) : _struct(s) {}

	~InterchangeabilitySet() {
    for (auto part : partition) {
			delete part.second;
		}
	}

	bool add(PFSymbol* p, unsigned int arg);
	bool add(const Sort* s);
	bool add(const DomainElement* de);
  
  void getDomain(std::unordered_set<const DomainElement*>& out, bool includeConstants=false);

	void calculateInterchangeableSets();
	void getIntchGroups(std::vector<InterchangeabilityGroup*>& out);
	void print(std::ostream& ostr);
};

class UFNode {
public:
	UFNode* parent;
	unsigned int depth;

	UFNode() : parent(this), depth(0) {
	}

	virtual ~UFNode() {
	}

	virtual void put(std::ostream& outstr) = 0;
	virtual bool addTo(InterchangeabilitySet* ichset) = 0;
};

class SymbolArgumentNode : public UFNode {
public:
	PFSymbol* symbol;
	const unsigned int arg;

	SymbolArgumentNode(PFSymbol* pf, unsigned int a) : symbol(pf), arg(a) {
	}

	virtual ~SymbolArgumentNode() {
	}

	void put(std::ostream& outstr);
	bool addTo(InterchangeabilitySet* ichset);
};

class VariableNode : public UFNode {
public:
	const Variable* var;

	VariableNode(const Variable* v) : var(v) {
	}

	virtual ~VariableNode() {
	}

	void put(std::ostream& outstr);
	bool addTo(InterchangeabilitySet* ichset);
};

class DomainElementNode : public UFNode {
public:
	const Sort* s;
	const DomainElement* de;

	DomainElementNode(const Sort* srt, const DomainElement* domel) : s(srt), de(domel) {
	}

	virtual ~DomainElementNode() {
	}

	void put(std::ostream& outstr);
	bool addTo(InterchangeabilitySet* ichset);
};

class ForbiddenNode : public UFNode {
public:

	ForbiddenNode() {
	}

	virtual ~ForbiddenNode() {
	}

	void put(std::ostream& outstr);
	bool addTo(InterchangeabilitySet* ichset);
};

/*
 * UFSymbolArg uses a union-find datastructure (also known as disjoint-set) to efficiently partition the set of symbol-argument pairs.
 */
class UFSymbolArg {
	std::unordered_map<PFSymbol*, std::vector<SymbolArgumentNode*>* > SAnodes; // map of PFSymbol and argument to SymbolArgumentNode;
	std::unordered_map<const Variable*, VariableNode*> VARnodes; // map of Variable to VariableNode
	ForbiddenNode* forbiddenNode;
	std::vector<UFNode*> twoValuedNodes;

public:
	UFSymbolArg();
	~UFSymbolArg();

	UFNode* get(PFSymbol* sym, unsigned int arg, bool twoValued = false);
	UFNode* get(const Variable* var);
	UFNode* get(const Sort* s, const DomainElement* de);
	UFNode* getForbiddenNode();

	UFNode* find(UFNode* in) const;
	void merge(UFNode* first, UFNode* second);

	void getPartition(std::unordered_multimap<UFNode*, UFNode*>& out);
	void printPartition(std::ostream& ostr);
};

/**
 * 	Theory analyzing visitor which extracts information relevant for symmetry detection.
 *	More specifically, it will extract
 *		a set of domain elements which should not be permuted by a symmetry,
 *		a set of sorts whose domain elements should not be permuted by a symmetry, and
 *		a set of relations which are used in the theory (to exclude unused but nonetheless defined relations)
 * */

class InterchangeabilityAnalyzer : public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	const Structure* _structure;
	UFNode* subNode; // functions as a return value for visiting terms, so that their occurrence in a parent symbol can be taken into account	

public:
	UFSymbolArg disjointSet;

	InterchangeabilityAnalyzer(const Structure* s);

	~InterchangeabilityAnalyzer() {
	}

	void analyze(const AbstractTheory* t);
	void analyzeForOptimization(const Term* t); // TODO use this, and push quantifiers

protected:
	virtual void visit(const PredForm*);
	virtual void visit(const EqChainForm*);
	virtual void visit(const EquivForm*);

	virtual void visit(const VarTerm*);
	virtual void visit(const FuncTerm*);
	virtual void visit(const DomainTerm*);
	virtual void visit(const AggTerm*);

	virtual void visit(const QuantSetExpr*);
};

struct SymbArg{
	PFSymbol* symb;
	unsigned int arg;
};

void detectInterchangeability(std::vector<InterchangeabilityGroup*>& out_groups, std::vector<Symmetry*>& out_syms, const AbstractTheory* t, const Structure* s, const Term* obj = nullptr);
void getIntchGroups(AbstractTheory* theo, const Structure* s, std::vector<InterchangeabilityGroup*>& out_groups, std::vector<Symmetry*>& out_syms, const std::vector<std::pair<PFSymbol*, unsigned int> >& symbargs); 
void getIntchGroups(AbstractTheory* theo, const Structure* s, std::vector<InterchangeabilityGroup*>& out_groups, std::vector<Symmetry*>& out_syms);