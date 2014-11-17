/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include <ctime>
#include "common.hpp"
#include "Grounder.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "inferences/grounding/GroundTranslator.hpp"
#include "errorhandling/UnsatException.hpp"

using namespace std;

Conn negateConn(Conn c) {
	switch (c) {
	case Conn::DISJ:
		return Conn::CONJ;
	case Conn::CONJ:
		return Conn::DISJ;
	}
	Assert(false);
	return Conn::DISJ;
}

void ConjOrDisj::put(std::ostream& stream) const {
	bool begin = true;
	for (auto lit : literals) {
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
		stream << print(lit);
	}
}
void ConjOrDisj::negate() {
	_type = negateConn(_type);
	for (size_t i = 0; i < literals.size(); ++i) {
		literals[i] = -literals.at(i);
	}
}

void addToGrounding(AbstractGroundTheory* gt, ConjOrDisj& formula) {
	if (formula.literals.size() == 0) {
		if (formula.getType() == Conn::DISJ) { // UNSAT
			gt->add(GroundClause{});
			throw UnsatException();
		}
	} else if (formula.literals.size() == 1) {
		auto l = formula.literals.back();
		auto falselit = gt->translator()->falseLit();
		if (l == gt->translator()->trueLit() or l == falselit) {
			if (formula.getType() == Conn::CONJ and l == falselit) { // UNSAT
				gt->add(GroundClause{});
				throw UnsatException();
			} // else SAT or irrelevant
		} else {
			gt->addUnitClause(l);
		}
	} else {
		if (formula.getType() == Conn::CONJ) {
			for (auto lit : formula.literals) {
				gt->addUnitClause(lit);
			}
		} else {
			gt->add(formula.literals);
		}
	}
}

int Grounder::_groundedatoms = 0;
tablesize Grounder::_fullgroundsize = tablesize(TableSizeType::TST_EXACT, 0);

Grounder::Grounder(AbstractGroundTheory* gt, const GroundingContext& context)
		: _grounding(gt), _context(context), _maxsize(tablesize(TableSizeType::TST_INFINITE, 0)){
}

GroundTranslator* Grounder::translator() const {
	return getGrounding()->translator();
}

Grounder::~Grounder() {
}

bool Grounder::toplevelRun(LazyGroundingRequest& request) {
	auto unsat = false;
	ConjOrDisj formula;
	try {
		run(formula, request);
		addToGrounding(getGrounding(), formula);
		getGrounding()->closeTheory(); // FIXME should move or be reentrant, as multiple grounders write to the same theory!
	} catch (UnsatException& ex) {
		if (verbosity() > 0) {
			clog << "Unsat found during grounding\n";
		}
		unsat = true;
	}

	if (verbosity() > 1) {
		clog << "Already grounded " << print(groundedAtoms()) << " for a full grounding of " << print(getMaxGroundSize()) << "\n";
	}
	return unsat;
}

#include <inferences/grounding/grounders/FormulaGrounders.hpp>

// TODO unfinished code for printing the timings of a grounder
void Grounder::run(ConjOrDisj& formula, LazyGroundingRequest& request){
	internalRun(formula, request);
}

Lit Grounder::groundAndReturnLit(LazyGroundingRequest& request) {
	ConjOrDisj formula;
	internalRun(formula, request);
	if (formula.literals.size() == 0) {
		if (formula.getType() == Conn::DISJ) {
			return _false;
		} else {
			return _true;
		}
	} else if (formula.literals.size() == 1) {
		return formula.literals.back();
	} else {
		return getGrounding()->translator()->reify(formula.literals, formula.getType() == Conn::CONJ, getContext()._tseitin);
	}
}

void Grounder::setMaxGroundSize(const tablesize& maxsize) {
	_maxsize = maxsize;
}

int Grounder::verbosity() const {
	return getOption(IntType::VERBOSE_GROUNDING);
}
