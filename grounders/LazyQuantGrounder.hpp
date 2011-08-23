/************************************
 LazyQuantGrounder.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef LAZYQUANTGROUNDER_HPP_
#define LAZYQUANTGROUNDER_HPP_

#include "ground.hpp"
#include "grounders/FormulaGrounders.hpp"

class LazyQuantGrounder : public QuantGrounder {
private:
	mutable bool negatedclause_;

	Lit createTseitin() const;
public:
#warning only create in cases where it reduces to an existential quantifier!!!!
#warning currently only non-defined!
	LazyQuantGrounder(GroundTranslator* gt, FormulaGrounder* sub, SIGN sign, QUANT q, InstGenerator* gen, const GroundingContext& ct):
			QuantGrounder(gt,sub,sign, q, gen,ct) {
	}

	void groundMore() const;

protected:
	virtual void	run(litlist&, bool negatedclause = true)	const;
};

#endif /* LAZYQUANTGROUNDER_HPP_ */
