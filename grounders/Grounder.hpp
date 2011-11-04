#ifndef GROUNDER_HPP_
#define GROUNDER_HPP_

#include "commontypes.hpp"
#include "ground.hpp"

enum class Conn {
	DISJ, CONJ
};

struct ConjOrDisj {
	litlist literals;
	Conn type;
};

class AbstractGroundTheory;

class Grounder{
private:
	AbstractGroundTheory* _grounding;
	GroundingContext _context;

public:
	Grounder(AbstractGroundTheory* gt, const GroundingContext& context) :
			_grounding(gt), _context(context) {
	}
	virtual ~Grounder() {
	}

	void toplevelRun() const; // Guaranteed toplevel run.
	Lit groundAndReturnLit() const; // Explicitly requesting one literal equisat with subgrounding. NOTE: interprete returnvalue as if in conjunction (false is unsat, true is sat)
	virtual void run(ConjOrDisj& formula) const = 0;

	AbstractGroundTheory* grounding() const {
		return _grounding;
	}

	const GroundingContext& context() const {
		return _context;
	}
};

#endif /* GROUNDER_HPP_ */