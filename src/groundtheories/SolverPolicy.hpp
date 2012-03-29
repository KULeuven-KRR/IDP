/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef SOLVERPOLICY_HPP_
#define SOLVERPOLICY_HPP_

#include "IncludeComponents.hpp"
#include "inferences/SolverInclude.hpp"

class TsSet;
class LazyGroundingManager;
class LazyRuleGrounder;
class DomainElement;
typedef std::vector<const DomainElement*> ElementTuple;

class ResidualAndFreeInst;
class LazyGroundingManager;

/**
 *	A SolverTheory is a ground theory, stored as an instance of a SAT solver
 */
template<class Solver>
class SolverPolicy {
private:
	GroundTermTranslator* _termtranslator;
	Solver* _solver; // The SAT solver
	std::map<PFSymbol*, std::set<int> > _defined; // Symbols that are defined in the theory. This set is used to
												  // communicate to the solver which ground atoms should be considered defined.
	std::set<unsigned int> _addedvarids; // Variable ids that have already been added, together with their domain.

	int _verbosity;

	const Solver& getSolver() const {
		return *_solver;
	}
	Solver& getSolver() {
		return *_solver;
	}

public:
	void initialize(Solver* solver, int verbosity, GroundTermTranslator* termtranslator);
	void polNotifyUnknBound(Context context, const Lit& boundlit, const ElementTuple& args, std::vector<DelayGrounder*> grounders);
	void polNotifyLazyResidual(ResidualAndFreeInst* inst, TsType type, LazyGroundingManager const* const grounder);

protected:
	void polRecursiveDelete() {
	}

	void polStartTheory(GroundTranslator*) {
	}
	void polEndTheory();

	MinisatID::Weight createWeight(double weight);

	void polAdd(const GroundClause& cl);
	void polAdd(const TsSet& tsset, int setnr, bool weighted);
	void polAdd(int defnr, PCGroundRule* rule);
	void polAdd(int defnr, AggGroundRule* rule);
	void polAdd(int defnr, int head, AggGroundRule* body, bool);
	void polAdd(int head, AggTsBody* body);
	void polAddWeightedSum(const MinisatID::Atom& head, const std::vector<VarId>& varids, const std::vector<int> weights, const int& bound,
			MinisatID::EqType rel);
	void polAdd(int tseitin, CPTsBody* body);

	void polAddOptimization(AggFunction function, int setid);

	// FIXME probably already exists in transform for add?
	void polAdd(Lit tseitin, TsType type, const GroundClause& rhs, bool conjunction);

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

#endif /* SOLVERPOLICY_HPP_ */
