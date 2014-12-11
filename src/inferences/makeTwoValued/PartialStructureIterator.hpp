/* 
 * File:   PartialStructureIterator.hpp
 * Author: rupsbant
 *
 * Created on December 5, 2014, 3:19 PM
 */

#ifndef PARTIALSTRUCTUREITERATOR_HPP
#define	PARTIALSTRUCTUREITERATOR_HPP

#include "structure/Structure.hpp"
#include "structure/StructureComponents.hpp"
#include <vector>
#include <unordered_set>
/**
 * This class creates all more precise structures of a given structure and function/predicate.
 */
class PreciseCommand {
public:
    virtual ~PreciseCommand();

    virtual void doNext(Structure*) = 0;
    virtual void undo(Structure*) = 0;
    virtual bool isFinished() = 0;
};

class PartialPredicatePreciseCommand : public PreciseCommand {
public:
    PartialPredicatePreciseCommand(const ElementTuple&, std::pair<Predicate*, PredInter*>);
    ~PartialPredicatePreciseCommand();
    void doNext(Structure*);
    void undo(Structure*);
    bool isFinished();
private:
    std::pair<Predicate*, PredInter*> _predicateInterpretation;
    ElementTuple _tuple;
    int state = 0;
};

class PartialFunctionPreciseCommand : public PreciseCommand {
public:
    PartialFunctionPreciseCommand (const ElementTuple& tuple, std::pair<Function*, FuncInter*>);
    ~PartialFunctionPreciseCommand();
    void init(Structure* s);    
    void doNext(Structure*);
    void undo(Structure*);
    bool isFinished();
private:
    std::pair<Function*, FuncInter*> _functionInterpretation;
    ElementTuple _tuple;
    SortIterator* _iterator;
    ElementTuple _prevTuple;
    bool _doPartial = false;
    std::vector<const DomainElement*> falsied;
};

/**
 * Creates a PartialStructureIterator that creates more precise structures.
 * Some function or predicate is chosen to make more precise.
 * @param A structure to make more precise.
 * @return An iterator generating more precise structures.
 */
std::vector<PreciseCommand*> create(Structure*);
std::vector<PreciseCommand*> createFunction(Structure* s);
std::vector<PreciseCommand*> createPredicate(Structure* s);
#endif	/* PARTIALSTRUCTUREITERATOR_HPP */

