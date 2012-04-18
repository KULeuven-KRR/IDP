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
#include "external/ExternalInterface.hpp"

class TsSet;
class LazyGroundingManager;
class LazyRuleGrounder;
class DomainElement;
typedef std::vector<const DomainElement*> ElementTuple;

typedef int SetId;
typedef int DefId;

class ResidualAndFreeInst;
class LazyGroundingManager;
class DelayGrounder;

/**
 *	A SolverTheory is a ground theory, stored as an instance of a SAT solver
 */
template<class Solver>
class SolverPolicy {
private:
	GroundTermTranslator* _termtranslator;
	Solver* _solver; // The SAT solver
	std::map<PFSymbol*, std::set<Atom> > _defined; // Symbols that are defined in the theory. This set is used to
												  // communicate to the solver which ground atoms should be considered defined.
	std::set<VarId> _addedvarids; // Variable ids that have already been added, together with their domain.

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

	MinisatID::Weight createWeight(Weight weight);

	void polAdd(const GroundClause& cl);
	void polAdd(const TsSet& tsset, SetId setnr, bool weighted);
	void polAdd(DefId defnr, PCGroundRule* rule);
	void polAdd(DefId defnr, AggGroundRule* rule);
	void polAdd(DefId defnr, Lit head, AggGroundRule* body, bool);
	void polAdd(Lit head, AggTsBody* body);
	void polAddWeightedSum(const MinisatID::Atom& head, const varidlist& varids, const intweightlist& weights, const int& bound, MinisatID::EqType rel);
	void polAdd(Lit tseitin, CPTsBody* body);

	void polAddOptimization(AggFunction function, SetId setid);
	void polAddOptimization(VarId varid);

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
	void polAddAggregate(DefId definitionID, Lit head, bool lowerbound, SetId setnr, AggFunction aggtype, TsType sem, double bound);
	void polAddCPVariables(const varidlist& varids, GroundTermTranslator* termtranslator);
	void polAddCPVariable(const VarId& varid, GroundTermTranslator* termtranslator);
	void polAddPCRule(DefId defnr, Lit head, litlist body, bool conjunctive, bool);
};

#endif /* SOLVERPOLICY_HPP_ */
