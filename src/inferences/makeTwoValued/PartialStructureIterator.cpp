/* 
 * File:   PartialStructureIterator.cpp
 * Author: rupsbant
 * 
 * Created on December 5, 2014, 3:19 PM
 */

#include <stddef.h>
#include <vector>
#include <map>
#include "PartialStructureIterator.hpp"
#include "vocabulary/vocabulary.hpp"
#include "../../Assert.hpp"

PartialPredicatePreciseCommand::PartialPredicatePreciseCommand
(const ElementTuple& tuple, std::pair<Predicate*, PredInter*> pair) : _tuple(tuple) {
    _predicateInterpretation = pair;
}

PreciseCommand::~PreciseCommand() {

}

PartialFunctionPreciseCommand::~PartialFunctionPreciseCommand() {
}

PartialPredicatePreciseCommand::~PartialPredicatePreciseCommand() {
}

void PartialPredicatePreciseCommand::doNext(Structure* s) {
    Assert(state != 2);
    auto pred = _predicateInterpretation.first;
    auto inter = _predicateInterpretation.second;
    Assert(inter != NULL);
    Assert(not inter->approxTwoValued());

    auto predInter = s->inter(pred);
    if (state == 0) {
        //Tuple is unknown.
        Assert(inter->isUnknown(_tuple));
        predInter->makeTrueExactly(_tuple);
        state++;
    } else if (state == 1) {
        predInter->makeFalseExactly(_tuple);
        state++;
    }
}

void PartialPredicatePreciseCommand::undo(Structure* s) {
    auto pred = _predicateInterpretation.first;
    auto predInter = s->inter(pred);
    predInter->makeUnknownExactly(_tuple);
    state = 0;
}

bool PartialPredicatePreciseCommand::isFinished() {
    return state == 2;
}

bool PartialFunctionPreciseCommand::isFinished() {
    return not _doPartial && _iterator == NULL;
}

PartialFunctionPreciseCommand::PartialFunctionPreciseCommand(const ElementTuple& tuple, std::pair<Function*, FuncInter*> pair) : _tuple(tuple) {
    _functionInterpretation = pair;

    auto inter = _functionInterpretation.second;
    auto universe = inter->graphInter()->universe();
    const auto& sorts = universe.tables();
    auto s = sorts.back()->sortBegin();
    _iterator = std::unique_ptr<SortIterator>(new SortIterator(s));
    _prevTuple = _tuple;
    //Assume at least 1 element in _iterator.
    _prevTuple.push_back(**_iterator);
}

void PartialFunctionPreciseCommand::init(Structure* s) {
    auto function = _functionInterpretation.first;
    auto inter = _functionInterpretation.second;
    Assert(not inter->approxTwoValued());
    auto graph = s->inter(function)->graphInter();
    //Find next unknown.
    while (not _iterator->isAtEnd()) {
        ElementTuple tuple(_tuple);
        tuple.push_back(**_iterator);
        if (graph->isUnknown(tuple)) {
            return;
        }
        ++(*_iterator);
    }
    if (_iterator->isAtEnd()) {
        _iterator = NULL;
        if (function->partial()) {
            _doPartial = true;
        }
    }
}

void PartialFunctionPreciseCommand::doNext(Structure* s) {
    Assert(not isFinished());
    auto function = _functionInterpretation.first;
    auto inter = _functionInterpretation.second;
    Assert(not inter->approxTwoValued());
    auto graph = s->inter(function)->graphInter();
    if (_doPartial) { //Make ALL false
        _doPartial = false;
        graph->makeFalseExactly(_prevTuple);
    } else { //Make next element true.
        ElementTuple tuple(_tuple);
        tuple.push_back(**_iterator);
        Assert(graph->isUnknown(tuple));
        falsied.push_back(**_iterator);
        graph->makeFalseExactly(_prevTuple);
        graph->makeTrueExactly(tuple);
        _prevTuple = tuple;
        ++(*_iterator);
        init(s);
    }
}

void PartialFunctionPreciseCommand::undo(Structure* s) {
    auto function = _functionInterpretation.first;
    auto graph = s->inter(function)->graphInter();
    auto universe = graph->universe();
    const auto& sorts = universe.tables();
    ElementTuple tuple(_tuple);
    for (auto it = falsied.cbegin(); it != falsied.cend(); ++it) {
        tuple.push_back(*it);
        graph->makeUnknownExactly(tuple);
        tuple.pop_back();
    }
    _iterator = std::unique_ptr<SortIterator>(new SortIterator(sorts.back()->sortBegin()));
    init(s);
    if (_iterator != NULL) {
        //Set the first found element as prevTuple
        _prevTuple = _tuple;
        _prevTuple.push_back(**_iterator);
    }
}

std::vector<PreciseCommand*> create(Structure* s) {
    auto out1 = createPredicate(s);
    auto out2 = createFunction(s);
    out1.insert(out1.end(), out2.begin(), out2.end());
    return out1;
}

std::vector<PreciseCommand*> createPredicate(Structure* s) {
    std::vector<PreciseCommand*> out;
    Structure* original = s;
    for (auto i = original->getPredInters().cbegin(); i != original->getPredInters().end(); i++) {
        PredInter* inter = (*i).second;
        Assert(inter != NULL);
        if (inter->approxTwoValued()) {
            continue;
        }
        auto pf = inter->pf();
        auto pt = inter->pt();
        for (auto ptIterator = pt->begin(); not ptIterator.isAtEnd(); ++ptIterator) {
            if (not pf->contains(*ptIterator)) {
                continue;
            }
            PreciseCommand* p = new PartialPredicatePreciseCommand(*ptIterator, *i);
            out.push_back(p);
        }
    }
    return out;
}

std::vector<PreciseCommand*> createFunction(Structure* s) {
    std::vector<PreciseCommand*> out;
    Structure* original = s;
    for (auto f2inter : original->getFuncInters()) {
        Function* function = f2inter.first;
        FuncInter* inter = f2inter.second;
        if (inter->approxTwoValued()) {
            continue;
        }
        auto universe = inter->graphInter()->universe();
        const auto& sorts = universe.tables();
        std::vector<SortIterator> domainIterators;
        for (auto sort : sorts) {
            const auto& temp = SortIterator(sort->internTable()->sortBegin());
            domainIterators.push_back(temp);
        }
        domainIterators.pop_back();
        auto ct = inter->graphInter()->ct();
        //Now, choose an image for this domainelement
        ElementTuple domainElementWithoutValue;
        if (sorts.size() == 0) {
            PreciseCommand* p = new PartialFunctionPreciseCommand(domainElementWithoutValue, f2inter);
            out.push_back(p);
            continue;
        }
        TableIterator domainIterator(new CartesianInternalTableIterator(domainIterators, domainIterators, true));

        auto ctIterator = ct->begin();
        FirstNElementsEqual eq(function->arity());
        StrictWeakNTupleOrdering so(function->arity());
        for (; not domainIterator.isAtEnd(); ++domainIterator) {
            // get unassigned domain element
            domainElementWithoutValue = *domainIterator;
            while (not ctIterator.isAtEnd() && so(*ctIterator, domainElementWithoutValue)) {
                ++ctIterator;
            }
            if (not ctIterator.isAtEnd() && eq(domainElementWithoutValue, *ctIterator)) {
                continue;
            }
            PartialFunctionPreciseCommand* p = new PartialFunctionPreciseCommand(domainElementWithoutValue, f2inter);
            p->init(s);
            out.push_back(p);
        }
    }
    return out;
}
