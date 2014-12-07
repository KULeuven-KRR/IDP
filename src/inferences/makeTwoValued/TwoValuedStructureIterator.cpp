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
    std::cout << "size: " << stack.size() << "\n";
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
        std::cout << "pos: " << position << "\n";
        std::cout << structure << "\n";
        if (position >= stack.size()) {
            std::cout << "out of specifiers\n";
            break;
        }
        stack[position]->doNext(structure);
        position++;
    }
    Structure* out = structure->clone();
    //clean(out);
    for (; position >= 0;) {
        position--;
        if (position < 0 or not stack[position]->isFinished()) {
            std::cout << "posNotFinished: " << position << "\n";
            break;
        }
        std::cout << "undo: " << position << "\n";
        stack[position]->undo(structure);
    }
    std::cout << "final: " << position << "\n";
    return out;
}
