/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "common.hpp"
#include "Grounder.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

using namespace std;

Conn negateConn(Conn c) {
	Conn result;
	switch (c) {
	case Conn::DISJ:
		result = Conn::CONJ;
		break;
	case Conn::CONJ:
		result = Conn::DISJ;
		break;
	}
	return result;
}

void ConjOrDisj::put(std::ostream& stream) const {
	bool begin = true;
	for (auto i = literals.cbegin(); i < literals.cend(); ++i) {
		if (not begin) {
			switch (getType()) {
			case Conn::DISJ:
				stream << " | ";
				break;
			case Conn::CONJ:
				stream << " & ";
				break;
			}
		}
		begin = false;
		stream << toString(*i);
	}
}
void ConjOrDisj::negate() {
	_type = negateConn(_type);
	for (size_t i = 0; i < literals.size(); ++i) {
		literals[i] = -literals.at(i);
	}
}

GroundTranslator* Grounder::getTranslator() const {
	return _grounding->translator();
}

void addToGrounding(AbstractGroundTheory* gt, ConjOrDisj& formula) {
	if (formula.literals.size() == 0) {
		if (formula.getType() == Conn::DISJ) { // UNSAT
			gt->addUnitClause(1);
			gt->addUnitClause(-1);
		}
	} else if (formula.literals.size() == 1) {
		Lit l = formula.literals.back();
		if (l == _true || l == _false) {
			if (formula.getType() == Conn::CONJ && l == _false) { // UNSAT
				gt->addUnitClause(1);
				gt->addUnitClause(-1);
			} // else SAT or irrelevant (TODO correct?)
		} else {
			gt->addUnitClause(l);
		}
	} else {
		if (formula.getType() == Conn::CONJ) {
			for (auto i = formula.literals.cbegin(); i < formula.literals.cend(); ++i) {
				gt->addUnitClause(*i);
			}
		} else {
			gt->add(formula.literals);
		}
	}
}

void Grounder::toplevelRun() const {
	//Assert(context()._conjunctivePathFromRoot);
	ConjOrDisj formula;
	run(formula);
	addToGrounding(getGrounding(), formula);
	if(not getOption(BoolType::GROUNDLAZILY)){
		getGrounding()->closeTheory(); // TODO very important and easily forgotten
	}
}

Lit Grounder::groundAndReturnLit() const {
	ConjOrDisj formula;
	run(formula);
	if (formula.literals.size() == 0) {
		if (formula.getType() == Conn::DISJ) {
			return _false;
		} else {
			return _true;
		}
	} else if (formula.literals.size() == 1) {
		return formula.literals.back();
	} else {
		return getGrounding()->translator()->translate(formula.literals, formula.getType() == Conn::CONJ, context()._tseitin);
	}
}
