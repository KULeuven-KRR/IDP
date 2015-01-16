/* 
 * File:   PartialStructureIterator.hpp
 * Author: rupsbant
 *
 * Created on December 5, 2014, 3:19 PM
 */

#pragma once

#include "structure/Structure.hpp"
#include "structure/StructureComponents.hpp"
#include <vector>
#include <unordered_set>
#include <memory>

/**
 * This class creates all more precise structures of a given structure and function/predicate.
 */
class TwoValuedSymbolIterator {
public:
	virtual ~TwoValuedSymbolIterator();

	virtual void doNext(Structure*) = 0;
	virtual void undo(Structure*) = 0;
	virtual bool isFinished() = 0;
};

class PredicateSymbolIterator : public TwoValuedSymbolIterator {
public:
	PredicateSymbolIterator(const ElementTuple&, std::pair<Predicate*, PredInter*>);
	~PredicateSymbolIterator();
	void doNext(Structure*);
	void undo(Structure*);
	bool isFinished();
private:
	std::pair<Predicate*, PredInter*> _predicateInterpretation;
	ElementTuple _tuple;
	int state;
};

class FunctionSymbolIterator : public TwoValuedSymbolIterator {
public:
	FunctionSymbolIterator(const ElementTuple& tuple, std::pair<Function*, FuncInter*>);
	~FunctionSymbolIterator();
	void init(Structure* s);
	void doNext(Structure*);
	void undo(Structure*);
	bool isFinished();
private:
	std::pair<Function*, FuncInter*> _functionInterpretation;
	ElementTuple _tuple;
	std::unique_ptr<SortIterator> _iterator;
	ElementTuple _prevTuple;
	bool _doPartial;
	std::vector<const DomainElement*> falsied;
};

/**
 * Creates a PartialStructureIterator that creates more precise structures.
 * Some function or predicate is chosen to make more precise.
 * @param A structure to make more precise.
 * @return An iterator generating more precise structures.
 */
std::vector<TwoValuedSymbolIterator*> create(Structure*);
std::vector<TwoValuedSymbolIterator*> createFunction(Structure* s);
std::vector<TwoValuedSymbolIterator*> createPredicate(Structure* s);

