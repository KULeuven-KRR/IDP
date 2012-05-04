/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "groundtheories/SolverPolicy.hpp"

#include "IncludeComponents.hpp"
#include "inferences/grounding/grounders/LazyFormulaGrounders.hpp"
#include "inferences/grounding/grounders/DefinitionGrounders.hpp"

#include "inferences/grounding/GroundTermTranslator.hpp"

#include "inferences/SolverConnection.hpp"

#include <cmath>

using namespace std;
using namespace SolverConnection;

template<typename Solver>
void SolverPolicy<Solver>::initialize(Solver* solver, int verbosity, GroundTermTranslator* termtranslator) {
	_solver = solver;
	_verbosity = verbosity;
	_termtranslator = termtranslator;
}

template<typename Solver>
void SolverPolicy<Solver>::polEndTheory() {
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(const GroundClause& cl) {
	MinisatID::Disjunction clause;
	for (size_t n = 0; n < cl.size(); ++n) {
		clause.literals.push_back(createLiteral(cl[n]));
	}
	extAdd(getSolver(), clause);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(const TsSet& tsset, SetId setnr, bool weighted) {
	if (not weighted) {
		MinisatID::WLSet set(setnr);
		for (size_t n = 0; n < tsset.size(); ++n) {
			set.wl.push_back(MinisatID::WLtuple { createLiteral(tsset.literal(n)), MinisatID::Weight(1) });
		}
		extAdd(getSolver(), set);
	} else {
		MinisatID::WLSet set(setnr);
		for (size_t n = 0; n < tsset.size(); ++n) {
			set.wl.push_back(MinisatID::WLtuple { createLiteral(tsset.literal(n)), createWeight(tsset.weight(n)) });
		}
		extAdd(getSolver(), set);
	}
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(DefId defnr, PCGroundRule* rule) {
	polAddPCRule(defnr, rule->head(), rule->body(), (rule->type() == RuleType::CONJ), rule->recursive());
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(DefId defnr, AggGroundRule* rule) {
	polAddAggregate(defnr, rule->head(), rule->lower(), rule->setnr(), rule->aggtype(), TsType::RULE, rule->bound());
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(DefId defnr, Lit head, AggGroundRule* body, bool) {
	polAddAggregate(defnr, head, body->lower(), body->setnr(), body->aggtype(), TsType::RULE, body->bound());
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(Lit head, AggTsBody* body) {
	Assert(body->type() != TsType::RULE);
	//FIXME getIDForUndefined() should be replaced by the number the SOLVER takes as undefined
	polAddAggregate(getIDForUndefined(), head, body->lower(), body->setnr(), body->aggtype(), body->type(), body->bound());
}

template<typename Solver>
void SolverPolicy<Solver>::polAddWeightedSum(const MinisatID::Atom& head, const varidlist& varids, const intweightlist& weights, const int& bound,
		MinisatID::EqType rel) {
	vector<MinisatID::Weight> w;
	for (auto i = weights.cbegin(); i < weights.cend(); ++i) {
		w.push_back(MinisatID::Weight(*i));
	}
	MinisatID::CPSumWeighted sentence(head, varids, w, rel, bound);
	extAdd(getSolver(), sentence);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(Lit tseitin, CPTsBody* body) {
	MinisatID::EqType comp;
	switch (body->comp()) {
	case CompType::EQ:
		comp = MinisatID::EqType::EQ;
		break;
	case CompType::NEQ:
		comp = MinisatID::EqType::NEQ;
		break;
	case CompType::LEQ:
		comp = MinisatID::EqType::LEQ;
		break;
	case CompType::GEQ:
		comp = MinisatID::EqType::GEQ;
		break;
	case CompType::LT:
		comp = MinisatID::EqType::L;
		break;
	case CompType::GT:
		comp = MinisatID::EqType::G;
		break;
	}
	CPTerm* left = body->left();
	CPBound right = body->right();
	if (sametypeid<CPVarTerm>(*left)) {
		CPVarTerm* term = dynamic_cast<CPVarTerm*>(left);
		polAddCPVariable(term->varid(), _termtranslator);
		if (right._isvarid) {
			polAddCPVariable(right._varid, _termtranslator);
			MinisatID::CPBinaryRelVar sentence(createAtom(tseitin), term->varid(), comp, right._varid);
			extAdd(getSolver(), sentence);
		} else {
			MinisatID::CPBinaryRel sentence(createAtom(tseitin), term->varid(), comp, right._bound);
			extAdd(getSolver(), sentence);
		}
	} else if (sametypeid<CPSumTerm>(*left)) {
		CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
		polAddCPVariables(term->varids(), _termtranslator);
		if (right._isvarid) {
			polAddCPVariable(right._varid, _termtranslator);
			varidlist varids = term->varids();
			intweightlist weights(term->varids().size(), 1);

			int bound = 0;
			varids.push_back(right._varid);
			weights.push_back(-1);

			polAddWeightedSum(createAtom(tseitin), varids, weights, bound, comp);
		} else {
			intweightlist weights(term->varids().size(), 1);
			polAddWeightedSum(createAtom(tseitin), term->varids(), weights, right._bound, comp);
		}
	} else {
		Assert(sametypeid<CPWSumTerm>(*left));
		CPWSumTerm* term = dynamic_cast<CPWSumTerm*>(left);
		polAddCPVariables(term->varids(), _termtranslator);
		if (right._isvarid) {
			polAddCPVariable(right._varid, _termtranslator);
			varidlist varids = term->varids();
			intweightlist weights = term->weights();

			int bound = 0;
			varids.push_back(right._varid);
			weights.push_back(-1);

			polAddWeightedSum(createAtom(tseitin), varids, weights, bound, comp);
		} else {
			polAddWeightedSum(createAtom(tseitin), term->varids(), term->weights(), right._bound, comp);
		}
	}
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(Lit tseitin, TsType type, const GroundClause& rhs, bool conjunction) {
	MinisatID::ImplicationType impltype;
	switch (type) {
	case TsType::RULE:
		Assert(false);
		break;
	case TsType::IMPL:
		impltype = MinisatID::ImplicationType::IMPLIES;
		break;
	case TsType::RIMPL:
		impltype = MinisatID::ImplicationType::IMPLIEDBY;
		break;
	case TsType::EQ:
		impltype = MinisatID::ImplicationType::EQUIVALENT;
		break;
	}
	extAdd(getSolver(), MinisatID::Implication(createLiteral(tseitin), impltype, createList(rhs), conjunction));
}


template<typename Solver>
void SolverPolicy<Solver>::polAddAggregate(DefId definitionID, Lit head, bool lowerbound, SetId setnr, AggFunction aggtype, TsType sem,
		double bound) {
	auto sign = lowerbound ? MinisatID::AggSign::LB : MinisatID::AggSign::UB;
	auto msem = MinisatID::AggSem::COMP;
	switch (sem) {
	case TsType::EQ:
	case TsType::IMPL:
	case TsType::RIMPL:
		msem = MinisatID::AggSem::COMP;
		break;
	case TsType::RULE:
		msem = MinisatID::AggSem::DEF;
		break;
	}
	extAdd(getSolver(), MinisatID::Aggregate(createLiteral(head), setnr, createWeight(bound), convert(aggtype), sign, msem, definitionID));
}

template<typename Solver>
void SolverPolicy<Solver>::polAddCPVariables(const varidlist& varids, GroundTermTranslator* termtranslator) {
	for (auto it = varids.begin(); it != varids.end(); ++it) {
		polAddCPVariable(*it, termtranslator);
	}
}

template<typename Solver>
void SolverPolicy<Solver>::polAddCPVariable(const VarId& varid, GroundTermTranslator* termtranslator) {
	if (_addedvarids.find(varid) == _addedvarids.end()) {
		_addedvarids.insert(varid);
		SortTable* domain = termtranslator->domain(varid);
		Assert(domain != NULL);
		Assert(domain->approxFinite());
		if (domain->isRange()) {
			// the domain is a complete range from minvalue to maxvalue.
			MinisatID::IntVarRange cpvar(varid, domain->first()->value()._int, domain->last()->value()._int);
			extAdd(getSolver(), cpvar);
		} else {
			// the domain is not a complete range.
			std::vector<MinisatID::Weight> w;
			for (auto it = domain->sortBegin(); not it.isAtEnd(); ++it) {
				w.push_back((MinisatID::Weight)(*it)->value()._int);
			}
			extAdd(getSolver(), MinisatID::IntVarEnum(varid, w));
		}
	}
}

template<typename Solver>
void SolverPolicy<Solver>::polAddPCRule(DefId defnr, Lit head, std::vector<int> body, bool conjunctive, bool arg_n) {
	MinisatID::litlist list;
	for (size_t n = 0; n < body.size(); ++n) {
		list.push_back(createLiteral(body[n]));
	}
	extAdd(getSolver(), MinisatID::Rule(createAtom(head), list, conjunctive, defnr));
}

template<typename Solver>
void SolverPolicy<Solver>::polAddOptimization(AggFunction function, SetId setid) {
	extAdd(getSolver(), MinisatID::MinimizeAgg(1, setid, convert(function)));
}

template<typename Solver>
void SolverPolicy<Solver>::polAddOptimization(VarId varid) {
	extAdd(getSolver(), MinisatID::MinimizeVar(1, varid));
}

class LazyRuleMon: public MinisatID::LazyGroundingCommand {
private:
	Lit lit;
	ElementTuple args;
	std::vector<DelayGrounder*> grounders;

public:
	LazyRuleMon(const Lit& lit, const ElementTuple& args, const std::vector<DelayGrounder*>& grounders) :
			lit(lit), args(args), grounders(grounders) {
	}

	virtual void requestGrounding() {
		if (not isAlreadyGround()) {
			notifyGrounded();
			for (auto i = grounders.begin(); i < grounders.end(); ++i) {
				(*i)->ground(lit, args);
			}
		}
	}
};

template<class Solver>
void SolverPolicy<Solver>::polNotifyUnknBound(Context context, const Lit& delaylit, const ElementTuple& args,
		std::vector<DelayGrounder*> grounders) {
	auto mon = new LazyRuleMon(delaylit, args, grounders);
	auto literal = createLiteral(delaylit);
	if (context == Context::POSITIVE) { // In a positive context, should watch when the literal becomes false, or it's negation becomes true
		literal = not literal;
	}
	MinisatID::LazyGroundLit lc(context == Context::BOTH, literal, mon);
	extAdd(getSolver(), lc);
}

typedef cb::Callback1<void, ResidualAndFreeInst*> callbackgrounding;
class LazyClauseMon: public MinisatID::LazyGroundingCommand {
private:
	ResidualAndFreeInst* inst;
	LazyGroundingManager const * const grounder;

public:
	LazyClauseMon(ResidualAndFreeInst* inst, LazyGroundingManager const * const grounder) :
			inst(inst), grounder(grounder) {
	}

	virtual void requestGrounding() {
		if (not isAlreadyGround()) {
			notifyGrounded();
			grounder->notifyDelayTriggered(inst);
		}
	}
};

template<class Solver>
void SolverPolicy<Solver>::polNotifyLazyResidual(ResidualAndFreeInst* inst, TsType type, LazyGroundingManager const* const grounder) {
	auto mon = new LazyClauseMon(inst, grounder);
	auto watchboth = type == TsType::RULE || type == TsType::EQ;
	auto lit = createLiteral(inst->residual);
	if (type == TsType::RIMPL) {
		lit = not lit;
	}
	MinisatID::LazyGroundLit lc(watchboth, lit, mon);
	extAdd(getSolver(), lc);
}

template<class Solver>
void SolverPolicy<Solver>::polAdd(const std::vector<std::map<Lit, Lit> >& symmetries){
	MinisatID::Symmetry s({});
	for (auto bs_it = symmetries.cbegin(); bs_it != symmetries.cend(); ++bs_it) {
		std::vector<MinisatID::Lit> newOne = {};
		s.symmetry.push_back(newOne);
		for (auto s_it = bs_it->begin(); s_it != bs_it->end(); ++s_it) {
			s.symmetry.back().push_back(SolverConnection::createLiteral(s_it->first));
		}
	}
	getSolver().add(s);
}

// Explicit instantiations
template class SolverPolicy<PCSolver> ;
