/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef SOLVERTHEORY_HPP_
#define SOLVERTHEORY_HPP_

#include "IncludeComponents.hpp"
#include "external/ExternalInterface.hpp"

namespace MinisatID {
class WrappedPCSolver;
}
typedef MinisatID::WrappedPCSolver SATSolver;

class TsSet;
class LazyRuleGrounder;
class DomainElement;
typedef std::vector<const DomainElement*> ElementTuple;

/**
 *	A SolverTheory is a ground theory, stored as an instance of a SAT solver
 */
class SolverPolicy {
private:
	GroundTermTranslator* _termtranslator;
	SATSolver* _solver; // The SAT solver
	std::map<PFSymbol*, std::set<int> > _defined; // Symbols that are defined in the theory. This set is used to
												  // communicate to the solver which ground atoms should be considered defined.
	std::set<VarId> _addedvarids; // Variable ids that have already been added, together with their domain.

	int _verbosity;

	const SATSolver& getSolver() const {
		return *_solver;
	}
	SATSolver& getSolver() {
		return *_solver;
	}

public:
	void polRecursiveDelete() {
	}

	void polStartTheory(GroundTranslator*) {
	}
	void polEndTheory() {
	}

	inline MinisatID::Atom createAtom(int lit) {
		return MinisatID::Atom(abs(lit));
	}

	inline MinisatID::Literal createLiteral(int lit) {
		return MinisatID::Literal(abs(lit), lit < 0);
	}

	MinisatID::Weight createWeight(double weight);

	void initialize(SATSolver* solver, int verbosity, GroundTermTranslator* termtranslator);

	void polAdd(const GroundClause& cl);
	void polAdd(const TsSet& tsset, int setnr, bool weighted);
	void polAdd(int defnr, PCGroundRule* rule);
	void polAdd(int defnr, AggGroundRule* rule);
	void polAdd(int defnr, int head, AggGroundRule* body, bool);
	void polAdd(int head, AggTsBody* body);
	void polAddWeightedSum(const MinisatID::Atom& head, const std::vector<VarId>& varids, const std::vector<int> weights, const int& bound, MinisatID::EqType rel, SATSolver& solver);
	void polAdd(int tseitin, CPTsBody* body);

	// FIXME probably already exists in transform for add?
	void polAdd(Lit tseitin, TsType type, const GroundClause& clause);

	void notifyLazyResidual(ResidualAndFreeInst* inst, LazyQuantGrounder const* const grounder);

	void polNotifyDefined(const Lit& lit, const ElementTuple& args, std::vector<LazyRuleGrounder*> grounders);

	std::ostream& polPut(std::ostream& s, GroundTranslator*, GroundTermTranslator*) const {
		Assert(false);
		return s;
	}
	std::string polToString(GroundTranslator*, GroundTermTranslator*, bool) const {
		Assert(false);
		return "";
	}

private:
	void polAddAggregate(int definitionID, int head, bool lowerbound, int setnr, AggFunction aggtype, TsType sem, double bound);

	void polAddCPVariables(const std::vector<VarId>& varids, GroundTermTranslator* termtranslator);

	void polAddCPVariable(const VarId& varid, GroundTermTranslator* termtranslator);

	void polAddPCRule(int defnr, int head, std::vector<int> body, bool conjunctive, bool);
};

#endif /* SOLVERTHEORY_HPP_ */
