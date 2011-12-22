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

#include <iostream>

#include "ecnf.hpp"
#include "commontypes.hpp"
#include "inferences/grounding/grounders/LazyQuantGrounder.hpp"
#include "inferences/grounding/grounders/DefinitionGrounders.hpp"

#include "inferences/grounding/GroundTermTranslator.hpp"
#include "structure.hpp"

using namespace std;

void SolverPolicy::initialize(SATSolver* solver, int verbosity, GroundTermTranslator* termtranslator){
	_solver = solver;
	_verbosity = verbosity;
	_termtranslator = termtranslator;
}

inline MinisatID::Weight SolverPolicy::createWeight(double weight){
#warning "Dangerous cast from double to int in adding rules to the solver"
	return MinisatID::Weight(int(weight));	// TODO: remove cast when supported by the solver
}

void SolverPolicy::polAdd(const GroundClause& cl) {
	MinisatID::Disjunction clause;
	for(size_t n = 0; n < cl.size(); ++n) {
		clause.literals.push_back(createLiteral(cl[n]));
	}
	getSolver().add(clause);
}

void SolverPolicy::polAdd(const TsSet& tsset, int setnr, bool weighted) {
	if(not weighted) {
		MinisatID::Set set;
		set.setID = setnr;
		for(size_t n = 0; n < tsset.size(); ++n) {
			set.literals.push_back(createLiteral(tsset.literal(n)));
		}
		getSolver().add(set);
	} else {
		MinisatID::WSet set;
		set.setID = setnr;
		for(size_t n = 0; n < tsset.size(); ++n) {
			set.literals.push_back(createLiteral(tsset.literal(n)));
			set.weights.push_back(createWeight(tsset.weight(n)));
		}
		getSolver().add(set);
	}
}

void SolverPolicy::polAdd(int defnr, PCGroundRule* rule) {
	polAddPCRule(defnr,rule->head(),rule->body(),(rule->type() == RT_CONJ), rule->recursive());
}

void SolverPolicy::polAdd(int defnr, AggGroundRule* rule) {
	polAddAggregate(defnr,rule->head(),rule->lower(),rule->setnr(),rule->aggtype(),TsType::RULE,rule->bound());
}

void SolverPolicy::polAdd(int defnr, int head, AggGroundRule* body, bool) {
	polAddAggregate(defnr,head,body->lower(),body->setnr(),body->aggtype(),TsType::RULE,body->bound());
}


void SolverPolicy::polAdd(int head, AggTsBody* body) {
	Assert(body->type() != TsType::RULE);
	//FIXME correct undefined id numbering instead of -1 (should be the number the solver takes as undefined, so should but it in the solver interface)
	polAddAggregate(-1,head,body->lower(),body->setnr(),body->aggtype(),body->type(),body->bound());
}

void SolverPolicy::polAddWeightedSum(const MinisatID::Atom& head, const std::vector<VarId>& varids, const std::vector<int> weights, const int& bound, MinisatID::EqType rel, SATSolver& solver){
	MinisatID::CPSumWeighted sentence;
	sentence.head = head;
	sentence.varIDs = varids;
	sentence.weights = weights;
	sentence.bound = bound;
	sentence.rel = rel;
	solver.add(sentence);
}

void SolverPolicy::polAdd(int tseitin, CPTsBody* body) {
	MinisatID::EqType comp;
	switch(body->comp()) {
		case CompType::EQ:	comp = MinisatID::MEQ; break;
		case CompType::NEQ:	comp = MinisatID::MNEQ; break;
		case CompType::LEQ:	comp = MinisatID::MLEQ; break;
		case CompType::GEQ:	comp = MinisatID::MGEQ; break;
		case CompType::LT:	comp = MinisatID::ML; break;
		case CompType::GT:	comp = MinisatID::MG; break;
	}
	CPTerm* left = body->left();
	CPBound right = body->right();
	if(typeid(*left) == typeid(CPVarTerm)) {
		CPVarTerm* term = dynamic_cast<CPVarTerm*>(left);
		polAddCPVariable(term->varid(), _termtranslator);
		if(right._isvarid) {
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
	} else if(typeid(*left) == typeid(CPSumTerm)) {
		CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
		polAddCPVariables(term->varids(), _termtranslator);
		if(right._isvarid) {
			polAddCPVariable(right._varid, _termtranslator);
			std::vector<VarId> varids = term->varids();
			std::vector<int> weights;
			weights.resize(1, term->varids().size());

			int bound = 0;
			varids.push_back(right._varid);
			weights.push_back(-1);

			polAddWeightedSum(createAtom(tseitin), varids, weights, bound, comp, getSolver());
		} else {
			std::vector<int> weights{(int)term->varids().size()};
			polAddWeightedSum(createAtom(tseitin), term->varids(), weights, right._bound, comp, getSolver());
		}
	} else {
		Assert(typeid(*left) == typeid(CPWSumTerm));
		CPWSumTerm* term = dynamic_cast<CPWSumTerm*>(left);
		polAddCPVariables(term->varids(), _termtranslator);
		if(right._isvarid) {
			polAddCPVariable(right._varid, _termtranslator);
			std::vector<VarId> varids = term->varids();
			std::vector<int> weights = term->weights();

			int bound = 0;
			varids.push_back(right._varid);
			weights.push_back(-1);

			polAddWeightedSum(createAtom(tseitin), varids, weights, bound, comp, getSolver());
		} else {
			polAddWeightedSum(createAtom(tseitin), term->varids(), term->weights(), right._bound, comp, getSolver());
		}
	}
}

// FIXME probably already exists in transform for add?
void SolverPolicy::polAdd(Lit tseitin, TsType type, const GroundClause& clause){
	switch(type){
		case TsType::RIMPL:{
			Assert(false);// FIXME add equivalence or rule or impl
			break;}
		case TsType::IMPL:{
			MinisatID::Disjunction d;
			d.literals.push_back(createLiteral(-tseitin));
			for(auto i=clause.begin(); i<clause.end(); ++i){
				d.literals.push_back(createLiteral(*i));
			}
			getSolver().add(d);
			break;}
		case TsType::RULE:{
			Assert(false);// FIXME add equivalence or rule or impl
			break;}
		case TsType::EQ:{
			MinisatID::Equivalence eq;
			eq.head = createLiteral(-tseitin);
			for(auto i=clause.begin(); i<clause.end(); ++i){
				eq.body.push_back(createLiteral(*i));
			}
			getSolver().add(eq);
			break;}
	}
}


typedef cb::Callback1<void, ResidualAndFreeInst*> callbackgrounding;
class LazyClauseMon: public MinisatID::LazyGroundingCommand{
private:
	ResidualAndFreeInst* inst;
	callbackgrounding requestGroundingCB;

public:
	LazyClauseMon(ResidualAndFreeInst* inst):inst(inst){}

	void setRequestMoreGrounding(callbackgrounding cb){
		requestGroundingCB = cb;
	}

	virtual void requestGrounding(){
		if(not alreadyGround()){
			MinisatID::LazyGroundingCommand::requestGrounding();
			requestGroundingCB(inst);
		}
	}
};

typedef cb::Callback2<void, const Lit&, const std::vector<const DomainElement*>&> callbackrulegrounding;
class LazyRuleMon: public MinisatID::LazyGroundingCommand{
private:
	Lit lit;
	ElementTuple args;
	std::vector<LazyRuleGrounder*> grounders;

public:
	LazyRuleMon(const Lit& lit, const ElementTuple& args, const std::vector<LazyRuleGrounder*>& grounders): lit(lit), args(args), grounders(grounders){}

	virtual void requestGrounding(){
		if(not alreadyGround()){
			MinisatID::LazyGroundingCommand::requestGrounding();
			for(auto i=grounders.begin(); i<grounders.end(); ++i){
				(*i)->ground(lit, args);
			}
		}
	}
};

void SolverPolicy::polNotifyDefined(const Lit& lit, const ElementTuple& args, std::vector<LazyRuleGrounder*> grounders){
	LazyRuleMon* mon = new LazyRuleMon(lit, args, grounders);
	MinisatID::LazyGroundLit lc(true, createLiteral(lit), mon);
	//callbackrulegrounding cbmore(grounder, &LazyRuleGrounder::ground); // FIXME for some reason, cannot seem to pass in const function pointers?
	//mon->setRequestRuleGrounding(cbmore);
	getSolver().add(lc);
}

void SolverPolicy::polAddAggregate(int definitionID, int head, bool lowerbound, int setnr, AggFunction aggtype, TsType sem, double bound) {
	MinisatID::Aggregate agg;
	agg.sign = lowerbound ? MinisatID::AGGSIGN_LB : MinisatID::AGGSIGN_UB;
	agg.setID = setnr;
	switch (aggtype) {
		case AggFunction::CARD:
			agg.type = MinisatID::CARD;
			if(_verbosity > 1) std::clog << "card ";
			break;
		case AggFunction::SUM:
			agg.type = MinisatID::SUM;
			if(_verbosity > 1) std::clog << "sum ";
			break;
		case AggFunction::PROD:
			agg.type = MinisatID::PROD;
			if(_verbosity > 1) std::clog << "prod ";
			break;
		case AggFunction::MIN:
			agg.type = MinisatID::MIN;
			if(_verbosity > 1) std::clog << "min ";
			break;
		case AggFunction::MAX:
			if(_verbosity > 1) std::clog << "max ";
			agg.type = MinisatID::MAX;
			break;
	}
	if(_verbosity > 1) std::clog << setnr << ' ';
	switch(sem) {
		case TsType::EQ: case TsType::IMPL: case TsType::RIMPL:
			agg.sem = MinisatID::COMP;
			break;
		case TsType::RULE:
			agg.sem = MinisatID::DEF;
			break;
	}
	if(_verbosity > 1) std::clog << (lowerbound ? " >= " : " =< ") << bound << "\n";
	agg.defID = definitionID;
	agg.head = createAtom(head);
	agg.bound = createWeight(bound);
	getSolver().add(agg);
}

void SolverPolicy::polAddCPVariables(const std::vector<VarId>& varids, GroundTermTranslator* termtranslator) {
	for(auto it = varids.begin(); it != varids.end(); ++it) {
		polAddCPVariable(*it, termtranslator);
	}
}

void SolverPolicy::polAddCPVariable(const VarId& varid, GroundTermTranslator* termtranslator) {
	if(_addedvarids.find(varid) == _addedvarids.end()) {
		_addedvarids.insert(varid);
		SortTable* domain = termtranslator->domain(varid);
		Assert(domain);
		Assert(domain->approxFinite());
		if(domain->isRange()) {
			// the domain is a complete range from minvalue to maxvalue.
			MinisatID::CPIntVarRange cpvar;
			cpvar.varID = varid;
			cpvar.minvalue = domain->first()->value()._int;
			cpvar.maxvalue = domain->last()->value()._int;
			if(_verbosity > 0) std::clog << "[" << cpvar.minvalue << "," << cpvar.maxvalue << "]";
			getSolver().add(cpvar);
		}
		else {
			// the domain is not a complete range.
			MinisatID::CPIntVarEnum cpvar;
			cpvar.varID = varid;
			if(_verbosity > 0) std::clog << "{ ";
			for(SortIterator it = domain->sortBegin(); not it.isAtEnd(); ++it) {
				int value = (*it)->value()._int;
				cpvar.values.push_back(value);
				if(_verbosity > 0) std::clog << value << "; ";
			}
			if(_verbosity > 0) std::clog << "}";
			getSolver().add(cpvar);
		}
		if(_verbosity > 0) std::clog << "\n";
	}
}

void SolverPolicy::polAddPCRule(int defnr, int head, std::vector<int> body, bool conjunctive, bool){
	MinisatID::Rule rule;
	rule.head = createAtom(head);
	for(unsigned int n = 0; n < body.size(); ++n) {
		rule.body.push_back(createLiteral(body[n]));
	}
	rule.conjunctive = conjunctive;
	rule.definitionID = defnr;
	getSolver().add(rule);
}

void SolverPolicy::notifyLazyResidual(ResidualAndFreeInst* inst, LazyQuantGrounder const* const grounder){
	LazyClauseMon* mon = new LazyClauseMon(inst);
	MinisatID::LazyGroundLit lc(false, createLiteral(inst->residual), mon);
	callbackgrounding cbmore(const_cast<LazyQuantGrounder*>(grounder), &LazyQuantGrounder::requestGroundMore);
	mon->setRequestMoreGrounding(cbmore);
	getSolver().add(lc);
}