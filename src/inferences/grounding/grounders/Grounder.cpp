/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include <ctime>
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
		if (l == _true or l == _false) {
			if (formula.getType() == Conn::CONJ and l == _false) { // UNSAT
				gt->addUnitClause(1);
				gt->addUnitClause(-1);
			} // else SAT or irrelevant
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

int Grounder::_groundedatoms = 0;
tablesize Grounder::_fullgroundsize = tablesize(TableSizeType::TST_EXACT, 0);

Grounder::Grounder(AbstractGroundTheory* gt, const GroundingContext& context)
		: _grounding(gt), _context(context), _maxsize(tablesize(TableSizeType::TST_UNKNOWN, 0)) {
}

void Grounder::toplevelRun() const {
	ConjOrDisj formula;
	wrapRun(formula);
	addToGrounding(getGrounding(), formula);
	getGrounding()->closeTheory(); // FIXME should move or be reentrant, as multiple grounders write to the same theory!

	addToFullGroundSize(getMaxGroundSize());
	if (verbosity() > 0) {
		clog << "Already grounded " << toString(groundedAtoms()) <<" for a full grounding of " << toString(getMaxGroundSize()) << "\n";
	}
}

#include <inferences/grounding/grounders/FormulaGrounders.hpp>

// TODO unfinished code
void Grounder::wrapRun(ConjOrDisj& formula) const{
	auto start = clock();
//	auto set = getGlobal()->getOptions()->verbosities();
	//auto printtimes = set.find("t")!=string::npos && context()._component==CompContext::SENTENCE;
	auto printtimes = false;
	if(printtimes){
		cerr <<"Grounding formula " <<toString(this) <<"\n";
	}
	run(formula);
	if(printtimes){
		cerr <<"Grounding it took " <<(clock()-start)/1000 <<"ms\n";
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

void Grounder::setMaxGroundSize(const tablesize& maxsize) {
	_maxsize = maxsize;
}
