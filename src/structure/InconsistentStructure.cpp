/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "InconsistentStructure.hpp"

#include "vocabulary/vocabulary.hpp"

using namespace std;

void InconsistentStructure::inter(Predicate*, PredInter*) {
	throw IdpException("Trying to set the interpretation of a predicate for an inconsistent structure");
}
void InconsistentStructure::inter(Function*, FuncInter*) {
	throw IdpException("Trying to set the interpretation of a functions for an inconsistent structure");
}

SortTable* InconsistentStructure::inter(Sort*) const {
	throw IdpException("Trying to get the interpretation of a sort in an inconsistent structure");
}
PredInter* InconsistentStructure::inter(Predicate*) const {
	throw IdpException("Trying to get the interpretation of a predicate in an inconsistent structure");
}
FuncInter* InconsistentStructure::inter(Function*) const {
	throw IdpException("Trying to get the interpretation of a function in an inconsistent structure");
}
PredInter* InconsistentStructure::inter(PFSymbol*) const {
	throw IdpException("Trying to get the interpretation of a symbol in an inconsistent structure");
}

const std::map<Predicate*, PredInter*>& InconsistentStructure::getPredInters() const {
	throw IdpException("Trying to get the list of interpretations of an inconsistent structure");
}
const std::map<Function*, FuncInter*>& InconsistentStructure::getFuncInters() const {
	throw IdpException("Trying to get the list of interpretations of an inconsistent structure");
}

void InconsistentStructure::makeTwoValued() {
	throw IdpException("Cannot make an inconsistent structure two-valued");
}

AbstractStructure* InconsistentStructure::clone() const {
	return new InconsistentStructure(_name, _pi);
}

Universe InconsistentStructure::universe(const PFSymbol*) const {
	throw IdpException("Trying to get the universe for a symbol in an inconsistent structure");
}

bool InconsistentStructure::approxTwoValued() const {
	return false;
}
bool InconsistentStructure::isConsistent() const {
	return false;
}
