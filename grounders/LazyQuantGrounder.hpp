/************************************
 LazyQuantGrounder.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef LAZYQUANTGROUNDER_HPP_
#define LAZYQUANTGROUNDER_HPP_

#include "ground.hpp"
#include "grounders/FormulaGrounders.hpp"

typedef GroundTheory<SolverPolicy> SolverTheory;

class LazyQuantGrounder : public QuantGrounder {
private:
	static unsigned int maxid;
	unsigned int id_;
	mutable bool negatedclause_;
	SolverTheory* groundtheory_;

	Lit createTseitin() const;
	public:
#warning only create in cases where it reduces to an existential quantifier!!!!
#warning currently only non-defined!
	LazyQuantGrounder(SolverTheory* groundtheory, GroundTranslator* gt, FormulaGrounder* sub, SIGN sign, QUANT q, InstGenerator* gen, const GroundingContext& ct):
			QuantGrounder(gt,sub,sign, q, gen,ct),
			id_(maxid++),
			negatedclause_(false),
			groundtheory_(groundtheory){
	}

	// TODO for some reason, the callback framework does not compile when using the const method groundmore directly.
	void requestGroundMore();
	void groundMore() const;

	unsigned int id() const { return id_; }

protected:
	virtual void	run(litlist&, bool negatedclause = true)	const;
};

#endif /* LAZYQUANTGROUNDER_HPP_ */
