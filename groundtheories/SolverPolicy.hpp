#ifndef SOLVERTHEORY_HPP_
#define SOLVERTHEORY_HPP_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cassert>

#include "ground.hpp" // TODO should remove
#include "ecnf.hpp" // TODO should remove

namespace MinisatID{
 	 class WrappedPCSolver;
}
typedef MinisatID::WrappedPCSolver SATSolver;

class TsSet;

/**
 *	A SolverTheory is a ground theory, stored as an instance of a SAT solver
 */
class SolverPolicy {
private:
	GroundTermTranslator* 				_termtranslator;
	SATSolver*							_solver;		// The SAT solver
	std::map<PFSymbol*,std::set<int> >	_defined;		// Symbols that are defined in the theory. This set is used to
														// communicate to the solver which ground atoms should be considered defined.
	std::set<unsigned int> 				_addedvarids;	// Variable ids that have already been added, together with their domain.

	int	_verbosity;

	const 	SATSolver& getSolver() const	{ return *_solver; }
			SATSolver& getSolver() 			{ return *_solver; }

public:
	void polRecursiveDelete() { }

	void polStartTheory(GroundTranslator*){}
	void polEndTheory(){}

	inline MinisatID::Atom createAtom(int lit){
		return MinisatID::Atom(abs(lit));
	}

	inline MinisatID::Literal createLiteral(int lit){
		return MinisatID::Literal(abs(lit),lit<0);
	}

	inline MinisatID::Weight createWeight(double weight){
	#warning "Dangerous cast from double to int in adding rules to the solver"
		return MinisatID::Weight(int(weight));	// TODO: remove cast when supported by the solver
	}

	void initialize(SATSolver* solver, int verbosity, GroundTermTranslator* termtranslator);

	void polAdd(const GroundClause& cl);

	void polAdd(const TsSet& tsset, int setnr, bool weighted);

	void polAdd(GroundDefinition* def);

	// NOTE: this method can be safely called from outside
	void polAdd(int defnr, PCGroundRule* rule);

	// NOTE: this method can be safely called from outside
	void polAdd(int defnr, AggGroundRule* rule);

	void polAdd(int defnr, int head, AggGroundRule* body, bool);

	void polAdd(int head, AggTsBody* body);

	void polAddWeightedSum(const MinisatID::Atom& head, const std::vector<VarId>& varids, const std::vector<int> weights, const int& bound, MinisatID::EqType rel, SATSolver& solver);

	void polAdd(int tseitin, CPTsBody* body);

	// FIXME probably already exists in transform for add?
	void polAdd(Lit tseitin, TsType type, const GroundClause& clause);

	void notifyLazyResidual(ResidualAndFreeInst* inst, LazyQuantGrounder const* const grounder);

	void polNotifyDefined(const Lit& lit, const ElementTuple& args, std::vector<LazyRuleGrounder*> grounders);

	std::ostream& polPut(std::ostream& s, GroundTranslator*, GroundTermTranslator*, bool)	const { assert(false); return s;	}
	std::string polToString(GroundTranslator*, GroundTermTranslator*, bool) const { assert(false); return "";		}

private:
	void polAddAggregate(int definitionID, int head, bool lowerbound, int setnr, AggFunction aggtype, TsType sem, double bound);

	void polAddCPVariables(const std::vector<VarId>& varids, GroundTermTranslator* termtranslator);

	void polAddCPVariable(const VarId& varid, GroundTermTranslator* termtranslator);

	void polAddPCRule(int defnr, int head, std::vector<int> body, bool conjunctive, bool);
};


#endif /* SOLVERTHEORY_HPP_ */
