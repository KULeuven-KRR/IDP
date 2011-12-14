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
			ss << "Splitting into monotone aggregates not supported for " << toString(agg);
			throw notyetimplemented(ss.str());
		}
	}
	Formula* splitInto(AggForm* f, CompType left, CompType right) {
		auto aggone = new AggForm(f->sign(), f->left()->clone(), left, f->right()->clone(), FormulaParseInfo());
		auto aggtwo = new AggForm(f->sign(), f->left()->clone(), right, f->right()->clone(), FormulaParseInfo());

		checkMonotonicityAndThrow(aggone);
		checkMonotonicityAndThrow(aggtwo);
		auto bf = new BoolForm(SIGN::POS, true, aggone, aggtwo, f->pi().clone());
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
