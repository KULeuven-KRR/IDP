/************************************
	SolverTheory.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SOLVERTHEORY_HPP_
#define SOLVERTHEORY_HPP_

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <ostream>
#include <assert.h>
#include <iostream>

#include "ground.hpp"
#include "ecnf.hpp"
#include "commontypes.hpp"
#include "grounders/LazyQuantGrounder.hpp"
#include "grounders/DefinitionGrounders.hpp"

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
	// Destructors
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

	void initialize(SATSolver* solver, int verbosity, GroundTermTranslator* termtranslator){
		_solver = solver;
		_verbosity = verbosity;
		_termtranslator = termtranslator;
	}

	void polAdd(GroundClause& cl) {
		MinisatID::Disjunction clause;
		for(unsigned int n = 0; n < cl.size(); ++n) {
			clause.literals.push_back(createLiteral(cl[n]));
		}
		getSolver().add(clause);
	}

	void polAdd(const TsSet& tsset, int setnr, bool weighted) {
		if(!weighted){
			MinisatID::Set set;
			set.setID = setnr;
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				set.literals.push_back(createLiteral(tsset.literal(n)));
			}
			getSolver().add(set);
		}else {
			MinisatID::WSet set;
			set.setID = setnr;
			for(unsigned int n = 0; n < tsset.size(); ++n) {
				set.literals.push_back(createLiteral(tsset.literal(n)));
				set.weights.push_back(createWeight(tsset.weight(n)));
			}
			getSolver().add(set);
		}
	}

	void polAdd(GroundDefinition* def){
		for(auto i=def->begin(); i!=def->end(); ++i){
			if(typeid(PCGroundRule*)==typeid((*i).second)){
				polAdd(def->id(), dynamic_cast<PCGroundRule*>((*i).second));
			}else{
				polAdd(def->id(), dynamic_cast<AggGroundRule*>((*i).second));
			}
		}
	}

	void polAdd(int defnr, PCGroundRule* rule) {
		polAddPCRule(defnr,rule->head(),rule->body(),(rule->type() == RT_CONJ), rule->recursive());
	}

	void polAdd(int defnr, AggGroundRule* rule) {
		polAddAggregate(defnr,rule->head(),rule->lower(),rule->setnr(),rule->aggtype(),TsType::RULE,rule->bound());
	}

	void polAdd(int defnr, int head, AggGroundRule* body, bool) {
		polAddAggregate(defnr,head,body->lower(),body->setnr(),body->aggtype(),TsType::RULE,body->bound());
	}


	void polAdd(int head, AggTsBody* body) {
		assert(body->type() != TsType::RULE);
		//FIXME correct undefined id numbering instead of -1 (should be the number the solver takes as undefined, so should but it in the solver interface)
		polAddAggregate(-1,head,body->lower(),body->setnr(),body->aggtype(),body->type(),body->bound());
	}

	void polAddWeightedSum(const MinisatID::Atom& head, const std::vector<VarId>& varids, const std::vector<int> weights, const int& bound, MinisatID::EqType rel, SATSolver& solver){
		MinisatID::CPSumWeighted sentence;
		sentence.head = head;
		sentence.varIDs = varids;
		sentence.weights = weights;
		sentence.bound = bound;
		sentence.rel = rel;
		solver.add(sentence);
	}

	void polAdd(int tseitin, CPTsBody* body) {
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
			polAddCPVariable(term->_varid, _termtranslator);
			if(right._isvarid) {
				polAddCPVariable(right._varid, _termtranslator);
				MinisatID::CPBinaryRelVar sentence;
				sentence.head = createAtom(tseitin);
				sentence.lhsvarID = term->_varid;
				sentence.rhsvarID = right._varid;
				sentence.rel = comp;
				getSolver().add(sentence);
			} else {
				MinisatID::CPBinaryRel sentence;
				sentence.head = createAtom(tseitin);
				sentence.varID = term->_varid;
				sentence.bound = right._bound;
				sentence.rel = comp;
				getSolver().add(sentence);
			}
		} else if(typeid(*left) == typeid(CPSumTerm)) {
			CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
			polAddCPVariables(term->_varids, _termtranslator);
			if(right._isvarid) {
				polAddCPVariable(right._varid, _termtranslator);
				std::vector<VarId> varids = term->_varids;
				std::vector<int> weights;
				weights.resize(1, term->_varids.size());

				int bound = 0;
				varids.push_back(right._varid);
				weights.push_back(-1);

				polAddWeightedSum(createAtom(tseitin), varids, weights, bound, comp, getSolver());
			} else {
				std::vector<int> weights;
				weights.resize(1, term->_varids.size());
				polAddWeightedSum(createAtom(tseitin), term->_varids, weights, right._bound, comp, getSolver());
			}
		} else {
			assert(typeid(*left) == typeid(CPWSumTerm));
			CPWSumTerm* term = dynamic_cast<CPWSumTerm*>(left);
			polAddCPVariables(term->_varids, _termtranslator);
			if(right._isvarid) {
				polAddCPVariable(right._varid, _termtranslator);
				std::vector<VarId> varids = term->_varids;
				std::vector<int> weights = term->_weights;

				int bound = 0;
				varids.push_back(right._varid);
				weights.push_back(-1);

				polAddWeightedSum(createAtom(tseitin), varids, weights, bound, comp, getSolver());
			} else {
				polAddWeightedSum(createAtom(tseitin), term->_varids, term->_weights, right._bound, comp, getSolver());
			}
		}
	}

	// FIXME probably already exists in transform for add?
	void polAdd(Lit tseitin, TsType type, const GroundClause& clause){
		switch(type){
			case TsType::RIMPL:{
				assert(false);// FIXME add equivalence or rule or impl
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
				assert(false);// FIXME add equivalence or rule or impl
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
			requestGroundingCB(inst);
		}
	};

	void notifyLazyResidual(ResidualAndFreeInst* inst, LazyQuantGrounder const* const grounder){
		LazyClauseMon* mon = new LazyClauseMon(inst);
		MinisatID::LazyGroundLit lc(false, createLiteral(inst->residual), mon);
		callbackgrounding cbmore(const_cast<LazyQuantGrounder*>(grounder), &LazyQuantGrounder::requestGroundMore); // FIXME for some reason, cannot seem to pass in const function pointers?
		mon->setRequestMoreGrounding(cbmore);
		getSolver().add(lc);
	}

	typedef cb::Callback2<void, const Lit&, const std::vector<const DomainElement*>&> callbackrulegrounding;
	class LazyRuleMon: public MinisatID::LazyGroundingCommand{
	private:
		Lit lit;
		ElementTuple args;
		callbackrulegrounding requestgrounding;

	public:
		LazyRuleMon(const Lit& lit, const ElementTuple& args): lit(lit), args(args){}

		void setRequestRuleGrounding(callbackrulegrounding cb){
			requestgrounding = cb;
		}

		virtual void requestGrounding(){
			requestgrounding(lit, args);
		}
	};

	void polNotifyDefined(const Lit& lit, const ElementTuple& args, LazyRuleGrounder* grounder){
		LazyRuleMon* mon = new LazyRuleMon(lit, args);
		MinisatID::LazyGroundLit lc(true, createLiteral(lit), mon);
		callbackrulegrounding cbmore(grounder, &LazyRuleGrounder::ground); // FIXME for some reason, cannot seem to pass in const function pointers?
		mon->setRequestRuleGrounding(cbmore);
		getSolver().add(lc);
	}

	std::ostream& polPut(std::ostream& s, GroundTranslator*, GroundTermTranslator*)	const { assert(false); return s;	}
	std::string polTo_string(GroundTranslator*, GroundTermTranslator*) const { assert(false); return "";		}

private:
	void polAddAggregate(int definitionID, int head, bool lowerbound, int setnr, AggFunction aggtype, TsType sem, double bound) {
		MinisatID::Aggregate agg;
		agg.sign = lowerbound ? MinisatID::AGGSIGN_LB : MinisatID::AGGSIGN_UB;
		agg.setID = setnr;
		switch (aggtype) {
			case AggFunction::CARD:
				agg.type = MinisatID::CARD;
				if(_verbosity > 0) std::clog << "card ";
				break;
			case AggFunction::SUM:
				agg.type = MinisatID::SUM;
				if(_verbosity > 0) std::clog << "sum ";
				break;
			case AggFunction::PROD:
				agg.type = MinisatID::PROD;
				if(_verbosity > 0) std::clog << "prod ";
				break;
			case AggFunction::MIN:
				agg.type = MinisatID::MIN;
				if(_verbosity > 0) std::clog << "min ";
				break;
			case AggFunction::MAX:
				if(_verbosity > 0) std::clog << "max ";
				agg.type = MinisatID::MAX;
				break;
		}
		if(_verbosity > 0) std::clog << setnr << ' ';
		switch(sem) {
			case TsType::EQ: case TsType::IMPL: case TsType::RIMPL:
				agg.sem = MinisatID::COMP;
				break;
			case TsType::RULE:
				agg.sem = MinisatID::DEF;
				break;
		}
		if(_verbosity > 0) std::clog << (lowerbound ? " >= " : " =< ") << bound << "\n";
		agg.defID = definitionID;
		agg.head = createAtom(head);
		agg.bound = createWeight(bound);
		getSolver().add(agg);
	}

	void polAddCPVariables(const std::vector<VarId>& varids, GroundTermTranslator* termtranslator) {
		for(auto it = varids.begin(); it != varids.end(); ++it) {
			polAddCPVariable(*it, termtranslator);
		}
	}

	void polAddCPVariable(const VarId& varid, GroundTermTranslator* termtranslator) {
		if(_addedvarids.find(varid) == _addedvarids.end()) {
			_addedvarids.insert(varid);
			SortTable* domain = termtranslator->domain(varid);
			assert(domain);
			assert(domain->approxfinite());
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
				for(SortIterator it = domain->sortbegin(); it.hasNext(); ++it) {
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

	void polAddPCRule(int defnr, int head, std::vector<int> body, bool conjunctive, bool){
		MinisatID::Rule rule;
		rule.head = createAtom(head);
		for(unsigned int n = 0; n < body.size(); ++n) {
			rule.body.push_back(createLiteral(body[n]));
		}
		rule.conjunctive = conjunctive;
		rule.definitionID = defnr;
		getSolver().add(rule);
	}
};


#endif /* SOLVERTHEORY_HPP_ */
