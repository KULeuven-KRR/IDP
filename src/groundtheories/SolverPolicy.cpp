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

#include "groundtheories/SolverPolicy.hpp"

#include "IncludeComponents.hpp"
#include "inferences/grounding/grounders/DefinitionGrounders.hpp"
#include "inferences/grounding/lazygrounders/LazyInst.hpp"
#include "errorhandling/UnsatException.hpp"

#include "inferences/grounding/GroundTranslator.hpp"
#include "inferences/grounding/LazyGroundingManager.hpp"

#include "inferences/SolverConnection.hpp"

#include <cmath>

using namespace std;
using namespace SolverConnection;

#define CHECKUNSAT \
		if(getSolver().isCertainlyUnsat()){\
			throw UnsatException();\
		}

template<typename Solver>
template<class Obj>
void SolverPolicy<Solver>::AddToSolver::operator() (Obj o){
	extAdd(getSolver(), o);
	CHECKUNSAT;
}

template<typename Solver>
void SolverPolicy<Solver>::initialize(Solver* solver, int verbosity, GroundTranslator* translator) {
	_solver = solver;
	_verbosity = verbosity;
	_translator = translator;
	adder.setExec(AddToSolver(_solver));
	adder.setTranslator(_translator);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(const GroundClause& cl) {
	adder.add(cl);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(const TsSet& tsset, SetId setnr, bool weighted) {
	adder.add(setnr, tsset.literals(), weighted, tsset.weights());
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(DefId defnr, const PCGroundRule& rule) {
	adder.add(defnr, rule);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(DefId defnr, AggGroundRule* rule) {
	polAddAggregate(defnr.id, rule->head(), rule->bound(), rule->lower(), rule->setnr(), rule->aggtype(), TsType::RULE);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(DefId defnr, Lit head, AggGroundRule* body, bool) {
	polAddAggregate(defnr.id, head, body->bound(), body->lower(), body->setnr(), body->aggtype(), TsType::RULE);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(Lit head, AggTsBody* body) {
	Assert(body->type() != TsType::RULE);
	//TODO getIDForUndefined() should be replaced by the number the SOLVER takes as undefined
	polAddAggregate(getIDForUndefined(), head, body->bound(), body->lower(), body->setnr(), body->aggtype(), body->type());
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(const GroundEquivalence& geq) {
	adder.add(geq.head, TsType::EQ, geq.body, geq.conj);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(Lit tseitin, TsType type, const GroundClause& rhs, bool conjunction) {
	adder.add(tseitin, type, rhs, conjunction);
}

template<typename Solver>
void SolverPolicy<Solver>::polAddAggregate(DefId defID, Lit head, double bound, bool lowerbound, SetId setnr, AggFunction aggtype, TsType sem) {
	adder.add(defID, head, bound, lowerbound, setnr, aggtype, sem);
}

template<typename Solver>
void SolverPolicy<Solver>::polAddOptimization(AggFunction function, SetId setid) {
	adder.addOptimization(function, setid);
}

template<typename Solver>
void SolverPolicy<Solver>::polAddOptimization(VarId varid) {
	adder.addOptimization(varid);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(Lit tseitin, CPTsBody* body) {
	adder.add(tseitin, body);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(Lit tseitin, VarId varid){
	adder.add(tseitin, varid);
}

class LazyClauseMon: public MinisatID::LazyGrounder {
private:
	LazyInstantiation* inst;

public:
	LazyClauseMon(LazyInstantiation* inst)
			: inst(inst) {
	}

	virtual void requestGrounding(int ID, bool groundall, bool& stilldelayed) {
		inst->notifyGroundingRequested(ID, groundall, stilldelayed);
	}
};

MinisatID::ImplicationType convert(TsType type){
	auto result = MinisatID::ImplicationType::EQUIVALENT;
	switch(type){
	case TsType::EQ:
	case TsType::RULE:
		break;
	case TsType::IMPL:
		result = MinisatID::ImplicationType::IMPLIES;
		break;
	case TsType::RIMPL:
		result = MinisatID::ImplicationType::IMPLIEDBY;
		break;
	}
	return result;
}

template<class Solver>
void SolverPolicy<Solver>::polAddLazyAddition(const litlist& glist, int ID) {
	adder.addLazyAddition(glist, ID);
}
template<class Solver>
void SolverPolicy<Solver>::polStartLazyFormula(LazyInstantiation* inst, TsType type, bool conjunction) {
	auto mon = new LazyClauseMon(inst);
	auto lit = createLiteral(inst->residual);
	extAdd(getSolver(), MinisatID::LazyGroundImpl(MinisatID::Implication(lit, convert(type), MinisatID::litlist { }, conjunction), mon));
	CHECKUNSAT;
}

class LazyLitMon: public MinisatID::LazyGroundingCommand {
private:
	LazyInstantiation* inst;

public:
	LazyLitMon(LazyInstantiation* inst)
			: inst(inst) {
	}

	virtual void requestGrounding(MinisatID::Value) {
		if(isAlreadyGround()) {
			return; // FIXME when can this happen?
		}
		bool temp;
		inst->notifyGroundingRequested(-1, false, temp);
		notifyGrounded();
	}
};
template<class Solver>
void SolverPolicy<Solver>::polNotifyLazyResidual(LazyInstantiation* inst, TsType type) {
	auto mon = new LazyLitMon(inst);
	auto watchboth = type == TsType::EQ;
	if(not useUFSAndOnlyIfSem()){
		watchboth |= type == TsType::RULE;
	}
	auto lit = createLiteral(inst->residual);
	if (type == TsType::RIMPL) {
		lit = not lit;
	}
	if(MinisatID::isNegative(lit) || watchboth){
		extAdd(getSolver(), MinisatID::LazyGroundLit(MinisatID::var(lit), MinisatID::Value::False, mon));
	}
	if(MinisatID::isPositive(lit) || watchboth){
		extAdd(getSolver(), MinisatID::LazyGroundLit(MinisatID::var(lit), MinisatID::Value::True, mon));
	}
	CHECKUNSAT;
}

class LazyDelayMon: public MinisatID::LazyGroundingCommand {
private:
	Atom atom;
	LazyGroundingManager* manager;
public:
	LazyDelayMon(Atom atom, LazyGroundingManager* manager)
			: atom(atom), manager(manager) {
	}

	virtual void requestGrounding(MinisatID::Value currentValue) {
		Assert(not isAlreadyGround());
		switch (currentValue) {
		case MinisatID::Value::True:
			manager->notifyBecameTrue(atom);
			break;
		case MinisatID::Value::False:
			manager->notifyBecameTrue(-atom);
			break;
		case MinisatID::Value::Unknown:
			break;
		}
	}
};

MinisatID::Value convert(TruthValue value){
	auto v = MinisatID::Value::True;
	switch (value) {
	case TruthValue::True:
		v = MinisatID::Value::True;
		break;
	case TruthValue::Unknown:
		v = MinisatID::Value::Unknown;
		break;
	case TruthValue::False:
		v = MinisatID::Value::False;
		break;
	}
	return v;
}

template<class Solver>
void SolverPolicy<Solver>::polNotifyLazyWatch(Atom atom, TruthValue watches, LazyGroundingManager* manager) {
	Assert(watches!=TruthValue::Unknown);
	auto mon = new LazyDelayMon(atom, manager);
	MinisatID::LazyGroundLit lc(atom, convert(watches), mon);
	extAdd(getSolver(), lc);
	CHECKUNSAT;
}

template<class Solver>
void SolverPolicy<Solver>::requestTwoValued(const std::vector<Lit>& lits) {
	MinisatID::TwoValuedRequirement req( { });
	for (auto lit : lits) {
		req.atoms.push_back(createAtom(lit));
	}
	extAdd(getSolver(), req);
	CHECKUNSAT;
}

template<class Solver>
void SolverPolicy<Solver>::requestTwoValued( VarId& varid) {
	MinisatID::TwoValuedVarIdRequirement req( convert(varid));
	extAdd(getSolver(), req);
	CHECKUNSAT;
}


class RealElementGrounder: public MinisatID::LazyAtomGrounder {
private:
	Lit headatom;
	std::vector<GroundTerm> args; // Constants!
	std::map<int,int> var2arg, arg2var;
	PFSymbol* symbol;
	std::vector<SortTable*> _tables;
	SymbolOffset symboloffset;
	AbstractGroundTheory* theory;
	bool recursive;

public:
	RealElementGrounder(Lit headatom, PFSymbol*symbol, const std::vector<GroundTerm>& args, AbstractGroundTheory* theory, bool recursive)
			: 	headatom(headatom),
				args(args),
				symbol(symbol),
				symboloffset(theory->translator()->addSymbol(symbol)),
				theory(theory),
				recursive(false){ // TODO comparison constraints do not support recursively defined symbols yet

		if(recursive){
			throw IdpException("No ground atoms over recursive symbols.");
		}

		int varpos = 0;
		for(uint i=0; i<args.size(); ++i){
			if(args[i].isVariable){
				var2arg[varpos]=i;
				arg2var[i]=varpos;
				varpos++;
			}
			_tables.push_back(theory->structure()->inter(symbol->sorts()[i]));
		}
	}

	bool isFunction() const{
		return symboloffset.functionlist;
	}
	std::string getSymbolName() const{
		return symbol->nameNoArity();
	}

	GroundTranslator* translator() const { return theory->translator(); }

	virtual void ground(bool headvalue, const std::vector<int>& varvalues) {
		Lit temphead;

		std::vector<GroundTerm> tuple;
		ElementTuple elemtuple;
		for(uint i=0; i<args.size(); ++i){
			if(isFunction() && i==args.size()-1){
				break;
			}
			if(args[i].isVariable){
				tuple.push_back(createDomElem(varvalues[arg2var.at(i)]));
				elemtuple.push_back(createDomElem(varvalues[arg2var.at(i)]));
			}else{
				tuple.push_back(args[i]._domelement);
				elemtuple.push_back(args[i]._domelement);
			}
		}

		if (symboloffset.functionlist) {
			auto tempunitables = theory->structure()->inter(symbol)->universe().tables();
			tempunitables.pop_back();
			auto tempuni = Universe(tempunitables);
			if (not tempuni.contains(elemtuple)) { // outside domain
				temphead = _false;
			} else {
				temphead = translator()->createNewUninterpretedNumber();
				auto lhsvar = translator()->translateTerm(symboloffset, tuple);
				CPVarTerm left(lhsvar);
				CPBound right(1);
				if(args.back().isVariable){
					right = CPBound(args.back()._varid);
				}else{
					if(args.back()._domelement->type()!=DomainElementType::DET_INT){
						throw InternalIdpException("Incorrect domain element type.");
					}
					right = CPBound(args.back()._domelement->value()._int);
				}
				CPTsBody b(TsType::EQ, &left, CompType::EQ, right);
				try{
					theory->add(temphead, &b);
				}catch(const UnsatException& ){

				}
			}
		} else {
			bool cf = false;
			for (size_t n = 0; n < elemtuple.size(); ++n) {
				if (elemtuple[n] == NULL || not _tables[n]->contains(elemtuple[n])) {
					if (getOption(VERBOSE_GROUNDING) > 3) {
						Warning::warning("Out of bounds on generalized atom grounder");
					}
					cf = true;
				}
			}
			temphead = cf?_false : translator()->translateReduced(symboloffset, elemtuple, recursive);
		}
		GroundClause clause;
		if (headvalue) {
			clause.push_back(-headatom);
			if (temphead == _true) {
				return; // Clause satisfied
			} else if (temphead != _false) {
				clause.push_back(temphead);
			}
		} else {
			clause.push_back(headatom);
			if (temphead == _false) {
				return; // Clause is satisfied
			} else if (temphead != _true) {
				clause.push_back(-temphead);
			}
		}
		for (uint i = 0; i < varvalues.size(); ++i) {
			if(args[var2arg[i]].isVariable){
				auto varterm = new CPVarTerm(args[var2arg.at(i)]._varid);
				CPBound bound(varvalues[i]);
				auto varlit = theory->translator()->reify(varterm, CompType::EQ, bound, TsType::EQ);
				if (varlit == _false) {
					return;
				} else if (varlit != _true) {
					clause.push_back(-varlit);
				}
			}
		}
		try{
			theory->add(clause);
		}catch(const UnsatException& ){

		}
	}
};

template<class Solver>
void SolverPolicy<Solver>::polAddLazyElement(Lit head, PFSymbol* symbol, const std::vector<GroundTerm>& args, AbstractGroundTheory* theory, bool recursive) {
	auto gr = new RealElementGrounder(head, symbol, args, theory, recursive);
	vector<MinisatID::VarID> vars;
	for (uint i=0; i<args.size(); ++i) {
		if(i==args.size()-1 && gr->isFunction()){
			break;
		}
		auto arg = args[i];
		if(arg.isVariable){
			adder.addFDVariable(arg._varid);
			vars.push_back(convert(arg._varid));
		}
	}
	auto le = MinisatID::LazyAtom(createLiteral(head), vars, gr);
	extAdd(getSolver(), le);
	CHECKUNSAT;
}

// Explicit instantiations
template class SolverPolicy<PCSolver> ;
