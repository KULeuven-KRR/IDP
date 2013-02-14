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

#pragma once

#include "IncludeComponents.hpp"
#include "inferences/SolverInclude.hpp"

class SymbolOffset;
class AbstractGroundTheory;

class TsSet;
class LazyTseitinGrounderInterface;
class LazyRuleGrounder;
class DomainElement;
typedef std::vector<const DomainElement*> ElementTuple;

class LazyInstantiation;
class DelayGrounder;

/**
 *	A SolverTheory is a ground theory, stored as an instance of a SAT solver
 */
template<class Solver>
class SolverPolicy {
private:
	GroundTranslator* _translator;
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
	void initialize(Solver* solver, int verbosity, GroundTranslator* translator);
	void polNotifyUnknBound(Context context, const Lit& boundlit, const ElementTuple& args, std::vector<DelayGrounder*> grounders);
	void polAddLazyAddition(const litlist& glist, int ID);
	void polStartLazyFormula(LazyInstantiation* inst, TsType type, bool conjunction);
	void polNotifyLazyResidual(LazyInstantiation* inst, TsType type);

protected:
	void polRecursiveDelete() {
	}

	void polStartTheory(GroundTranslator*) {
	}
	void polEndTheory() {
	}

	void polAdd(const GroundClause& cl);
	void polAdd(const TsSet& tsset, SetId setnr, bool weighted);
	void polAdd(DefId defnr, PCGroundRule* rule);
	void polAdd(DefId defnr, AggGroundRule* rule);
	void polAdd(DefId defnr, Lit head, AggGroundRule* body, bool);
	void polAdd(Lit head, AggTsBody* body);
	void polAddWeightedSum(const MinisatID::Atom& head, const varidlist& varids, const intweightlist& weights, const int& bound, MinisatID::EqType rel);
	void polAddWeightedProd(const MinisatID::Atom& head, const varidlist& varids, const int& weight, VarId bound, MinisatID::EqType rel);
	void polAdd(Lit tseitin, CPTsBody* body);
	void polAdd(const std::vector<std::map<Lit, Lit> >& symmetry);
	void polAddLazyElement(Lit head, PFSymbol* symbol, const std::vector<VarId>& args, AbstractGroundTheory* theory);

	void polAddOptimization(AggFunction function, SetId setid);
	void polAddOptimization(VarId varid);

	// FIXME probably already exists in transform for add?
	void polAdd(Lit tseitin, TsType type, const GroundClause& rhs, bool conjunction);

	std::ostream& polPut(std::ostream& s, GroundTranslator*) const {
		Assert(false);
		return s;
	}

private:
	void polAddAggregate(DefId definitionID, Lit head, bool lowerbound, SetId setnr, AggFunction aggtype, TsType sem, double bound);
	void polAddCPVariables(const varidlist& varids, GroundTranslator* mtranslator);
	void polAddCPVariable(const VarId& varid, GroundTranslator* translator);
	void polAddPCRule(DefId defnr, Lit head, litlist body, bool conjunctive);

	litlist addList(const varidlist& varids, MinisatID::EqType comp, VarId rhsvarid);
};
