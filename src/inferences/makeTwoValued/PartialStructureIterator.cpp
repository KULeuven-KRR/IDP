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

PartialFunctionPreciseCommand::PartialFunctionPreciseCommand(const ElementTuple& tuple, std::pair<Function*, FuncInter*> pair) : _tuple(tuple) {
    _functionInterpretation = pair;

    auto inter = _functionInterpretation.second;
    auto universe = inter->graphInter()->universe();
    const auto& sorts = universe.tables();
    auto s = sorts.back()->sortBegin();
    _iterator = new SortIterator(s);
    _prevTuple = _tuple;
    _prevTuple.push_back(**_iterator);
    auto function = _functionInterpretation.first;
    if (function->partial()) {
        _doPartial = true;
    }
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
        Assert(inter->pf()->contains(_tuple));
        Assert(inter->pt()->contains(_tuple));
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
    return _iterator == NULL;
}

void PartialFunctionPreciseCommand::undo(Structure* s) {
    auto function = _functionInterpretation.first;
    auto graph = s->inter(function)->graphInter();
    auto universe = graph->universe();
    const auto& sorts = universe.tables();
    for (; _iterator != NULL && not _iterator->isAtEnd(); ++_iterator) { //Make the rest unknown
        ElementTuple tuple(_tuple);
        tuple.push_back(**_iterator);
        graph->makeUnknownExactly(tuple);
    }
    _iterator = new SortIterator(sorts.back()->sortBegin());
    _prevTuple = _tuple;
    _prevTuple.push_back(**_iterator);
    if (function->partial()) {
        _doPartial = true;
    }
}

void PartialFunctionPreciseCommand::doNext(Structure* s) {
    Assert(_doPartial || _iterator != NULL);
    auto function = _functionInterpretation.first;
    auto inter = _functionInterpretation.second;
    Assert(not inter->approxTwoValued());
    auto graph = s->inter(function)->graphInter();
    auto universe = graph->universe();
    const auto& sorts = universe.tables();
    if (_doPartial) { //Make ALL false
        _doPartial = false;
        auto it = sorts.back()->sortBegin();
        for (; not it.isAtEnd(); ++it) {
            ElementTuple tuple(_tuple);
            tuple.push_back(*it);
            graph->makeFalseExactly(tuple);
        }
    } else { //Make next element true.
        ElementTuple tuple(_tuple);
        tuple.push_back(**_iterator);
        graph->makeUnknownExactly(_prevTuple);
        graph->makeTrueExactly(tuple);
        _prevTuple = tuple;
        if (_iterator->isAtEnd()) {
            _iterator = NULL;
        } else {
            ++_iterator;
        }
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
        auto internaliterator = new CartesianInternalTableIterator(domainIterators, domainIterators, true);
        TableIterator domainIterator(internaliterator);

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
            PreciseCommand* p = new PartialFunctionPreciseCommand(domainElementWithoutValue, f2inter);
            out.push_back(p);
        }
    }
    return out;
}
