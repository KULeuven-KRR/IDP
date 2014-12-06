/* 
 * File:   TwoValuedStructureIterator.cpp
 * Author: rupsbant
 * 
 * Created on December 5, 2014, 3:20 PM
 */

#include "TwoValuedStructureIterator.hpp"
#include "PartialStructureIterator.hpp"
#include <stddef.h>

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
    while (not structure->approxTwoValued()) {
        stack[position]->doNext(structure);
        position++;
    }
    Structure* out = structure->clone();
    for (; position >= 0; position--) {
        if (not stack[position]->isFinished()) {
            break;
        }
        stack[position]->undo(structure);
    }
    return out;
}
