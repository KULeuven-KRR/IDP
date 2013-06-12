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

#include "inferences/SolverConnection.hpp"

#include <cmath>

using namespace std;
using namespace SolverConnection;

#define CHECKUNSAT \
		if(getSolver().isCertainlyUnsat()){\
			throw UnsatException();\
		}

template<typename Solver>
void SolverPolicy<Solver>::initialize(Solver* solver, int verbosity, GroundTranslator* translator) {
	_solver = solver;
	_verbosity = verbosity;
	_translator = translator;
	_origTrueLit = _translator->createNewUninterpretedNumber();
	_trueLit = createLiteral(_origTrueLit);
	extAdd(getSolver(), MinisatID::Disjunction(getDefConstrID(), {_trueLit}));
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(const GroundClause& cl) {
	MinisatID::Disjunction clause(getDefConstrID(), { });
	for (size_t n = 0; n < cl.size(); ++n) {
		clause.literals.push_back(createLiteral(cl[n]));
	}
	extAdd(getSolver(), clause);
	CHECKUNSAT;
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(const TsSet& tsset, SetId setnr, bool weighted) {
	if (not weighted) {
		MinisatID::WLSet set(setnr.id);
		for (size_t n = 0; n < tsset.size(); ++n) {
			set.wl.push_back(MinisatID::WLtuple { createLiteral(tsset.literal(n)), MinisatID::Weight(1) });
		}
		extAdd(getSolver(), set);
	} else {
		MinisatID::WLSet set(setnr.id);
		for (size_t n = 0; n < tsset.size(); ++n) {
			set.wl.push_back(MinisatID::WLtuple { createLiteral(tsset.literal(n)), createWeight(tsset.weight(n)) });
		}
		extAdd(getSolver(), set);
	}
	CHECKUNSAT;
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(DefId defnr, PCGroundRule* rule) {
	polAddPCRule(defnr.id, rule->head(), rule->body(), (rule->type() == RuleType::CONJ));
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
	//FIXME getIDForUndefined() should be replaced by the number the SOLVER takes as undefined
	polAddAggregate(getIDForUndefined(), head, body->bound(), body->lower(), body->setnr(), body->aggtype(), body->type());
}

template<typename Solver>
void SolverPolicy<Solver>::polAddWeightedSum(const MinisatID::Atom& head, const litlist& conditions, const varidlist& varids, const intweightlist& weights, const int& bound,
		MinisatID::EqType rel) {

	polAddCPVariables(varids, _translator);

	vector<MinisatID::Weight> w;
	for (auto i = weights.cbegin(); i < weights.cend(); ++i) {
		w.push_back(MinisatID::Weight(*i));
	}
	vector<MinisatID::VarID> vars;
	for (auto var : varids) {
		vars.push_back(convert(var));
	}
	vector<MinisatID::Lit> lits;
	for(auto lit:conditions){
		lits.push_back(lit==_true?_trueLit:createLiteral(lit));
	}
	MinisatID::CPSumWeighted sentence(getDefConstrID(), MinisatID::mkPosLit(head), lits, vars, w, rel, bound);
	extAdd(getSolver(), sentence);
	CHECKUNSAT;
}

template<typename Solver>
void SolverPolicy<Solver>::polAddWeightedProd(const MinisatID::Atom& head, const litlist& conditions, const varidlist& varids, const int& weight, VarId bound, MinisatID::EqType rel) {
	polAddCPVariables(varids, _translator);
	polAddCPVariables( { bound }, _translator);

	MinisatID::Weight w = weight;
	vector<MinisatID::VarID> vars;
	for (auto var : varids) {
		vars.push_back(convert(var));
	}
	vector<MinisatID::Lit> lits;
	for(auto lit:conditions){
		lits.push_back(lit==_true?_trueLit:createLiteral(lit));
	}
	MinisatID::CPProdWeighted sentence(getDefConstrID(), MinisatID::mkPosLit(head), lits, vars, w, rel, convert(bound));
	extAdd(getSolver(), sentence);
	CHECKUNSAT;
}

/**
 * Generates a list of literals which enforce some comparison whenever the associated condition is true
 */
template<typename Solver>
litlist SolverPolicy<Solver>::getConditionalComparisonList(const litlist& conditions, const varidlist& varids, MinisatID::EqType comp, VarId rhsvarid){
	litlist tseitins;
	for(uint i=0; i<varids.size(); ++i){
		auto impltseitin = _translator->createNewUninterpretedNumber();
		auto comptseitin = _translator->createNewUninterpretedNumber();
		tseitins.push_back(impltseitin);
		MinisatID::CPBinaryRelVar sentence(getDefConstrID(), createLiteral(comptseitin), convert(varids[i]), comp, convert(rhsvarid));
		extAdd(getSolver(), sentence);
		auto cond = conditions[i];
		polAdd(impltseitin, TsType::EQ, {~(cond==_true?_origTrueLit:cond), comptseitin}, false);
		CHECKUNSAT;
	}
	return tseitins;
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(Lit tseitin, CPTsBody* body) {
	MinisatID::EqType comp = MinisatID::EqType::EQ;
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
	if (isa<CPVarTerm>(*left)) {
		auto term = dynamic_cast<CPVarTerm*>(left);
		polAddCPVariable(term->varid(), _translator);
		if (right._isvarid) {
			polAddCPVariable(right._varid, _translator);
			MinisatID::CPBinaryRelVar sentence(getDefConstrID(), createLiteral(tseitin), convert(term->varid()), comp, convert(right._varid));
			extAdd(getSolver(), sentence);
			CHECKUNSAT;
		} else {
			MinisatID::CPBinaryRel sentence(getDefConstrID(), createLiteral(tseitin), convert(term->varid()), comp, right._bound);
			extAdd(getSolver(), sentence);
			CHECKUNSAT;
		}
	} else if (isa<CPSetTerm>(*left)) {
		auto term = dynamic_cast<CPSetTerm*>(left);

		switch(term->type()){
		case AggFunction::SUM:{
			polAddCPVariables(term->varids(), _translator);

			auto varids = term->varids();
			auto weights = term->weights();
			auto conditions = term->conditions();
			auto bound = 0;
			if (right._isvarid) {
				bound = 0;
				varids.push_back(right._varid);
				weights.push_back(-1);
				conditions.push_back(_true);
			}else{
				bound = right._bound;
			}
			polAddWeightedSum(createAtom(tseitin), conditions, varids, weights, bound, comp);
			break;
		}
		case AggFunction::PROD:{
			VarId rhsvarid;
			if(right._isvarid){
				rhsvarid = right._varid;
			}else{
				rhsvarid = _translator->translateTerm(createDomElem(right._bound));
			}

			polAddWeightedProd(createAtom(tseitin), term->conditions(), term->varids(), term->weights().back(), rhsvarid, comp);
			break;
		}
		case AggFunction::MIN:{
			VarId rhsvarid;
			if(right._isvarid){
				rhsvarid = right._varid;
			}else{
				rhsvarid = _translator->translateTerm(createDomElem(right._bound));
			}

			polAddCPVariable(rhsvarid, _translator);
			polAddCPVariables(term->varids(), _translator);

			/**
			 * ==
			 * 		forall setelem: elem >= rightvar
			 * 		exists setelem: elem =< rightvar
			 * ~=
			 * 		exists setelem: elem > rightvar  |
			 * 				forall setelem: elem < rightvar
			 * <
			 * 		forall setelem: elem < rightvar
			 * >
			 * 		exists setelem: elem > rightvar
			 * =<
			 * 		forall setelem: elem =< rightvar
			 * >=
			 * 		exists setelem: elem >= rightvar
			 */
			switch (body->comp()) {
			case CompType::EQ:{
				auto ts1 = _translator->createNewUninterpretedNumber(), ts2 = _translator->createNewUninterpretedNumber();
				polAdd(ts1, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::GEQ, rhsvarid), true);
				polAdd(ts2, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::LEQ, rhsvarid), false);
				polAdd(tseitin, TsType::EQ, {ts1,ts2}, true);
				break;}
			case CompType::NEQ:{
				auto ts1 = _translator->createNewUninterpretedNumber(), ts2 = _translator->createNewUninterpretedNumber();
				polAdd(ts1, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::G, rhsvarid), false);
				polAdd(ts2, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::L, rhsvarid), true);
				polAdd(tseitin, TsType::EQ, {ts1,ts2}, false);
				break;}
			case CompType::LEQ:
				polAdd(tseitin, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::LEQ, rhsvarid), false);
				break;
			case CompType::GEQ:
				polAdd(tseitin, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::GEQ, rhsvarid), true);
				break;
			case CompType::LT:
				polAdd(tseitin, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::L, rhsvarid), false);
				break;
			case CompType::GT:
				polAdd(tseitin, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::G, rhsvarid), true);
				break;
			}
			break;
		}
		case AggFunction::MAX:{
			VarId rhsvarid;
			if(right._isvarid){
				rhsvarid = right._varid;
			}else{
				rhsvarid = _translator->translateTerm(createDomElem(right._bound));
			}

			polAddCPVariable(rhsvarid, _translator);
			polAddCPVariables(term->varids(), _translator);

			switch (body->comp()) {
			case CompType::EQ:{
				auto ts1 = _translator->createNewUninterpretedNumber(), ts2 = _translator->createNewUninterpretedNumber();
				polAdd(ts1, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::LEQ, rhsvarid), true);
				polAdd(ts2, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::GEQ, rhsvarid), false);
				polAdd(tseitin, TsType::EQ, {ts1,ts2}, true);
				break;}
			case CompType::NEQ:{
				auto ts1 = _translator->createNewUninterpretedNumber(), ts2 = _translator->createNewUninterpretedNumber();
				polAdd(ts1, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::L, rhsvarid), false);
				polAdd(ts2, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::G, rhsvarid), true);
				polAdd(tseitin, TsType::EQ, {ts1,ts2}, false);
				break;}
			case CompType::LEQ:
				polAdd(tseitin, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::LEQ, rhsvarid), true);
				break;
			case CompType::GEQ:
				polAdd(tseitin, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::GEQ, rhsvarid), false);
				break;
			case CompType::LT:
				polAdd(tseitin, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::L, rhsvarid), true);
				break;
			case CompType::GT:
				polAdd(tseitin, TsType::EQ, getConditionalComparisonList(term->conditions(), term->varids(), MinisatID::EqType::G, rhsvarid), false);
				break;
			}
			break;
		}
		default:
			throw IdpException("Invalid code path");
		}
	}
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(Lit tseitin, TsType type, const GroundClause& rhs, bool conjunction) {
	auto newtseitin = tseitin;
	MinisatID::ImplicationType impltype = MinisatID::ImplicationType::IMPLIES;
	auto newconj = conjunction;
	auto newrhs = rhs;
	switch (type) {
	case TsType::RULE:
		throw IdpException("Invalid code path");
		break;
	case TsType::IMPL:
		impltype = MinisatID::ImplicationType::IMPLIES;
		break;
	case TsType::RIMPL:
		for (uint i = 0; i < rhs.size(); ++i) {
			newrhs[i] = -rhs[i];
		}
		newtseitin = -newtseitin;
		newconj = not newconj;
		impltype = MinisatID::ImplicationType::IMPLIES;
		break;
	case TsType::EQ:
		impltype = MinisatID::ImplicationType::EQUIVALENT;
		break;
	}
	extAdd(getSolver(), MinisatID::Implication(getDefConstrID(), createLiteral(newtseitin), impltype, createList(newrhs), newconj));
	CHECKUNSAT;
}

template<typename Solver>
void SolverPolicy<Solver>::polAddAggregate(DefId definitionID, Lit head, double bound, bool lowerbound, SetId setnr, AggFunction aggtype, TsType sem) {
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
	extAdd(getSolver(),
			MinisatID::Aggregate(getDefConstrID(), createLiteral(head), setnr.id, createWeight(bound), convert(aggtype), sign, msem, definitionID.id, useUFSAndOnlyIfSem()));
	CHECKUNSAT;
}

template<typename Solver>
void SolverPolicy<Solver>::polAddCPVariables(const varidlist& varids, GroundTranslator* termtranslator) {
	for (auto it = varids.begin(); it != varids.end(); ++it) {
		polAddCPVariable(*it, termtranslator);
	}
}

template<typename Solver>
void SolverPolicy<Solver>::polAddCPVariable(const VarId& varid, GroundTranslator* termtranslator) {
	if (_addedvarids.find(varid) != _addedvarids.cend()) {
		return;
	}
	_addedvarids.insert(varid);
	auto domain = termtranslator->domain(varid);
	Assert(domain != NULL);
	if(not domain->approxFinite()){
		throw notyetimplemented("Derived a sort to range over an infinite domain, which cannot be handled by the solver.");
	}
	if (domain->isRange()) {
		// the domain is a complete range from minvalue to maxvalue.
		MinisatID::IntVarRange range(getDefConstrID(), convert(varid), domain->first()->value()._int, domain->last()->value()._int);
		extAdd(getSolver(), range);
	} else {
		// the domain is not a complete range.
		std::vector<MinisatID::Weight> w;
		for (auto it = domain->sortBegin(); not it.isAtEnd(); ++it) {
			CHECKTERMINATION;
			w.push_back((MinisatID::Weight) (*it)->value()._int);
		}
		extAdd(getSolver(), MinisatID::IntVarEnum(getDefConstrID(), convert(varid), w));
	}
	CHECKUNSAT;
}

template<typename Solver>
void SolverPolicy<Solver>::polAddPCRule(DefId defnr, Lit head, std::vector<int> body, bool conjunctive) {
	MinisatID::litlist list;
	for (size_t n = 0; n < body.size(); ++n) {
		list.push_back(createLiteral(body[n]));
	}
	extAdd(getSolver(), MinisatID::Rule(getDefConstrID(), createAtom(head), list, conjunctive, defnr.id, useUFSAndOnlyIfSem()));
	CHECKUNSAT;
}

template<typename Solver>
void SolverPolicy<Solver>::polAddOptimization(AggFunction function, SetId setid) {
	extAdd(getSolver(), MinisatID::MinimizeAgg(1, setid.id, convert(function)));
	CHECKUNSAT;
}

template<typename Solver>
void SolverPolicy<Solver>::polAddOptimization(VarId varid) {
	polAddCPVariable(varid, _translator);
	extAdd(getSolver(), MinisatID::MinimizeVar(1, convert(varid)));
	CHECKUNSAT;
}

class LazyRuleMon: public MinisatID::LazyGroundingCommand {
private:
	Lit lit;
	ElementTuple args;
	std::vector<DelayGrounder*> grounders;

public:
	LazyRuleMon(const Lit& lit, const ElementTuple& args, const std::vector<DelayGrounder*>& grounders)
			: 	lit(lit),
				args(args),
				grounders(grounders) {
	}

	virtual void requestGrounding(MinisatID::Value) {
		if (not isAlreadyGround()) {
			notifyGrounded();
			for (auto i = grounders.begin(); i < grounders.end(); ++i) {
				throw notyetimplemented("Further grounding delayed grounders.");
				// FIXME (*i)->ground(lit, args);
			}
		}
	}
};

template<class Solver>
void SolverPolicy<Solver>::polNotifyUnknBound(Context context, const Lit& delaylit, const ElementTuple& args, std::vector<DelayGrounder*> grounders) {
	auto mon = new LazyRuleMon(delaylit, args, grounders);
	auto literal = createLiteral(delaylit);
	auto value = MinisatID::Value::True;
	if (context == Context::BOTH) {
		value = MinisatID::Value::Unknown;
	} else if (context == Context::POSITIVE) { // In a positive context, should watch when the literal becomes false, or it's negation becomes true
		value = MinisatID::Value::False;
	}
	MinisatID::LazyGroundLit lc(literal.getAtom(), value, mon);
	extAdd(getSolver(), lc);
	CHECKUNSAT;
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

template<class Solver>
void SolverPolicy<Solver>::polAddLazyAddition(const litlist& glist, int ID) {
	MinisatID::litlist list;
	for (auto i = glist.cbegin(); i < glist.cend(); ++i) {
		list.push_back(createLiteral(*i));
	}
	extAdd(getSolver(), MinisatID::LazyAddition(list, ID));
	CHECKUNSAT;
}
template<class Solver>
void SolverPolicy<Solver>::polStartLazyFormula(LazyInstantiation* inst, TsType type, bool conjunction) {
	auto mon = new LazyClauseMon(inst);
	auto watchboth = type == TsType::RULE || type == TsType::EQ;
	auto lit = createLiteral(inst->residual);
	if (type == TsType::RIMPL) {
		lit = not lit;
	}
	MinisatID::Implication implic(getDefConstrID(), lit, watchboth ? MinisatID::ImplicationType::EQUIVALENT : MinisatID::ImplicationType::IMPLIES,
			MinisatID::litlist { }, conjunction);
	extAdd(getSolver(), MinisatID::LazyGroundImpl(getDefConstrID(), implic, mon));
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
		Assert(not isAlreadyGround());
		bool temp;
		inst->notifyGroundingRequested(-1, false, temp);
		notifyGrounded();
	}
};
template<class Solver>
void SolverPolicy<Solver>::polNotifyLazyResidual(LazyInstantiation* inst, TsType type) {
	auto mon = new LazyLitMon(inst);
	auto watchboth = type == TsType::RULE || type == TsType::EQ;
	auto lit = createLiteral(inst->residual);
	if (type == TsType::RIMPL) {
		lit = not lit;
	}
	auto value = MinisatID::Value::True;
	if (watchboth) {
		value = MinisatID::Value::Unknown;
	} else if (lit.hasSign()) {
		value = MinisatID::Value::False;
	}
	extAdd(getSolver(), MinisatID::LazyGroundLit(lit.getAtom(), value, mon));
	CHECKUNSAT;
}

template<class Solver>
void SolverPolicy<Solver>::polAdd(const std::vector<std::map<Lit, Lit> >& symmetries) {
	for(auto symmap:symmetries){
		std::map<MinisatID::Lit,MinisatID::Lit> symdata;
		for(auto litpair:symmap){
			symdata.insert({SolverConnection::createLiteral(litpair.first),SolverConnection::createLiteral(litpair.second)});
		}
		getSolver().add(MinisatID::Symmetry(symdata));
	}
}

class RealElementGrounder: public MinisatID::LazyAtomGrounder {
private:
	Lit headatom;
	std::vector<GroundTerm> args; // Constants!
	std::map<int,int> var2arg, arg2var;
	PFSymbol* symbol;
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
		}
	}

	bool isFunction() const{
		return symboloffset.functionlist;
	}
	std::string getSymbolName() const{
		return symbol->nameNoArity();
	}

	virtual void ground(bool headvalue, const std::vector<int>& varvalues) {
		Lit temphead;
		auto translator = theory->translator();

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
				temphead = translator->createNewUninterpretedNumber();
				auto lhsvar = translator->translateTerm(symboloffset, tuple);
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
			temphead = translator->translateReduced(symboloffset, elemtuple, recursive);
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
			polAddCPVariable(arg._varid, _translator);
			vars.push_back(convert(arg._varid));
		}
	}
	auto le = MinisatID::LazyAtom(getDefConstrID(), createLiteral(head), vars, gr);
	extAdd(getSolver(), le);
	CHECKUNSAT;
}

// Explicit instantiations
template class SolverPolicy<PCSolver> ;
