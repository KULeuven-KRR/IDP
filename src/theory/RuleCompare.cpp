/*
 * RuleCompare.cpp
 *
 *  Created on: Mar 14, 2013
 *      Author: joachim
 */

#include "RuleCompare.hpp"
#include "theory.hpp"
#include "vocabulary/vocabulary.hpp"

using namespace std;

bool RuleCompare::operator()(const Rule* lhs, const Rule* rhs) const {
	return lhs->getID() < rhs->getID();
}
