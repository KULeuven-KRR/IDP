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

#ifndef SPLITAGGBOUNDS_HPP_
#define SPLITAGGBOUNDS_HPP_

#include "visitors/TheoryMutatingVisitor.hpp"

class SplitIntoMonotoneAgg: public TheoryMutatingVisitor {
	VISITORFRIENDS()
public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}
protected:
	void checkMonotonicityAndThrow(AggForm* agg) {
		if (not FormulaUtils::isMonotone(agg) && not FormulaUtils::isAntimonotone(agg)) {
			std::stringstream ss;
			ss << "Splitting into monotone aggregates for " << print(agg);
			throw notyetimplemented(ss.str());
		}
	}
	Formula* splitInto(AggForm* f, CompType left, CompType right) {
		auto aggone = new AggForm(f->sign(), f->getBound()->clone(), left, f->getAggTerm()->clone(), FormulaParseInfo());
		auto aggtwo = new AggForm(f->sign(), f->getBound()->clone(), right, f->getAggTerm()->clone(), FormulaParseInfo());

		checkMonotonicityAndThrow(aggone);
		checkMonotonicityAndThrow(aggtwo);
		auto bf = new BoolForm(SIGN::POS, true, aggone, aggtwo, f->pi());
		f->recursiveDelete();
		return bf;
	}

	Formula* visit(AggForm* f) {
		if (f->comp() == CompType::EQ) {
			return splitInto(f, CompType::LEQ, CompType::GEQ);
		} else if (f->comp() == CompType::NEQ) {
			return splitInto(f, CompType::LT, CompType::GT);
		} else {
			checkMonotonicityAndThrow(f);
			return f;
		}
	}
};

#endif /* SPLITAGGBOUNDS_HPP_ */
