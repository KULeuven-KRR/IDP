 #ifndef LAZYQUANTGROUNDER_HPP_
#define LAZYQUANTGROUNDER_HPP_

#include "inferences/grounding/grounders/FormulaGrounders.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"

#include <map>

typedef GroundTheory<SolverPolicy> SolverTheory;

class LazyQuantGrounder : public QuantGrounder {
private:
	static unsigned int maxid;
	unsigned int id_;

	SolverTheory* groundtheory_;

	mutable bool _negatedformula;

	mutable bool currentlyGrounding; // If true, groundMore is currently already on the stack, so do not call it again!
	mutable std::queue<ResidualAndFreeInst *> queuedtseitinstoground; // Stack of what we still have to ground

	const std::set<Variable*> freevars; // The freevariables according to which we have to ground

	public:
	LazyQuantGrounder(const std::set<Variable*>& freevars,
						SolverTheory* groundtheory,
						FormulaGrounder* sub,
						SIGN sign,
						QUANT q,
						InstGenerator* gen,
						InstChecker* checker,
						const GroundingContext& ct);

	// TODO for some reason, the callback framework does not compile when using the const method groundmore directly.
	void requestGroundMore(ResidualAndFreeInst* instance);
	void groundMore() const;

	unsigned int id() const { return id_; }

	void notifyTheoryOccurence(ResidualAndFreeInst* instance) const;

protected:
	virtual void run(ConjOrDisj& literals, bool negatedformula) const;
};

#endif /* LAZYQUANTGROUNDER_HPP_ */
