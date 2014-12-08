/* 
 * File:   TwoValuedStructureIterator.cpp
 * Author: rupsbant
 * 
 * Created on December 5, 2014, 3:20 PM
 */

#include "TwoValuedStructureIterator.hpp"
#include "PartialStructureIterator.hpp"
#include <stddef.h>
#include "Assert.hpp"

TwoValuedStructureIterator::TwoValuedStructureIterator(Structure* original) {
    structure = original->clone();
    stack = create(original);
}

TwoValuedStructureIterator::~TwoValuedStructureIterator() {
}

bool TwoValuedStructureIterator::isFinished() {
    return position < 0;
}

Structure* TwoValuedStructureIterator::next() {
    if (position < 0) {
        return NULL;
    }
    while (position < stack.size()) {
        stack[position]->doNext(structure);
        position++;
    }
    Structure* out = structure->clone();
    out->clean();
    Assert(out->approxTwoValued());
    for (; position >= 0;) {
        position--;
        if (position < 0 or not stack[position]->isFinished()) {
            break;
        }
        stack[position]->undo(structure);
    }
    return out;
}
