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
#include <list>
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
class PredInter;
class FuncInter;

/** 
 *	\brief	Abstract base class to represent a symmetry group
 */

/**
 * Definition of an IVSet: 
 * 	a set of domain elements elements_, 
 * 		occurring in the domain of all the sorts of sorts_,
 * 		not occurring in the domain of all sorts not in sorts_, but whose parents are in sorts_.
 *	a set of sorts sorts_,
 *		which is a fixpoint under the "add parent sorts" operation.
 * 	and a set of PFSymbols relations_, 
 * 		which contains all the relations occurring in a theory who have as argument at least one of the elements of sorts_
 *
 * INVARIANT: every IVSet has getSize()>1.
 * INVARIANT: the domain elements of an IVSet all belong to the youngest sort in sorts_. The other sorts in sorts_ are parents of this youngest sort
 * (unfortunately, the last invariant is not enforced by an assertion, even though it is assumed it holds)
 */
class IVSet {
private:
	// Attributes
	const Structure* structure_; //!< The structure over which this symmetry ranges
	const std::set<const DomainElement*> elements_; //!< Elements which are permuted by the symmetry's
	const std::set<Sort*> sorts_; //!< Sorts considered for the elements 
	const std::set<PFSymbol*> relations_; //!< Relations which are permuted by the symmetry's

	// Inspectors
	std::pair<std::list<int>, std::list<int> > getSymmetricLiterals(AbstractGroundTheory*, const DomainElement*, const DomainElement*) const;

public:
	IVSet(const Structure*, const std::set<const DomainElement*>, const std::set<Sort*>, const std::set<PFSymbol*>);

	~IVSet() {
	}

	// Mutators
	// none should come here, class is immutable :)

	// Inspectors
	const Structure* getStructure() const;
	const std::set<const DomainElement*>& getElements() const;
	const std::set<Sort*>& getSorts() const;
	const std::set<PFSymbol*>& getRelations() const;

	int getSize() const {
		return getElements().size();
	}
	bool containsMultipleElements() const;
	bool hasRelevantRelationsAndSorts() const;
	bool isRelevantSymmetry() const;
	bool isDontCare() const;
	bool isEnkelvoudig() const;
	std::vector<const IVSet*> splitBasedOnOccurrences(OccurrencesCounter*) const;
	std::vector<const IVSet*> splitBasedOnBinarySymmetries() const;

	void addSymBreakingPreds(AbstractGroundTheory*, bool) const;
	std::vector<std::map<int, int> > getBreakingSymmetries(AbstractGroundTheory*) const;
	std::vector<std::list<int> > getInterchangeableLiterals(AbstractGroundTheory*) const;

	// Output
	std::ostream& put(std::ostream& output) const;
};

std::vector<const IVSet*> findIVSets(const AbstractTheory*, const Structure*, const Term*);

void addSymBreakingPredicates(AbstractGroundTheory*, std::vector<const IVSet*>, bool nbModelsEquivalent);

/**
 * 	Theory analyzing visitor which extracts information relevant for symmetry detection.
 *	More specifically, it will extract
 *		a set of domain elements which should not be permuted by a symmetry,
 *		a set of sorts whose domain elements should not be permuted by a symmetry, and
 *		a set of relations which are used in the theory (to exclude unused but nonetheless defined relations)
 */
class TheorySymmetryAnalyzer : public DefaultTraversingTheoryVisitor {
	VISITORFRIENDS()
private:
	const Structure* structure_;
	std::set<Sort*> forbiddenSorts_;
	std::set<const DomainElement*> forbiddenElements_;
	std::set<PFSymbol*> usedRelations_;

	void markAsUnfitForSymmetry(const std::vector<Term*>&);
	void markAsUnfitForSymmetry(Sort*);
	void markAsUnfitForSymmetry(const DomainElement*);

	const Structure* getStructure() const {
		return structure_;
	}

public:

	TheorySymmetryAnalyzer(const Structure* s)
	: structure_(s) {
		/*		markAsUnfitForSymmetry(VocabularyUtils::intsort());
				markAsUnfitForSymmetry(VocabularyUtils::floatsort());
				markAsUnfitForSymmetry(VocabularyUtils::natsort());*/
	}

	void analyze(const AbstractTheory* t);
	void analyzeForOptimization(const Term* t);

	const std::set<Sort*>& getForbiddenSorts() const {
		return forbiddenSorts_;
	}

	const std::set<const DomainElement*>& getForbiddenElements() const {
		return forbiddenElements_;
	}

	const std::set<PFSymbol*>& getUsedRelations() const {
		return usedRelations_;
	}

	void addForbiddenSort(Sort* sort) {
		forbiddenSorts_.insert(sort);
	}

	void addUsedRelation(PFSymbol* relation) {
		usedRelations_.insert(relation);
	}
protected:
	void visit(const PredForm*);
	void visit(const FuncTerm*);
	void visit(const DomainTerm*);
	void visit(const EqChainForm*);
	void visit(const AggForm*);
	void visit(const AggTerm*);
};

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

class InterchangeabilityGroup {
	
private:
	std::unordered_map<PFSymbol*, std::unordered_set<unsigned int>* > symbolargs;
	std::unordered_set<const DomainElement*> elements;
  
  void getSymmetricLiterals(AbstractGroundTheory* gt, Structure* struc, const DomainElement* smaller, const DomainElement* bigger, std::vector<int>& out, std::vector<int>& sym_out) const;
	
public:
	InterchangeabilityGroup(std::vector<const DomainElement*>& domels, std::vector<PFSymbol*> symbs3val, 
		std::unordered_map<PFSymbol*, std::unordered_set<unsigned int>* >& symbargs);
	~InterchangeabilityGroup();
	void print(std::ostream& ostr);

	unsigned int getNrSwaps();
	bool hasSymbArg(PFSymbol* symb, unsigned int arg);
  
  void breakSymmetry(AbstractGroundTheory* gt, Structure* struc, bool nbModelsEquivalent) const;
};

class InterchangeabilitySet {
private:
	std::unordered_set<const Sort*> sorts;
	std::unordered_set<const DomainElement*> occursAsConstant;
	std::unordered_map<std::shared_ptr<ElementOccurrence>, std::vector<const DomainElement*>*, ElOcHash, ElOcEqual> partition;

public:
	const Structure* _struct;
	std::unordered_map<PFSymbol*, std::unordered_set<unsigned int>* > symbolargs; // symbols with corresponding arguments

	InterchangeabilitySet(const Structure* s) : _struct(s) {
		for (auto part : partition) {
			delete part.second;
		}
	}

	~InterchangeabilitySet() {
		for (auto sa : symbolargs) {
			delete sa.second;
		}
	}

	bool add(PFSymbol* p, unsigned int arg);
	bool add(const Sort* s);
	bool add(const DomainElement* de);

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

	UFNode* find(UFNode* in);
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
	//virtual void visit(const Theory*);
	//virtual void visit(const AbstractGroundTheory*);
	//virtual void visit(const GroundTheory<GroundPolicy>*);

	virtual void visit(const PredForm*);
	virtual void visit(const EqChainForm*);
	virtual void visit(const EquivForm*);
	//virtual void visit(const BoolForm*);
	//virtual void visit(const QuantForm*);
	//virtual void visit(const AggForm*);

	//virtual void visit(const GroundDefinition*);
	//virtual void visit(const PCGroundRule*);
	//virtual void visit(const AggGroundRule*);
	//virtual void visit(const GroundSet*);
	//virtual void visit(const GroundAggregate*);

	//virtual void visit(const CPReification*) {
	// TODO
	//}

	//virtual void visit(const Rule*);
	//virtual void visit(const Definition*);
	//virtual void visit(const FixpDef*);

	virtual void visit(const VarTerm*);
	virtual void visit(const FuncTerm*);
	virtual void visit(const DomainTerm*);
	virtual void visit(const AggTerm*);

	//virtual void visit(const CPVarTerm*) {
	// TODO
	//}
	//virtual void visit(const CPSetTerm*) {
	// TODO
	//}

	//virtual void visit(const EnumSetExpr*);
	virtual void visit(const QuantSetExpr*);
};

struct SymbArg{
	PFSymbol* symb;
	unsigned int arg;
};

void detectInterchangeability(std::vector<InterchangeabilityGroup*>& out_groups, const AbstractTheory* t, const Structure* s, const Term* obj = nullptr);
// NOTE: t will be modified, so make sure a clone was made before!
void getIntchGroups2(AbstractTheory* theo, const Structure* s, std::vector<InterchangeabilityGroup*>& out_groups, std::vector<std::pair<PFSymbol*, unsigned int> >& symbargs); 
void getIntchGroups(AbstractTheory* theo, const Structure* s, std::vector<InterchangeabilityGroup*>& out_groups);
// TODO: fix the "3 and "2"
void detectNeighborhoods(const Theory* t, const Structure* s, const Term* obj, std::vector<const Definition*>& objDefs, 
		std::vector<InterchangeabilityGroup*>& outHard, std::vector<InterchangeabilityGroup*>& outRelaxed, Theory* relaxedConstraints);