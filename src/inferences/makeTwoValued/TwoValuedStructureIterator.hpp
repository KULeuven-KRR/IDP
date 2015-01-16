/* 
 * File:   TwoValuedStructureIterator.hpp
 * Author: rupsbant
 *
 * Created on December 5, 2014, 3:20 PM
 */
#pragma once

#include "PartialStructureIterator.hpp"
#include "structure/Structure.hpp"

/**
 * This class generates all more precise two valued structures.
 */
class TwoValuedStructureIterator {
public:
	TwoValuedStructureIterator(Structure*);
	TwoValuedStructureIterator(const TwoValuedStructureIterator& orig);
	virtual ~TwoValuedStructureIterator();
	Structure* next();
	bool isFinished();
private:
	std::vector<TwoValuedSymbolIterator*> stack;
	int position;
	Structure* structure;
};
