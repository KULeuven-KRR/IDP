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

#include "external/ExternalInterface.hpp"
#include "external/FlatZincRewriter.hpp"

#include <cmath>

using namespace std;

inline MinisatID::Atom createAtom(int lit) {
	return MinisatID::Atom(abs(lit));
}

inline MinisatID::Literal createLiteral(int lit) {
	return MinisatID::Literal(abs(lit), lit < 0);
}

MinisatID::literallist createList(const litlist& origlist){
	MinisatID::literallist list;
	for(auto i=origlist.cbegin(); i<origlist.cend(); i++){
		list.push_back(createLiteral(*i));
	}
	return list;
}

template<typename Solver>
void SolverPolicy<Solver>::initialize(Solver* solver, int verbosity, GroundTermTranslator* termtranslator) {
	_solver = solver;
	_verbosity = verbosity;
	_termtranslator = termtranslator;
}

template<typename Solver>
void SolverPolicy<Solver>::polEndTheory(){
}

template<>
void SolverPolicy<MinisatID::FlatZincRewriter>::polEndTheory(){
	getSolver().finishParsing();
}

double test;

template<typename Solver>
inline MinisatID::Weight SolverPolicy<Solver>::createWeight(double weight) {
	if(modf(weight, &test)!=0){
		throw notyetimplemented("MinisatID does not support doubles yet.");
	}
	return int(weight);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(const GroundClause& cl) {
	MinisatID::Disjunction clause;
	for (size_t n = 0; n < cl.size(); ++n) {
		clause.literals.push_back(createLiteral(cl[n]));
	}
	getSolver().add(clause);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(const TsSet& tsset, SetId setnr, bool weighted) {
	if (not weighted) {
		MinisatID::Set set;
		set.setID = setnr;
		for (size_t n = 0; n < tsset.size(); ++n) {
			set.literals.push_back(createLiteral(tsset.literal(n)));
		}
		getSolver().add(set);
	} else {
		MinisatID::WSet set;
		set.setID = setnr;
		for (size_t n = 0; n < tsset.size(); ++n) {
			set.literals.push_back(createLiteral(tsset.literal(n)));
			set.weights.push_back(createWeight(tsset.weight(n)));
		}
		getSolver().add(set);
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
void SolverPolicy<Solver>::polAddWeightedSum(const MinisatID::Atom& head, const varidlist& varids, const intweightlist& weights, const int& bound, MinisatID::EqType rel) {
	MinisatID::CPSumWeighted sentence;
	sentence.head = head;
	sentence.varIDs = varids;
	sentence.weights = weights;
	sentence.bound = bound;
	sentence.rel = rel;
	getSolver().add(sentence);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(Lit tseitin, CPTsBody* body) {
	MinisatID::EqType comp;
	switch (body->comp()) {
	case CompType::EQ:
		comp = MinisatID::MEQ;
		break;
	case CompType::NEQ:
		comp = MinisatID::MNEQ;
		break;
	case CompType::LEQ:
		comp = MinisatID::MLEQ;
		break;
	case CompType::GEQ:
		comp = MinisatID::MGEQ;
		break;
	case CompType::LT:
		comp = MinisatID::ML;
		break;
	case CompType::GT:
		comp = MinisatID::MG;
		break;
	}
	CPTerm* left = body->left();
	CPBound right = body->right();
	if (sametypeid<CPVarTerm>(*left)) {
		CPVarTerm* term = dynamic_cast<CPVarTerm*>(left);
		polAddCPVariable(term->varid(), _termtranslator);
		if (right._isvarid) {
			polAddCPVariable(right._varid, _termtranslator);
			MinisatID::CPBinaryRelVar sentence;
			sentence.head = createAtom(tseitin);
			sentence.lhsvarID = term->varid();
			sentence.rhsvarID = right._varid;
			sentence.rel = comp;
			getSolver().add(sentence);
		} else {
			MinisatID::CPBinaryRel sentence;
			sentence.head = createAtom(tseitin);
			sentence.varID = term->varid();
			sentence.bound = right._bound;
			sentence.rel = comp;
			getSolver().add(sentence);
		}
	} else if (sametypeid<CPSumTerm>(*left)) {
		CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
		polAddCPVariables(term->varids(), _termtranslator);
		if (right._isvarid) {
			polAddCPVariable(right._varid, _termtranslator);
			varidlist varids = term->varids();
			intweightlist weights(term->varids().size(),1);

			int bound = 0;
			varids.push_back(right._varid);
			weights.push_back(-1);

			polAddWeightedSum(createAtom(tseitin), varids, weights, bound, comp);
		} else {
			intweightlist weights(term->varids().size(),1);
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
	switch(type){
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
	getSolver().add(MinisatID::Implication(createLiteral(tseitin), impltype, createList(rhs), conjunction));
}

MinisatID::AggType convert(AggFunction agg){
	MinisatID::AggType type = MinisatID::AggType::CARD;
	switch (agg) {
	case AggFunction::CARD:
		type = MinisatID::CARD;
		break;
	case AggFunction::SUM:
		type = MinisatID::SUM;
		break;
	case AggFunction::PROD:
		type = MinisatID::PROD;
		break;
	case AggFunction::MIN:
		type = MinisatID::MIN;
		break;
	case AggFunction::MAX:
		type = MinisatID::MAX;
		break;
	}
	return type;
}

template<typename Solver>
void SolverPolicy<Solver>::polAddAggregate(DefId definitionID, Lit head, bool lowerbound, SetId setnr, AggFunction aggtype, TsType sem, double bound) {
	MinisatID::Aggregate agg;
	agg.sign = lowerbound ? MinisatID::AGGSIGN_LB : MinisatID::AGGSIGN_UB;
	agg.setID = setnr;
	agg.type = convert(aggtype);
	switch (sem) {
	case TsType::EQ:
	case TsType::IMPL:
	case TsType::RIMPL:
		agg.sem = MinisatID::COMP;
		break;
	case TsType::RULE:
		agg.sem = MinisatID::DEF;
		break;
	}
	agg.defID = definitionID;
	agg.head = createAtom(head);
	agg.bound = createWeight(bound);
	getSolver().add(agg);
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
			MinisatID::CPIntVarRange cpvar;
			cpvar.varID = varid;
			cpvar.minvalue = domain->first()->value()._int;
			cpvar.maxvalue = domain->last()->value()._int;
			getSolver().add(cpvar);
		} else {
			// the domain is not a complete range.
			MinisatID::CPIntVarEnum cpvar;
			cpvar.varID = varid;
			for (SortIterator it = domain->sortBegin(); not it.isAtEnd(); ++it) {
				int value = (*it)->value()._int;
				cpvar.values.push_back(value);
			}
			getSolver().add(cpvar);
		}
	}
}

template<typename Solver>
void SolverPolicy<Solver>::polAddPCRule(DefId defnr, Lit head, litlist body, bool conjunctive, bool) {
	MinisatID::Rule rule;
	rule.head = createAtom(head);
	for (size_t n = 0; n < body.size(); ++n) {
		rule.body.push_back(createLiteral(body[n]));
	}
	rule.conjunctive = conjunctive;
	rule.definitionID = defnr;
	getSolver().add(rule);
}

template<typename Solver>
void SolverPolicy<Solver>::polAddOptimization(AggFunction function, SetId setid) {
	MinisatID::MinimizeAgg minim;
	minim.setid = setid;
	minim.type = convert(function);
	getSolver().add(minim);
}

template<typename Solver>
void SolverPolicy<Solver>::polAddOptimization(VarId varid) {
	MinisatID::MinimizeVar minim;
	minim.varID = varid;
	getSolver().add(minim);
}

class LazyRuleMon: public MinisatID::LazyGroundingCommand {
private:
	Lit lit;
	ElementTuple args;
	std::vector<DelayGrounder*> grounders;

public:
	LazyRuleMon(const Lit& lit, const ElementTuple& args, const std::vector<DelayGrounder*>& grounders)
			: lit(lit), args(args), grounders(grounders) {
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

template<>
void SolverPolicy<MinisatID::FlatZincRewriter>::polNotifyUnknBound(Context, const Lit&, const ElementTuple&, std::vector<DelayGrounder*>){}

template<>
void SolverPolicy<MinisatID::WrappedPCSolver>::polNotifyUnknBound(Context context, const Lit& delaylit, const ElementTuple& args, std::vector<DelayGrounder*> grounders){
	auto mon = new LazyRuleMon(delaylit, args, grounders);
	auto literal = createLiteral(delaylit);
	if(context==Context::POSITIVE){ // In a positive context, should watch when the literal becomes false, or it's negation becomes true
		literal = not literal;
	}
	MinisatID::LazyGroundLit lc(context==Context::BOTH, literal, mon);
	getSolver().add(lc);
}

typedef cb::Callback1<void, ResidualAndFreeInst*> callbackgrounding;
class LazyClauseMon: public MinisatID::LazyGroundingCommand {
private:
	ResidualAndFreeInst* inst;
	LazyGroundingManager const * const grounder;

public:
	LazyClauseMon(ResidualAndFreeInst* inst, LazyGroundingManager const * const grounder)
			: inst(inst), grounder(grounder) {
	}

	virtual void requestGrounding() {
		if (not isAlreadyGround()) {
			notifyGrounded();
			grounder->notifyDelayTriggered(inst);
		}
	}
};

template<>
void SolverPolicy<MinisatID::FlatZincRewriter>::polNotifyLazyResidual(ResidualAndFreeInst*, TsType, LazyGroundingManager const* const) {

}

template<>
void SolverPolicy<MinisatID::WrappedPCSolver>::polNotifyLazyResidual(ResidualAndFreeInst* inst, TsType type, LazyGroundingManager const* const grounder) {
	auto mon = new LazyClauseMon(inst, grounder);
	auto watchboth = type==TsType::RULE || type==TsType::EQ;
	auto lit = createLiteral(inst->residual);
	if(type==TsType::RIMPL){
		lit = not lit;
	}
	MinisatID::LazyGroundLit lc(watchboth, lit, mon);
	getSolver().add(lc);
}

// Explicit instantiations
template class SolverPolicy<MinisatID::WrappedPCSolver>;
template class SolverPolicy<MinisatID::FlatZincRewriter>;
